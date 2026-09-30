// urconnect-cli: headless command-line interface for the UrConnect
// street-network analysis engine (reach / directional distance / OD paths).
//
// This program runs the same analysis code paths as the GUI
// (MainWindow::calculate*_Single_thread in depthmapX/mainwindow.cpp)
// without any Qt dependency, so it can be embedded in reproducible
// pipelines, CI jobs and servers.

#include "ShapeFileAccessor.h"
#include "Calculation.h"
#include "version_defs.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// Globals referenced by the engine (defined in mainwindow.cpp in the GUI).
bool NeedStop = false;
int loadedCount = 0;

namespace {

const char *kUsage = R"(
urconnect-cli - headless UrConnect network analysis

Usage:
  urconnect-cli <command> <input.shp> [options]

Whole-network commands (append result columns to the input .dbf):
  reach     Metric reach              --radius 400,800 [--junction-turns N] [--weight ATTR,...]
  md        Metric distance           --radius 400,800 [--junction-turns N] [--weight ATTR,...]
  dr        Directional reach         --angle 45[,90] --turns 0,1,2 [--junction-turns N] [--weight ATTR,...]
  ddl       Directional distance      --radius 400,800 --angle 45[,90] [--weight ATTR,...]
  mdr       Metric + directional reach --radius 400,800 --angle 45[,90] --turns 0,1,2 [--weight ATTR,...]
  jnr       Junction reach            --junction-degree 3 --junction-limit 2,3 [--weight ATTR,...]
  jnd       Junction distance         --radius 400,800 --junction-degree 3 [--weight ATTR,...]

Origin-based commands:
  netreach  Reachable subset from origin road(s); writes Reach_* shapefiles
              --from 1,2,3 --type mr|dr|jnr plus the matching parameters above
  stepdepth Distance from origin road(s) to every road; writes a StepD_* column
              --from 1,2,3 --type mr|dr|jnr [--angle A] [--junction-degree N]
  od        Shortest path(s) between roads; writes Path_* shapefiles, or a
              result CSV when --pairs is used
              --from 1 [--to 5] [--pairs pairs.csv] --type mr|dr|jnr
              [--angle A] [--junction-turns N]

General options:
  -h, --help      Show this help
  --version       Show version

Parameter lists accept comma-separated values (e.g. --radius 400,800,1000),
each value producing its own result column, exactly as in the GUI.
)";

// Same sanitisation as the GUI's transform() in mainwindow.cpp.
std::string transformName(std::string newstr) {
    for (int i = 0; i < 2; i++) {
        if (newstr.find(".csv") != std::string::npos) {
            newstr = newstr.substr(0, newstr.length() - 4);
            while (newstr.find("\\") != std::string::npos) {
                size_t pos = newstr.find("\\");
                newstr = newstr.substr(pos + 1, newstr.length());
            }
        }
        if (newstr.find(">") != std::string::npos) {
            size_t pos = newstr.find(">");
            newstr.replace(pos, 1, " more than ");
        }
        if (newstr.find("<") != std::string::npos) {
            size_t pos = newstr.find("<");
            newstr.replace(pos, 1, " less than ");
        }
        std::replace(newstr.begin(), newstr.end(), ';', '-');
    }
    return newstr;
}

std::vector<std::string> splitList(const std::string &str) {
    std::vector<std::string> out;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) {
            out.push_back(item);
        }
    }
    return out;
}

struct Options {
    std::string command;
    std::string input;
    std::map<std::string, std::string> values;
};

