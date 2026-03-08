#include "stdafx.h"
#include "ShapeFileAccessor.h"
#include <map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <streambuf>
#include <thread>
#include <math.h>
#include <mutex>
#include <cmath>
#include <QDir>
#include <iomanip>

#define PI 3.141592653
//shapefile.h中的静态成员
int ShapeFileAccessor::thread_num = 4;
std::vector<bool> ShapeFileAccessor::Flags;
std::vector<AttributesData> ShapeFileAccessor::AttributesDataVec;

std::vector<std::map<int, std::set<int>>> ShapeFileAccessor::AdjRoadListVec;
std::vector<std::map<std::string, double>> ShapeFileAccessor::AdjRoadAngleVec;

std::vector < std::map <std::string, int >> ShapeFileAccessor::AdjTurnVec;	//有向图两条边之间是否有拐弯
std::vector < std::unordered_map<int, std::unordered_map<int, int>>> ShapeFileAccessor::AdjTurnMPVec;
std::vector < std::map<int, int>> ShapeFileAccessor::JncVec;	//道路端点是否为交叉路口(0/1)
std::vector < std::map<int, int>> ShapeFileAccessor::DcVec;	//道路端点是否存在转弯路(0/1)

std::vector<std::set<int>> ShapeFileAccessor::roadIDVec;
std::vector < std::map<int, std::vector<int>>> ShapeFileAccessor::roadNodeVec;		//无向图每条道路对应的端点编号
std::vector < std::map<int, std::vector<int>>> ShapeFileAccessor::roadNode2Vec;		//有向图每条道路对应的端点编号
std::vector < std::map<int, std::vector<double>>> ShapeFileAccessor::RouteVec;	//无向图路的坐标数据
std::vector < std::map<int, std::vector<double>>> ShapeFileAccessor::Route2Vec;	//有向图路的坐标数据
std::vector < std::map<int, std::set<int>>> ShapeFileAccessor::NodeStartRoadVec;
std::vector < std::map<int, double>> ShapeFileAccessor::LengthVec;	//无向图路长
std::vector < std::map<std::string, int>> ShapeFileAccessor::nodeToEdgeVec;
std::vector<Graph> ShapeFileAccessor::gVec;


std::mutex lock;
std::mutex FlagsMutex; //Flags的锁

std::stringbuf buf;
std::istream inFileToMap(&buf);
std::ostream outFileToMap(&buf);

std::string convertDtoStr(double x) {

	std::ostringstream outx;
	outx.precision(10);
	outx << x;
	return outx.str();
}


inline double calculateDistance(double x, double y, double x1, double y1) {

	return (double)pow((pow((x - x1), 2) + pow((y - y1), 2)), 0.5);
}


void ShapeFileAccessor::init(std::string filepath, AttributesData &Attributes)
{
	m_filepath = filepath;
	myAttributes = Attributes;
}

void ShapeFileAccessor::clearOldData() {
	Mean_Angle_of_Deviation = 0;
	roadID.clear();
	roadNode.clear();		//无向图每条道路对应的端点编号
	roadNode2.clear();		//有向图每条道路对应的端点编号
	Route.clear();	//无向图路的坐标数据
	Route2.clear();	//有向图路的坐标数据
	NodeStartRoad.clear();
	Length.clear();	//无向图路长
	Length2.clear();	//有向图路长
	AdjRoadList.clear();	//有向图每条边的邻接边队列
	AdjRoadAngle.clear();	//有向图两条边之间的夹角，key是(边1，边2)有序对
	AdjTurn.clear();	//有向图两条边之间是否有拐弯
	Jnc.clear();	//道路端点是否为交叉路口(0/1)
	Dc.clear();	//道路端点是否存在转弯路(0/1)
	Road_to_roads.clear();
	AdjTurnMP.clear();
	AdjTurnTP.clear();
	length_all = 0;
	dc_all = 0;
	jnc_all = 0;

	//输出指定起点、终点最短路径需要使用
	nodeToEdge.clear();
	mapNodes.clear();	//坐标数据映射得到的端点ID
	mapNodes2.clear();	//两条路的ID映射得到的两边连接ID

	shapeType = 0;
	m_filepath = "";
	g.clear();


}


inline std::string PointToKey(double x, double y)
{	
	std::string str = convertDtoStr(x) + ", " + convertDtoStr(y);
	return str;
}

inline std::string IndexToKey(int x, int y)
{
	std::string str = std::to_string(x) + ", " + std::to_string(y);
	return str;
}

inline std::vector<int> get_index_roads(std::string index_str) {
	if (index_str.find(", ") == std::string::npos) {
		return {};
	}
	int pos = index_str.find(", ");
	int road_1 = std::stoi(index_str.substr(0, pos));
	int road_2 = std::stoi(index_str.substr(pos+2));
	return { road_1,road_2 };
}

int ShapeFileAccessor::generateTXT(std::string shpfilename, std::string txtFileName, std::string idFieldIndex, 
	int last_count, std::map<int, int> &Ref_to_Id, std::map<int, int> &Id_to_Ref)
{
	extern int loadedCount;
	loadedCount = 0;

	Ref_to_Id.clear();
	Id_to_Ref.clear();
	
	std::string dbFilePath = shpfilename.substr(0, shpfilename.length() - 4) + ".dbf";
	//std::string txtAllFileName = txtFileName.substr(0, txtFileName.length() - 4) + "All.txt";
	std::string txtAllFileName = txtFileName;

	//获取shp数据
	int result = shapefile.Open(shpfilename);
	shapeType = shapefile.GetType();

	if (result != 0) {
		AfxMessageBox(_T("Read Shape File failed"), MB_OK | MB_ICONERROR);
		return -1;
	}
	else {
		DBFHandle	hDBF;
		hDBF = DBFOpen(dbFilePath.c_str(), "rb+");
		int fieldIndex;
		if (idFieldIndex != "FID") {
			if (hDBF == NULL) {
				AfxMessageBox(_T("Read dbf File failed"), MB_OK | MB_ICONERROR);
				return -1;
			}
			fieldIndex = DBFGetFieldIndex(hDBF, idFieldIndex.c_str());
			if (fieldIndex < 0) {
				AfxMessageBox(_T("Read dbf File failed"), MB_OK | MB_ICONERROR);
				return -1;
			}
		}		

		int count = shapefile.GetEntityCount();

		CString shapeType(shapefile.GetTypeString().c_str());

		ShapeObject spObject;

		//std::ofstream out;
		//out.open(txtFileName.c_str());

		std::ofstream outAll;
		outAll.open(txtAllFileName.c_str());
		outAll << "Ref\tx1\ty1\tx2\ty2\tID\t";
		int fieldCount = DBFGetFieldCount(hDBF);
		char pszFieldName[100];
		for (int iField = 0; iField < fieldCount; iField++) {			
			memset(pszFieldName, '\0', sizeof(pszFieldName));
			DBFGetFieldInfo(hDBF, iField, pszFieldName, NULL, NULL);
			std::string fieldname(pszFieldName);
			if (fieldname == "ID") {
				fieldname = "DbfID";
			}
			outAll << fieldname << '\t';
		}
		outAll << std::endl;

		int id, ref;
		for (int i = 0; i < count; i++)
		{
			shapefile.GetShape(i, spObject);

			if (spObject.GetVertexCount() == 2) {
				// Polyline only have two point 获取到路的两端节点坐标
				const sPoint<double>* points = spObject.GetVertices();

				int route_index = spObject.GetIndex();
				if (idFieldIndex != "FID") {
					id = DBFReadIntegerAttribute(hDBF, i, fieldIndex);
				}
				else
				{
					id = i;
				}

				outAll << route_index << '\t' << std::setprecision(16) << points[0].x << '\t' << std::setprecision(16) <<
					points[0].y << '\t' << std::setprecision(16) << points[1].x << '\t' << std::setprecision(16) <<
					points[1].y << '\t' << id << '\t';

				for (int iField = 0; iField < fieldCount; iField++) {
					DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, iField, NULL, NULL, NULL);
					if (fieldtype == FTInteger) {
						int value = DBFReadIntegerAttribute(hDBF, i, iField);
						outAll << value << '\t';
					}
					if (fieldtype == FTDouble) {
						double value = DBFReadDoubleAttribute(hDBF, i, iField);
						outAll << value << '\t';
					}
					if (fieldtype == FTString) {
						std::string value = DBFReadStringAttribute(hDBF, i, iField);
						outAll << value << '\t';
					}
				}
				outAll << std::endl;

				ref = last_count + i;
				Ref_to_Id[ref] = id;
				Id_to_Ref[id] = ref;
			}

			++loadedCount;
		}
		
		//std::ostream tt("000000000000000000.txt");

		//out.close();
		outAll.close();
		shapefile.Close();

		return count;
	}

	return 0;
}

