#pragma once
#include <vector>
#include <string>
#include <map>
#include "Calculation.h"
#include "ShapeFileAccessor.h"

using namespace std;

class GeoMap {
public:
	int startRoad;	//起点
	int endRoad;	//终点

	int red = 0, green = 255, blue = 0;

	int RecordCount;	//记录总条数

	std::map<int, int> Ref_to_Id;
	std::map<int, int> Id_to_Ref;

	//reach范围
	set<int> AllInRoads;

public:
	void init(Calculation &CA, ShapeFileAccessor &FA, int startRoad);
	void subset_init(Calculation &CA, ShapeFileAccessor &FA, std::vector<std::string> &items);
};