bool parseArgs(int argc, char **argv, Options &opts, std::string &error) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            opts.command = "help";
            return true;
        }
        if (arg == "--version") {
            opts.command = "version";
            return true;
        }
        if (arg.compare(0, 2, "--") == 0) {
            std::string key, value;
            size_t eq = arg.find('=');
            if (eq != std::string::npos) {
                key = arg.substr(2, eq - 2);
                value = arg.substr(eq + 1);
            } else {
                key = arg.substr(2);
                if (i + 1 >= argc) {
                    error = "missing value for option --" + key;
                    return false;
                }
                value = argv[++i];
            }
            if (opts.values.count(key) != 0) {
                error = "duplicate option --" + key;
                return false;
            }
            opts.values[key] = value;
        } else if (opts.command.empty()) {
            opts.command = arg;
        } else if (opts.input.empty()) {
            opts.input = arg;
        } else {
            error = "unexpected argument: " + arg;
            return false;
        }
    }
    return true;
}

bool hasOpt(const Options &opts, const std::string &key) {
    return opts.values.count(key) > 0;
}

std::string getOpt(const Options &opts, const std::string &key, const std::string &fallback = "") {
    auto it = opts.values.find(key);
    return it == opts.values.end() ? fallback : it->second;
}

bool requireOpt(const Options &opts, const std::string &key, std::string &error) {
    if (!hasOpt(opts, key) || getOpt(opts, key).empty()) {
        error = "command '" + opts.command + "' requires --" + key;
        return false;
    }
    return true;
}

bool fileExists(const std::string &path) {
    std::ifstream f(path.c_str(), std::ios::binary);
    return f.good();
}

std::vector<std::string> weightAttributes(const Options &opts);

std::string trim(const std::string &text) {
    size_t first = 0;
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first]))) {
        ++first;
    }
    size_t last = text.size();
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) {
        --last;
    }
    return text.substr(first, last - first);
}

bool parseNumber(const std::string &text, bool integerOnly, bool allowUnlimited, double &value) {
    if (allowUnlimited && text == "n") {
        value = -1;
        return true;
    }
    if (text.empty()) {
        return false;
    }

    errno = 0;
    char *end = nullptr;
    value = std::strtod(text.c_str(), &end);
    if (errno != 0 || end == text.c_str() || *end != '\0' || !std::isfinite(value) || value < 0) {
        return false;
    }
    return !integerOnly || (std::floor(value) == value && value <= INT_MAX);
}

bool validateNumberList(const Options &opts, const std::string &key, bool integerOnly,
                        bool allowUnlimited, std::string &error) {
    if (!hasOpt(opts, key)) {
        return true;
    }

    const std::string value = getOpt(opts, key);
    if (value.empty() || value.front() == ',' || value.back() == ',' || value.find(",,") != std::string::npos) {
        error = "option --" + key + " requires a non-empty comma-separated list";
        return false;
    }

    for (const std::string &item : splitList(value)) {
        double parsed = 0;
        if (!parseNumber(item, integerOnly, allowUnlimited, parsed)) {
            error = "invalid value '" + item + "' for --" + key;
            return false;
        }
    }
    return true;
}

bool validateOptionNames(const Options &opts, std::string &error) {
    static const std::map<std::string, std::set<std::string>> allowed = {
        {"reach", {"radius", "junction-turns", "weight"}},
        {"md", {"radius", "junction-turns", "weight"}},
        {"dr", {"angle", "turns", "junction-turns", "weight"}},
        {"ddl", {"radius", "angle", "weight"}},
        {"mdr", {"radius", "angle", "turns", "junction-turns", "weight"}},
        {"jnr", {"junction-degree", "junction-limit", "weight"}},
        {"jnd", {"radius", "junction-degree", "weight"}},
        {"netreach", {"from", "type", "radius", "angle", "turns", "junction-turns",
                      "junction-degree", "junction-limit", "weight"}},
        {"stepdepth", {"from", "type", "angle", "junction-degree", "weight"}},
        {"od", {"from", "to", "pairs", "type", "angle", "junction-turns", "weight"}}
    };

    auto command = allowed.find(opts.command);
    if (command == allowed.end()) {
        return true;
    }
    for (const auto &entry : opts.values) {
        if (command->second.count(entry.first) == 0) {
            error = "unknown option --" + entry.first + " for command '" + opts.command + "'";
            return false;
        }
    }
    return true;
}