int ShapeFileAccessor::generateFileStream(std::string shpfilename, std::string idFieldIndex,
	int last_count, std::map<int, int> &Ref_to_Id, std::map<int, int> &Id_to_Ref) {
	
	//清空ostream
	buf.str("");
	inFileToMap.clear();
	outFileToMap.clear();

	extern int loadedCount;
	loadedCount = 0;

	Ref_to_Id.clear();
	Id_to_Ref.clear();

	std::string dbFilePath = shpfilename.substr(0, shpfilename.length() - 4) + ".dbf";

	//获取shp数据
	int result = shapefile.Open(shpfilename);
	shapeType = shapefile.GetType();

	if (result != 0) {
		AfxMessageBox(_T("Read Shape File failed"), MB_OK | MB_ICONERROR);
		return -1;
	}
	else {
		DBFHandle	hDBF;
		hDBF = DBFOpen(dbFilePath.c_str(), "rb+");
		int fieldIndex;
		if (idFieldIndex != "FID") {
			if (hDBF == NULL) {
				AfxMessageBox(_T("Read dbf File failed"), MB_OK | MB_ICONERROR);
				return -1;
			}
			fieldIndex = DBFGetFieldIndex(hDBF, idFieldIndex.c_str());
			if (fieldIndex < 0) {
				AfxMessageBox(_T("Read dbf File failed"), MB_OK | MB_ICONERROR);
				return -1;
			}
		}

		int count = shapefile.GetEntityCount();

		CString shapeType(shapefile.GetTypeString().c_str());

		ShapeObject spObject;

		//std::ofstream out;
		//out.open(txtFileName.c_str());
		outFileToMap << "Ref\tx1\ty1\tx2\ty2\tID\t";

		int fieldCount = DBFGetFieldCount(hDBF);
		char pszFieldName[100];
		for (int iField = 0; iField < fieldCount; iField++) {
			memset(pszFieldName, '\0', sizeof(pszFieldName));
			DBFGetFieldInfo(hDBF, iField, pszFieldName, NULL, NULL);
			std::string fieldname(pszFieldName);
			if (fieldname == "ID") {
				fieldname = "DbfID";
			}
			outFileToMap << fieldname << '\t';
		}
		outFileToMap << std::endl;

		int id, ref;
		for (int i = 0; i < count; i++)
		{
			shapefile.GetShape(i, spObject);

			if (spObject.GetVertexCount() == 2) {
				// Polyline only have two point 获取到路的两端节点坐标
				const sPoint<double>* points = spObject.GetVertices();

				int route_index = spObject.GetIndex();
				if (idFieldIndex != "FID") {
					id = DBFReadIntegerAttribute(hDBF, i, fieldIndex);
				}
				else
				{
					id = i;
				}

				outFileToMap << route_index << '\t' << std::setprecision(16) << points[0].x << '\t' << std::setprecision(16) <<
					points[0].y << '\t' << std::setprecision(16) << points[1].x << '\t' << std::setprecision(16) <<
					points[1].y << '\t' << id << '\t';

				for (int iField = 0; iField < fieldCount; iField++) {
					DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, iField, NULL, NULL, NULL);
					if (fieldtype == FTInteger) {
						int value = DBFReadIntegerAttribute(hDBF, i, iField);
						outFileToMap << value << '\t';
					}
					if (fieldtype == FTDouble) {
						double value = DBFReadDoubleAttribute(hDBF, i, iField);
						outFileToMap << value << '\t';
					}
					if (fieldtype == FTString) {
						std::string value = DBFReadStringAttribute(hDBF, i, iField);
						outFileToMap << value << '\t';
					}
				}
				outFileToMap << std::endl;

				ref = last_count + i;
				Ref_to_Id[ref] = id;
				Id_to_Ref[id] = ref;
			}

			++loadedCount;
		}

		//std::ostream tt("000000000000000000.txt");

		//out.close();
		shapefile.Close();

		//std::string str;
		//getline(inFileToMap, str);

		return count;
	}

	return 0;
}

inline void getAttributesNames(std::string lineStr, std::vector<std::string> &AttributesNames) {
	std::stringstream ss(lineStr);
	std::string str;
	while (getline(ss, str, '\t')) {
		if (str.length() && std::count(AttributesNames.begin(), AttributesNames.end(),str) == 0)
			AttributesNames.emplace_back(str);
	}
}

