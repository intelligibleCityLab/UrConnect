"""Compare exported serial/parallel results and preserve the DR-first DMR oracle.

Only the Python standard library is used. Each invocation receives a fresh
Shapefile copy so an accidentally omitted output cannot inherit an earlier run.
"""
import argparse
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile


def run(arguments):
    return subprocess.run(arguments, capture_output=True, text=True, timeout=30)


def read_dbf(path):
    data = path.read_bytes()
    count, header, size = struct.unpack_from('<IHH', data, 4)
    result = {}
    schema = []
    offset = 1
    for pos in range(32, header - 1, 32):
        desc = data[pos:pos + 32]
        name = desc[:11].split(b'\0')[0].decode('ascii')
        kind, width, decimals = chr(desc[11]), desc[16], desc[17]
        assert name not in result, ('duplicate DBF field', name)
        assert kind in ('N', 'F'), ('unexpected field type', name, kind)
        values = []
        for row in range(count):
            record = header + row * size
            assert data[record:record + 1] == b' ', ('deleted record', row)
            raw = data[record + offset:record + offset + width].strip()
            assert raw, ('missing value', name, row)
            value = float(raw)
            assert math.isfinite(value), ('non-finite value', name, row, raw)
            values.append(value)
        schema.append((name, kind, width, decimals))
        result[name] = values
        offset += width
    assert offset == size, ('unexpected record width', offset, size)
    return schema, result


def competing_fixture(base, network_tool, reverse=False):
    """Write the hand-derived eight-road fixture from test_reach.cpp.

    Reuse the network tool's DBF schema, and write standard two-point PolyLine
    SHP/SHX records directly; no third-party Shapefile library is needed.
    """
    result = run([network_tool, 'create', str(base)])
    assert result.returncode == 0, result.stderr
    segments = [(-10, 0, 0, 0), (0, 0, 40, 0), (40, 0, 40, 20),
                (40, 20, 10, 20), (10, 20, 0, 20), (0, 0, 0, 10),
                (0, 10, 10, 10), (10, 10, 10, 20)]
    if reverse:
        segments = [(x2, y2, x1, y1) for x1, y1, x2, y2 in segments]
    box = (-10.0, 0.0, 40.0, 20.0)

    def header(words):
        return (struct.pack('>7i', 9994, 0, 0, 0, 0, 0, words)
                + struct.pack('<2i8d', 1000, 3, *box, 0, 0, 0, 0))

    records, index, offset = [], [], 50
    for number, (x1, y1, x2, y2) in enumerate(segments, 1):
        content = struct.pack('<i4d3i4d', 3, min(x1, x2), min(y1, y2),
                              max(x1, x2), max(y1, y2), 1, 2, 0, x1, y1, x2, y2)
        words = len(content) // 2
        index.append(struct.pack('>2i', offset, words))
        records.append(struct.pack('>2i', number, words) + content)
        offset += 4 + words
    base.with_suffix('.shp').write_bytes(header(offset) + b''.join(records))
    base.with_suffix('.shx').write_bytes(header(50 + 4 * len(segments)) + b''.join(index))
    original = base.with_suffix('.dbf').read_bytes()
    _, header_size, record_size = struct.unpack_from('<IHH', original, 4)
    dbf_header = bytearray(original[:header_size])
    struct.pack_into('<I', dbf_header, 4, len(segments))
    rows = []
    for fid in range(len(segments)):
        # The existing test-network tool defines FID N(8,0), weight N(12,2).
        row = (' ' + str(fid).rjust(8) + ('%.2f' % (10 * (fid + 1))).rjust(12)).encode('ascii')
        assert len(row) == record_size
        rows.append(row)
    base.with_suffix('.dbf').write_bytes(bytes(dbf_header) + b''.join(rows) + b'\x1a')


