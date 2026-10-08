#include "Calculation.h"
#include "ShapeFileAccessor.h"

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

bool NeedStop = false;
int loadedCount = 0;

namespace {
using Segment = std::array<double, 4>;

void near(double actual, double expected, const std::string& message) {
    if (!std::isfinite(actual) || std::fabs(actual - expected) > 1e-9)
        throw std::runtime_error(message + ": " + std::to_string(actual) +
                                 " != " + std::to_string(expected));
}

void prepare(ShapeFileAccessor& accessor, const std::vector<Segment>& segments) {
    AttributesData attributes;
    attributes.AttributesNames = {"Ref", "ID", "x1", "y1", "x2", "y2"};
    for (size_t i = 0; i < segments.size(); ++i) {
        attributes.AttributesDouble["Ref"][i] = i;
        attributes.AttributesDouble["ID"][i] = i;
        const char* names[] = {"x1", "y1", "x2", "y2"};
        for (int j = 0; j < 4; ++j)
            attributes.AttributesDouble[names[j]][i] = segments[i][j];
    }
    accessor.init("fixture.shp", attributes);
    accessor.ProcessShapeFile();
}

void initialize(Calculation& calculation, ShapeFileAccessor& accessor,
                const std::string& radii) {
    calculation.clearOldData();
    std::vector<std::string> weights;
    calculation.init_DDL_para(accessor, *accessor.ProcessShapeFile(), "fixture.shp",
                             radii, "45", "", false, false, "", weights);
    calculation.subRoadVec.assign(accessor.roadID.begin(), accessor.roadID.end());
}

void compare(const Calculation& a, const Calculation& b) {
    for (const auto& radius : a.DDL_all) {
        for (const auto& road : radius.second) {
            near(b.DDL_all.at(radius.first).at(road.first), road.second, "DDL consistency");
            near(b.DD_all.at(radius.first).at(road.first),
                 a.DD_all.at(radius.first).at(road.first), "DD consistency");
        }
    }
}

void crossTests() {
    for (double length : {10.0, 0.25}) {
        for (int reversed = 0; reversed < 16; ++reversed) {
            std::vector<Segment> segments = {{-length, 0, 0, 0}, {0, 0, length, 0},
                                              {0, -length, 0, 0}, {0, 0, 0, length}};
            for (int road = 0; road < 4; ++road) {
                if (reversed & (1 << road)) {
                    std::swap(segments[road][0], segments[road][2]);
                    std::swap(segments[road][1], segments[road][3]);
                }
            }
            ShapeFileAccessor accessor;
            prepare(accessor, segments);
            Calculation single, direct, threaded;
            const std::string radii = "n," + std::to_string(length * 0.2) + "," +
                                     std::to_string(length) + "," + std::to_string(length * 2);
            initialize(single, accessor, radii);
            initialize(direct, accessor, radii);
            initialize(threaded, accessor, radii);
            single.calculateDDL(accessor);
            direct.calculateDDLbyDij(accessor);
            threaded.MultiCalculate(accessor, 0);
            compare(single, direct);
            compare(single, threaded);
            for (int road = 0; road < 4; ++road) {
                near(single.DD_all.at(-1).at(road), 0.5, "cross DD");
                near(single.DDL_all.at(-1).at(road), 0.5, "cross DDL");
                near(single.DDL_all.at(length * 2).at(road), 0.5, "full cross DDL");
                near(single.DDL_all.at(length).at(road), 0.4, "boundary cross DDL");
                near(single.DDL_all.at(length * 0.2).at(road), 0, "origin-only DDL");
            }
        }
    }
}

void boundaryAndWeightTests() {
    std::vector<Segment> segments = {{0, 0, 10, 0}, {10, 0, 10, 10},
                                      {10, 10, 0, 10}, {0, 10, 0, 0},
                                      {100, 100, 110, 100}};
    ShapeFileAccessor accessor;
    prepare(accessor, segments);
    Calculation combined, independent, reversed, left, right;
    initialize(combined, accessor, "n,2,17,30");
    combined.isWgt = true;
    for (int road = 0; road < 5; ++road) {
        combined.weight["ones"][road] = 1;
        combined.weight["zeros"][road] = 0;
        combined.weight["ddl"][road] = 1;
    }
    combined.calculateDDL(accessor);
    near(combined.DDL_all.at(17).at(0), 28.0 / 34.0, "two-ended boundary counted once");
    near(combined.WDD_all.at(17).at("ones").at(0), 1, "boundary attribute counted once");
    near(combined.WDD_all.at(17).at("ddl").at(0), 1, "weight names cannot overwrite DDL");
    for (const auto& radius : combined.DDL_all) {
        initialize(independent, accessor, radius.first < 0 ? "n" : std::to_string(radius.first));
        independent.calculateDDL(accessor);
        compare(independent, combined);
        for (int road = 0; road < 5; ++road) {
            if (radius.first < 0)
                near(combined.WDD_all.at(radius.first).at("ones").at(road),
                     combined.DD_all.at(radius.first).at(road), "unit weighting");
            near(combined.WDD_all.at(radius.first).at("zeros").at(road), 0, "zero weighting");
        }
    }
    for (auto& segment : segments) {
        std::swap(segment[0], segment[2]);
        std::swap(segment[1], segment[3]);
    }
    ShapeFileAccessor backwards;
    prepare(backwards, segments);
    initialize(reversed, backwards, "n,2,17,30");
    reversed.calculateDDL(backwards);
    compare(combined, reversed);
    initialize(left, accessor, "n,2,17,30");
    initialize(right, accessor, "n,2,17,30");
    left.subRoadVec = {0, 2, 4};
    right.subRoadVec = {1, 3};
    std::thread a([&] { left.MultiCalculate(accessor, 0); });
    std::thread b([&] { right.MultiCalculate(accessor, 0); });
    a.join();
    b.join();
    compare(left, combined);
    compare(right, combined);
}

void unequalLengthTest() {
    ShapeFileAccessor accessor;
    prepare(accessor, {{-10, 0, 0, 0}, {0, 0, 10, 0},
                       {0, -20, 0, 0}, {0, 0, 0, 30}});
    Calculation calculation;
    initialize(calculation, accessor, "n");
    calculation.calculateDDL(accessor);
    for (int road = 0; road < 4; ++road) {
        near(calculation.DD_all.at(-1).at(road), 0.5, "unequal cross DD");
        near(calculation.DDL_all.at(-1).at(road), road < 2 ? 5.0 / 7.0 : 2.0 / 7.0,
             "DDL uses actual lengths, not segment counts");
    }
}

void exportFieldTests(const std::string& path) {
    DBFHandle dbf = DBFCreate(path.c_str());
    if (!dbf) throw std::runtime_error("Cannot create export fixture");
    const int fid = DBFAddField(dbf, "FID", FTInteger, 8, 0);
    for (int road = 0; road < 4; ++road) DBFWriteIntegerAttribute(dbf, road, fid, road);
    DBFClose(dbf);

    ShapeFileAccessor accessor;
    prepare(accessor, {{-0.25, 0, 0, 0}, {0, 0, 0.25, 0},
                       {0, -0.25, 0, 0}, {0, 0, 0, 0.25}});
    Calculation calculation;
    initialize(calculation, accessor,
               "n,0,0.05,0.25,0.5,10.1,10.9,400,800,1000,0.12345671,0.12345672");
    calculation.dbfFilePath = path;
    calculation.isWgt = true;
    for (int road = 0; road < 4; ++road) {
        calculation.weight["ones"][road] = 1;
        calculation.weight["zeros"][road] = 0;
    }
    calculation.calculateDDL(accessor);
    calculation.OutputData(accessor);
    const auto aliases = calculation.DBFFieldSources;

    dbf = DBFOpen(path.c_str(), "rb");
    if (!dbf) throw std::runtime_error("Cannot read export fixture");
    const int count = DBFGetFieldCount(dbf);
    const int expected = 2 + 4 * static_cast<int>(calculation.MRLimitSet2.size());
    if (count != expected) throw std::runtime_error("Export fields were lost or duplicated");
    for (const auto& entry : aliases) {
        const int field = DBFGetFieldIndex(dbf, entry.first.second.c_str());
        if (field < 0 || entry.first.second.size() > 10)
            throw std::runtime_error("Invalid DBF alias");
        for (const auto& result : calculation.TempModifyData.at(entry.second)) {
            if (std::fabs(DBFReadDoubleAttribute(dbf, result.first, field) - result.second) > 0.000051)
                throw std::runtime_error("DBF alias overwrote another parameter result");
        }
    }
    for (const char* name : {"DL45an", "DL45a0", "DL45a0p05", "DL45a0p25", "DL45a0p5",
                             "DL45a10p1", "DL45a10p9", "DL45a400", "DL45a800", "DL45a1k"}) {
        if (DBFGetFieldIndex(dbf, name) < 0)
            throw std::runtime_error(std::string("Missing radius field: ") + name);
    }
    near(DBFReadDoubleAttribute(dbf, 0, DBFGetFieldIndex(dbf, "DL45a0p25")), 0.4,
         "exported partial-length DDL");
    DBFClose(dbf);

    calculation.OutputData(accessor);
    dbf = DBFOpen(path.c_str(), "rb");
    if (!dbf || DBFGetFieldCount(dbf) != count)
        throw std::runtime_error("Repeated export must reuse fields");
    DBFClose(dbf);

    Calculation independent;
    initialize(independent, accessor, "0.12345672");
    independent.dbfFilePath = path;
    independent.calculateDDL(accessor);
    independent.OutputData(accessor);
    for (const auto& entry : independent.DBFFieldSources) {
        if (aliases.at(entry.first) != entry.second)
            throw std::runtime_error("Alias changed in an independent run");
    }
    dbf = DBFOpen(path.c_str(), "rb");
    if (!dbf || DBFGetFieldCount(dbf) != count)
        throw std::runtime_error("Independent export duplicated an alias");
    DBFClose(dbf);
    std::cout << "Fractional radii, long aliases, weights, GUI full names and repeated export passed\n";
}

void realNetworkTests(const std::string& path) {
    ShapeFileAccessor accessor;
    AttributesData attributes;
    accessor.multiThreadReadFile(path, "FID", attributes);
    accessor.init(path, attributes);
    accessor.ProcessShapeFile();
    Calculation single, threaded, reversed, independent;
    initialize(single, accessor, "n,400,800");
    initialize(threaded, accessor, "n,400,800");
    single.subRoadVec.clear();
    const int count = static_cast<int>(accessor.roadID.size());
    for (int i = 0; i < count; i += std::max(1, count / 8)) single.subRoadVec.push_back(i);
    threaded.subRoadVec = single.subRoadVec;
    single.calculateDDL(accessor);
    threaded.MultiCalculate(accessor, 0);
    compare(single, threaded);
    for (const auto& radius : single.DDL_all) {
        initialize(independent, accessor, radius.first < 0 ? "n" : std::to_string(radius.first));
        independent.subRoadVec = single.subRoadVec;
        independent.calculateDDL(accessor);
        compare(independent, single);
    }
    for (int road : accessor.roadID) {
        std::swap(attributes.AttributesDouble["x1"][road], attributes.AttributesDouble["x2"][road]);
        std::swap(attributes.AttributesDouble["y1"][road], attributes.AttributesDouble["y2"][road]);
    }
    ShapeFileAccessor backwards;
    backwards.init(path, attributes);
    backwards.ProcessShapeFile();
    initialize(reversed, backwards, "n,400,800");
    reversed.subRoadVec = single.subRoadVec;
    reversed.calculateDDL(backwards);
    compare(single, reversed);
    std::cout << "Real network: " << count << " roads; " << single.subRoadVec.size()
              << " origins, three radii, reversed geometry and worker consistency passed\n";
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 3) {
            if (std::string(argv[1]) == "--export-fields") {
                exportFieldTests(argv[2]);
                return 0;
            }
            ShapeFileAccessor accessor;
            AttributesData attributes;
            const bool missing = std::string(argv[1]) == "--missing-dbf";
            int caught = 0;
            for (bool worker : {false, true}) {
                try {
                    const std::string field = missing ? "FID" : "missing_field";
                    if (worker) accessor.multiThreadReadFile(argv[2], field, attributes);
                    else {
                        std::map<int, int> refs, ids;
                        accessor.generateDispalyStream(argv[2], field, attributes, refs, ids);
                    }
                } catch (const std::runtime_error&) {
                    ++caught;
                }
            }
            if (caught != 2) throw std::runtime_error("Importer unexpectedly accepted malformed input");
            std::cout << "GUI and worker importer errors propagated without terminating the process\n";
            return 0;
        }
        crossTests();
        unequalLengthTest();
        boundaryAndWeightTests();
        if (argc == 2) realNetworkTests(argv[1]);
        std::cout << "DDL cross, orientation, fractional-length, boundary, radius, weight and worker tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
