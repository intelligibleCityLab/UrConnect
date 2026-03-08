#include "GeoMap.h"

void GeoMap::init(Calculation &CA, ShapeFileAccessor &FA, int startRoad) {
	this->startRoad = startRoad;

	Ref_to_Id.clear();
	Id_to_Ref.clear();

	AllInRoads.clear();
	//添加完全通过的道路
	AllInRoads.insert(CA.routeRoad_all[startRoad].begin(), CA.routeRoad_all[startRoad].end());

	this->RecordCount = CA.routeRoad_all[startRoad].size();
}

void GeoMap::subset_init(Calculation &CA, ShapeFileAccessor &FA, std::vector<std::string> &items) {
	Ref_to_Id.clear();
	Id_to_Ref.clear();

	AllInRoads.clear();
	for (std::string item : items) {
		//添加完全通过的道路
		AllInRoads.insert(CA.subset_reachRoad_all[item].begin(), CA.subset_reachRoad_all[item].end());
	}

	this->RecordCount = AllInRoads.size();
}