int ShapeFileAccessor::generateDispalyStream(std::string shpfilename, std::string idFieldIndex, AttributesData &Attributes,
	std::map<int, int> &Ref_to_Id, std::map<int, int> &Id_to_Ref) {

	int last_count = 0;
	std::string firstLineStr;

	//清空ostream
	buf.str("");
	inFileToMap.clear();
	outFileToMap.clear();

	extern int loadedCount;
	loadedCount = 0;

	Ref_to_Id.clear();
	Id_to_Ref.clear();

	std::string dbFilePath = shpfilename.substr(0, shpfilename.length() - 4) + ".dbf";

	//获取shp数据
	int result = shapefile.Open(shpfilename);
	shapeType = shapefile.GetType();

	if (result != 0) {
		AfxMessageBox(_T("Read Shape File failed"), MB_OK | MB_ICONERROR);
		shapefile.Close();
		return -1;
	}
	
	DBFHandle	hDBF;
	auto path = dbFilePath.c_str();
	hDBF = DBFOpen(path, "rb+");
	int fieldIndex;
	if (idFieldIndex != "FID") {
		if (hDBF == NULL) {
			AfxMessageBox(_T("Read dbf File failed"), MB_OK | MB_ICONERROR);
			return -1;
		}
		fieldIndex = DBFGetFieldIndex(hDBF, idFieldIndex.c_str());
		if (fieldIndex < 0) {
			AfxMessageBox(_T("Read dbf File failed"), MB_OK | MB_ICONERROR);
			return -1;
		}
	}

	int count = shapefile.GetEntityCount();

	CString shapeType(shapefile.GetTypeString().c_str());

	ShapeObject spObject;

	outFileToMap << "Ref\tx1\ty1\tx2\ty2\tID\t";
	firstLineStr += "Ref\tx1\ty1\tx2\ty2\tID\t";

	int fieldCount = DBFGetFieldCount(hDBF);
	char pszFieldName[100];
	for (int iField = 0; iField < fieldCount; iField++) {
		memset(pszFieldName, '\0', sizeof(pszFieldName));
		DBFGetFieldInfo(hDBF, iField, pszFieldName, NULL, NULL);
		std::string fieldname(pszFieldName);
		DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, iField, NULL, NULL, NULL);
		if (fieldtype == FTInteger || fieldtype == FTDouble) {
			if(fieldname=="ID"){
				fieldname = "DbfID";
			}
			outFileToMap << fieldname << '\t';
			firstLineStr += fieldname + '\t';
		}
	}
	outFileToMap << std::endl;

	//收集属性列的名字
	getAttributesNames(firstLineStr, Attributes.AttributesNames);

	int id, ref;
	for (int i = 0; i < count; i++)
	{
		shapefile.GetShape(i, spObject);

		if (spObject.GetVertexCount() == 2) {
			// Polyline only have two point 获取到路的两端节点坐标
			const sPoint<double>* points = spObject.GetVertices();

			int route_index = spObject.GetIndex();
			if (idFieldIndex != "FID") {
				id = DBFReadIntegerAttribute(hDBF, i, fieldIndex);
			}
			else
			{
				id = i;
			}

			outFileToMap << route_index << '\t' << std::setprecision(16) << points[0].x << '\t' << std::setprecision(16) <<
				points[0].y << '\t' << std::setprecision(16) << points[1].x << '\t' << std::setprecision(16) <<
				points[1].y << '\t' << id << '\t';
			Attributes.AttributesDouble["Ref"][route_index] = route_index;
			Attributes.AttributesDouble["x1"][route_index] = points[0].x;
			Attributes.AttributesDouble["y1"][route_index] = points[0].y;
			Attributes.AttributesDouble["x2"][route_index] = points[1].x;
			Attributes.AttributesDouble["y2"][route_index] = points[1].y;
			Attributes.AttributesDouble["ID"][route_index] = id;

			for (int iField = 0; iField < fieldCount; iField++) {
				DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, iField, NULL, NULL, NULL);
				memset(pszFieldName, '\0', sizeof(pszFieldName));
				DBFGetFieldInfo(hDBF, iField, pszFieldName, NULL, NULL);
				std::string fieldname(pszFieldName);
				//避开“ID”字段
				if (fieldname == "ID") {
					fieldname = "DbfID";
					if (std::count(Attributes.AttributesNames.begin(), Attributes.AttributesNames.end(), fieldname) == 0)
						Attributes.AttributesNames.push_back(fieldname);
				}
				if (fieldtype == FTInteger) {
					int value = DBFReadIntegerAttribute(hDBF, i, iField);
					outFileToMap << value << '\t';
					Attributes.AttributesDouble[fieldname][route_index] = value;
				}
				if (fieldtype == FTDouble) {
					double value = DBFReadDoubleAttribute(hDBF, i, iField);
					outFileToMap << value << '\t';
					//outFileToMap << std::fixed << std::setprecision(4)<< value << '\t';
					Attributes.AttributesDouble[fieldname][route_index] = value;
				}
				if (fieldtype == FTString) {
					//不允许string做属性值，do nothing
				}
			}
			outFileToMap << std::endl;

			ref = last_count + i;
			Ref_to_Id[ref] = id;
			Id_to_Ref[id] = ref;
		}

		++loadedCount;
	}

	//删除没有数据的字段名
	for (auto it = Attributes.AttributesNames.begin(); it != Attributes.AttributesNames.end();) {
		if (Attributes.AttributesDouble.count(*it)) {
			it++;
		}
		else {
			it = Attributes.AttributesNames.erase(it);
		}
	}

	shapefile.Close();

	return count;
}

void ShapeFileAccessor::getCoordinateData(std::string shpfilename, std::vector<std::vector<double>> &coordinateData)
{
	std::vector<std::vector<double>>().swap(coordinateData);

	int result = shapefile.Open(shpfilename);
	shapeType = shapefile.GetType();

	if (result != 0) {
		AfxMessageBox(_T("Read Shape File failed"), MB_OK | MB_ICONERROR);
		return;
	}
	else {
		int count = shapefile.GetEntityCount();

		//申请空间
		//coordinateData.reserve(count * 4 * sizeof(double));
		std::vector<double> tmpVec;

		CString shapeType(shapefile.GetTypeString().c_str());

		ShapeObject spObject;

		for (int i = 0; i < count; i++)
		{
			shapefile.GetShape(i, spObject);

			if (spObject.GetVertexCount() == 2) {
				// Polyline only have two point 获取到路的两端节点坐标
				const sPoint<double>* points = spObject.GetVertices();

				tmpVec.clear();
				tmpVec.push_back(points[0].x);
				tmpVec.push_back(points[0].y);
				tmpVec.push_back(points[1].x);
				tmpVec.push_back(points[1].y);

				//存入数据
				coordinateData.push_back(tmpVec);
			}
		}
	
		shapefile.Close();

		return;
	}
}

void ShapeFileAccessor::BaseInputError(std::string str)
{
	std::string baseStr = " Input Error. Please Check";
	//std::string strT = str + baseStr;
	std::string strT = str;

	CString cstrT;
	cstrT = strT.c_str();
	AfxMessageBox(cstrT, MB_OK | MB_ICONERROR);
}

void ShapeFileAccessor::FileNotExist(std::string str)
{
	AfxMessageBox(_T("File not exist, please check"), MB_OK | MB_ICONERROR);
}

Graph* ShapeFileAccessor::ProcessShapeFile()
{
	if (g.m_vertices.size() > 0)
		return &g;

	multiThreadProcessShapeFile(this, this->myAttributes);

	return &g;
}

//计算平均夹角
void ShapeFileAccessor::calculate_Mean_Angle_of_Deviation() {

	//用夹角的两条边组成的三个端点id做key，去重
	std::map<std::set<int>, double> angleMap;
	for (auto it = this->AdjRoadAngle.begin(); it != this->AdjRoadAngle.end(); it++) {
		std::vector<int> roads = get_index_roads(it->first);
		//获取两条边的端点id
		std::set<int> nodes_set;
		for (int road_id : roads) {
			nodes_set.insert(this->roadNode2[road_id][0]);
			nodes_set.insert(this->roadNode2[road_id][1]);
		}
		//过滤掉180度的角
		if (int(it->second) == 180)
			continue;
		//插入一个夹角
		angleMap[nodes_set] = it->second;
	}

	
	this->Mean_Angle_of_Deviation = 0;
	double sum_angle = 0;
	for (auto it = angleMap.begin(); it != angleMap.end(); it++) {
		sum_angle += it->second;
	}
	this->Mean_Angle_of_Deviation = sum_angle / double(angleMap.size());
}

//获取平均夹角
double ShapeFileAccessor::get_Mean_Angle_of_Deviation() {
	return this->Mean_Angle_of_Deviation;
}



inline double AngleCalculate(double vec1_x, double vec1_y, double vec2_x, double vec2_y)
{
	double cosr, angle;

	//计算余弦：cos=a*b/[|a|*|b|]= (x1x2 + y1y2) / [√[x1 ^ 2 + y1 ^ 2] * √[x2 ^ 2 + y2 ^ 2]]
	cosr = (vec1_x * vec2_x + vec1_y * vec2_y) / sqrt(pow(vec1_x, 2) + pow(vec1_y, 2)) / sqrt(pow(vec2_x, 2) + pow(vec2_y, 2));
	cosr = max(-1, cosr);
	cosr = min(1, cosr);

	//计算角度
	angle = acos(cosr) * 180 / PI;

	return angle;
}

