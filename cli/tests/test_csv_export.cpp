#include "depthmapX/AttributeCsvExport.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

class RegionalNumbers : public std::numpunct<char> {
public:
    explicit RegionalNumbers(char decimal) : decimal_(decimal) {}
protected:
    char do_decimal_point() const override { return decimal_; }
    char do_thousands_sep() const override { return decimal_ == '.' ? ',' : '.'; }
    std::string do_grouping() const override { return "\3"; }
private:
    char decimal_;
};

void checkExport(char decimal)
{
    std::locale::global(std::locale(std::locale::classic(), new RegionalNumbers(decimal)));
    std::map<int, double> ids;
    for (int id = 0; id < 5767; ++id) ids[id] = id;
    const std::vector<std::string> fields = {"D45a400", "D45a800", "DL45a400", "DL45a800"};
    auto value = [&fields](int id, const std::string& field) {
        if (field == fields[0]) return 12345.125;
        if (field == fields[1]) return -12345.5;
        if (field == fields[2]) return 0.0;
        return id + 0.25;
    };
    std::ostringstream output;
    urconnect::writeAttributeCsv(output, ids, fields, value, 4);
    std::istringstream input(output.str());
    std::string line;
    std::getline(input, line);
    if (line != "ID,D45a400,D45a800,DL45a400,DL45a800")
        throw std::runtime_error("CSV header changed");
    for (int id = 0; id < 5767; ++id) {
        std::ostringstream expected;
        expected.imbue(std::locale::classic());
        expected << id << ",12345.1250,-12345.5000,0.0000,"
                 << std::fixed << std::setprecision(4) << id + 0.25;
        if (!std::getline(input, line) || line != expected.str())
            throw std::runtime_error("Regional formatting corrupted a CSV row");
    }
    if (std::getline(input, line)) throw std::runtime_error("Unexpected CSV row");

    std::ostringstream legacy;
    urconnect::writeAttributeCsv(legacy, {{0, 1000}}, {"value"}, value);
    if (legacy.str() != "ID,value\n1000,1000.25\n")
        throw std::runtime_error("Default-precision CSV formatting changed");
}

int main()
{
    const std::locale original;
    try {
        checkExport('.');
        checkExport(',');
        std::locale::global(original);
        std::cout << "CSV IDs and values remain ungrouped and dot-decimal under two regional locales\n";
        return 0;
    } catch (const std::exception& error) {
        std::locale::global(original);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
