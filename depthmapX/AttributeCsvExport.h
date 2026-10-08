#pragma once

#include <iomanip>
#include <locale>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace urconnect {

template <typename ValueGetter>
void writeAttributeCsv(std::ostream& out, const std::map<int, double>& ids,
                       const std::vector<std::string>& fields, ValueGetter value,
                       int precision = -1)
{
    // CSV separators must not depend on the desktop's regional number format.
    out.imbue(std::locale::classic());
    if (precision >= 0) out << std::fixed << std::setprecision(precision);
    out << "ID";
    for (const auto& field : fields) out << ',' << field;
    out << '\n';
    for (const auto& entry : ids) {
        const int id = static_cast<int>(entry.second);
        out << id;
        for (const auto& field : fields) out << ',' << value(id, field);
        out << '\n';
    }
}

} // namespace urconnect
