#include "GlobalMap.h"

#include "Shapefile.h"
#include "ShapeObject.h"
#include "GraphDefine.h"
#include <iomanip>
#include <fstream>
#include <streambuf>

using namespace std;
void GlobalMap::removeAttribute(string name) {
	auto idx = std::find(Attributes.AttributesNames.begin(), Attributes.AttributesNames.end(), name);
	if (idx != Attributes.AttributesNames.end()) {
		Attributes.AttributesNames.erase(idx);
	}
	auto i = Attributes.AttributesDouble.find(name);
	if (i!= Attributes.AttributesDouble.end()) {
		Attributes.AttributesDouble.erase(i);
	}
}

//更新属性数据
void GlobalMap::ModifyDoubleData(string name, map<int, double> &data) {
	if (this->Attributes.AttributesDouble.count(name) == 0) {
		this->Attributes.AttributesNames.push_back(name);
	}
	this->Attributes.AttributesDouble[name] = data;
}

void GlobalMap::clearOldData() {
	ShpFileName = "";

	Attributes.AttributesDouble.clear();
	Attributes.AttributesNames.clear();

	RecordCount = 0;	//记录总条数

	Ref_to_Id.clear();	//value=<Ref, id>
	Id_to_Ref.clear();	//需要通过框选Ref找出对应的ID
}

//从shp和csv文件读取数据
bool GlobalMap::readFile(std::string shpfilename, std::string idFieldIndex) {

	this->ShpFileName = shpfilename;

	//处理shp文件
	ShapeFileAccessor fileAccessor;
	fileAccessor.init(shpfilename, this->Attributes);
	fileAccessor.generateDispalyStream(shpfilename, idFieldIndex,this->Attributes, this->Ref_to_Id, this->Id_to_Ref);

	this->RecordCount = this->Id_to_Ref.size();

	return true;
}

//从graph中读取数据
bool GlobalMap::readGraph(MetaGraph *graph) {
	if (!graph)
		return false;

	AttributeTable& tab = graph->getAttributeTable();

	//把所有属性字段全部扒下来，注意：字段长度超过10个字符的放弃掉
	std::vector<std::string> unvalid;
	for (int i = 0; i < tab.getNumColumns(); i++) {
		std::string fieldname = tab.getColumnName(i);
		if (fieldname == "ID")
			continue;
		if (fieldname.size() <= 10) {
			this->Attributes.AttributesNames.push_back(fieldname);
		}
		else {
			unvalid.push_back(fieldname);
		}
	}
	////删除字段长度超过10个字符的属性
	//for (string name : unvalid) tab.removeColumn(tab.getColumnIndex(name));

	//获取属性字段内容
	for (int row_idx = 0; row_idx < tab.getNumRows(); row_idx++) {
		const AttributeRow& rowData = tab.getRow(AttributeKey(row_idx));
		for (string column_name : this->Attributes.AttributesNames) {
			if (column_name == "ID")
				this->Attributes.AttributesDouble["DbfID"][row_idx] = rowData.getValue(column_name);
			else
				this->Attributes.AttributesDouble[column_name][row_idx] = rowData.getValue(column_name);
		}
		this->Attributes.AttributesDouble["Ref"][row_idx] = row_idx;
		this->Attributes.AttributesDouble["ID"][row_idx] = row_idx;
	}

	this->Id_to_Ref.clear();
	this->Ref_to_Id.clear();

	//获取每条线段的坐标数据
	//ShapeGraph&   shape_graph = graph->getDisplayedShapeGraph();
	ShapeMap& shape_graph = graph->getDisplayedDataMap();
	std::map<int, SalaShape>& shapes_map = shape_graph.getAllShapes();
	for (int row_idx = 0; row_idx < tab.getNumRows(); row_idx++) {
		const Line& line = shapes_map[row_idx].getLine();
		this->Attributes.AttributesDouble["x1"][row_idx] = line.ax();
		this->Attributes.AttributesDouble["y1"][row_idx] = line.ay();
		this->Attributes.AttributesDouble["x2"][row_idx] = line.bx();
		this->Attributes.AttributesDouble["y2"][row_idx] = line.by();

		this->Id_to_Ref[row_idx] = row_idx;
		this->Ref_to_Id[row_idx] = row_idx;
	}

	//删除没有数据的字段名
	Attributes.AttributesNames.clear();
	for (auto it = Attributes.AttributesDouble.begin(); it != Attributes.AttributesDouble.end(); it++) {
		this->Attributes.AttributesNames.push_back(it->first);
	}

	this->RecordCount = this->Id_to_Ref.size();

	return true;
}

bool GlobalMap::multiThreadReadFile(ShapeFileAccessor &fileAccessor, std::string shpfilename, std::string idFieldIndex) {
	this->ShpFileName = shpfilename;

	//处理shp文件
	fileAccessor.multiThreadReadFile(shpfilename, idFieldIndex, this->Attributes);

	if (this->Attributes.AttributesDouble.count("ID") == 0)
		return false;

	this->RecordCount = this->Attributes.AttributesDouble["ID"].size();

	for (int i = 0; i < this->RecordCount; i++) {
		this->Id_to_Ref[i] = i;
		this->Ref_to_Id[i] = i;
	}

	return true;
}