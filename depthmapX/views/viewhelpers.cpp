#include "viewhelpers.h"
#include <time.h>

namespace ViewHelpers {
    Point2f calculateCenter(const QPoint& point, const QPoint &oldCentre, double factor)
    {
        int diffX = oldCentre.x() - point.x();
        int diffY = oldCentre.y() - point.y();
        return Point2f(point.x() + double(diffX) * factor,
                       point.y() + double(diffY) * factor);
    }

    std::string getCurrentDate()
    {
        time_t now = time(NULL);
        char timeString[11];
        struct tm timeinfo;
#ifdef _WIN32
        localtime_s(&timeinfo, &now);
#else
        localtime_r(&now, &timeinfo);
#endif
        strftime(timeString, 11, "%Y/%m/%d", &timeinfo);
        return timeString;
    }

}
