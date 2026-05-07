#pragma once

#include "GraphDefine.h"
#include "ShapeFileAccessor.h"
#include <stdlib.h>
#include <string.h>
#include "shapefil.h"
#include <climits>
#include "Definition.h"
#include <thread>
#include <mutex>

struct Netreach
{
	std::vector<int> From_OID;
	std::vector<int> To_OID;
	std::vector<double>  Radius;
	std::vector<double> DirChg;
	std::vector<double> JncLmt;
	//剩下的要计算
	std::vector<double> Relen;
	std::vector<int> Jnc;
	std::vector<std::map<std::string, double>> Wgt;	//Wgt，即ddl
};

struct Geodesics
{
	std::vector<int> From_OID;
	std::vector<int> To_OID;
	//剩下的要计算
	std::vector<int> DC;
	std::vector<double> DD;
	std::vector<double> DDL;
	std::vector<std::map<std::string, double>> WDD;
	std::vector<double>  PathLen;
	std::vector<int> Jnc;		//如选择计算 Junction Distance 则撇掉 
	std::vector<std::map<std::string, double>> Wgt;	//Wgt，即ddl
};

class Calculation
{
public:
	//输入、输出路径
	std::string infilepath;
	std::string outfilepath;
	std::string dbfFilePath;
	std::string csvFilePath;
	//MR开关
	bool isMR = false;
	bool isMD = false;
	//DR开关
	bool isDR = false;
	bool isDDL = false;
	bool isMDR = false;
	//JncR开关
	bool isJncR = false; //Junction Reach
	bool isJncDDL = false; 
	bool isJncD = false;//Junction Distance
	//Option functions开关
	bool isJnc = false; //computing junctions
	bool isWgt = false; //computing weighting
	//Visualization开关
	bool isNet = false, isGeo = false;

	//MR参数
	std::set<double> MRLimitSet;
	//DR参数
	std::set<double> MRLimitSet2;
	double angleLimitDR = -1;
	std::set<double> DRLimitSet;
	//JncR参数
	double Jnc_t_limit_JncR = -1;
	std::set<double> Jnc_maxNumSet;
	double angleLimitJncDDL = -1;
	//Option functions参数
	double Jnc_t_limit_Jnc = -1;
	std::string WgtLimitStr;
	std::map<std::string, std::map<int, double>> weight;
	double weight_all = 0;
	//Visualization参数
	std::vector<int> FromIDVec;
	std::vector<int> ToIDVec;
	std::map<int, std::set<int>> FromToMap;
	bool isOneToOne = false;
	bool outputPath = false;
	std::map<int, std::set<int>> reachRoad_all;
	std::map<int, std::map<int, double>> partInReachRoadLen;
	//起点id->终点id->start_node->对应引出的线长
	std::map<int, std::map<int, std::map<int, double>>> partInReachRoadNodeLen;
	std::map<int, std::set<int>> routeRoad_all;
	std::map<std::pair <int, int>, std::vector<int>> routeRoad_allMap;
	std::map<std::pair <int, int>, double> routeLen_all;
	std::map<int, std::map<int, std::vector<double>>> partInRoads_all;
	std::map<int, std::map<std::string, double>> GeoDataCount;

	//subset
	std::map<std::string, std::set<int>> subset_reachRoad_all;	//全覆盖线条
	std::map<std::string, std::map<int, std::vector<double>>> subset_partInRoads_all;	//部分覆盖线条的坐标

	//输入排错
	bool err_beyond = false, err_noData = false;

	//计算输出，准备接数据

	//多线程参数
	int order = 1, length = INT_MAX;
	std::vector<int> subRoadVec;
	std::vector<int> subFromIDVec;

	//1：里程限制
	std::map<double, std::map<int, double>> MR_all;
	std::map<double, std::map<int, double>> meanMD_all;
	//可选
	std::map<double, std::map<int, double>> JncMR_all;
	std::map<double, std::map<std::string, std::map<int, double>>> wgtMR_all;