bool validateOptions(const Options &opts, std::string &error) {
    if (!validateOptionNames(opts, error)) {
        return false;
    }

    if (!validateNumberList(opts, "radius", false, true, error) ||
        !validateNumberList(opts, "angle", false, false, error) ||
        !validateNumberList(opts, "turns", true, true, error) ||
        !validateNumberList(opts, "junction-turns", true, false, error) ||
        !validateNumberList(opts, "junction-degree", true, false, error) ||
        !validateNumberList(opts, "junction-limit", true, true, error)) {
        return false;
    }

    if ((hasOpt(opts, "junction-turns") && getOpt(opts, "junction-turns").find(',') != std::string::npos) ||
        (hasOpt(opts, "junction-degree") && getOpt(opts, "junction-degree").find(',') != std::string::npos)) {
        error = "--junction-turns and --junction-degree each accept one value";
        return false;
    }
    if ((opts.command == "stepdepth" || opts.command == "od" || opts.command == "netreach") &&
        hasOpt(opts, "angle") && getOpt(opts, "angle").find(',') != std::string::npos) {
        error = "command '" + opts.command + "' accepts one --angle value";
        return false;
    }

    if (hasOpt(opts, "type")) {
        const std::string type = getOpt(opts, "type");
        if (type != "mr" && type != "dr" && type != "jnr") {
            error = "invalid --type '" + type + "' (expected mr, dr or jnr)";
            return false;
        }
    }
    if (hasOpt(opts, "pairs") && (hasOpt(opts, "from") || hasOpt(opts, "to"))) {
        error = "--pairs cannot be combined with --from or --to";
        return false;
    }
    if (hasOpt(opts, "to") && !hasOpt(opts, "from")) {
        error = "--to requires --from";
        return false;
    }
    return true;
}

bool validateRoadIds(const std::string &text, const ShapeFileAccessor &FA,
                     const std::string &option, std::string &error) {
    if (text.empty() || text.front() == ',' || text.back() == ',' || text.find(",,") != std::string::npos) {
        error = "option --" + option + " requires a comma-separated list of road IDs";
        return false;
    }

    for (const std::string &item : splitList(text)) {
        double parsed = 0;
        if (!parseNumber(item, true, false, parsed) || parsed > INT_MAX) {
            error = "invalid road ID '" + item + "' for --" + option;
            return false;
        }
        int road = static_cast<int>(parsed);
        if (FA.roadID.count(road) == 0) {
            error = "road ID " + item + " from --" + option + " is outside the input network";
            return false;
        }
    }
    return true;
}

bool validatePairsFile(const std::string &path, const ShapeFileAccessor &FA, std::string &error) {
    std::ifstream input(path.c_str());
    if (!input) {
        error = "OD pairs CSV not found or unreadable: " + path;
        return false;
    }

    std::string line;
    if (!std::getline(input, line)) {
        error = "OD pairs CSV is empty: " + path;
        return false;
    }

    size_t row = 1;
    size_t validPairs = 0;
    while (std::getline(input, line)) {
        ++row;
        if (trim(line).empty()) {
            continue;
        }
        std::stringstream fields(line);
        std::string from;
        std::string to;
        if (!std::getline(fields, from, ',') || !std::getline(fields, to, ',')) {
            error = "OD pairs CSV row " + std::to_string(row) + " must contain origin,destination";
            return false;
        }
        from = trim(from);
        to = trim(to);
        double fromValue = 0;
        double toValue = 0;
        if (!parseNumber(from, true, false, fromValue) || !parseNumber(to, true, false, toValue) ||
            fromValue > INT_MAX || toValue > INT_MAX ||
            FA.roadID.count(static_cast<int>(fromValue)) == 0 ||
            FA.roadID.count(static_cast<int>(toValue)) == 0) {
            error = "OD pairs CSV row " + std::to_string(row) + " contains an invalid road ID";
            return false;
        }
        ++validPairs;
    }
    if (validPairs == 0) {
        error = "OD pairs CSV contains no data rows: " + path;
        return false;
    }
    return true;
}

