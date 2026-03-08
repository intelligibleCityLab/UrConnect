#pragma once
#include <map>

using namespace std;

struct GlobalRoads {
	map<int, int> Ref_to_Id;	
	map<int, int> Id_to_Ref;
};

struct NetreachRoads {
	map<int, int> Ref_to_Id;
	map<int, int> Id_to_Ref;
};

struct GeodesicsRoads {
	map<int, int> Ref_to_Id;
	map<int, int> Id_to_Ref;
};