//首次计算路口间的夹角、构建每条路的邻接边集合
void ShapeFileAccessor::FirstCalculateAngles() {
	if (AdjRoadList.size() && AdjRoadAngle.size())
		return;

	//多线程计算夹角和邻接边
	multiThreadCalculateAngles(this);

}

void ShapeFileAccessor::thread_calculate_angle(ShapeFileAccessor *temp, int pos, int startPos, int roadLength,long long cpu_pos) {
	SetThreadAffinityMask(GetCurrentThread(), cpu_pos);
	int stopPos = startPos + roadLength;
	stopPos = min(stopPos, temp->roadNode2.size());
	for (int startRoad=startPos;startRoad< stopPos;startRoad++){
		//新建当前边的邻接表
		std::set<int> adj_list;
		AdjRoadListVec[pos].insert(std::make_pair(startRoad, adj_list));

		//计算当前边与邻接边的夹角
		double vec1_x = temp->Route2[startRoad][2] - temp->Route2[startRoad][0];
		double vec1_y = temp->Route2[startRoad][3] - temp->Route2[startRoad][1];

		//遍历查找邻接边
		int endNode = temp->roadNode2[startRoad][1];
		for (auto iter = temp->NodeStartRoad[endNode].begin(); iter != temp->NodeStartRoad[endNode].end(); iter++)
		{
			int endRoad = *iter;

			AdjRoadListVec[pos][startRoad].insert(endRoad);

			double vec2_x = temp->Route2[endRoad][2] - temp->Route2[endRoad][0];
			double vec2_y = temp->Route2[endRoad][3] - temp->Route2[endRoad][1];

			double angle = AngleCalculate(vec1_x, vec1_y, vec2_x, vec2_y);

			//添加计算的夹角
			AdjRoadAngleVec[pos].insert(std::make_pair(IndexToKey(startRoad, endRoad), angle));
		}
	}
	std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
	Flags[pos] = true;
}

void ShapeFileAccessor::calculateAngle(double angleLimit, double Jnc_t_limit)
{
	this->FirstCalculateAngles();

	multiThreadJudgeAngles(this, angleLimit, Jnc_t_limit);
}

inline int  ShapeFileAccessor::GetNodeNum(double x, double y) {

	std::string key = convertDtoStr(x) + "," + convertDtoStr(y);

	std::map<std::string, int>::iterator iter;

	iter = mapNodes.find(key);

	if (iter != mapNodes.end()) {
		return mapNodes[key];
	}
	else {
		lock.lock();
		int nodeNum = int(mapNodes.size());
		mapNodes.insert(std::pair<std::string, int>(key, nodeNum));
		lock.unlock();
		return nodeNum;
	}

}

inline int  ShapeFileAccessor::GetNodeNum2(int x, int y) {

	std::string key = std::to_string(x) + "," + std::to_string(y);

	std::map<std::string, int>::iterator iter;

	iter = mapNodes2.find(key);

	if (iter != mapNodes2.end()) {
		return mapNodes2[key];
	}
	else {
		int nodeNum = int(mapNodes2.size());
		mapNodes2.insert(std::pair<std::string, int>(key, nodeNum));
		return nodeNum;
	}

}

void ShapeFileAccessor::multiWriteFile(std::string filepath, std::string filename, AttributesData &Attributes) {
	std::string outShpPath = filepath + "/" + filename + ".shp";
	std::string outDbfPath = filepath + "/" + filename + ".dbf";

	if (Attributes.AttributesNames.size() == 0)
		return;

	//创建shp文件
	SHPHandle	hSHPHandle;
	SHPObject	*psShape;
	int nSHPType = SHPT_ARC;
	double	x[2], y[2], z[2], m[2];
	hSHPHandle = SHPCreate(outShpPath.c_str(), nSHPType);

	//写入shp数据
	int record = 0;
	for (int road_idx = 0; road_idx < Attributes.AttributesDouble["ID"].size(); road_idx++) {
		//添加shp数据
		for (int i = 0; i < 2; i++) {
			if (i == 0) {
				x[i] = Attributes.AttributesDouble["x1"][road_idx];
				y[i] = Attributes.AttributesDouble["y1"][road_idx];
			}
			else {
				x[i] = Attributes.AttributesDouble["x2"][road_idx];
				y[i] = Attributes.AttributesDouble["y2"][road_idx];
			}
			z[i] = 0;
			m[i] = 0;
		}

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL, 2, x, y, z, m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);
	}
	SHPClose(hSHPHandle);

	//创建dbf文件
	DBFHandle	hDBF;

	//创建Dbf文件		
	hDBF = DBFCreate(outDbfPath.c_str());

	//添加表列
	std::vector<std::string> vec;
	for (int i = 0; i < Attributes.AttributesNames.size(); i++) {
		if (Attributes.AttributesNames[i] == "ID" || Attributes.AttributesNames[i] == "Ref" ||
			Attributes.AttributesNames[i] == "x1" || Attributes.AttributesNames[i] == "y1" || 
			Attributes.AttributesNames[i] == "x2" || Attributes.AttributesNames[i] == "y2")
			continue;
		vec.push_back(Attributes.AttributesNames[i]);
		DBFAddField(hDBF, Attributes.AttributesNames[i].c_str(), FTDouble, 20, 4);
	}
	
	//对每个表列加载数据
	std::vector<std::string> vec2;
	for (int road_idx = 0; road_idx < Attributes.AttributesDouble["ID"].size(); road_idx++) {
		int column_idx = 0;
		for (int i = 0; i < Attributes.AttributesNames.size(); i++) {	//i就是对应字段在dbf文件中的位置
			if (Attributes.AttributesNames[i] == "ID" || Attributes.AttributesNames[i] == "Ref" ||
				Attributes.AttributesNames[i] == "x1" || Attributes.AttributesNames[i] == "y1" ||
				Attributes.AttributesNames[i] == "x2" || Attributes.AttributesNames[i] == "y2")
				continue;
			DBFWriteDoubleAttribute(hDBF, road_idx, column_idx, Attributes.AttributesDouble[Attributes.AttributesNames[i]][road_idx]);
			++column_idx;
			if (road_idx == 0) {
				vec2.push_back(Attributes.AttributesNames[i]);
			}
		}
	}

	//如果没有任何数据添入dbf，则添入ID数据
	if (vec2.size() == 0) {
		DBFAddField(hDBF, "ID", FTDouble, 20, 4);
		for (int road_idx = 0; road_idx < Attributes.AttributesDouble["ID"].size(); road_idx++) {
			DBFWriteDoubleAttribute(hDBF, road_idx, 0, Attributes.AttributesDouble["ID"][road_idx]);
		}
	}

	DBFClose(hDBF);
}