	//2：转向限制
	std::map<std::pair <double, double>, std::map<int, double>> DR_all; //pair<double,double>,第一个是DRLimit，第二个是MRLimit。
	std::map<double, std::map<int, double>> DD_all;
	std::map<double, std::map<int, double>> DDL_all;
	std::map<double, std::map<std::string, std::map<int, double>>> WDD_all;
	//可选
	std::map<std::pair <double, double>, std::map<int, double>> JD_all;
	std::map<std::pair <double, double>, std::map<int, double>> JncDR_all;
	std::map<std::pair <double, double>, std::map<std::string, std::map<int, double>>> wgtDR_all;

	//3：交叉口限制
	std::map<double, std::map<int, double>> JncR_all;
	std::map<double, std::map<int, double>> JncDD_all;
	std::map<double, std::map<int, double>> JncDDL_all;
	std::map<double, std::map<std::string, std::map<int, double >>> JncWDD_all;
	//可选
	std::map<double, std::map<std::string, std::map<int, double>>> wgtJncR_all;

	//Netreach数据
	std::map<std::string, Netreach> NetreachData;	//有不受限制的三种：MR、DR、JncR
	//Geodesics数据
	std::map<std::string, Geodesics> GeodesicsData;	//有不收限制的两种：MR、JncR，和收限制的一种：DR

	//增加
	double newMRLimit = 0, newDRLimit = 0, newJnc_maxNum = 0;
	std::string outShpFileName;
	std::string searchLimitStr;
	int r1 = 0, g1 = 0, b1 = 0;
	int r2 = 0, g2 = 0, b2 = 0;
	int thick1 = 0, thick2 = 0;
	bool isOutputVisualDataOver = false;
	std::string NGlimitStr;
	std::set<int> fromIDSet;

	//Step Depth
	bool isStepDepth = false;
	bool SD_MR = false, SD_DR = false, SD_JnR = false;
	double stepDepth_Angle = 0;
	double stepDepth_JunctionDegree = 0;
	std::map<int, std::set<double>> distanceAll;
	static std::string stepDepthName; 
	static std::string stepDepthStartRoad;

	//csv file
	std::string incsvfilename;

	//进度条
	volatile bool isFinished = false;
	volatile int finishedCount = 0;

	//stepdepth半径限制
	double needRadius = -1;
	std::map<int, std::set<int>> needRoads;

	//modify字段信息
	std::string modifyFieldName;

	//为了修改NRA属性栏留出的临时数据，修改完马上清空
	std::map<std::string, std::map<int, double>> TempModifyData;

	//path/reach输出
	std::string net_file_name, geo_file_name;
	std::string net_str, geo_str;
	std::map<int, std::map<int, std::vector<double>>> net_partInRoads_all;
	std::map<int, std::map<int, std::vector<double>>> geo_partInRoads_all;

