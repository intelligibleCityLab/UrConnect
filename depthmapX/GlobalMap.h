#pragma once

#include <vector>
#include <string>
#include <map>
#include "Attributes.h"
#include "ShapeFileAccessor.h"
#include "depthmapX/GraphDoc.h"

class GlobalMap {
public:
	std::string ShpFileName;

	AttributesData Attributes;

	int RecordCount;	//记录总条数

	std::map<int, int> Ref_to_Id;	//value=<Ref, id>
	std::map<int, int> Id_to_Ref;	//需要通过框选Ref找出对应的ID

public:
	bool readFile(std::string shpfilename, std::string idFieldIndex);	//从shp文件读取数据

	bool readGraph(MetaGraph *graph);	//从graph中读取数据

	bool multiThreadReadFile(ShapeFileAccessor &fileAccessor, std::string shpfilename, std::string idFieldIndex);	//多线程从shp文件读取数据

	void ModifyDoubleData(std::string name, std::map<int, double> &data);

	void removeAttribute(std::string name);

	void clearOldData();
};