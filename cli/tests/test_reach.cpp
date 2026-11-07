#include "Calculation.h"
#include "ShapeFileAccessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

bool NeedStop = false;
int loadedCount = 0;

namespace {
using Segment = std::array<double, 4>;
const double tolerance = 1e-9; // Metres, before DBF rounding.
enum class Measure { MR, DR, JNR, MDR, NetMDR, SubsetMDR, SubsetMDRGeometry };

struct Case {
    std::string name;
    Measure measure;
    double radius;
    int limit;
    int degree;
    std::vector<double> expected;
    std::string sweepRadii;
    std::string sweepLimits;
    std::set<int> subsetSources;
};

struct Fixture {
    std::string name;
    std::vector<Segment> segments;
    std::vector<Case> cases;
    // Empty means every source. Selected sources permit a compact analytic
    // oracle for a competing-route network without recording engine output.
    std::vector<int> sources;
};

// These literals are hand-derived, not snapshots of UrConnect output.
// Geometry, endpoint distances and arithmetic: REACH_VALIDATION.md.
Fixture treeFixture() {
    return {"tree", {{-10, 0, 0, 0}, {0, 0, 20, 0}, {0, 0, 0, 30},
                     {20, 0, 20, 40}, {100, 0, 150, 0}}, {
        {"MR_R2", Measure::MR, 2, 0, 0, {4, 4, 4, 4, 4}},
        {"MR_R15", Measure::MR, 15, 0, 0, {30, 35, 30, 30, 30}},
        {"MR_R35", Measure::MR, 35, 0, 0, {70, 80, 60, 55, 50}},
        {"MR_R60", Measure::MR, 60, 0, 0, {95, 100, 85, 90, 50}},
        {"MR_unlimited", Measure::MR, -1, 0, 0, {100, 100, 100, 100, 50}},
        {"DR_T0_A45", Measure::DR, -1, 0, 0, {30, 30, 30, 40, 50}},
        {"DR_T1_A45", Measure::DR, -1, 1, 0, {100, 100, 60, 70, 50}},
        {"DR_T2_A45", Measure::DR, -1, 2, 0, {100, 100, 100, 100, 50}},
        {"JNR_D2_J0", Measure::JNR, -1, 0, 2, {10, 20, 30, 40, 50}},
        {"JNR_D2_J1", Measure::JNR, -1, 1, 2, {60, 100, 60, 60, 50}},
        {"JNR_D2_J2", Measure::JNR, -1, 2, 2, {100, 100, 100, 100, 50}},
        {"JNR_D3_J0", Measure::JNR, -1, 0, 3, {10, 60, 30, 60, 50}},
        {"JNR_D3_J1", Measure::JNR, -1, 1, 3, {100, 100, 100, 100, 50}},
        {"JNR_D3_J2", Measure::JNR, -1, 2, 3, {100, 100, 100, 100, 50}},
        {"MDR_R2_T0_A45", Measure::MDR, 2, 0, 0, {4, 4, 4, 4, 4}},
        {"MDR_R2_T1_A45", Measure::MDR, 2, 1, 0, {4, 4, 4, 4, 4}},
        {"MDR_R2_T2_A45", Measure::MDR, 2, 2, 0, {4, 4, 4, 4, 4}},
        {"MDR_R15_T0_A45", Measure::MDR, 15, 0, 0, {20, 25, 30, 30, 30}},
        {"MDR_R15_T1_A45", Measure::MDR, 15, 1, 0, {30, 35, 30, 30, 30}},
        {"MDR_R15_T2_A45", Measure::MDR, 15, 2, 0, {30, 35, 30, 30, 30}},
        {"MDR_R35_T0_A45", Measure::MDR, 35, 0, 0, {30, 30, 30, 40, 50}},
        {"MDR_R35_T1_A45", Measure::MDR, 35, 1, 0, {70, 80, 60, 55, 50}},
        {"MDR_R35_T2_A45", Measure::MDR, 35, 2, 0, {70, 80, 60, 55, 50}},
        {"MDR_R60_T0_A45", Measure::MDR, 60, 0, 0, {30, 30, 30, 40, 50}},
        {"MDR_R60_T1_A45", Measure::MDR, 60, 1, 0, {95, 100, 60, 70, 50}},
        {"MDR_R60_T2_A45", Measure::MDR, 60, 2, 0, {95, 100, 85, 90, 50}}
    }};
}

Fixture squareFixture() {
    Fixture fixture{"square", {{0, 0, 10, 0}, {10, 0, 10, 10},
                                {10, 10, 0, 10}, {0, 10, 0, 0}}, {}};
    const auto add = [&](const std::string& name, Measure measure, double radius,
                         int limit, int degree, double expected) {
        fixture.cases.push_back({name, measure, radius, limit, degree,
                                 std::vector<double>(4, expected)});
    };
    add("MR_R2", Measure::MR, 2, 0, 0, 4);
    add("MR_R7", Measure::MR, 7, 0, 0, 14);
    add("MR_R17", Measure::MR, 17, 0, 0, 34);
    add("MR_R25", Measure::MR, 25, 0, 0, 40);
    add("MR_unlimited", Measure::MR, -1, 0, 0, 40);
    for (int turns = 0; turns <= 2; ++turns) {
        const double directional[] = {10, 30, 40};
        const double r7[] = {10, 14, 14};
        const double r17[] = {10, 30, 34};
        const std::string suffix = std::to_string(turns);
        add("DR_T" + suffix + "_A45", Measure::DR, -1, turns, 0, directional[turns]);
        add("JNR_D2_J" + suffix, Measure::JNR, -1, turns, 2, directional[turns]);
        add("JNR_D3_J" + suffix, Measure::JNR, -1, turns, 3, 40);
        add("MDR_R2_T" + suffix + "_A45", Measure::MDR, 2, turns, 0, 4);
        add("MDR_R7_T" + suffix + "_A45", Measure::MDR, 7, turns, 0, r7[turns]);
        add("MDR_R17_T" + suffix + "_A45", Measure::MDR, 17, turns, 0, r17[turns]);
        add("MDR_R25_T" + suffix + "_A45", Measure::MDR, 25, turns, 0, directional[turns]);
    }
    return fixture;
}

Fixture competingRoutesFixture() {
    Fixture fixture{"competing_routes", {
        {-10, 0, 0, 0}, {0, 0, 40, 0}, {40, 0, 40, 20},
        {40, 20, 10, 20}, {10, 20, 0, 20}, {0, 0, 0, 10},
        {0, 10, 10, 10}, {10, 10, 10, 20}
    }, {}, {0}};
    // Source S is the first edge. At two turns, the DR subset excludes F
    // (the last edge), so metric search cannot use F as a short cut.
    // Each row contains the DR-first MR totals at turn limits 0, 1, 2, 3.
    const double radii[] = {2.5, 35, 70, 100, -1};
    const double expected[][4] = {
        {5, 5, 5, 5}, {40, 50, 60, 70}, {50, 80, 95, 140},
        {50, 80, 125, 140}, {50, 80, 130, 140}
    };
    for (size_t r = 0; r < 5; ++r) {
        for (int turns = 0; turns <= 3; ++turns) {
            const std::string name = "MDR_R" + std::to_string(radii[r])
                + "_T" + std::to_string(turns) + "_A45";
            fixture.cases.push_back({name, Measure::MDR, radii[r], turns, 0,
                                     {expected[r][turns]}});
            // Deliberately unsorted input and a mixture of finite/unlimited
            // radii exercise the same values in a parameter sweep.
            fixture.cases.push_back({name + "_sweep", Measure::MDR, radii[r], turns, 0,
                                     {expected[r][turns]}, "70,n,2.5,100,35", "3,0,2,1"});
        }
    }
    const double directional[] = {50, 80, 130, 140};
    for (int turns = 0; turns <= 3; ++turns)
        fixture.cases.push_back({"DR_T" + std::to_string(turns) + "_A45", Measure::DR,
                                 -1, turns, 0, {directional[turns]}});
    for (size_t r = 0; r < 5; ++r) {
        const std::string suffix = "_R" + std::to_string(radii[r]) + "_T2_A45";
        fixture.cases.push_back({"Net_MDR_geometry" + suffix, Measure::NetMDR,
                                 radii[r], 2, 0, {expected[r][2]}});
        fixture.cases.push_back({"Subset_MDR_total" + suffix, Measure::SubsetMDR,
                                 radii[r], 2, 0, {expected[r][2]}});
        fixture.cases.push_back({"Subset_MDR_geometry" + suffix, Measure::SubsetMDRGeometry,
                                 radii[r], 2, 0, {expected[r][2]}});
    }
    // Source S reaches A's first 10 m; source A reaches its central 30 m.
    // Their union on A is 35 m, plus full S and D (10 m each).
    fixture.cases.push_back({"Subset_MDR_union_total_R15_T2_sources0_1", Measure::SubsetMDR,
                             15, 2, 0, {55}, "", "", {0, 1}});
    fixture.cases.push_back({"Subset_MDR_union_geometry_R15_T2_sources0_1", Measure::SubsetMDRGeometry,
                             15, 2, 0, {55}, "", "", {0, 1}});
    return fixture;
}

void prepare(ShapeFileAccessor& accessor, std::vector<Segment> segments, int orientation) {
    AttributesData attributes;
    attributes.AttributesNames = {"Ref", "ID", "x1", "y1", "x2", "y2"};
    for (size_t i = 0; i < segments.size(); ++i) {
        if (orientation == 1 || (orientation == 2 && i % 2 == 0)) {
            std::swap(segments[i][0], segments[i][2]);
            std::swap(segments[i][1], segments[i][3]);
        }
        attributes.AttributesDouble["Ref"][i] = i;
        attributes.AttributesDouble["ID"][i] = i;
        const char* names[] = {"x1", "y1", "x2", "y2"};
        for (int j = 0; j < 4; ++j)
            attributes.AttributesDouble[names[j]][i] = segments[i][j];
    }
    accessor.init("hand-derived-reach.shp", attributes);
    accessor.ProcessShapeFile();
}

double coverageLength(const ShapeFileAccessor& accessor, const std::set<int>& roads,
                      const std::map<int, std::vector<double>>& partial) {
    double length = 0;
    for (int road : roads)
        if (!partial.count(road)) length += accessor.Length.at(road);
    for (const auto& entry : partial) {
        if (entry.second.empty() || entry.second.size() % 4 != 0)
            throw std::runtime_error("Invalid partial geometry");
        const auto& original = accessor.Route.at(entry.first);
        const double dx = original[2] - original[0], dy = original[3] - original[1];
        const double squaredLength = dx * dx + dy * dy;
        for (size_t i = 0; i < entry.second.size(); i += 4) {
            for (size_t end = i; end < i + 4; end += 2) {
                const double x = entry.second[end] - original[0];
                const double y = entry.second[end + 1] - original[1];
                const double fraction = (x * dx + y * dy) / squaredLength;
                if (std::fabs(x * dy - y * dx) > tolerance ||
                    fraction < -tolerance || fraction > 1 + tolerance)
                    throw std::runtime_error("Partial geometry leaves its source segment");
            }
            length += std::hypot(entry.second[i + 2] - entry.second[i],
                                 entry.second[i + 3] - entry.second[i + 1]);
        }
    }
    return length;
}

std::map<int, double> calculate(const Case& test, ShapeFileAccessor& accessor) {
    Calculation calculation;
    calculation.clearOldData();
    std::vector<std::string> weights;
    Graph& graph = *accessor.ProcessShapeFile();
    const std::string path = "hand-derived-reach.shp";
    const std::string radius = test.sweepRadii.empty()
        ? (test.radius < 0 ? "n" : std::to_string(test.radius)) : test.sweepRadii;
    const std::string limit = test.sweepLimits.empty()
        ? std::to_string(test.limit) : test.sweepLimits;
    switch (test.measure) {
    case Measure::MR:
        calculation.init_MR_para(accessor, graph, path, radius, false, false, "", weights);
        break;
    case Measure::DR:
        calculation.init_DR_para(accessor, graph, path, "", "45", limit, false, false, "", weights);
        break;
    case Measure::JNR:
        calculation.init_JnR_para(accessor, graph, path, std::to_string(test.degree), limit,
                                 false, "", false, weights);
        break;
    case Measure::MDR:
    case Measure::SubsetMDR:
    case Measure::SubsetMDRGeometry:
        calculation.init_MDR_para(accessor, graph, path, radius, "45", limit, false, false, "", weights);
        break;
    case Measure::NetMDR:
        calculation.init_Net_DR_para(accessor, graph, path, radius, "45", limit, "", weights);
        break;
    }
    calculation.setMultiPara(accessor, 1, INT_MAX - 1, 1);
    // Same entry points called by the application and command-line interface.
    switch (test.measure) {
    case Measure::MR:
        calculation.calculateMR(accessor);
        return calculation.MR_all.at(test.radius);
    case Measure::DR:
        calculation.calculateDR(accessor);
        return calculation.DR_all.at({test.limit, -1});
    case Measure::JNR:
        calculation.calculateJncR(accessor);
        return calculation.JncR_all.at(test.limit);
    case Measure::MDR:
        calculation.calculateMDR(accessor);
        return calculation.DR_all.at({test.limit, test.radius});
    case Measure::NetMDR: {
        calculation.subFromIDVec = {0};
        calculation.Net_calculateDR(accessor, test.limit, test.radius);
        const auto& records = calculation.NetreachData.at("MDR");
        const size_t count = records.From_OID.size();
        if (records.To_OID.size() != count || records.Radius.size() != count ||
            records.DirChg.size() != count || records.Relen.size() != count)
            throw std::runtime_error("Network reach attribute rows are misaligned");
        return {{0, coverageLength(accessor, calculation.reachRoad_all.at(0),
                                    calculation.partInRoads_all[0])}};
    }
    case Measure::SubsetMDR:
    case Measure::SubsetMDRGeometry: {
        subsetPara parameters;
        parameters.isMDR = true;
        parameters.MDR_Para.mr_limit = test.radius;
        parameters.MDR_Para.angle_limit = 45;
        parameters.MDR_Para.dc_limit = test.limit;
        std::set<int> sources = test.subsetSources.empty() ? std::set<int>{0} : test.subsetSources;
        Calculation::get_subset_collective_reach(&calculation, accessor, parameters, sources);
        const double value = test.measure == Measure::SubsetMDR
            ? calculation.results.result_1.sumMDR_2
            : coverageLength(accessor, calculation.subset_reachRoad_all.at("MDR"),
                             calculation.subset_partInRoads_all.at("MDR"));
        return {{0, value}};
    }
    }
    throw std::runtime_error("Unknown reach measure");
}

int run(const std::string& prefix) {
    std::ofstream csv(prefix + ".csv"), summary(prefix + ".md");
    if (!csv || !summary) throw std::runtime_error("Cannot create validation reports: " + prefix);
    csv << std::setprecision(17)
        << "fixture,orientation,case,segment_id,expected_m,actual_m,absolute_error_m,passed\n";
    summary << std::setprecision(17)
            << "# Reach correctness: hand-derived synthetic networks\n\n"
            << "Unweighted shared-engine results before export rounding. Tolerance: "
            << tolerance << " m. Reference derivation: `cli/tests/REACH_VALIDATION.md`.\n\n"
            << "| Fixture | Orientation | Case | Segments | Maximum absolute error (m) | Result |\n"
            << "|---|---|---|---:|---:|---|\n";
    size_t checked = 0, failures = 0, caseCount = 0;
    double maximum = 0;
    const char* orientations[] = {"original", "reversed", "alternating"};
    for (const auto& fixture : {treeFixture(), squareFixture(), competingRoutesFixture()}) {
        std::vector<int> sources = fixture.sources;
        if (sources.empty())
            for (size_t i = 0; i < fixture.segments.size(); ++i)
                sources.push_back(static_cast<int>(i));
        for (int orientation = 0; orientation < 3; ++orientation) {
            for (const auto& test : fixture.cases) {
                ShapeFileAccessor accessor;
                prepare(accessor, fixture.segments, orientation);
                if (test.expected.size() != sources.size())
                    throw std::runtime_error("Incomplete reference vector: " + test.name);
                const auto actual = calculate(test, accessor);
                const bool selectedMode = test.measure == Measure::NetMDR ||
                    test.measure == Measure::SubsetMDR || test.measure == Measure::SubsetMDRGeometry;
                bool passed = actual.size() == (selectedMode ? sources.size() : fixture.segments.size());
                double caseMaximum = passed ? 0 : std::numeric_limits<double>::infinity();
                for (size_t row = 0; row < test.expected.size(); ++row) {
                    const auto found = actual.find(sources[row]);
                    const double value = found == actual.end()
                        ? std::numeric_limits<double>::quiet_NaN() : found->second;
                    const double error = std::isfinite(value) ? std::fabs(value - test.expected[row])
                        : std::numeric_limits<double>::infinity();
                    const bool rowPassed = error <= tolerance;
                    passed = passed && rowPassed;
                    caseMaximum = std::max(caseMaximum, error);
                    csv << fixture.name << ',' << orientations[orientation] << ',' << test.name
                        << ',' << sources[row] << ',' << test.expected[row] << ',' << value << ',' << error
                        << ',' << (rowPassed ? "true" : "false") << '\n';
                    if (!rowPassed)
                        std::cerr << fixture.name << '/' << orientations[orientation] << '/' << test.name
                                  << " segment " << sources[row] << ": expected " << test.expected[row]
                                  << ", actual " << value << ", absolute error " << error << " m\n";
                    ++checked;
                }
                ++caseCount;
                if (!passed) ++failures;
                maximum = std::max(maximum, caseMaximum);
                summary << "| " << fixture.name << " | " << orientations[orientation] << " | "
                        << test.name << " | " << test.expected.size() << " | " << caseMaximum
                        << " | " << (passed ? "PASS" : "FAIL") << " |\n";
            }
        }
    }
    summary << "\nChecked " << checked << " segment results in " << caseCount
            << " cases. Maximum absolute error: " << maximum << " m. Failed cases: " << failures << ".\n";
    csv.flush();
    summary.flush();
    if (!csv || !summary) throw std::runtime_error("Failed to write validation reports");
    std::cout << "Reach correctness: " << checked << " segment results, " << caseCount
              << " cases, maximum absolute error " << maximum << " m, " << failures << " failed cases\n";
    return failures == 0 ? 0 : 1;
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3 || std::string(argv[1]) != "--report")
            throw std::runtime_error("Usage: urconnect-test-reach --report <output-prefix>");
        return run(argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "Reach correctness validation failed: " << error.what() << '\n';
        return 1;
    }
}