void ShapeFileAccessor::Multi_thread_for_WriteFile(std::string filepath, AttributesData &Attributes) {
	int pos = -1;
	for (int i = filepath.size() - 1; i >= 0; i--) {
		if (filepath[i] == '/') {
			pos = i;
			break;
		}
	}
	std::string dirpath = "";
	std::string filename = filepath;
	if (pos) {
		dirpath = filepath.substr(0, pos);
		filename=filepath.substr(pos+1);
	}
		
	std::string shpfilename = filename + ".shp";
	std::string dbffilename = filename + ".dbf";

	////判断shp文件是否存在，存在则删除
	//std::string shpfilepath = dirpath + "/" + shpfilename;
	//if (FILE *file = fopen(shpfilepath.c_str(), "r")) {
	//	fclose(file);

	//	//return;

	//	//删除现有文件夹
	//	QDir qDirPath = QString::fromLocal8Bit(dirpath);
	//	qDirPath.removeRecursively();
	//}

	//创建文件夹
	//bool flag = CreateDirectory(filepath.c_str(), NULL);
	QDir qFilePath = QString::fromLocal8Bit(filepath.c_str());
	qFilePath.mkpath(QString::fromLocal8Bit(filepath.c_str()));

	//增开一个detach线程写入shp文件,主线程不等待
	std::thread thrd(&ShapeFileAccessor::multiWriteFile, filepath, filename, Attributes);
	thrd.detach();
}

void ShapeFileAccessor::Multi_thread_for_ReadFile(ShapeFileAccessor *temp, std::string shpfilename, std::string idFieldIndex, AttributesData &Attributes) {

	//清空ostream
	buf.str("");
	inFileToMap.clear();
	outFileToMap.clear();

	//清空静态成员变量
	{
		std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
		Flags.clear();
	}
	AttributesDataVec.clear();

	//探测cpu逻辑核心数目
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	thread_num = int(si.dwNumberOfProcessors);

	//获取cpu最大提供线程数，超了会降低效率
	int threadMaxNum = std::thread::hardware_concurrency();
	thread_num = min(threadMaxNum, int(si.dwNumberOfProcessors)) * 2;

	//准备接数据结构
	std::vector<int> StartPosVec, ReadLengthVec;
	//计划各线程读取range
	Shapefile shapefile;
	shapefile.Open(shpfilename);
	int total_count = shapefile.GetEntityCount();
	shapefile.Close();
	int single_count = total_count / thread_num;
	for (int i = 0; i < thread_num; i++) {
		{
			std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
			Flags.push_back(false);
		}
		AttributesData AD;
		AttributesDataVec.push_back(AD);

		int length = single_count;
		if (i == thread_num - 1)
			length = total_count - single_count * (thread_num - 1);
		ReadLengthVec.push_back(length);
		StartPosVec.push_back(single_count*i);
	}

	//将线程跟CPU逻辑核心进行绑定，不允许出现线程切换
	int cpu_num = int(si.dwNumberOfProcessors); 
	long long cpu_pos = long long(pow(2, cpu_num - 1));

	//按边顺序依次启动
	for (int i = 0; i < thread_num; i++)
	{
		std::thread thrd(&ShapeFileAccessor::multiReadFile, temp, shpfilename,idFieldIndex, i, StartPosVec[i], ReadLengthVec[i],cpu_pos);
		thrd.detach();

		cpu_pos = cpu_pos >> 1;
		if (cpu_pos == 0)
		{
			cpu_pos = long long(pow(2, cpu_num - 1));
		}
	}

	//循环等待全部结束，拼接文件流
	while (true) {
		bool isAllFinished = true;
		for (int i = 0; i < thread_num; i++){
			{
				std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
				if (!Flags[i])
					isAllFinished = false;
			}
			
		}
		if (isAllFinished) {	//文件流拼接完毕，但是Attributes这里还需要拼接
			Attributes.AttributesNames = AttributesDataVec[0].AttributesNames;
			Attributes.AttributesDouble = AttributesDataVec[0].AttributesDouble;
			for (int i = 1; i < thread_num; i++) {
				for (auto it = Attributes.AttributesDouble.begin(); it != Attributes.AttributesDouble.end(); it++) {
					it->second.insert(AttributesDataVec[i].AttributesDouble[it->first].begin(), AttributesDataVec[i].AttributesDouble[it->first].end());
				}
			}

			break;
		}
	}
}

void ShapeFileAccessor::multiThreadCalculateAngles(ShapeFileAccessor *temp) {
	//清空静态成员变量
	{
		std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
		Flags.clear();
		for (int i = 0; i < thread_num; i++) {
			Flags.push_back(false);
		}
	}
	AdjRoadListVec.clear();
	AdjRoadAngleVec.clear();

	//准备接数据结构
	std::vector<int> StartPosVec, ReadLengthVec;
	int total_count = temp->roadNode2.size();
	int single_count = total_count / thread_num;
	for (int i = 0; i < thread_num; i++) {
		std::map<int, std::set<int>> tmpAdjRoadList;
		AdjRoadListVec.push_back(tmpAdjRoadList);

		std::map<std::string, double> tmpAdjRoadAngle;
		AdjRoadAngleVec.push_back(tmpAdjRoadAngle);

		int length = single_count;
		if (i == thread_num - 1)
			length = total_count - single_count * (thread_num - 1);
		ReadLengthVec.push_back(length);
		StartPosVec.push_back(single_count*i);
	}

	//将线程跟CPU逻辑核心进行绑定，不允许出现线程切换
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpu_num = int(si.dwNumberOfProcessors); 
	long long cpu_pos = long long(pow(2, cpu_num - 1));

	//按边顺序依次启动
	for (int i = 0; i < thread_num; i++)
	{
		std::thread thrd(&ShapeFileAccessor::thread_calculate_angle, temp, i, StartPosVec[i], ReadLengthVec[i],cpu_pos);
		thrd.detach();

		//设置用哪个CPU核心处理该线程			
		
		cpu_pos = cpu_pos >> 1;
		if (cpu_pos == 0)
		{
			cpu_pos = long long(pow(2, cpu_num - 1));
		}
	}

	//循环等待全部结束
	while (true) {
		bool isAllFinished = true;
		for (int i = 0; i < thread_num; i++) {
			{
				std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
				if (!Flags[i])
					isAllFinished = false;
			}
		}
		if (isAllFinished) {	
			for (int i = 0; i < thread_num; i++) {
				temp->AdjRoadList.insert(AdjRoadListVec[i].begin(), AdjRoadListVec[i].end());
				temp->AdjRoadAngle.insert(AdjRoadAngleVec[i].begin(), AdjRoadAngleVec[i].end());
			}

			//Add:计算平均夹角
			temp->calculate_Mean_Angle_of_Deviation();

			break;
		}
	}
}