bool validateLoadedOptions(const Options &opts, const ShapeFileAccessor &FA, std::string &error) {
    for (const std::string &attribute : weightAttributes(opts)) {
        if (FA.myAttributes.AttributesDouble.count(attribute) == 0) {
            error = "weight attribute not found or not numeric: " + attribute;
            return false;
        }
    }
    if (hasOpt(opts, "from") && !validateRoadIds(getOpt(opts, "from"), FA, "from", error)) {
        return false;
    }
    if (hasOpt(opts, "to") && !validateRoadIds(getOpt(opts, "to"), FA, "to", error)) {
        return false;
    }
    if (hasOpt(opts, "pairs") && !validatePairsFile(getOpt(opts, "pairs"), FA, error)) {
        return false;
    }
    return true;
}

// Loads a shapefile the same way the GUI does (GlobalMap::readFile with the
// "FID" id field + ShapeFileAccessor::init + ProcessShapeFile).
bool loadNetwork(const std::string &shpPath, ShapeFileAccessor &FA, Graph *&g, std::string &error) {
    if (shpPath.size() < 4) {
        error = "input must be a .shp file: " + shpPath;
        return false;
    }
    std::string extension = shpPath.substr(shpPath.size() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension != ".shp") {
        error = "input must be a .shp file: " + shpPath;
        return false;
    }
    if (!fileExists(shpPath)) {
        error = "input shapefile not found: " + shpPath;
        return false;
    }
    std::string dbfPath = shpPath.substr(0, shpPath.length() - 4) + ".dbf";
    std::string shxPath = shpPath.substr(0, shpPath.length() - 4) + ".shx";
    if (!fileExists(dbfPath)) {
        error = "attribute table not found: " + dbfPath;
        return false;
    }
    if (!fileExists(shxPath)) {
        error = "shapefile index not found: " + shxPath;
        return false;
    }

    SHPHandle shapeHandle = SHPOpen(shpPath.c_str(), "rb");
    if (shapeHandle == nullptr) {
        error = "invalid or unreadable shapefile: " + shpPath;
        return false;
    }
    SHPClose(shapeHandle);
    DBFHandle dbfHandle = DBFOpen(dbfPath.c_str(), "rb+");
    if (dbfHandle == nullptr) {
        error = "invalid or unwritable attribute table: " + dbfPath;
        return false;
    }
    DBFClose(dbfHandle);

    AttributesData attributes;
    std::map<int, int> refToId, idToRef;
    ShapeFileAccessor loader;
    loader.init(shpPath, attributes);
    // Returns the entity count on success, -1 on failure.
    if (loader.generateDispalyStream(shpPath, "FID", attributes, refToId, idToRef) < 0) {
        error = "failed to read shapefile: " + shpPath;
        return false;
    }

    FA.init(shpPath, attributes);
    g = FA.ProcessShapeFile();
    if (g == nullptr) {
        error = "failed to build network graph from: " + shpPath;
        return false;
    }
    if (FA.roadID.empty()) {
        error = "input contains no two-point road segments: " + shpPath;
        return false;
    }
    std::cerr << "Loaded " << FA.roadID.size() << " road segments from " << shpPath << std::endl;
    return true;
}

std::vector<std::string> weightAttributes(const Options &opts) {
    return splitList(getOpt(opts, "weight"));
}

bool loadAndValidateNetwork(const Options &opts, ShapeFileAccessor &FA, Graph *&g, std::string &error) {
    return loadNetwork(opts.input, FA, g, error) && validateLoadedOptions(opts, FA, error);
}

} // namespace
namespace {

// ---------------------------------------------------------------------------
// Whole-network analyses (mirror MainWindow::calculate*_Single_thread)
// ---------------------------------------------------------------------------

int runReach(const Options &opts) {
    std::string error;
    if (!requireOpt(opts, "radius", error)) { std::cerr << "Error: " << error << std::endl; return 1; }

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);
    bool ckJnc = hasOpt(opts, "junction-turns");
    std::string txNewJnc = getOpt(opts, "junction-turns");