def compare_case(cli, root, fixture, name, command, options, oracle=None):
    reference = None
    counts = {}
    for threads in (None, '1', '2', '12', 'auto'):
        label = threads or 'default'
        directory = root / (name + '-' + label)
        directory.mkdir()
        for source in fixture.parent.glob(fixture.name + '.*'):
            shutil.copyfile(source, directory / source.name)
        base = directory / fixture.name
        before_schema, before_values = read_dbf(base.with_suffix('.dbf'))
        argv = [cli, command, str(base.with_suffix('.shp'))] + options
        if threads is not None:
            argv += ['--threads', threads]
        result = run(argv)
        assert result.returncode == 0, (name, label, result.stdout, result.stderr)
        workers = [int(value) for value in re.findall(r'Analysis workers:\s*(\d+)', result.stderr)]
        assert workers and len(set(workers)) == 1, (name, label, result.stderr)
        counts[label] = workers[0]
        schema, values = read_dbf(base.with_suffix('.dbf'))
        assert len(schema) > len(before_schema), (name, label, 'no analysis fields written')
        for field in before_values:
            assert values[field] == before_values[field], (name, label, 'input attribute changed', field)
        if oracle is not None:
            oracle(values)
        if reference is None:
            reference = (schema, values)
        else:
            assert schema == reference[0], (name, label, 'field schema differs')
            for field in values:
                assert values[field] == reference[1][field], (name, label, field,
                                                             values[field], reference[1][field])
    roads = len(next(iter(reference[1].values())))
    assert counts['default'] == counts['1'] == 1, (name, counts)
    assert 1 <= counts['auto'] <= roads, (name, counts)
    assert counts['2'] == min(2, counts['auto']), (name, counts)
    assert counts['12'] == min(12, counts['auto']), (name, counts)
    return counts['auto'] > 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cli', required=True)
    parser.add_argument('--network-tool', required=True)
    args = parser.parse_args()
    args.cli = str(Path(args.cli).resolve())
    args.network_tool = str(Path(args.network_tool).resolve())
    # Scalar and unsorted sweep profiles exercise per-worker output merging,
    # including finite boundaries, unlimited reach and multiple angle passes.
    profiles = {
        'reach': (['--radius', '15'], ['--radius', '30,n,0.25,15']),
        'md': (['--radius', '15'], ['--radius', '30,n,0.25,15']),
        'dr': (['--angle', '45', '--turns', '1'],
               ['--angle', '45,30', '--turns', '2,0,n,1']),
        'ddl': (['--angle', '45', '--radius', '15'],
                ['--angle', '45,30', '--radius', '30,n,0.25,15']),
        'mdr': (['--angle', '45', '--radius', '15', '--turns', '1'],
                ['--angle', '45,30', '--radius', '30,n,0.25,15', '--turns', '2,0,n,1']),
        'jnr': (['--junction-degree', '3', '--junction-limit', '1'],
                ['--junction-degree', '3', '--junction-limit', '2,0,n,1']),
        'jnd': (['--junction-degree', '3', '--radius', '15'],
                ['--junction-degree', '3', '--radius', '30,n,0.25,15']),
    }
    cases = 0
    exercised_parallel = False
    with tempfile.TemporaryDirectory(prefix='urconnect parallel ') as temporary:
        root = Path(temporary)
        fixtures = root / 'fixtures'
        fixtures.mkdir()
        simple = fixtures / 'simple'
        created = run([args.network_tool, 'create', str(simple)])
        assert created.returncode == 0, created.stderr
        for command, (scalar, sweep) in profiles.items():
            for profile, options in [('scalar', scalar), ('sweep', sweep),
                                     ('weighted', sweep + ['--weight', 'weight'])]:
                exercised_parallel |= compare_case(args.cli, root, simple,
                    command + '-' + profile, command, options)
                cases += 1
        # Additional junction-counting rules must merge identically as well.
        for command in ('reach', 'md', 'dr', 'mdr'):
            exercised_parallel |= compare_case(args.cli, root, simple,
                command + '-junction-turns', command,
                profiles[command][1] + ['--junction-turns', '3', '--weight', 'weight'])
            cases += 1

        # These accepted values reach the engine's INT_MAX-10 sentinel path,
        # where preprocessing leaves junction/turn lookup entries absent.
        # Workers must preserve serial default-zero semantics without racing
        # to insert missing entries into a shared map.
        for threshold in ('2147483637', '2147483647'):
            for command in ('jnr', 'jnd'):
                options = list(profiles[command][0])
                options[options.index('--junction-degree') + 1] = threshold
                exercised_parallel |= compare_case(args.cli, root, simple,
                    command + '-large-degree-' + threshold, command, options)
                cases += 1
            for command in ('reach', 'md', 'dr', 'mdr'):
                exercised_parallel |= compare_case(args.cli, root, simple,
                    command + '-large-junction-turns-' + threshold, command,
                    profiles[command][0] + ['--junction-turns', threshold])
                cases += 1
            for command in ('dr', 'ddl', 'mdr'):
                options = list(profiles[command][0])
                options[options.index('--angle') + 1] = threshold
                exercised_parallel |= compare_case(args.cli, root, simple,
                    command + '-large-angle-' + threshold, command, options)
                cases += 1

        # This graph distinguishes DR-first from MR-first. Check an independent
        # known answer in every worker mode, not just agreement between modes.
        for reverse in (False, True):
            fixture = fixtures / ('competing-reversed' if reverse else 'competing')
            competing_fixture(fixture, args.network_tool, reverse)
            for radius, expected in [('35', 60), ('70', 95), ('100', 125)]:
                def check(values, expected=expected):
                    outputs = [v for k, v in values.items() if k not in ('FID', 'weight', 'seglength')]
                    assert len(outputs) == 1, ('unexpected DMR fields', list(values))
                    assert abs(outputs[0][0] - expected) <= 0.0001, (outputs[0][0], expected)
                exercised_parallel |= compare_case(args.cli, root, fixture,
                    fixture.name + '-R' + radius, 'mdr',
                    ['--radius', radius, '--angle', '45', '--turns', '2'], check)
                cases += 1
            exercised_parallel |= compare_case(args.cli, root, fixture,
                fixture.name + '-weighted-sweep', 'mdr',
                ['--radius', '70,n,2.5,100,35', '--angle', '45',
                 '--turns', '3,0,2,1', '--weight', 'weight'])
            cases += 1

        for value in ('0', '-1', '1.5', '1,2', 'nan', 'inf', 'Auto', '+2',
                      '999999999999999999999', ''):
            result = run([args.cli, 'reach', str(root / 'missing.shp'),
                          '--radius', '15', '--threads', value])
            assert result.returncode == 1 and '--threads' in result.stderr, (value, result.stderr)
        for command, options in [('netreach', ['--from', '0', '--radius', '15']),
                                 ('stepdepth', ['--from', '0']), ('od', ['--from', '0'])]:
            result = run([args.cli, command, str(root / 'missing.shp'),
                          '--threads', '2'] + options)
            assert result.returncode == 1 and 'unknown option --threads' in result.stderr, (command, result.stderr)
        if os.environ.get('CI') == 'true':
            assert exercised_parallel, 'CI must exercise at least two real analysis workers'
    print('Parallel parity: %d cases, %d exact DBF comparisons; default/1/2/12/auto workers;' %
          (cases, cases * 4), 'DR-first oracle and invalid options passed.')
    print('Multiple analysis workers exercised:', exercised_parallel)


if __name__ == '__main__':
    main()
