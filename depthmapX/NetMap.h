#pragma once

#include <vector>
#include <string>
#include <map>
#include "Calculation.h"
#include "ShapeFileAccessor.h"

using namespace std;

class NetMap {
public:
	int startRoad;	//NetReach的起点
	
	int red = 0, green = 255, blue = 0;

	int RecordCount;	//记录总条数

	std::map<int, int> Ref_to_Id;
	std::map<int, int> Id_to_Ref;

	//reach范围
	set<int> AllInRoads;
	set<int> PartInRoads;
	std::map<int, std::vector<double>> PartInRoadsCoordinate;	//部分通过线条的坐标数据
	set<int> NewLineRefs;

public:
	void init(Calculation &CA, ShapeFileAccessor &FA, int startRoad);

	void subset_init(Calculation &CA, ShapeFileAccessor &FA, std::vector<std::string> &items);
};