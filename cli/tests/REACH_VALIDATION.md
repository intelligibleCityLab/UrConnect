# Numerical reach validation

The reach test compares every source segment against literal, independently
hand-derived expected lengths for two small networks. The expected values below
were derived from the geometry and reach definitions before running the engine;
they are not snapshots of production output. This document specifies the oracle
and test scope, and does not claim that a particular build has passed.

After configuring a build as described in the repository README, reproduce the
numerical check with:

```bash
cmake --build build --config Release --target urconnect-test-reach
ctest --test-dir build -C Release -R '^reach_correctness$' --output-on-failure -V
```

Reports are written to `build/cli/reach-validation-Release.csv` and `.md`.
For single-configuration generators, configure `CMAKE_BUILD_TYPE=Release`.
The current fixtures yield 156 parameter/orientation cases and 702 individual
source-segment comparisons. CSV segment IDs are zero-based: A through E are
0 through 4, and the square's four edges are 0 through 3 in listed order.

## Definitions and measurement

Coordinates and segment lengths are in metres. All calculations are unweighted.

- **MR:** total network length within shortest-path metric distance R from the
  source segment's midpoint. A partially reached segment contributes only its
  reached length. Unlimited radius (`n`) reaches the source's connected component,
  not disconnected components.
- **DR:** total length reachable with at most the specified number of directional
  changes. A change is counted when the deflection angle is at least the angle
  threshold. The fixtures use a 45-degree threshold, straight continuations, and
  90-degree bends, avoiding equality and rounding ambiguity at the threshold.
- **JNR:** total length reachable after crossing at most the specified number of
  qualifying junctions. A node qualifies when its degree is **at least** the
  degree threshold. Crossing a qualifying shared endpoint between two segments
  costs one; the source segment itself costs zero.
- **MDR:** reached length subject to both the metric radius and the directional
  limit. Partial metric contributions are retained only where the directional
  constraint permits access.

The source is available in both travel directions. Each physical segment, or
reached portion of it, contributes once even if it is accessible by multiple
routes. In particular, a source-only metric result is `min(source_length, 2R)`;
it must not exceed the source's physical length.

For each parameter case and endpoint orientation, the test checks every source
segment's in-memory result. The absolute tolerance is **1e-9 m**. The reported
maximum deviation is `max_source(abs(actual - expected))`, not an average or a
check of selected source segments. This tolerance concerns in-memory lengths;
DBF, CSV, and other export rounding are outside this test's measurement.

Each fixture is repeated with its original endpoint order, all endpoints
reversed, and alternating segments reversed. Endpoint order changes directed
storage but must not change the undirected reach values.

## Asymmetric tree and disconnected segment

The fixture has five segments in the following source order:

| ID | Start | End | Length |
| --- | --- | --- | ---: |
| A | (-10, 0) | (0, 0) | 10 |
| B | (0, 0) | (20, 0) | 20 |
| C | (0, 0) | (0, 30) | 30 |
| D | (20, 0) | (20, 40) | 40 |
| E | (100, 0) | (150, 0) | 50 |

A, B, and C meet at a degree-3 junction. B and D meet at a degree-2 bend.
The connected component A-D has length 100; E is an isolated component of
length 50. There are no alternative routes in the tree.

For a non-source segment, the distances below are from the source midpoint to
the first endpoint of the target on the unique connecting path:

| Source / target | A | B | C | D | E |
| --- | ---: | ---: | ---: | ---: | ---: |
| A | - | 5 | 5 | 25 | infinity |
| B | 10 | - | 10 | 10 | infinity |
| C | 15 | 15 | - | 35 | infinity |
| D | 40 | 20 | 40 | - | infinity |
| E | infinity | infinity | infinity | infinity | - |

Given an entry distance d and target length L, a non-source segment contributes
`min(L, max(0, R - d))` to MR. The source contributes `min(L, 2R)`. For example,
A at R=35 reaches `10 + 20 + 30 + 10 = 70`; D at R=60 reaches
`10 + 20 + 20 + 40 = 90`.

All vectors below use the source order **[A, B, C, D, E]**.

| MR radius | Expected lengths |
| --- | --- |
| 2 | [4, 4, 4, 4, 4] |
| 15 | [30, 35, 30, 30, 30] |
| 35 | [70, 80, 60, 55, 50] |
| 60 | [95, 100, 85, 90, 50] |
| n | [100, 100, 100, 100, 50] |

At 45 degrees, A-B is a zero-turn continuation. A-C, B-C, and B-D
each require one turn; A-D requires one turn via B; C-D requires two.

| DR turn limit | Expected lengths |
| ---: | --- |
| 0 | [30, 30, 30, 40, 50] |
| 1 | [100, 100, 60, 70, 50] |
| 2 | [100, 100, 100, 100, 50] |