	//线程同步
	int needProcessCount = 0;
	static int subset_finishedCount;
	static std::vector<bool> FinishedVec;
	static std::mutex subset_lock;
	static std::mutex count_lock;
	static std::mutex csvMutex; // csv互斥锁
	//subset
	Results results;
	std::map<int, double> subset_3_mr_all;
	std::map<int, double> subset_3_dd_all;
	std::map<int, double> subset_3_ddl_all;
	std::map<int, double> subset_3_jnc_all;

public:
	//初始化参数
	void clearOldData();
	void initPara(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr,
		bool ckMetricReach, bool ckDirectionalReach, bool ckJncR, bool ckJncDDL, bool ckJunctions, bool ckWgt,
		std::string txRadiiThreshold, std::string txRadiiThreshold2, std::string txAngleThreshold, std::string txDirectionalChanges,
		std::string txJunctionDegree, std::string txJunctionsLimit, std::string txAngleThreshold2, std::string txNewJnc, std::vector<std::string> &weightAttributesSet,
		bool ckNetreach, bool ckGeodesics, bool ckPath);
	void init_MR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold, bool ckJnc,
		bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_MD_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold, bool ckJnc,
		bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_DR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold2,
		std::string txAngleThreshold, std::string txDirectionalChanges, bool ckJnc, bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_DDL_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold2,
		std::string txAngleThreshold, std::string txDirectionalChanges, bool ckJnc, bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_MDR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold2,
		std::string txAngleThreshold, std::string txDirectionalChanges, bool ckJnc, bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_JnR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txJunctionDegree,
		std::string txJunctionsLimit, bool ckJnDD, std::string txAngleThreshold2, bool ckWgt, std::vector<std::string> &weightAttributesSet);
	void init_JnD_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold,
		std::string txJunctionDegree, bool ckJnDD, std::string txAngleThreshold2, bool ckWgt, std::vector<std::string> &weightAttributesSet);
	void init_Net_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, bool ckMR, std::string txRadiiThreshold,
		bool ckDR, std::string txRadiiThreshold2, std::string txAngleThreshold, std::string txDirectionalChanges, bool ckJnR,
		std::string txJunctionDegree, std::string txJunctionsLimit, std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_Net_MR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold, 
		std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_Net_DR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold2, std::string txAngleThreshold,
		std::string txDirectionalChanges, std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_Net_JnR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txJunctionDegree,
		std::string txJunctionsLimit, std::vector<std::string> &weightAttributesSet);
	void init_Geo_MR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txAngleThreshold, 
		std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_Geo_DR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txAngleThreshold, 
		std::string txNewJnc, std::vector<std::string> &weightAttributesSet);
	void init_Geo_JnR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txAngleThreshold, 
		std::string txNewJnc, std::vector<std::string> &weightAttributesSet);

	//subset
	void init_subset_para(ShapeFileAccessor &fileAccessor, subsetPara &para);

	bool getWeightData(std::string infilepath, std::string WgtLimitStr);
	bool getWeightData(std::string infilepath, std::vector<std::string> &weightAttributesSet);
	bool getWeightDataByName(std::string infilepath, std::string WgtLimitStr, std::map<int, double> &data);

	//多线程设置
	void setMultiParaByRand(std::set<int> start_roads, int whichone);
	void setMultiPara(ShapeFileAccessor &fileAccessor, int ord, int len, int whichone);		//whichone用1表示主计算，用2表示可视化计算
	void setMultiPara_modify(ShapeFileAccessor &fileAccessor, int lastend, int ord, int len, int whichone);

	//主计算函数
	void MultiCalculate(ShapeFileAccessor &fileAccessor,long long pos);
	void calculateMR(ShapeFileAccessor &fileAccessor);
	void calculateDR(ShapeFileAccessor &fileAccessor);
	void calculateMDR(ShapeFileAccessor &fileAccessor);
	void calculateJncR(ShapeFileAccessor &fileAccessor);

	//Distance
	void calculateMD(ShapeFileAccessor &fileAccessor);
	void calculateDDL(ShapeFileAccessor &fileAccessor);
	void calculateDDLbyDij(ShapeFileAccessor& fileAccessor); //xlj 2024-04-17

	void calculateJncD(ShapeFileAccessor &fileAccessor);

	//测试BFS
	void calculateMRbyDij(ShapeFileAccessor &fileAccessor);
	//xlj 2023-10-09
	void calculateDRbyDijkstraDeprecated(ShapeFileAccessor& fileAccessor);
	void calculateJncRbyBFS(ShapeFileAccessor &fileAccessor);

	//可视化多线程函数
	void MultiVisualize(ShapeFileAccessor &fileAccessor, long long pos);

	//Net-计算指定起点通行范围
	void Net_calculateMR(ShapeFileAccessor &fileAccessor, double subMRLimit);
	void Net_calculateDR(ShapeFileAccessor &fileAccessor, double subDRLimit, double subMRLimit);
	void Net_calculateJncR(ShapeFileAccessor &fileAccessor, double subJnc_maxNum);

	//Geo-计算指定起点(指定终点)的最短路径
	void Geo_calculateMR(ShapeFileAccessor &fileAccessor);
	void Geo_calculateDR(ShapeFileAccessor &fileAccessor);
	void Geo_calculateJncR(ShapeFileAccessor &fileAccessor);

	//输出数据
	void OutputData(ShapeFileAccessor &fileAccessor);	//输出所有的MR、DR等数据修改到原有Dbf文件中
	void modifyDBF2(std::map<int, int> &Result, std::string outPath, std::string fieldName);
	void modifyDBF(std::map<int, double> &Result, std::string outPath, std::string fieldName, std::string fullName="");
	void outputNetreach(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath);
	void outputGeodesics(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath);
	void OutputVisualData(ShapeFileAccessor &fileAccessor);
	void OutputVisualData(ShapeFileAccessor &fileAccessor, std::string outfilepath);
	void OutputVisualData(ShapeFileAccessor &fileAccessor, std::string outfilepath, std::string filename, std::string net_geo, std::string mr_dr_jnr);
	std::string getOutFilePath();
	std::string getOutFilePath2();
	void outputStepDepth(ShapeFileAccessor &fileAccessor, std::string stepDepthName);
	void outputPathCount(std::map<int, double> &data, std::string fieldname);

	//增加输出函数
	void outputNetAll(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath);
	void outputNetSub(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath);
	void outputGeoAll(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath);
	void outputGeoSub(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath);

	//可视化相关函数
	void getFromToID(ShapeFileAccessor &fileAccessor, std::string fromidStr);
	void getSearchID(ShapeFileAccessor &fileAccessor, std::string searchidStr, std::vector<int> &validRoad);
	bool findLimitData(std::string dbfPath, std::string fieldName, std::map<int, double> &limitData);
	void getLimitID(ShapeFileAccessor &fileAccessor, std::map<int, double> &roadData, std::string str, std::vector<int> &validRoad);
	std::string getSearchIDByAttribute(ShapeFileAccessor &fileAccessor, std::string searchidStr, std::vector<int> &validRoad);

	//增加
	void clearNGdata();
	void getNeedRoads(ShapeFileAccessor &fileAccessor, double limit);
	void init_dbf_path(std::string infileStr);

	int CalculateJncByPart(ShapeFileAccessor &fileAccessor, std::set<int> &inRoad, std::map<int, std::map<int, double>> &PartRoadsStartNodeLen);
	int CalculateJnc(ShapeFileAccessor &fileAccessor, std::set<int> &inRoad);
	std::map<std::string, double> CalculateWgt(std::map<std::string, std::map<int, double>> &weight, std::set<int> &inRoads);
	std::map<std::string, double> CalculateWgtPart(std::map<std::string, std::map<int, double>> &weight, std::set<int> &inRoads, int startRoad, int endRoad, double dis_flow);
	std::map<std::string, double> CalculateWgtNet(ShapeFileAccessor &fileAccessor, std::map<std::string, std::map<int, double>> &weight, std::set<int> &inRoads, std::map<int,double> &partReachRoadsLength);

	//同步dbf文件
	void RenameField(std::string dbfFilePath, std::string old_name, std::string new_name);
	void AddField(std::string dbfFilePath, std::string new_name);
	void DeleteField(std::string dbfFilePath, std::string old_name);
	void UpdateField(std::string dbfFilePath, std::string old_name, std::map<int, double> &data);

	//同步csv文件
	void CSVRenameField(std::string csvFilePath, std::string old_name, std::string new_name);
	void CSVAddField(std::string csvFilePath, std::string new_name);
	void CSVDeleteField(std::string csvFilePath, std::string old_name);
	void CSVUpdateField(std::string csvFilePath, std::string old_name, std::map<int, double>& data);

	void modifyCSV(const std::map<int, double>& result, const std::string& outPath, const std::string& fieldName);

	//子集线分析

	//subset_1
	void calculate_subset_1(ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs);				//通过多线程同时拉起下面三个子选项计算功能
	static void get_nearest_distance_to_subset(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs);
	static void get_subset_collective_reach(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs);
	static void get_members_within_subset(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs);
	bool is_all_isolation(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs, std::string type);
	//subset_2
	void calculate_subset_2(ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs);
	static void get_members_between_subsets(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs);
	static void get_nearest_distance_to_other_subsets(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs);
	//subset_3
	void calculate_subset_3(ShapeFileAccessor &fileAccessor, subsetPara &para);
	static void ouput_subset_3(Calculation *temp, ShapeFileAccessor &fileAccessor);
	static void calculateMR_subset(Calculation *temp, ShapeFileAccessor &fileAccessor);
	static void calculateDR_subset(Calculation *temp, ShapeFileAccessor &fileAccessor);
	static void calculateJncR_subset(Calculation *temp, ShapeFileAccessor &fileAccessor);

	//线程同步
	static void addFinishedCount();
	static void setFinished(bool is_finished);
	bool IsSubsetsAllFinished();
	double getFinishedRate();
	void resetFinishedState();	//在开启进度条之前进行

	//文件输出
	static void outputSubsetReach(ShapeFileAccessor &fileAccessor, std::set<int> &outRoad, std::string outShpPath);
	static void outputSubsetReachMR(ShapeFileAccessor &fileAccessor, std::set<int> &mr_reachAllInRoads_total, 
		std::map<int, std::vector<double>> &mr_partInRoads_total, std::string outShpPath);
	static void ouputSubsetPartReach(ShapeFileAccessor &fileAccessor, std::set<int> &outRoad,
		std::map<std::string, std::map<int, double>> &attributes, std::string outShpPath);
	static void ouputSubsetPartReachAll(ShapeFileAccessor &fileAccessor, std::map<int, std::set<int>> &SubsetIDs,
		std::map<int, std::map<std::string, std::map<int, double>>> &attributes, std::string outShpPath);

	static void CalculateMinDistace(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs, int pos_1, int pos_2,
		Graph &edgGraph, Graph_d &edgGraph_DR, Graph &edgGraph3, std::map<int, std::map<std::string, std::map<int, double>>> &attributes,
		std::map<std::pair<int, int>, std::map<std::string, std::set<int>>> &routeRoads);
	static void CalaculateAverDistace(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs, int pos_1, int pos_2,
		Graph &edgGraph, Graph_d &edgGraph_DR, Graph &edgGraph3, std::map<int, std::map<std::string, std::map<int, double>>> &attributes);

};



