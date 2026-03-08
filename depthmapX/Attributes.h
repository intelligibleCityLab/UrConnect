#pragma once

#include <set>
#include <string>
#include <map>

struct AttributesData {
	std::vector<std::string> AttributesNames;	//导入的属性列名
	std::map<std::string, std::map<int, double>> AttributesDouble; //属性列名，id，数值
};
