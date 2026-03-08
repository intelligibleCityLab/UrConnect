#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "Attributes.h"

using namespace std;


class DisPlay {
public:

	string ShpFileName;

	AttributesData Attributes;
	int RecordCount;	//记录总条数

	//Ref-->ID
	std::map<int, int> Ref_to_Id;
	std::map<int, int> Id_to_Ref;

public:
	void ModifyDoubleData(string name, map<int, double> &data);

	void CopyAllData(AttributesData &SourceData);

	void RegenerateStream();
	void clearOldData();
};