template<typename T>
class DistanceFunction {
public:
	virtual std::set<int> getConnectedRoads(int startRoad, int road) = 0; 
	virtual T operator()(int road1, int road2) = 0; 
	virtual ~DistanceFunction() {}
};

template<typename T>
class MetricDistanceFunction: public DistanceFunction<T> {
public:
	MetricDistanceFunction(const ShapeFileAccessor& fileAccessor)
		: fileAccessor(fileAccessor) {}

	std::set<int> getConnectedRoads(int startRoad, int road) {
		std::set<int> connectedRoads = fileAccessor.Road_to_roads.find(road)->second;
		if (connectedRoads.find(startRoad) != connectedRoads.end()) {
			connectedRoads.erase(startRoad);
		}
		return connectedRoads;
	}

	T operator()(int road1, int road2) {
		return fileAccessor.Length.find(road2)->second;
	}

private:
	const ShapeFileAccessor& fileAccessor; 
	
};

template<typename T>
class DirectionDistanceFunction : public DistanceFunction<T> {
public:
	DirectionDistanceFunction(const ShapeFileAccessor& fileAccessor)
		: fileAccessor(fileAccessor) {}

	std::set<int> getConnectedRoads(int startRoad,int road) {
		std::set<int> connectedRoads = fileAccessor.AdjRoadList.find(road)->second;
		return connectedRoads;
	}