    CA.init_MR_para(FA, *g, opts.input, getOpt(opts, "radius"), ckJnc, false, txNewJnc, weights);
    CA.setMultiPara(FA, 1, INT_MAX - 1, 1);
    CA.calculateMR(FA);
    CA.OutputData(FA);
    return 0;
}

int runMd(const Options &opts) {
    std::string error;
    if (!requireOpt(opts, "radius", error)) { std::cerr << "Error: " << error << std::endl; return 1; }

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);
    bool ckJnc = hasOpt(opts, "junction-turns");
    std::string txNewJnc = getOpt(opts, "junction-turns");

    CA.init_MD_para(FA, *g, opts.input, getOpt(opts, "radius"), ckJnc, false, txNewJnc, weights);
    CA.setMultiPara(FA, 1, INT_MAX - 1, 1);
    CA.calculateMD(FA);
    CA.OutputData(FA);
    return 0;
}

int runDr(const Options &opts) {
    std::string error;
    if (!requireOpt(opts, "angle", error) || !requireOpt(opts, "turns", error)) {
        std::cerr << "Error: " << error << std::endl; return 1;
    }

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);
    bool ckJnc = hasOpt(opts, "junction-turns");
    std::string txNewJnc = getOpt(opts, "junction-turns");

    // One run per angle threshold, as in the GUI.
    for (const std::string &angle : splitList(getOpt(opts, "angle"))) {
        CA.clearOldData();
        CA.init_DR_para(FA, *g, opts.input, "", angle, getOpt(opts, "turns"), ckJnc, false, txNewJnc, weights);
        CA.setMultiPara(FA, 1, INT_MAX - 1, 1);
        CA.calculateDR(FA);
        CA.OutputData(FA);
    }
    return 0;
}

int runDdl(const Options &opts) {
    std::string error;
    if (!requireOpt(opts, "radius", error) || !requireOpt(opts, "angle", error)) {
        std::cerr << "Error: " << error << std::endl; return 1;
    }

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);

    for (const std::string &angle : splitList(getOpt(opts, "angle"))) {
        CA.clearOldData();
        CA.init_DDL_para(FA, *g, opts.input, getOpt(opts, "radius"), angle, "", false, false, "", weights);
        CA.setMultiPara(FA, 1, INT_MAX - 1, 1);
        CA.calculateDDL(FA);
        CA.OutputData(FA);
    }
    return 0;
}

int runMdr(const Options &opts) {
    std::string error;
    if (!requireOpt(opts, "radius", error) || !requireOpt(opts, "angle", error) || !requireOpt(opts, "turns", error)) {
        std::cerr << "Error: " << error << std::endl; return 1;
    }

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);
    bool ckJnc = hasOpt(opts, "junction-turns");
    std::string txNewJnc = getOpt(opts, "junction-turns");

    for (const std::string &angle : splitList(getOpt(opts, "angle"))) {
        CA.clearOldData();
        CA.init_MDR_para(FA, *g, opts.input, getOpt(opts, "radius"), angle, getOpt(opts, "turns"),
                         ckJnc, false, txNewJnc, weights);
        CA.setMultiPara(FA, 1, INT_MAX - 1, 1);
        CA.calculateMDR(FA);
        CA.OutputData(FA);
    }
    return 0;
}

int runJnr(const Options &opts) {
    std::string error;
    if (!requireOpt(opts, "junction-degree", error) || !requireOpt(opts, "junction-limit", error)) {
        std::cerr << "Error: " << error << std::endl; return 1;
    }

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);

    CA.init_JnR_para(FA, *g, opts.input, getOpt(opts, "junction-degree"), getOpt(opts, "junction-limit"),
                     false, "", false, weights);
    CA.setMultiPara(FA, 1, INT_MAX - 1, 1);
    CA.calculateJncR(FA);
    CA.OutputData(FA);
    return 0;
}