void ShapeFileAccessor::thread_process_shapefile(ShapeFileAccessor *temp, int pos, int startPos, int roadLength) {
	Shapefile shapefile;
	int result = shapefile.Open(temp->m_filepath);

	if (result != 0) {
		AfxMessageBox(_T("Read Shape File failed"), MB_OK | MB_ICONERROR);
		return;
	}
	
	int count = shapefile.GetEntityCount();

	CString shapeType(shapefile.GetTypeString().c_str());

	ShapeObject spObject;

	int edgeIndex = startPos;
	int stopPos = startPos + roadLength;
	for (int i = startPos; i < stopPos; i++){
		shapefile.GetShape(i, spObject);

		if (spObject.GetVertexCount() == 2) {
			const sPoint<double>* points = spObject.GetVertices();

			//计算路长
			double dis = calculateDistance(points[0].x, points[0].y, points[1].x, points[1].y);

			//获取路的两个节点编号,一个坐标组是一个节点编号
			int nodeNum1 = temp->GetNodeNum(points[0].x, points[0].y);
			int nodeNum2 = temp->GetNodeNum(points[1].x, points[1].y);
			int route_index = spObject.GetIndex();
			roadIDVec[pos].insert(route_index);

			//添加每条路的两个节点ID
			std::vector<int> roadIn{ nodeNum1,nodeNum2 };
			std::vector<int> roadIn2{ nodeNum2,nodeNum1 };
			roadNodeVec[pos].insert(std::make_pair(route_index, roadIn));
			roadNode2Vec[pos].insert(std::make_pair(route_index, roadIn));
			roadNode2Vec[pos].insert(std::make_pair(route_index+count, roadIn2));

			//为指定起点、终点输出最短路径准备数据
			nodeToEdgeVec[pos].insert(std::make_pair(IndexToKey(nodeNum1, nodeNum2), route_index));
			nodeToEdgeVec[pos].insert(std::make_pair(IndexToKey(nodeNum2, nodeNum1), route_index));

			//添加node为起点的路id
			NodeStartRoadVec[pos][nodeNum1].insert(route_index);
			NodeStartRoadVec[pos][nodeNum2].insert(route_index + count);

			//添加每条路的长度
			LengthVec[pos].insert(std::make_pair(route_index, dis));

			//添加每条路的两个节点坐标
			std::vector<double> tmp{ points[0].x, points[0].y, points[1].x, points[1].y };
			std::vector<double> tmp2{ points[1].x, points[1].y, points[0].x, points[0].y };
			RouteVec[pos].insert(std::make_pair(route_index, tmp));
			Route2Vec[pos].insert(std::make_pair(route_index, tmp));
			Route2Vec[pos].insert(std::make_pair(route_index + count, tmp2));

			//将路的节点两端编号、路长添加入图g
			EdgeProperty ep;
			ep.m_base = dis;
			ep.m_value = edgeIndex;
			boost::add_edge(nodeNum1, nodeNum2, ep, gVec[pos]);

			edgeIndex++;
		}
	}	
	shapefile.Close();
	std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
	Flags[pos] = true;
}

void ShapeFileAccessor::thread_process_attributes(ShapeFileAccessor *temp, AttributesData &Attributes, int pos, int startPos, int roadLength, long long cpu_pos) {
	int count = Attributes.AttributesDouble["ID"].size();

	int edgeIndex = startPos;
	int stopPos = startPos + roadLength;
	for (int row_idx = startPos; row_idx < stopPos; row_idx++) {

		//计算路长
		double dis = calculateDistance(Attributes.AttributesDouble["x1"][row_idx], Attributes.AttributesDouble["y1"][row_idx], 
			Attributes.AttributesDouble["x2"][row_idx], Attributes.AttributesDouble["y2"][row_idx]);

		//获取路的两个节点编号,一个坐标组是一个节点编号
		int nodeNum1 = temp->GetNodeNum(Attributes.AttributesDouble["x1"][row_idx], Attributes.AttributesDouble["y1"][row_idx]);
		int nodeNum2 = temp->GetNodeNum(Attributes.AttributesDouble["x2"][row_idx], Attributes.AttributesDouble["y2"][row_idx]);
		int route_index = Attributes.AttributesDouble["ID"][row_idx];
		roadIDVec[pos].insert(route_index);

		//添加每条路的两个节点ID
		std::vector<int> roadIn{ nodeNum1,nodeNum2 };
		std::vector<int> roadIn2{ nodeNum2,nodeNum1 };
		roadNodeVec[pos].insert(std::make_pair(route_index, roadIn));
		roadNode2Vec[pos].insert(std::make_pair(route_index, roadIn));
		roadNode2Vec[pos].insert(std::make_pair(route_index + count, roadIn2));

		//为指定起点、终点输出最短路径准备数据
		nodeToEdgeVec[pos].insert(std::make_pair(IndexToKey(nodeNum1, nodeNum2), route_index));
		nodeToEdgeVec[pos].insert(std::make_pair(IndexToKey(nodeNum2, nodeNum1), route_index));

		//添加node为起点的路id
		NodeStartRoadVec[pos][nodeNum1].insert(route_index);
		NodeStartRoadVec[pos][nodeNum2].insert(route_index + count);

		//添加每条路的长度
		LengthVec[pos].insert(std::make_pair(route_index, dis));

		//添加每条路的两个节点坐标
		std::vector<double> tmp{ Attributes.AttributesDouble["x1"][row_idx], Attributes.AttributesDouble["y1"][row_idx],
			Attributes.AttributesDouble["x2"][row_idx], Attributes.AttributesDouble["y2"][row_idx] };
		std::vector<double> tmp2{ Attributes.AttributesDouble["x2"][row_idx], Attributes.AttributesDouble["y2"][row_idx],
			Attributes.AttributesDouble["x1"][row_idx], Attributes.AttributesDouble["y1"][row_idx] };
		RouteVec[pos].insert(std::make_pair(route_index, tmp));
		Route2Vec[pos].insert(std::make_pair(route_index, tmp));
		Route2Vec[pos].insert(std::make_pair(route_index + count, tmp2));

		//将路的节点两端编号、路长添加入图g
		EdgeProperty ep;
		ep.m_base = dis;
		ep.m_value = edgeIndex;
		boost::add_edge(nodeNum1, nodeNum2, ep, gVec[pos]);

		edgeIndex++;
	}
	std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
	Flags[pos] = true;
}

void ShapeFileAccessor::thread_judge_angle(ShapeFileAccessor *temp, int pos, int startPos, int roadLength, double angleLimit, double Jnc_t_limit, long long cpu_pos) {
	SetThreadAffinityMask(GetCurrentThread(), cpu_pos);
	int stopPos = startPos + roadLength;
	stopPos = min(stopPos, temp->roadNode2.size());
	for (int startRoad = startPos; startRoad < stopPos; startRoad++){
		//遍历查找邻接边
		int endNode = temp->roadNode2[startRoad][1];
		for (auto iter = temp->NodeStartRoad[endNode].begin(); iter != temp->NodeStartRoad[endNode].end(); iter++)
		{
			int endRoad = *iter;

			//判断节点是否为交叉路口
			if (Jnc_t_limit < INT_MAX - 10) {
				if (int(temp->NodeStartRoad[endNode].size()) >= Jnc_t_limit)
					JncVec[pos][endNode] = 1;
				else
					JncVec[pos][endNode] = 0;
			}

			if (angleLimit < INT_MAX - 10) {
				//判断拐弯
				if (temp->AdjRoadAngle[IndexToKey(startRoad, endRoad)] < angleLimit) {
					AdjTurnVec[pos][IndexToKey(startRoad, endRoad)] = 0;
					AdjTurnMPVec[pos][startRoad][endRoad] = 0;
					DcVec[pos][endNode] = 0;
				}
				else {
					AdjTurnVec[pos][IndexToKey(startRoad, endRoad)] = 1;
					AdjTurnMPVec[pos][startRoad][endRoad] = 1;
					DcVec[pos][endNode] = 1;
				}
			}
		}
	}
	std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
	Flags[pos] = true;
}

