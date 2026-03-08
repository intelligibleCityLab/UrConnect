#pragma once
#include "Shapefile.h"
#include "ShapeObject.h"
#include <math.h>
#include <atomic>
#include <map>
#include "GraphDefine.h"
#include <iostream>
#include "Attributes.h"
#include <tuple>

class ShapeFileAccessor
{
public:

	/*my data*/
	int shapeType;
	std::set<int> roadID;
	std::map<int, std::vector<int>> roadNode;		//无向图每条道路对应的端点编号
	std::map<int, std::vector<int>> roadNode2;		//有向图每条道路对应的端点编号
	std::map<int, std::vector<double>> Route;	//无向图路的坐标数据
	std::map<int, std::vector<double>> Route2;	//有向图路的坐标数据
	std::map<int, std::set<int>> NodeStartRoad;
	std::map<int, double> Length;	//无向图路长
	std::map<int, double> Length2;	//有向图路长
	std::map<int, std::set<int>> AdjRoadList;	//有向图每条边的邻接边队列
	std::map<std::string, double> AdjRoadAngle;	//有向图两条边之间的夹角，key是(边1，边2)有序对
	std::map <std::string, int > AdjTurn;	//有向图两条边之间是否有拐弯
	std::unordered_map<int, std::unordered_map<int, int>> AdjTurnMP;
	std::unordered_map<int, std::unordered_map<int, int>> AdjTurnTP;
	std::map<int, int> Jnc;	//道路端点是否为交叉路口(0/1)
	std::map<int, int> Dc;	//道路端点是否存在转弯路(0/1)
	std::map<int, std::set<int>> Road_to_roads;
	double length_all = 0;
	int dc_all = 0;
	int jnc_all = 0;

	//输出指定起点、终点最短路径需要使用
	std::map<std::string, int> nodeToEdge;	
	std::map<std::string, int>  mapNodes;	//坐标数据映射得到的端点ID
	std::map<std::string, int> mapNodes2;	//两条路的ID映射得到的两边连接ID

	AttributesData myAttributes;

	void clearOldData();

private:
	std::string m_filepath;

protected:

	Shapefile shapefile;	
	Graph g;

public:
	void init(std::string filepath, AttributesData &Attributes);
	Graph* ProcessShapeFile();	
	double Mean_Angle_of_Deviation = 0;
	void calculate_Mean_Angle_of_Deviation();
	double get_Mean_Angle_of_Deviation();

	inline int   GetNodeNum(double x, double y);
	inline int   GetNodeNum2(int x, int y);
	void calculateAngle(double angleLimit, double Jnc_t_limit);
	void FirstCalculateAngles();

	int generateTXT(std::string shpfilename, std::string txtFileName, std::string idFieldIndex, 
		int last_count, std::map<int, int> &Ref_to_Id, std::map<int, int> &Id_to_Ref);
	int generateFileStream(std::string shpfilename, std::string idFieldIndex,
		int last_count, std::map<int, int> &Ref_to_Id, std::map<int, int> &Id_to_Ref);
	int generateDispalyStream(std::string shpfilename, std::string idFieldIndex, AttributesData &Attributes,
		std::map<int, int> &Ref_to_Id, std::map<int, int> &Id_to_Ref);
	void getCoordinateData(std::string shpfilename, std::vector<std::vector<double>> &coordinateData);

	void BaseInputError(std::string str="");
	void FileNotExist(std::string str = "");

	void multiThreadReadFile(std::string shpfilename, std::string idFieldIndex, AttributesData &Attributes);

public:

	//多线程读取shp文件
	static int thread_num;
	static std::vector<bool> Flags;
	static std::vector<AttributesData> AttributesDataVec;
	static void multiReadFile(ShapeFileAccessor *temp,std::string shpfilename, std::string idFieldIndex, int pos, int startPos, int readLength,long long cpu_pos);
	static void Multi_thread_for_ReadFile(ShapeFileAccessor *temp,std::string shpfilename, std::string idFieldIndex, AttributesData &Attributes);

	//多线程写入shp文件
	static void multiWriteFile(std::string filepath, std::string filename, AttributesData &Attributes);
	static void Multi_thread_for_WriteFile(std::string filepath, AttributesData &Attributes);

	//多线程计算夹角
	static std::vector<std::map<int, std::set<int>>> AdjRoadListVec;
	static std::vector<std::map<std::string, double>> AdjRoadAngleVec;
	static void thread_calculate_angle(ShapeFileAccessor *temp, int pos, int startPos, int roadLength,long long cpu_pos);
	static void multiThreadCalculateAngles(ShapeFileAccessor *temp);

	//多线程计算转往和交叉路口
	static std::vector < std::map <std::string, int >> AdjTurnVec;	//有向图两条边之间是否有拐弯
	static std::vector < std::unordered_map<int, std::unordered_map<int, int>>> AdjTurnMPVec;
	static std::vector < std::map<int, int>> JncVec;	//道路端点是否为交叉路口(0/1)
	static std::vector < std::map<int, int>> DcVec;	//道路端点是否存在转弯路(0/1)
	static void thread_judge_angle(ShapeFileAccessor *temp, int pos, int startPos, int roadLength, double angleLimit, double Jnc_t_limit, long long cpu_pos);
	static void multiThreadJudgeAngles(ShapeFileAccessor *temp, double angleLimit, double Jnc_t_limit);

	//多线程读取坐标数据构造地图
	static std::vector<std::set<int>> roadIDVec;
	static std::vector < std::map<int, std::vector<int>>> roadNodeVec;		//无向图每条道路对应的端点编号
	static std::vector < std::map<int, std::vector<int>>> roadNode2Vec;		//有向图每条道路对应的端点编号
	static std::vector < std::map<int, std::vector<double>>> RouteVec;	//无向图路的坐标数据
	static std::vector < std::map<int, std::vector<double>>> Route2Vec;	//有向图路的坐标数据
	static std::vector < std::map<int, std::set<int>>> NodeStartRoadVec;
	static std::vector < std::map<int, double>> LengthVec;	//无向图路长
	static std::vector < std::map<std::string, int>> nodeToEdgeVec;
	static std::vector<Graph> gVec;
	static void thread_process_shapefile(ShapeFileAccessor *temp, int pos, int startPos, int roadLength);
	static void thread_process_attributes(ShapeFileAccessor *temp, AttributesData &Attributes, int pos, int startPos, int roadLength, long long cpu_pos);
	static void multiThreadProcessShapeFile(ShapeFileAccessor *temp, AttributesData &Attributes);

};