int runJnd(const Options &opts) {
    std::string error;
    if (!requireOpt(opts, "radius", error) || !requireOpt(opts, "junction-degree", error)) {
        std::cerr << "Error: " << error << std::endl; return 1;
    }

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);

    CA.init_JnD_para(FA, *g, opts.input, getOpt(opts, "radius"), getOpt(opts, "junction-degree"),
                     false, "", false, weights);
    CA.setMultiPara(FA, 1, INT_MAX - 1, 1);
    CA.calculateJncD(FA);
    CA.OutputData(FA);
    return 0;
}

} // namespace
namespace {

// ---------------------------------------------------------------------------
// Origin-based analyses (mirror MainWindow::Net_*/Geo_*_Single_thread)
// ---------------------------------------------------------------------------

int runNetreach(const Options &opts) {
    std::string error;
    if (!requireOpt(opts, "from", error)) { std::cerr << "Error: " << error << std::endl; return 1; }
    std::string type = getOpt(opts, "type", "mr");

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);
    std::string txNewJnc = getOpt(opts, "junction-turns");
    std::string fromStr = getOpt(opts, "from");

    if (type == "mr") {
        if (!requireOpt(opts, "radius", error)) { std::cerr << "Error: " << error << std::endl; return 1; }
        CA.init_Net_MR_para(FA, *g, opts.input, getOpt(opts, "radius"), txNewJnc, weights);
    } else if (type == "dr") {
        if (!requireOpt(opts, "angle", error) || !requireOpt(opts, "turns", error)) {
            std::cerr << "Error: " << error << std::endl; return 1;
        }
        CA.init_Net_DR_para(FA, *g, opts.input, "", getOpt(opts, "angle"), getOpt(opts, "turns"),
                            txNewJnc, weights);
    } else if (type == "jnr") {
        if (!requireOpt(opts, "junction-degree", error) || !requireOpt(opts, "junction-limit", error)) {
            std::cerr << "Error: " << error << std::endl; return 1;
        }
        CA.init_Net_JnR_para(FA, *g, opts.input, getOpt(opts, "junction-degree"),
                             getOpt(opts, "junction-limit"), weights);
    } else {
        std::cerr << "Error: unknown netreach --type '" << type << "' (expected mr, dr or jnr)" << std::endl;
        return 1;
    }

    CA.searchLimitStr = transformName(fromStr);
    CA.getFromToID(FA, fromStr);
    CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

    if (type == "mr") {
        CA.Net_calculateMR(FA, CA.newMRLimit);
    } else if (type == "dr") {
        CA.Net_calculateDR(FA, CA.newDRLimit, CA.newMRLimit);
    } else {
        CA.Net_calculateJncR(FA, CA.newJnc_maxNum);
    }

    CA.OutputVisualData(FA);
    std::cerr << "Wrote " << CA.outShpFileName << std::endl;
    return 0;
}

int runStepdepth(const Options &opts) {
    std::string error;
    if (!requireOpt(opts, "from", error)) { std::cerr << "Error: " << error << std::endl; return 1; }
    std::string type = getOpt(opts, "type", "mr");

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);
    std::string fromStr = getOpt(opts, "from");

    if (type == "mr") {
        CA.init_Net_MR_para(FA, *g, opts.input, "0", "", weights);
    } else if (type == "dr") {
        if (!requireOpt(opts, "angle", error)) { std::cerr << "Error: " << error << std::endl; return 1; }
        CA.init_Net_DR_para(FA, *g, opts.input, "", getOpt(opts, "angle"), "0", "", weights);
    } else if (type == "jnr") {
        if (!requireOpt(opts, "junction-degree", error)) { std::cerr << "Error: " << error << std::endl; return 1; }
        CA.init_Net_JnR_para(FA, *g, opts.input, getOpt(opts, "junction-degree"), "0", weights);
    } else {
        std::cerr << "Error: unknown stepdepth --type '" << type << "' (expected mr, dr or jnr)" << std::endl;
        return 1;
    }

    CA.searchLimitStr = transformName(fromStr);
    CA.getFromToID(FA, fromStr);
    CA.isStepDepth = true;
    if (type == "mr") CA.SD_MR = true;
    if (type == "dr") CA.SD_DR = true;
    if (type == "jnr") CA.SD_JnR = true;

    CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

    if (type == "mr") {
        CA.Net_calculateMR(FA, CA.newMRLimit);
        CA.outputStepDepth(FA, "StepD_mr");
    } else if (type == "dr") {
        CA.Net_calculateDR(FA, CA.newDRLimit, CA.newMRLimit);
        CA.outputStepDepth(FA, "StepD_dr");
    } else {
        CA.Net_calculateJncR(FA, CA.newJnc_maxNum);
        CA.outputStepDepth(FA, "StepD_jnr");
    }
    return 0;
}