void ShapeFileAccessor::multiThreadJudgeAngles(ShapeFileAccessor *temp, double angleLimit, double Jnc_t_limit) {
	//清空old数据
	temp->AdjTurn.clear();
	temp->AdjTurnMP.clear();
	temp->Jnc.clear();
	temp->Dc.clear();
	temp->AdjTurnTP.clear();
	{
		std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
		Flags.clear();
	}
	AdjTurnVec.clear();
	AdjTurnMPVec.clear();
	JncVec.clear();
	DcVec.clear();

	if (!(Jnc_t_limit < INT_MAX - 10) && !(angleLimit < INT_MAX - 10))
		return;

	//准备接数据结构
	std::vector<int> StartPosVec, ReadLengthVec;
	int total_count = temp->roadNode2.size();
	int single_count = total_count / thread_num;
	for (int i = 0; i < thread_num; i++) {
		{
			std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
			Flags.push_back(false);
		}
		std::map <std::string, int > tmpAdjTurn;
		std::unordered_map<int, std::unordered_map<int, int>> tmpAdjTurnMP;
		std::map<int, int> tmpJnc;	
		std::map<int, int> tmpDc;	
		AdjTurnVec.push_back(tmpAdjTurn);
		AdjTurnMPVec.push_back(tmpAdjTurnMP);
		JncVec.push_back(tmpJnc);
		DcVec.push_back(tmpDc);

		int length = single_count;
		if (i == thread_num - 1)
			length = total_count - single_count * (thread_num - 1);
		ReadLengthVec.push_back(length);
		StartPosVec.push_back(single_count*i);
	}

	//将线程跟CPU逻辑核心进行绑定，不允许出现线程切换
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpu_num = int(si.dwNumberOfProcessors);
	long long cpu_pos = long long(pow(2, cpu_num - 1));

	//按边顺序依次启动
	for (int i = 0; i < thread_num; i++)
	{
		std::thread thrd(&ShapeFileAccessor::thread_judge_angle, temp, i, StartPosVec[i], ReadLengthVec[i], angleLimit, Jnc_t_limit, cpu_pos);
		thrd.detach();

		//计算下一个核心的cpu_pos		
		cpu_pos = cpu_pos >> 1;
		if (cpu_pos == 0)
		{
			cpu_pos = long long(pow(2, cpu_num - 1));
		}
	}

	//循环等待全部结束
	while (true) {
		bool isAllFinished = true;
		for (int i = 0; i < thread_num; i++) {
			//OutputDebugString((std::to_string(i) + ":" + std::to_string(Flags[i]) + "\n").c_str());
			{
				std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
				if (!Flags[i])
					isAllFinished = false;
			}
		}
		if (isAllFinished) {

			for (int i = 0; i < thread_num; i++) {
				temp->AdjTurn.insert(AdjTurnVec[i].begin(), AdjTurnVec[i].end());
				temp->AdjTurnMP.insert(AdjTurnMPVec[i].begin(), AdjTurnMPVec[i].end());
				temp->Jnc.insert(JncVec[i].begin(), JncVec[i].end());
				temp->Dc.insert(DcVec[i].begin(), DcVec[i].end());
			}
			//收集转弯数据
			int total = temp->roadID.size();
			for (auto it = temp->AdjTurnMP.begin(); it != temp->AdjTurnMP.end(); it++) {
				int startEdge = it->first%total; //有向路的无向id
				for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
					int endEdge = iter->first%total; //有向路it连接的路iter的无向id
					temp->AdjTurnTP[startEdge][endEdge] = iter->second;
				}
			}
			//计算总的交叉口、转弯数目
			temp->dc_all = 0, temp->jnc_all = 0;
			for (auto it = temp->Dc.begin(); it != temp->Dc.end(); it++) {
				temp->dc_all += it->second;
			}
			for (auto it = temp->Jnc.begin(); it != temp->Jnc.end(); it++) {
				temp->jnc_all += it->second;
			}

			break;
		}
	}
}

void ShapeFileAccessor::multiThreadProcessShapeFile(ShapeFileAccessor *temp, AttributesData &Attributes) {
	//清空old数据
	{
		std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
		Flags.clear();
		for (int i = 0; i < thread_num; i++) {
			Flags.push_back(false);
		}
	}
	roadIDVec.clear();
	roadNodeVec.clear();
	roadNode2Vec.clear();
	RouteVec.clear();
	Route2Vec.clear();
	NodeStartRoadVec.clear();
	LengthVec.clear();
	gVec.clear();
	nodeToEdgeVec.clear();

	//准备接数据结构
	std::vector<int> StartPosVec, ReadLengthVec;
	int total_count = Attributes.AttributesDouble["ID"].size();
	int single_count = total_count / thread_num;
	for (int i = 0; i < thread_num; i++) {
		std::set<int> tmp_roadID;
		std::map<int, std::vector<int>> tmp_roadNode;		//无向图每条道路对应的端点编号
		std::map<int, std::vector<int>> tmp_roadNode2;		//有向图每条道路对应的端点编号
		std::map<int, std::vector<double>> tmp_Route;	//无向图路的坐标数据
		std::map<int, std::vector<double>> tmp_Route2;	//有向图路的坐标数据
		std::map<int, std::set<int>> tmp_NodeStartRoad;
		std::map<int, double> tmp_Length;	//无向图路长
		Graph tmp_g;
		std::map<std::string, int> tmp_nodeToEdge;
		std::map<std::string, int>  tmp_mapNodes;	//坐标数据映射得到的端点ID
		std::map<std::string, int> tmp_mapNodes2;	//两条路的ID映射得到的两边连接ID
		
		roadIDVec.push_back(tmp_roadID);
		roadNodeVec.push_back(tmp_roadNode);
		roadNode2Vec.push_back(tmp_roadNode2);
		RouteVec.push_back(tmp_Route);
		Route2Vec.push_back(tmp_Route2);
		NodeStartRoadVec.push_back(tmp_NodeStartRoad);
		LengthVec.push_back(tmp_Length);
		gVec.push_back(tmp_g);
		nodeToEdgeVec.push_back(tmp_nodeToEdge);

		int length = single_count;
		if (i == thread_num - 1)
			length = total_count - single_count * (thread_num - 1);
		ReadLengthVec.push_back(length);
		StartPosVec.push_back(single_count*i);
	}

	//将线程跟CPU逻辑核心进行绑定，不允许出现线程切换
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpu_num = int(si.dwNumberOfProcessors);
	long long cpu_pos = long long(pow(2, cpu_num - 1));

	//按边顺序依次启动
	for (int i = 0; i < thread_num; i++)
	{
		std::thread thrd(&ShapeFileAccessor::thread_process_attributes, temp, Attributes, i, StartPosVec[i], ReadLengthVec[i], cpu_pos);
		thrd.detach();

		//设置用哪个CPU核心处理该线程			
		
		cpu_pos = cpu_pos >> 1;
		if (cpu_pos == 0)
		{
			cpu_pos = long long(pow(2, cpu_num - 1));
		}
	}

	//循环等待全部结束
	while (true) {
		bool isAllFinished = true;
		for (int i = 0; i < thread_num; i++) {
			{
				std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
				if (!Flags[i])
					isAllFinished = false;
			}
		}
		if (isAllFinished) {
			for (int i = 0; i < thread_num; i++) {
				temp->roadID.insert(roadIDVec[i].begin(), roadIDVec[i].end());
				temp->roadNode.insert(roadNodeVec[i].begin(), roadNodeVec[i].end());
				temp->roadNode2.insert(roadNode2Vec[i].begin(), roadNode2Vec[i].end());
				temp->Route.insert(RouteVec[i].begin(), RouteVec[i].end());
				temp->Route2.insert(Route2Vec[i].begin(), Route2Vec[i].end());
				//temp->NodeStartRoad.insert(NodeStartRoadVec[i].begin(), NodeStartRoadVec[i].end());
				for (auto it = NodeStartRoadVec[i].begin(); it != NodeStartRoadVec[i].end(); it++) {
					if (temp->NodeStartRoad.count(it->first)) {
						temp->NodeStartRoad[it->first].insert(it->second.begin(), it->second.end());
					}
					else {
						temp->NodeStartRoad[it->first] = it->second;
					}
				}
				temp->Length.insert(LengthVec[i].begin(), LengthVec[i].end());
				temp->nodeToEdge.insert(nodeToEdgeVec[i].begin(), nodeToEdgeVec[i].end());

				//距离无向图
				temp->g.copy_impl(gVec[i]);
			}

			//计算邻接边
			if (temp->AdjRoadList.size() == 0) {
				//一条道路的两个方向，it是道路的一个方向，it+total是另一个方向。
				for (auto it = temp->roadNode2.begin(); it != temp->roadNode2.end(); it++) {
					//it->first是道路的有向id,范围0~2*total-1。0~total-1是一个方向，total~2total-1是第二个方向。假如取余total相同，则是同一条路。
					int startRoad = it->first;
					int endNode = temp->roadNode2[startRoad][1];//(it->second)[1]
					//找到以endNode为startNode的道路，确保一个方向。
					for (auto iter = temp->NodeStartRoad[endNode].begin(); iter != temp->NodeStartRoad[endNode].end(); iter++)
					{
						int endRoad = *iter;
						temp->AdjRoadList[startRoad].insert(endRoad);
					}
				}
			}
			if (temp->Road_to_roads.size() == 0) {
				int total = temp->roadID.size();
				for (auto it = temp->AdjRoadList.begin(); it != temp->AdjRoadList.end(); it++) {
					int road1 = (it->first%total);
					for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
						int road2 = ((*iter) % total);
						temp->Road_to_roads[road1].insert(road2);
						temp->Road_to_roads[road2].insert(road1);
					}
				}
			}
			//计算总路长
			temp->length_all = 0;
			for (int id = 0; id < temp->roadID.size(); id++) {
				temp->length_all += temp->Length[id];
			}

			break;
		}
	}
}

