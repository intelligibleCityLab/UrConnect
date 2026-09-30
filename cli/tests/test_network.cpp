#include "shapefil.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

int createNetwork(const std::string &basePath) {
    SHPHandle shp = SHPCreate((basePath + ".shp").c_str(), SHPT_ARC);
    DBFHandle dbf = DBFCreate((basePath + ".dbf").c_str());
    if (shp == nullptr || dbf == nullptr) {
        if (shp != nullptr) SHPClose(shp);
        if (dbf != nullptr) DBFClose(dbf);
        std::cerr << "failed to create test network" << std::endl;
        return 1;
    }

    int fidField = DBFAddField(dbf, "FID", FTInteger, 8, 0);
    int weightField = DBFAddField(dbf, "weight", FTDouble, 12, 2);
    if (fidField < 0 || weightField < 0) {
        SHPClose(shp);
        DBFClose(dbf);
        return 1;
    }

    const double coordinates[3][4] = {
        {0.0, 0.0, 10.0, 0.0},
        {10.0, 0.0, 20.0, 0.0},
        {10.0, 0.0, 10.0, 10.0}
    };
    for (int i = 0; i < 3; ++i) {
        double x[2] = {coordinates[i][0], coordinates[i][2]};
        double y[2] = {coordinates[i][1], coordinates[i][3]};
        SHPObject *object = SHPCreateSimpleObject(SHPT_ARC, 2, x, y, nullptr);
        if (object == nullptr || SHPWriteObject(shp, -1, object) < 0) {
            if (object != nullptr) SHPDestroyObject(object);
            SHPClose(shp);
            DBFClose(dbf);
            return 1;
        }
        SHPDestroyObject(object);
        if (!DBFWriteIntegerAttribute(dbf, i, fidField, i) ||
            !DBFWriteDoubleAttribute(dbf, i, weightField, 10.0 * (i + 1))) {
            SHPClose(shp);
            DBFClose(dbf);
            return 1;
        }
    }

    SHPClose(shp);
    DBFClose(dbf);
    return 0;
}

int hasField(const std::string &dbfPath, const std::string &fieldName) {
    DBFHandle dbf = DBFOpen(dbfPath.c_str(), "rb");
    if (dbf == nullptr) {
        return 1;
    }
    int field = DBFGetFieldIndex(dbf, fieldName.c_str());
    DBFClose(dbf);
    return field >= 0 ? 0 : 1;
}

int compareFields(const char *leftPath, const char *leftName,
                  const char *rightPath, const char *rightName) {
    DBFHandle left = DBFOpen(leftPath, "rb");
    DBFHandle right = DBFOpen(rightPath, "rb");
    int result = 1;
    if (left != nullptr && right != nullptr) {
        int leftField = DBFGetFieldIndex(left, leftName);
        int rightField = DBFGetFieldIndex(right, rightName);
        int count = DBFGetRecordCount(left);
        if (leftField >= 0 && rightField >= 0 && count > 0 && count == DBFGetRecordCount(right)) {
            result = 0;
            for (int row = 0; row < count; ++row) {
                double a = DBFReadDoubleAttribute(left, row, leftField);
                double b = DBFReadDoubleAttribute(right, row, rightField);
                if (!std::isfinite(a) || !std::isfinite(b) || std::fabs(a - b) > 0.001) {
                    std::cerr << "field mismatch at record " << row << ": " << a << " != " << b << std::endl;
                    result = 1;
                    break;
                }
            }
        }
    }
    if (left != nullptr) DBFClose(left);
    if (right != nullptr) DBFClose(right);
    return result;
}

int fieldValue(const char *path, const char *name, int row, double expected) {
    DBFHandle dbf = DBFOpen(path, "rb");
    if (dbf == nullptr) return 1;
    int field = DBFGetFieldIndex(dbf, name);
    int result = 1;
    if (field >= 0 && row >= 0 && row < DBFGetRecordCount(dbf)) {
        double value = DBFReadDoubleAttribute(dbf, row, field);
        result = std::isfinite(value) && std::fabs(value - expected) <= 0.001 ? 0 : 1;
        if (result != 0) std::cerr << "unexpected field value: " << value << " != " << expected << std::endl;
    }
    DBFClose(dbf);
    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc == 3 && std::string(argv[1]) == "create") {
        return createNetwork(argv[2]);
    }
    if (argc == 4 && std::string(argv[1]) == "has-field") {
        return hasField(argv[2], argv[3]);
    }
    if (argc == 6 && std::string(argv[1]) == "compare-fields") {
        return compareFields(argv[2], argv[3], argv[4], argv[5]);
    }
    if (argc == 6 && std::string(argv[1]) == "field-value") {
        return fieldValue(argv[2], argv[3], std::atoi(argv[4]), std::strtod(argv[5], nullptr));
    }
    std::cerr << "usage: urconnect-test-network create <base-path> | has-field <dbf> <field> | "
                 "compare-fields <dbf> <field> <dbf> <field> | field-value <dbf> <field> <row> <value>" << std::endl;
    return 2;
}