	T operator()(int road1, int road2) {
		if (road1 == road2)return 0;
		return (fileAccessor.AdjTurnMP.find(road1)->second).find(road2)->second;
	}

private:
	const ShapeFileAccessor& fileAccessor;
};

template<typename T>
class PartDirectionDistanceFunction : public DirectionDistanceFunction<T> {
public:
	PartDirectionDistanceFunction(const ShapeFileAccessor& fileAccessor, const std::set<int>& validRoads)
		: DirectionDistanceFunction<T>(fileAccessor),fileAccessor(fileAccessor),validRoads(validRoads) {}

	std::set<int> getConnectedRoads(int startRoad ,int road) {
		std::set<int> connectedRoads=fileAccessor.AdjRoadList.find(road)->second;
		std::set<int> toRemove;
		for (int connectedRoad : connectedRoads) {
			if (validRoads.find(connectedRoad % fileAccessor.roadID.size()) == validRoads.end()) {
				toRemove.insert(connectedRoad);
			}
		}
		for (int road : toRemove) {
			connectedRoads.erase(road);
		}
		return connectedRoads;
	}

private:
	const ShapeFileAccessor& fileAccessor;
	const std::set<int>& validRoads; 
};

struct partInNode {
	int leftPreRoad = -1;
	double leftLength = 0;
	int rightPreRoad = -1;
	double rightLength = 0;
};