void ShapeFileAccessor::multiReadFile(ShapeFileAccessor *temp, std::string shpfilename, std::string idFieldIndex, int pos,
	int startPos, int readLength, long long cpu_pos) {
	SetThreadAffinityMask(GetCurrentThread(), cpu_pos);
	std::stringbuf tmp_buf;
	std::istream inFile(&tmp_buf);
	std::ostream outFile(&tmp_buf);

	//读取指定范围数据
	std::string dbFilePath = shpfilename.substr(0, shpfilename.length() - 4) + ".dbf";

	//获取shp数据
	Shapefile shapefile;
	int result = shapefile.Open(shpfilename);
	int shapeType = shapefile.GetType();

	if (result != 0) {
		AfxMessageBox(_T("Read Shape File failed"), MB_OK | MB_ICONERROR);
		return;
	}

	DBFHandle	hDBF;
	hDBF = DBFOpen(dbFilePath.c_str(), "rb+");
	int fieldIndex;

	int count = shapefile.GetEntityCount();
	int fieldCount = DBFGetFieldCount(hDBF);
	std::string firstLineStr;
	char pszFieldName[100];

	if (idFieldIndex != "FID") {
		if (hDBF == NULL) {
			AfxMessageBox(_T("Read dbf File failed"), MB_OK | MB_ICONERROR);
			shapefile.Close();
			return;
		}
		fieldIndex = DBFGetFieldIndex(hDBF, idFieldIndex.c_str());
		if (fieldIndex < 0) {
			AfxMessageBox(_T("Read dbf File failed"), MB_OK | MB_ICONERROR);
			shapefile.Close();
			return;
		}
	}

	if (pos == 0) {	//首线程才允许构造首行
		outFile << "Ref\tx1\ty1\tx2\ty2\tID\t";
		firstLineStr = "Ref\tx1\ty1\tx2\ty2\tID\t";

		for (int iField = 0; iField < fieldCount; iField++) {
			memset(pszFieldName, '\0', sizeof(pszFieldName));
			DBFGetFieldInfo(hDBF, iField, pszFieldName, NULL, NULL);
			std::string fieldname(pszFieldName);
			if (fieldname == "ID") {
				fieldname = "DbfID";
			}
			DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, iField, NULL, NULL, NULL);
			if (fieldtype == FTInteger || fieldtype == FTDouble) {
				outFile << fieldname << '\t';
				firstLineStr += fieldname + '\t';
			}
		}
		outFile << std::endl;

		//收集属性列的名字
		getAttributesNames(firstLineStr, AttributesDataVec[pos].AttributesNames);
	}

	ShapeObject spObject;
	int id, ref, stopPos = startPos + readLength;
	for (int i = startPos; i < stopPos; i++)
	{
		shapefile.GetShape(i, spObject);

		if (spObject.GetVertexCount() == 2) {
			// Polyline only have two point 获取到路的两端节点坐标
			const sPoint<double>* points = spObject.GetVertices();

			int route_index = spObject.GetIndex();
			if (idFieldIndex != "FID") {
				id = DBFReadIntegerAttribute(hDBF, i, fieldIndex);
			}
			else
			{
				id = i;
			}

			outFile << route_index << '\t' << std::setprecision(16) << points[0].x << '\t' << std::setprecision(16) <<
				points[0].y << '\t' << std::setprecision(16) << points[1].x << '\t' << std::setprecision(16) <<
				points[1].y << '\t' << id << '\t';
			AttributesDataVec[pos].AttributesDouble["Ref"][route_index] = route_index;
			AttributesDataVec[pos].AttributesDouble["x1"][route_index] = points[0].x;
			AttributesDataVec[pos].AttributesDouble["y1"][route_index] = points[0].y;
			AttributesDataVec[pos].AttributesDouble["x2"][route_index] = points[1].x;
			AttributesDataVec[pos].AttributesDouble["y2"][route_index] = points[1].y;
			AttributesDataVec[pos].AttributesDouble["ID"][route_index] = id;

			temp->GetNodeNum(points[0].x, points[0].y);
			temp->GetNodeNum(points[1].x, points[1].y);

			for (int iField = 0; iField < fieldCount; iField++) {
				DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, iField, NULL, NULL, NULL);
				memset(pszFieldName, '\0', sizeof(pszFieldName));
				DBFGetFieldInfo(hDBF, iField, pszFieldName, NULL, NULL);
				std::string fieldname(pszFieldName);
				if (fieldtype == FTInteger) {
					int value = DBFReadIntegerAttribute(hDBF, i, iField);
					outFile << value << '\t';
					AttributesDataVec[pos].AttributesDouble[fieldname][route_index] = value;
				}
				if (fieldtype == FTDouble) {
					double value = DBFReadDoubleAttribute(hDBF, i, iField);
					outFile << value << '\t';
					AttributesDataVec[pos].AttributesDouble[fieldname][route_index] = value;
				}
				if (fieldtype == FTString) {
					//不允许string做属性值，do nothing
				}
			}
			outFile << std::endl;
		}
	}

	shapefile.Close();

	//等待输出
	if (pos == 0) {	//首线程，直接输出
		outFileToMap << tmp_buf.str();
	}
	else {	//必须前面i个线程都输出完了才轮到该线程输出
		while (true) {
			bool isPreOver = true;
			for (int i = 0; i < pos; i++) {
				{
					std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
					if (!Flags[i])
						isPreOver = false;
				}
			}
			if (isPreOver) {
				outFileToMap << tmp_buf.str();
				break;
			}	
		}
	}
	std::lock_guard<std::mutex> flagsLock(FlagsMutex); //2023-10-12 xlj 上锁
	Flags[pos] = true;
}

void ShapeFileAccessor::multiThreadReadFile(std::string shpfilename, std::string idFieldIndex, AttributesData &Attributes) {
	Multi_thread_for_ReadFile(this, shpfilename, idFieldIndex, Attributes);
}