// Replicates MainWindow::output_shp_data: writes the OD result CSV for
// one-to-one/batch runs and a PathCount column on the input .dbf.
void writeOdOutputs(Calculation &CA, ShapeFileAccessor &FA) {
    if (!CA.isOneToOne) {
        return;
    }

    std::string outcsvfilename = CA.incsvfilename;
    if (outcsvfilename.size() >= 4) {
        outcsvfilename = outcsvfilename.substr(0, outcsvfilename.length() - 4);
    }
    if (CA.isMR) {
        outcsvfilename += "_MR.csv";
    } else if (CA.isDR) {
        outcsvfilename += "_DR.csv";
    } else if (CA.isJncR) {
        outcsvfilename += "_JnR.csv";
    } else {
        outcsvfilename += "_OD.csv";
    }

    std::ofstream out(outcsvfilename.c_str());
    out << "origin,destination,distance,path\n";
    for (size_t i = 0; i < CA.FromIDVec.size(); i++) {
        int startedge = CA.FromIDVec[i];
        int endedge = CA.ToIDVec[i];
        std::pair<int, int> od(startedge, endedge);
        if (CA.outputPath) {
            out << startedge << "," << endedge << "," << CA.routeLen_all[od] << ",\"";
            std::vector<int> &roads = CA.routeRoad_allMap[od];
            int allCount = int(roads.size());
            int newCount = 0;
            for (int road : roads) {
                ++newCount;
                if (road == startedge || road == endedge) {
                    continue;
                }
                if (newCount < allCount - 1) {
                    out << road << ",";
                } else {
                    out << road;
                }
            }
            out << "\"\n";
        } else {
            out << startedge << "," << endedge << "," << CA.routeLen_all[od] << "\n";
        }
    }
    out.close();
    std::cerr << "Wrote " << outcsvfilename << std::endl;

    // PathCount column on the input .dbf.
    std::map<int, double> passCount;
    for (int road : FA.roadID) {
        passCount[road] = 0;
    }
    for (size_t i = 0; i < CA.FromIDVec.size(); i++) {
        int startedge = CA.FromIDVec[i];
        int endedge = CA.ToIDVec[i];
        ++passCount[startedge];
        ++passCount[endedge];
        for (int road : CA.routeRoad_allMap[std::pair<int, int>(startedge, endedge)]) {
            ++passCount[road];
        }
    }
    if (!passCount.empty()) {
        CA.outputPathCount(passCount, "PathCount");
    }
}