At degree threshold 3, B-D costs zero junctions. At degree threshold 2,
B-D costs one junction, and A-D or C-D costs two. A-B, A-C, and B-C
cost one junction at either threshold.

| JNR degree threshold | Junction limit | Expected lengths |
| ---: | ---: | --- |
| 2 | 0 | [10, 20, 30, 40, 50] |
| 2 | 1 | [60, 100, 60, 60, 50] |
| 2 | 2 | [100, 100, 100, 100, 50] |
| 3 | 0 | [10, 60, 30, 60, 50] |
| 3 | 1 | [100, 100, 100, 100, 50] |
| 3 | 2 | [100, 100, 100, 100, 50] |

Because tree routes are unique, MDR follows directly by keeping the metric
contributions whose route satisfies the turn limit. At R=60 and one turn, A
remains metric-limited at 95, while C and D are directional-limited at 60 and 70.

| MDR radius | Turn limit | Expected lengths |
| ---: | ---: | --- |
| 2 | 0 | [4, 4, 4, 4, 4] |
| 2 | 1 | [4, 4, 4, 4, 4] |
| 2 | 2 | [4, 4, 4, 4, 4] |
| 15 | 0 | [20, 25, 30, 30, 30] |
| 15 | 1 | [30, 35, 30, 30, 30] |
| 15 | 2 | [30, 35, 30, 30, 30] |
| 35 | 0 | [30, 30, 30, 40, 50] |
| 35 | 1 | [70, 80, 60, 55, 50] |
| 35 | 2 | [70, 80, 60, 55, 50] |
| 60 | 0 | [30, 30, 30, 40, 50] |
| 60 | 1 | [95, 100, 60, 70, 50] |
| 60 | 2 | [95, 100, 85, 90, 50] |

## Square cycle

The second fixture is the four edges of the square
`(0,0) -> (10,0) -> (10,10) -> (0,10) -> (0,0)`.
Each edge has length 10 and each node has degree 2. Symmetry makes every source
value identical; the test nevertheless checks all four source segments.

The source midpoint is 5 m from either endpoint. Its adjacent edges begin at
distance 5; the opposite edge can be entered from either endpoint at distance
15. Thus R=7 reaches the source plus 2 m on each adjacent edge, totaling 14.
R=17 reaches the source, both adjacent edges, and 2 m from each end of the
opposite edge, totaling 34. Overlapping coverage on the opposite edge must be
counted only once. R=25 and unlimited radius cover all 40 m.

| MR radius | Expected length for each source |
| --- | ---: |
| 2 | 4 |
| 7 | 14 |
| 17 | 34 |
| 25 | 40 |
| n | 40 |

At 45 degrees, each adjacent edge requires one turn and the opposite edge
requires two. The two paths to the opposite edge do not duplicate its length.

| DR turn limit | Expected length for each source |
| ---: | ---: |
| 0 | 10 |
| 1 | 30 |
| 2 | 40 |

| JNR degree threshold | Junction limit | Expected length for each source |
| ---: | ---: | ---: |
| 2 | 0 | 10 |
| 2 | 1 | 30 |
| 2 | 2 | 40 |
| 3 | 0 | 40 |
| 3 | 1 | 40 |
| 3 | 2 | 40 |

| MDR radius | Turn limit | Expected length for each source |
| ---: | ---: | ---: |
| 2 | 0 | 4 |
| 2 | 1 | 4 |
| 2 | 2 | 4 |
| 7 | 0 | 10 |
| 7 | 1 | 14 |
| 7 | 2 | 14 |
| 17 | 0 | 10 |
| 17 | 1 | 30 |
| 17 | 2 | 34 |
| 25 | 0 | 10 |
| 25 | 1 | 30 |
| 25 | 2 | 40 |

## Coverage and reports

This validation covers unweighted whole-network MR, DR, JNR, and MDR through
the shared analysis engine used by the application and CLI. The tree exercises
unequal lengths, source clipping, partial target coverage, disconnected
components, degree thresholds, and independent metric and turn constraints.
The square adds two-sided entry, cycles, and prevention of duplicate coverage.
The three endpoint orientations check invariance to segment storage direction.

The test binary produces CSV and Markdown reports containing numerical
comparisons and maximum deviations across source segments. CI publishes these
reports as artifacts for each operating system. Actual pass/fail status and
measured deviations belong to the reports from the relevant run; the literal
oracle tables above are not measured results.

These are compact analytical regression fixtures, not a proof of correctness
for all networks. They do not validate weighted outputs, mean-distance metrics,
interactive selections, OD paths, export formatting, every angle boundary, or
arbitrary competing metric/directional routes. They are also **not** a comparison
with an external implementation or with the program cited as reference [19].
Such a comparison requires separately obtained software, matching parameters
and inputs, and independently recorded results.
