#include "NetMap.h"

void NetMap::init(Calculation &CA, ShapeFileAccessor &FA, int startRoad) {
	this->startRoad = startRoad;

	Ref_to_Id.clear();
	Id_to_Ref.clear();
	NewLineRefs.clear();

	PartInRoadsCoordinate.clear();
	//添加部分通过的道路
	PartInRoadsCoordinate.insert(CA.partInRoads_all[startRoad].begin(), CA.partInRoads_all[startRoad].end());
	//添加完全通过的道路
	AllInRoads.clear();
	for (auto it = CA.reachRoad_all[startRoad].begin(); it != CA.reachRoad_all[startRoad].end(); it++) {
		int road_id = *it;
		if (PartInRoadsCoordinate.count(road_id) == 0) {
			AllInRoads.insert(road_id);
		}
		else {
			PartInRoads.insert(road_id);
		}
	}

	this->RecordCount = CA.reachRoad_all[startRoad].size();
}

void NetMap::subset_init(Calculation &CA, ShapeFileAccessor &FA, std::vector<std::string> &items) {
	Ref_to_Id.clear();
	Id_to_Ref.clear();
	NewLineRefs.clear();

	PartInRoadsCoordinate.clear();
	AllInRoads.clear();
	for (std::string item : items) {
		//添加部分通过的道路
		PartInRoadsCoordinate.insert(CA.subset_partInRoads_all[item].begin(), CA.subset_partInRoads_all[item].end());
		//添加完全通过的道路
		AllInRoads.insert(CA.subset_reachRoad_all[item].begin(), CA.subset_reachRoad_all[item].end());
	}

	this->RecordCount = PartInRoadsCoordinate.size() + AllInRoads.size();
}