int runOd(const Options &opts) {
    std::string error;
    std::string type = getOpt(opts, "type", "mr");
    std::string pairsCsv = getOpt(opts, "pairs");
    if (pairsCsv.empty() && !requireOpt(opts, "from", error)) {
        std::cerr << "Error: " << error << " (or use --pairs pairs.csv)" << std::endl;
        return 1;
    }

    ShapeFileAccessor FA;
    Calculation CA;
    Graph *g = nullptr;
    if (!loadAndValidateNetwork(opts, FA, g, error)) { std::cerr << "Error: " << error << std::endl; return 2; }

    std::vector<std::string> weights = weightAttributes(opts);
    // GUI defaults: unlimited angle / junction-turns unless explicitly given.
    std::string txAngleThreshold = getOpt(opts, "angle", std::to_string(INT_MAX - 1));
    std::string txNewJnc = getOpt(opts, "junction-turns", std::to_string(INT_MAX - 1));

    if (type == "mr") {
        CA.init_Geo_MR_para(FA, *g, opts.input, txAngleThreshold, txNewJnc, weights);
    } else if (type == "dr") {
        if (!hasOpt(opts, "angle")) { std::cerr << "Error: od --type dr requires --angle" << std::endl; return 1; }
        CA.init_Geo_DR_para(FA, *g, opts.input, txAngleThreshold, txNewJnc, weights);
    } else if (type == "jnr") {
        if (!hasOpt(opts, "junction-turns")) {
            std::cerr << "Error: od --type jnr requires --junction-turns" << std::endl; return 1;
        }
        CA.init_Geo_JnR_para(FA, *g, opts.input, txAngleThreshold, txNewJnc, weights);
    } else {
        std::cerr << "Error: unknown od --type '" << type << "' (expected mr, dr or jnr)" << std::endl;
        return 1;
    }

    std::string fromToStr = getOpt(opts, "from");
    std::string toStr = getOpt(opts, "to");
    if (!toStr.empty()) {
        fromToStr += ";" + toStr;
    }
    CA.outputPath = true;
    if (!pairsCsv.empty()) {
        fromToStr = pairsCsv;
        CA.incsvfilename = pairsCsv;
    }

    CA.searchLimitStr = transformName(fromToStr);
    CA.getFromToID(FA, fromToStr);
    CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

    if (type == "mr") {
        CA.Geo_calculateMR(FA);
    } else if (type == "dr") {
        CA.Geo_calculateDR(FA);
    } else {
        CA.Geo_calculateJncR(FA);
    }

    writeOdOutputs(CA, FA);
    if (!CA.isOneToOne) {
        CA.OutputVisualData(FA);
        std::cerr << "Wrote " << CA.outShpFileName << std::endl;
    }
    return 0;
}

} // namespace

int main(int argc, char **argv) {
    try {
        Options opts;
        std::string error;
        if (!parseArgs(argc, argv, opts, error)) {
            std::cerr << "Error: " << error << std::endl;
            std::cerr << kUsage << std::endl;
            return 1;
        }

        if (opts.command.empty() || opts.command == "help") {
            std::cout << kUsage << std::endl;
            return opts.command.empty() ? 1 : 0;
        }
        if (opts.command == "version") {
            std::cout << "urconnect-cli " << URCONNECT_VERSION
                      << " (UrConnect network analysis engine)" << std::endl;
            return 0;
        }
        if (opts.input.empty()) {
            std::cerr << "Error: missing input shapefile" << std::endl;
            std::cerr << kUsage << std::endl;
            return 1;
        }
        if (!validateOptions(opts, error)) {
            std::cerr << "Error: " << error << std::endl;
            return 1;
        }

        if (opts.command == "reach") return runReach(opts);
        if (opts.command == "md") return runMd(opts);
        if (opts.command == "dr") return runDr(opts);
        if (opts.command == "ddl") return runDdl(opts);
        if (opts.command == "mdr") return runMdr(opts);
        if (opts.command == "jnr") return runJnr(opts);
        if (opts.command == "jnd") return runJnd(opts);
        if (opts.command == "netreach") return runNetreach(opts);
        if (opts.command == "stepdepth") return runStepdepth(opts);
        if (opts.command == "od") return runOd(opts);

        std::cerr << "Error: unknown command '" << opts.command << "'" << std::endl;
        std::cerr << kUsage << std::endl;
        return 1;
    } catch (const std::exception &exception) {
        std::cerr << "Error: " << exception.what() << std::endl;
        return 2;
    } catch (...) {
        std::cerr << "Error: unexpected analysis failure" << std::endl;
        return 2;
    }
}
