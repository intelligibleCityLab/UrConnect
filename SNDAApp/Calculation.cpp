#include "stdafx.h"
#include "Calculation.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <algorithm>
//#include <boost/filesystem.hpp>
//namespace bf = boost::filesystem;
#include <QFile>
#include <Windows.h>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <queue>
#include <tuple>
#include <thread>
#include <limits>
#include "CSVProcess.h"

int Calculation::subset_finishedCount;
std::vector<bool> Calculation::FinishedVec;
std::mutex Calculation::subset_lock;
std::mutex Calculation::count_lock;
//extern std::ofstream logOut;
std::string Calculation::stepDepthName = "";
std::string Calculation::stepDepthStartRoad = "";

inline std::string IndexToKey(int x, int y)
{
	std::string str = std::to_string(x) + ", " + std::to_string(y);
	return str;
}

void getReachRoads(std::set<int> &reach_roads, double T, std::vector<double> &distances) {
	reach_roads.clear();
	for (int i = 0; i < distances.size(); i++) {
		if (distances[i] <= T)
			reach_roads.insert(i);
	}
}

inline std::set<std::pair<int, int>> GetAllPairs(int max_pos) {
	std::set<std::pair<int, int>> ans;

	for (int i = 1; i <= max_pos; i++) {
		for (int j = 1; j <= max_pos; j++) {
			if (i == j)
				continue;
			ans.insert({ i,j });
		}
	}

	return ans;
}

void getReachRoadsDR(std::set<int> &reach_roads, double T, int old_num, std::map<int, double> &distances) {
	reach_roads.clear();
	for (auto it = distances.begin(); it != distances.end(); it++) {
		if (it->second <= T) {
			int edge = it->first;
			if (edge >= old_num) 
				edge = edge - old_num;
			reach_roads.insert(edge);
		}
	}
}

inline std::string GetNowTime() {
	time_t setTime;
	time(&setTime);
	tm* ptm = localtime(&setTime);
	std::string time = std::to_string(ptm->tm_year + 1900)
		+ "/"
		+ std::to_string(ptm->tm_mon + 1)
		+ "/"
		+ std::to_string(ptm->tm_mday)
		+ " "
		+ std::to_string(ptm->tm_hour) + ":"
		+ std::to_string(ptm->tm_min) + ":"
		+ std::to_string(ptm->tm_sec);
	return time;
}


inline double getValue(std::string txTest)
{
	if (txTest.length() > 0)
	{
		std::string number("0123456789");
		std::string str = txTest;
		if (str == "n")
			return -1;
		else
			return stof(str.substr(str.find_first_of(number)));
	}
	else
		return -1;
}

inline void  GenerateUndirectedGraph(ShapeFileAccessor &fileAccessor, std::set<int> &inRoad, std::map<int, std::vector<int>> &roadNode, Graph &outGraph)
{
	for (auto it = inRoad.begin(); it != inRoad.end(); it++)
	{
		int edge = *it;
		int newNode1 = roadNode[edge][0];
		int newNode2 = roadNode[edge][1];

		EdgeProperty ep;
		ep.m_base = fileAccessor.Length[edge];
		ep.m_value = edge;

		boost::add_edge(newNode1, newNode2, ep, outGraph);
	}
}

inline void GenerateUndirectedGraph_Jnc_Part(ShapeFileAccessor &fileAccessor, std::set<int> &inRoads, int subCount, Graph &outGraph) {
	//遍历每条有向边,对于超出subCount的反向边要换算原道路ID
	int newEdgeIndex = 0;
	for (auto it = inRoads.begin(); it != inRoads.end(); it++)
	{
		int edge1 = *it;
		int endNode = fileAccessor.roadNode2[edge1][1];

		if (fileAccessor.Jnc[endNode] > 0)	//有交叉口，连接权重设1
		{
			for (auto iter = fileAccessor.AdjRoadList[edge1].begin(); iter != fileAccessor.AdjRoadList[edge1].end(); iter++)
			{
				int edge2 = *iter;

				int sub_edge1 = edge1 >= subCount ? edge1 - subCount : edge1;
				int sub_edge2 = edge2 >= subCount ? edge2 - subCount : edge2;

				if (sub_edge1 == sub_edge2)
					continue;

				if (inRoads.count(sub_edge2) == 0)
					continue;

				EdgeProperty ep;
				ep.m_base = 1;
				ep.m_value = newEdgeIndex;

				boost::add_edge(sub_edge1, sub_edge2, ep, outGraph);
				newEdgeIndex++;
			}
		}
		else	//没有交叉口，连接权重设0
		{
			for (auto iter = fileAccessor.AdjRoadList[edge1].begin(); iter != fileAccessor.AdjRoadList[edge1].end(); iter++)
			{
				int edge2 = *iter;

				int sub_edge1 = edge1 >= subCount ? edge1 - subCount : edge1;
				int sub_edge2 = edge2 >= subCount ? edge2 - subCount : edge2;

				if (sub_edge1 == sub_edge2)
					continue;

				if (inRoads.count(sub_edge2) == 0)
					continue;

				EdgeProperty ep;
				ep.m_base = 0;
				ep.m_value = newEdgeIndex;

				boost::add_edge(sub_edge1, sub_edge2, ep, outGraph);
				newEdgeIndex++;
			}
		}

		edge1 = *it + subCount;
		endNode = fileAccessor.roadNode2[edge1][1];

		if (fileAccessor.Jnc[endNode] > 0)	//有交叉口，连接权重设1
		{
			for (auto iter = fileAccessor.AdjRoadList[edge1].begin(); iter != fileAccessor.AdjRoadList[edge1].end(); iter++)
			{
				int edge2 = *iter;

				int sub_edge1 = edge1 >= subCount ? edge1 - subCount : edge1;
				int sub_edge2 = edge2 >= subCount ? edge2 - subCount : edge2;

				if (sub_edge1 == sub_edge2)
					continue;

				if (inRoads.count(sub_edge2) == 0)
					continue;

				EdgeProperty ep;
				ep.m_base = 1;
				ep.m_value = newEdgeIndex;

				boost::add_edge(sub_edge1, sub_edge2, ep, outGraph);
				newEdgeIndex++;
			}
		}
		else	//没有交叉口，连接权重设0
		{
			for (auto iter = fileAccessor.AdjRoadList[edge1].begin(); iter != fileAccessor.AdjRoadList[edge1].end(); iter++)
			{
				int edge2 = *iter;

				int sub_edge1 = edge1 >= subCount ? edge1 - subCount : edge1;
				int sub_edge2 = edge2 >= subCount ? edge2 - subCount : edge2;

				if (sub_edge1 == sub_edge2)
					continue;

				if (inRoads.count(sub_edge2) == 0)
					continue;

				EdgeProperty ep;
				ep.m_base = 0;
				ep.m_value = newEdgeIndex;

				boost::add_edge(sub_edge1, sub_edge2, ep, outGraph);
				newEdgeIndex++;
			}
		}
	}
}

//构造无向图--边边图
inline void  GenerateUndirectedGraph_Jnc(ShapeFileAccessor &fileAccessor, int subCount, Graph &outGraph)
{
	//遍历每条有向边,对于超出subCount的反向边要换算原道路ID
	int newEdgeIndex = 0;
	for (auto it = fileAccessor.AdjRoadList.begin(); it != fileAccessor.AdjRoadList.end(); it++)
	{
		int edge1 = it->first;
		int endNode = fileAccessor.roadNode2[edge1][1];
		if (fileAccessor.Jnc[endNode] > 0)	//有交叉口，连接权重设1
		{
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++)
			{
				int edge2 = *iter;

				EdgeProperty ep;
				ep.m_base = 1;
				ep.m_value = newEdgeIndex;

				edge1 = edge1 >= subCount ? edge1 - subCount : edge1;
				edge2 = edge2 >= subCount ? edge2 - subCount : edge2;

				if (edge1 == edge2)
					continue;

				boost::add_edge(edge1, edge2, ep, outGraph);
				newEdgeIndex++;
			}
		}
		else	//没有交叉口，连接权重设0
		{
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++)
			{
				int edge2 = *iter;

				EdgeProperty ep;
				ep.m_base = 0;
				ep.m_value = newEdgeIndex;

				edge1 = edge1 >= subCount ? edge1 - subCount : edge1;
				edge2 = edge2 >= subCount ? edge2 - subCount : edge2;

				if (edge1 == edge2)
					continue;

				boost::add_edge(edge1, edge2, ep, outGraph);
				newEdgeIndex++;
			}
		}
	}
}

//生成有向图
inline void  GenerateDirectedGraph(ShapeFileAccessor &fileAccessor, Graph_d &outGraph) {

	//遍历每条边，生成两个方向的新边
	int newEdgeIndex = 0;
	for (auto it = fileAccessor.AdjRoadList.begin(); it != fileAccessor.AdjRoadList.end(); it++)
	{
		int edge1 = it->first;
		for (auto iter = it->second.begin(); iter != it->second.end(); iter++)
		{
			int edge2 = *iter;

			////判断是不是当前边的反向边，若是，则不应计算角度，即不准原路返回
			//int oldnum = int(fileAccessor.AdjRoadList.size()) / 2;
			//int newstart = edge1, newend = edge2;
			//if (edge1 >= oldnum)
			//	newstart = edge1 - oldnum;
			//if (edge2 >= oldnum)
			//	newend = edge2 - oldnum;
			//if (newstart == newend)
			//	continue;

			int dis = fileAccessor.AdjTurn[IndexToKey(edge1, edge2)];

			EdgeProperty ep;
			ep.m_base = dis;
			ep.m_value = newEdgeIndex;

			boost::add_edge(edge1, edge2, ep, outGraph);
			newEdgeIndex++;
		}
	}
}

//根据<node1,node2>给出唯一id编号，根据唯一id编号找出<node1,node2>，通过toIndex再找出road_id
int getEdgeID(std::tuple<int, int, int> key, int base, std::map<std::tuple<int, int, int>, int> &nodeToEdge, std::map<int, std::tuple<int, int, int>> &edgeToNode) {
	auto iter = nodeToEdge.find(key);

	//key：<交点node，起点node，终点node>
	if (iter != nodeToEdge.end()) {
		return nodeToEdge[key];
	}
	else {
		int edgeNum = int(nodeToEdge.size());
		nodeToEdge[key] = edgeNum + base;
		edgeToNode[nodeToEdge[key]] = key;
		return nodeToEdge[key];
	}
}

//根据全覆盖和部分覆盖线条重构有向图
inline void  GenerateDirectedGraph3(ShapeFileAccessor &fileAccessor, std::set<int> &inRoads, std::set<int> &allInRoads, std::map<int, std::map<int, double>> &partInRoadsNodeLen,
	int old_numEdges, int old_nodeNums, std::map<int, std::map<int, int>> &partInNodeIDs, std::map<std::tuple<int, int, int>, int> &nodeToEdge, 
	std::map<int, std::tuple<int, int, int>> &edgeToNode, Graph_d &outGraph) {
	partInNodeIDs.clear();	//收集被收录部分覆盖的正向边：<road, node, edge_id>

	//遍历每条边，生成两个方向的新边
	int newEdgeIndex = 0;
	for (auto it = inRoads.begin(); it != inRoads.end(); it++){
		int edge1 = *it;
		//edge1作为正向边出去
		for (auto iter = fileAccessor.AdjRoadList[edge1].begin(); iter != fileAccessor.AdjRoadList[edge1].end(); iter++){
			int edge2 = *iter;

			//不在通行路径ID序列的道路，不允许进入构图
			int Edge = edge2;
			if (edge2 >= old_numEdges)
				Edge = edge2 - old_numEdges;

			if (edge1 == Edge)	//不允许掉头
				continue;

			int edge1_idx = edge1, edge2_idx = edge2;

			//只有属于全覆盖和部分覆盖的线条并且有部分覆盖的节点为连接点的才配搭建连接
			int edge1_node1 = fileAccessor.roadNode2[edge1][0];
			int edge1_node2 = fileAccessor.roadNode2[edge1][1];
			int edge2_node1 = fileAccessor.roadNode2[edge2][0];
			int edge2_node2 = fileAccessor.roadNode2[edge2][1];
			if (allInRoads.count(edge1)) {
				if (allInRoads.count(Edge)) {	//全覆盖->全覆盖
					//do nothing
				}
				else if (partInRoadsNodeLen.count(Edge)) {	//全覆盖->部分覆盖
					if (edge2_node1 == edge1_node1 || edge2_node1 == edge1_node2) {
						if (partInRoadsNodeLen[Edge].count(edge2_node1) == 0)	//部分覆盖的节点不存在
							continue;
						//对部分覆盖的边重新定义id编号
						int new_edge = getEdgeID({ edge2_node1, edge2_node1, edge2_node2 }, fileAccessor.AdjRoadList.size(), nodeToEdge, edgeToNode);
						partInNodeIDs[Edge][edge2_node1] = new_edge;
						edge2_idx = new_edge;
					}
					else if (edge2_node2 == edge1_node1 || edge2_node2 == edge1_node2) {
						if (partInRoadsNodeLen[Edge].count(edge2_node2) == 0)	//部分覆盖的节点不存在
							continue;
						//对部分覆盖的边重新定义id编号
						int new_edge = getEdgeID({ edge2_node2 ,edge2_node2 ,edge2_node1 }, fileAccessor.AdjRoadList.size(), nodeToEdge, edgeToNode);
						partInNodeIDs[Edge][edge2_node2] = new_edge;
						edge2_idx = new_edge;
					}
					else {
						continue;
					}
				}
				else {
					continue;
				}
			}
			else {
				continue;
			}

			int dis = fileAccessor.AdjTurn[IndexToKey(edge1, edge2)];

			EdgeProperty ep;
			ep.m_base = dis;
			ep.m_value = newEdgeIndex;

			boost::add_edge(edge1_idx, edge2_idx, ep, outGraph);
			newEdgeIndex++;
		}

		edge1 = *it + old_numEdges;
		//edge1作为反向边出去
		for (auto iter = fileAccessor.AdjRoadList[edge1].begin(); iter != fileAccessor.AdjRoadList[edge1].end(); iter++)
		{
			int edge2 = *iter;

			//不在通行路径ID序列的道路，不允许进入构图
			int Edge = edge2;
			if (edge2 >= old_numEdges)
				Edge = edge2 - old_numEdges;

			if (edge1- old_numEdges == Edge)	//不允许掉头
				continue;

			int edge1_idx = edge1, edge2_idx = edge2;
			
			//只有属于全覆盖和部分覆盖的线条并且有部分覆盖的节点为连接点的才配搭建连接
			int edge1_node1 = fileAccessor.roadNode2[edge1][0];
			int edge1_node2 = fileAccessor.roadNode2[edge1][1];
			int edge2_node1 = fileAccessor.roadNode2[edge2][0];
			int edge2_node2 = fileAccessor.roadNode2[edge2][1];
			if (allInRoads.count(edge1 - old_numEdges)) {
				if (allInRoads.count(Edge)) {	//全覆盖->全覆盖
					//do nothing
				}
				else if (partInRoadsNodeLen.count(Edge)) {	//全覆盖->部分覆盖
					if (edge2_node1 == edge1_node1 || edge2_node1 == edge1_node2) {
						if (partInRoadsNodeLen[Edge].count(edge2_node1) == 0)	//部分覆盖的节点不存在
							continue;
						//对部分覆盖的边重新定义id编号
						int new_edge = getEdgeID({ edge2_node1 ,edge2_node1 ,edge2_node2 }, fileAccessor.AdjRoadList.size(), nodeToEdge, edgeToNode);
						partInNodeIDs[Edge][edge2_node1] = new_edge;
						edge2_idx = new_edge;
					}
					else if (edge2_node2 == edge1_node1 || edge2_node2 == edge1_node2) {
						if (partInRoadsNodeLen[Edge].count(edge2_node2) == 0)	//部分覆盖的节点不存在
							continue;
						//对部分覆盖的边重新定义id编号
						int new_edge = getEdgeID({ edge2_node2 ,edge2_node2 ,edge2_node1 }, fileAccessor.AdjRoadList.size(), nodeToEdge, edgeToNode);
						partInNodeIDs[Edge][edge2_node2] = new_edge;
						edge2_idx = new_edge;
					}
					else {
						continue;
					}
				}
				else {
					continue;
				}
			}
			else {
				continue;
			}

			int dis = fileAccessor.AdjTurn[IndexToKey(edge1, edge2)];

			EdgeProperty ep;
			ep.m_base = dis;
			ep.m_value = newEdgeIndex;

			boost::add_edge(edge1_idx, edge2_idx, ep, outGraph);
			newEdgeIndex++;
		}
	}
}

//根据指定路径ID生成有向图
inline void  GenerateDirectedGraph2(ShapeFileAccessor &fileAccessor, std::set<int> &inRoads, int old_numEdges, Graph_d &outGraph) {

	//遍历每条边，生成两个方向的新边
	int newEdgeIndex = 0;
	for (auto it = inRoads.begin(); it != inRoads.end(); it++)
	{
		int edge1 = *it;
		for (auto iter = fileAccessor.AdjRoadList[edge1].begin(); iter != fileAccessor.AdjRoadList[edge1].end(); iter++)
		{
			int edge2 = *iter;

			//不在通行路径ID序列的道路，不允许进入构图
			int Edge = edge2;
			if (edge2 >= old_numEdges)
				Edge = edge2 - old_numEdges;
			if (inRoads.count(Edge) == 0 || edge1==Edge)
				continue;

			int dis = fileAccessor.AdjTurn[IndexToKey(edge1, edge2)];

			EdgeProperty ep;
			ep.m_base = dis;
			ep.m_value = newEdgeIndex;

			boost::add_edge(edge1, edge2, ep, outGraph);
			newEdgeIndex++;
		}

		edge1 = *it + old_numEdges;
		for (auto iter = fileAccessor.AdjRoadList[edge1].begin(); iter != fileAccessor.AdjRoadList[edge1].end(); iter++)
		{
			int edge2 = *iter;

			//不在通行路径ID序列的道路，不允许进入构图
			int Edge = edge2;
			if (edge2 >= old_numEdges)
				Edge = edge2 - old_numEdges;
			if (inRoads.count(Edge) == 0 || edge1- old_numEdges == Edge)
				continue;

			int dis = fileAccessor.AdjTurn[IndexToKey(edge1, edge2)];

			EdgeProperty ep;
			ep.m_base = dis;
			ep.m_value = newEdgeIndex;

			boost::add_edge(edge1, edge2, ep, outGraph);
			newEdgeIndex++;
		}
	}
}

inline std::set<double> getParaSet(std::string cstrTest)
{
	std::set<double> result;

	if (cstrTest.length() == 0)
		return result;

	std::string str;
	std::string number("0123456789");
	std::stringstream ss(cstrTest);

	while (std::getline(ss, str, ','))
	{
		if (str == "n")
			result.insert(-1);
		else
			result.insert(stof(str.substr(str.find_first_of(number))));
	}

	return result;
}

inline double getPara(std::string cstrTest)
{
	std::string number("0123456789");
	std::string str = cstrTest;
	if (str.length() == 0)
		return INT_MAX;
	return stof(str.substr(str.find_first_of(number)));
}

template <typename DistanceFunction>
inline void getDistDijkstra(DistanceFunction distFunc, int startRoad, double max_Limit, std::vector<double>& dist) {
	int total = dist.size();
	using PDI = std::pair<double, int>; // pair of (distance, road)
	std::priority_queue<PDI, std::vector<PDI>, std::greater<PDI>> q;
	std::vector<bool> visited(total, false);

	
	dist[startRoad] = distFunc(startRoad, startRoad) * 0.5; //equal to `dist[startRoad] = fileAccessor.Length[startRoad] * 0.5;`
	q.push(PDI(dist[startRoad], startRoad));

	while (!q.empty()) {
		PDI pdi = q.top();
		double currentDist = pdi.first;
		int nowRoad = pdi.second;
		q.pop();

		if (visited[nowRoad])
			continue;
		visited[nowRoad] = true;

		for (int connectedRoad : distFunc.getConnectedRoads(startRoad,nowRoad)) {
			double length = distFunc(nowRoad, connectedRoad);
			if (currentDist + length < dist[connectedRoad]) {
				dist[connectedRoad] = currentDist + length;
				if (max_Limit == -1 || dist[connectedRoad] <= max_Limit) {
					q.push(PDI{ dist[connectedRoad], connectedRoad });
				}
			}
		}
	}
}


inline void getMRDistBFS(ShapeFileAccessor &fileAccessor, int startRoad ,double max_Limit, std::vector<double> &dist) {
	int total = fileAccessor.roadID.size();
	//准备数据结构
	std::queue<int> q;
	std::vector<bool> visited(total, false);
	std::set<int> outRoad;

	//初始化数据
	q.push(startRoad);
	double rootseglength = fileAccessor.Length[startRoad];
	dist[startRoad] = rootseglength * 0.5;
	outRoad.insert(fileAccessor.roadID.begin(), fileAccessor.roadID.end());

	//BFS计算到连接线段的最短距离
	while (!q.empty()) {
		int nowRoad = q.front();
		q.pop();

		if (visited[nowRoad])
			continue;
		visited[nowRoad] = true;

		for (auto next_iter = fileAccessor.Road_to_roads[nowRoad].begin(); next_iter != fileAccessor.Road_to_roads[nowRoad].end(); next_iter++) {
			double length = fileAccessor.Length[*next_iter];
			int connected_cursor = *next_iter;
			if (connected_cursor == startRoad)
				continue;

			//如果更新了已经reach的线条，需要重新遍历
			if (dist[nowRoad] + length < dist[connected_cursor]) {
				visited[connected_cursor] = false;
				dist[connected_cursor] = dist[nowRoad] + length;
			}
			if (max_Limit==-1||dist[connected_cursor] <= max_Limit) {
				q.push(connected_cursor);
			}
		}
	}
}

inline void CalculateRoadsDRbyBFS(ShapeFileAccessor &fileAccessor, int startRoad, std::set<double> &DRLimitSet, std::map<int, double> &new_dict_real_min_dist, 
	std::map<double, std::set<int>> &validRoad) {

	//清空旧数据
	new_dict_real_min_dist.clear();
	validRoad.clear();

	int total = fileAccessor.roadID.size();
	double max_DRLimit = INT_MAX;

	for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++)
	{
		max_DRLimit = *iter;
	}

	//准备数据结构
	std::queue<int> q;
	std::vector<double> dist(2 * total, 0x7fffffff);
	std::vector<bool> visited(2 * total, false);

	//初始化数据
	q.push(startRoad);
	dist[startRoad] = 0;

	//BFS计算到连接线段的最短距离
	while (!q.empty()) {
		int nowRoad = q.front();
		q.pop();

		if (visited[nowRoad])
			continue;
		visited[nowRoad] = true;

		for (auto next_iter = fileAccessor.AdjRoadList[nowRoad].begin(); next_iter != fileAccessor.AdjRoadList[nowRoad].end(); next_iter++) {
			int connected_cursor = *next_iter;
			if (nowRoad % total == connected_cursor % total)
				continue;

			//如果更新了已经reach的线条，需要重新遍历
			int turn = fileAccessor.AdjTurnMP[nowRoad][connected_cursor];
			if (dist[nowRoad] + turn < dist[connected_cursor]) {
				visited[connected_cursor] = false;
				dist[connected_cursor] = dist[nowRoad] + turn;
			}
			if (dist[connected_cursor] <= max_DRLimit)
				q.push(connected_cursor);
		}
	}

	//将起点设置为反向边计算最短路径
	int startRoad2 = startRoad + total;

	//准备数据结构
	std::queue<int> q2;
	std::vector<double> dist2(2 * total, 0x7fffffff);
	std::vector<bool> visited2(2 * total, false);

	//初始化数据
	q2.push(startRoad2);
	dist2[startRoad2] = 0;

	//BFS计算到连接线段的最短距离
	while (!q2.empty()) {
		int nowRoad = q2.front();
		q2.pop();

		if (visited2[nowRoad])
			continue;
		visited2[nowRoad] = true;

		for (auto next_iter = fileAccessor.AdjRoadList[nowRoad].begin(); next_iter != fileAccessor.AdjRoadList[nowRoad].end(); next_iter++) {
			int connected_cursor = *next_iter;
			if (nowRoad % total == connected_cursor % total)
				continue;

			//如果更新了已经reach的线条，需要重新遍历
			int turn = fileAccessor.AdjTurnMP[nowRoad][connected_cursor];
			if (dist2[nowRoad] + turn < dist2[connected_cursor]) {
				visited2[connected_cursor] = false;
				dist2[connected_cursor] = dist2[nowRoad] + turn;
			}
			if (dist2[connected_cursor] <= max_DRLimit)
				q2.push(connected_cursor);
		}
	}

	//计算真正的最短路径
	validRoad.clear();
	for (int k = 0; k < 2 * total; k++) {
		dist[k] = min(dist[k], dist2[k]);
	}
	std::vector<double> dict_real_min_dist(total, INT_MAX);
	for (int k = 0; k < total; k++)
	{
		//找出有向边i对应的反向边id
		int reverse_k = k + total;
		double min_d = min(dist[k], dist[reverse_k]);
		dict_real_min_dist[k] = min_d;
		new_dict_real_min_dist[k] = min_d;

		for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++) {
			double DRLimit = *iter;

			if (DRLimit == -1)
				validRoad[DRLimit].insert(k);
			if (dict_real_min_dist[k] <= DRLimit)
				validRoad[DRLimit].insert(k);
		}
	}
}

void CalculateRoadsMRbyBFS(ShapeFileAccessor &fileAccessor, std::vector<double> &distances, double MRLimit, int startRoad, std::set<int> &outRoad,
	std::map<int, std::vector<double>> &mr_partIn, std::set<int> &mr_reachAllInRoads, std::map<int, std::map<int, std::vector<double>>> &mr_partInRoads_all,
	std::map<int, std::map<int, std::map<int, double>>> &mr_partInReachRoadNodeLen, std::map<int, std::map<int, std::map<int, int>>> &mr_partInReachRoadNodeCoor, 
	std::map<int, std::map<int, std::vector<double>>> &partInRoads_all) {

	//清空旧数据
	outRoad.clear();
	mr_partIn.clear();
	mr_reachAllInRoads.clear();
	mr_partInRoads_all.clear();
	mr_partInReachRoadNodeLen.clear();
	mr_partInReachRoadNodeCoor.clear();
	partInRoads_all.clear();

	int total = fileAccessor.roadID.size();

	//准备数据结构
	struct partInNode {
		int leftNode = -1;
		int leftPreRoad = -1;
		double leftLength = 0;
		int rightNode = -1;
		int rightPreRoad = -1;
		double rightLength = 0;
	};
	std::map<int, partInNode> partInLength;

	if (MRLimit == -1)
	{
		std::vector<double> dist(total, 0x7fffffff);
		getMRDistBFS(fileAccessor, startRoad, -1, dist);

		//输出数据
		outRoad.insert(fileAccessor.roadID.begin(), fileAccessor.roadID.end());
		distances = dist;
		mr_reachAllInRoads = outRoad;
	}
	else
	{
		//准备数据结构
		std::queue<int> q;
		std::vector<double> dist(total, 0x7fffffff);
		std::vector<bool> visited(total, false);
		partInLength.clear();
		outRoad.clear();

		//初始化数据
		q.push(startRoad);
		double rootseglength = fileAccessor.Length[startRoad];
		dist[startRoad] = rootseglength * 0.5;

		//BFS计算到连接线段的最短距离
		while (!q.empty()) {
			int nowRoad = q.front();
			q.pop();

			if (visited[nowRoad])
				continue;
			visited[nowRoad] = true;

			for (auto next_iter = fileAccessor.Road_to_roads[nowRoad].begin(); next_iter != fileAccessor.Road_to_roads[nowRoad].end(); next_iter++) {
				double length = fileAccessor.Length[*next_iter];
				int connected_cursor = *next_iter;
				if (connected_cursor == startRoad)
					continue;

				//如果更新了已经reach的线条，需要重新遍历
				if (dist[nowRoad] + length < dist[connected_cursor]) {
					visited[connected_cursor] = false;
					dist[connected_cursor] = dist[nowRoad] + length;
				}
				if (dist[connected_cursor] <= MRLimit)
					q.push(connected_cursor);
			}
		}

		//BFS收集全覆盖
		std::queue<int> qq;
		std::vector<bool> visited2(total, false);
		qq.push(startRoad);

		//判断能否冲出起点线条
		if (0.5*rootseglength >= MRLimit) {		//不能冲出起点
			outRoad.insert(startRoad);
			mr_partIn[startRoad].push_back(2 * MRLimit);

			//计算坐标
			double x1 = fileAccessor.Route[startRoad][0];
			double y1 = fileAccessor.Route[startRoad][1];
			double x2 = fileAccessor.Route[startRoad][2];
			double y2 = fileAccessor.Route[startRoad][3];
			double x_mid = (x1 + x2) / 2;
			double y_mid = (y1 + y2) / 2;

			int node1 = fileAccessor.roadNode[startRoad][0];		//endrode的端点1
			int node2 = fileAccessor.roadNode[startRoad][1];		//endrode的端点2

			double rate = 2 * MRLimit / fileAccessor.Length[startRoad];

			double xn1 = fileAccessor.Route[startRoad][0];
			double yn1 = fileAccessor.Route[startRoad][1];
			double xn2 = fileAccessor.Route[startRoad][2];
			double yn2 = fileAccessor.Route[startRoad][3];
			double x_1 = x_mid + (xn1 - x_mid)*rate;
			double y_1 = y_mid + (yn1 - y_mid)*rate;
			double x_2 = x_mid + (xn2 - x_mid)*rate;
			double y_2 = y_mid + (yn2 - y_mid)*rate;

			partInRoads_all[startRoad][startRoad].push_back(x_1);
			partInRoads_all[startRoad][startRoad].push_back(y_1);
			partInRoads_all[startRoad][startRoad].push_back(x_2);
			partInRoads_all[startRoad][startRoad].push_back(y_2);
		}
		else {		//可以冲出起点线
			while (!qq.empty()) {
				int nowRoad = qq.front();
				qq.pop();

				if (visited2[nowRoad])
					continue;

				double len = fileAccessor.Length[nowRoad];
				outRoad.insert(nowRoad);
				mr_reachAllInRoads.insert(nowRoad);
				visited2[nowRoad] = true;

				for (auto next_iter = fileAccessor.Road_to_roads[nowRoad].begin(); next_iter != fileAccessor.Road_to_roads[nowRoad].end(); next_iter++) {
					double length = fileAccessor.Length[*next_iter];
					int connected_cursor = *next_iter;
					if (connected_cursor == startRoad)
						continue;

					if (dist[connected_cursor] <= MRLimit) {	//全覆盖
						qq.push(connected_cursor);
						mr_reachAllInRoads.insert(connected_cursor);
					}
					else if (dist[nowRoad]< MRLimit && dist[connected_cursor] > MRLimit) {	//部分覆盖
						//找出覆盖节点方向
						int pre_node1 = fileAccessor.roadNode[nowRoad][0];
						int pre_node2 = fileAccessor.roadNode[nowRoad][1];
						int node1 = fileAccessor.roadNode[connected_cursor][0];
						int node2 = fileAccessor.roadNode[connected_cursor][1];
						int same_node = node1;
						if (pre_node1 == node2 || pre_node2 == node2)
							same_node = node2;
						//更新该方向覆盖距离
						if (same_node == node1) {
							if (MRLimit - dist[nowRoad] > partInLength[connected_cursor].leftLength) {
								partInLength[connected_cursor].leftNode = same_node;
								partInLength[connected_cursor].leftPreRoad = nowRoad;
								partInLength[connected_cursor].leftLength = MRLimit - dist[nowRoad];
							}
						}
						else {
							if (MRLimit - dist[nowRoad] > partInLength[connected_cursor].rightLength) {
								partInLength[connected_cursor].rightNode = same_node;
								partInLength[connected_cursor].rightPreRoad = nowRoad;
								partInLength[connected_cursor].rightLength = MRLimit - dist[nowRoad];
							}
						}
					}
				}
			}
		}

		//数据输出
		distances = dist;
	}

	//处理部分覆盖
	for (auto next_iter = partInLength.begin(); next_iter != partInLength.end(); next_iter++) {
		int endRoad = next_iter->first;
		outRoad.insert(endRoad);

		if (next_iter->second.leftLength + next_iter->second.rightLength < fileAccessor.Length[endRoad]) {		//不足以全覆盖
			if (next_iter->second.leftLength > 0) {
				int node = next_iter->second.leftNode;
				double dis_overflow = next_iter->second.leftLength;
				mr_partIn[endRoad].push_back(dis_overflow);
				mr_partInReachRoadNodeLen[startRoad][endRoad][node] = dis_overflow;
				mr_partInReachRoadNodeCoor[startRoad][endRoad][node] = partInRoads_all[startRoad][endRoad].size();

				//计算坐标
				double x2 = fileAccessor.Route[endRoad][0];
				double y2 = fileAccessor.Route[endRoad][1];
				double x1 = fileAccessor.Route[endRoad][2];
				double y1 = fileAccessor.Route[endRoad][3];
				double rate = dis_overflow / fileAccessor.Length[endRoad];

				double x = x1 + (x2 - x1)*rate;
				double y = y1 + (y2 - y1)*rate;

				partInRoads_all[startRoad][endRoad].push_back(x1);
				partInRoads_all[startRoad][endRoad].push_back(y1);
				partInRoads_all[startRoad][endRoad].push_back(x);
				partInRoads_all[startRoad][endRoad].push_back(y);
			}
			if (next_iter->second.rightLength > 0) {
				int node = next_iter->second.rightNode;
				double dis_overflow = next_iter->second.rightLength;
				mr_partIn[endRoad].push_back(dis_overflow);
				mr_partInReachRoadNodeLen[startRoad][endRoad][node] = dis_overflow;
				mr_partInReachRoadNodeCoor[startRoad][endRoad][node] = partInRoads_all[startRoad][endRoad].size();

				//计算坐标
				double x2 = fileAccessor.Route[endRoad][0];
				double y2 = fileAccessor.Route[endRoad][1];
				double x1 = fileAccessor.Route[endRoad][2];
				double y1 = fileAccessor.Route[endRoad][3];
				double rate = dis_overflow / fileAccessor.Length[endRoad];

				double x = x1 + (x2 - x1)*rate;
				double y = y1 + (y2 - y1)*rate;

				partInRoads_all[startRoad][endRoad].push_back(x1);
				partInRoads_all[startRoad][endRoad].push_back(y1);
				partInRoads_all[startRoad][endRoad].push_back(x);
				partInRoads_all[startRoad][endRoad].push_back(y);
			}
		}
		else {		//双端部分覆盖重叠，形成全覆盖
			mr_reachAllInRoads.insert(endRoad);
		}
	}
}

int Calculation::CalculateJncByPart(ShapeFileAccessor &fileAccessor, std::set<int> &inRoad, std::map<int, std::map<int, double>> &PartRoadsStartNodeLen) {
	//经过转折的路口
	int result = 0;
	std::set<int> midNode;

	//遍历所有路口
	std::set<int> tmpNode;
	std::vector<int> allNode;
	for (auto it = inRoad.begin(); it != inRoad.end(); it++)
	{
		if (PartRoadsStartNodeLen.count(*it)) {
			for (auto iter = PartRoadsStartNodeLen[*it].begin(); iter != PartRoadsStartNodeLen[*it].end(); iter++) {
				int node = iter->first;
				tmpNode.insert(node);
				allNode.push_back(node);
			}			
		}
		else {
			tmpNode.insert(fileAccessor.roadNode[*it][0]);
			tmpNode.insert(fileAccessor.roadNode[*it][1]);
			allNode.push_back(fileAccessor.roadNode[*it][0]);
			allNode.push_back(fileAccessor.roadNode[*it][1]);
		}
	}

	//找出转折路口
	for (auto it = tmpNode.begin(); it != tmpNode.end(); it++)
	{
		if (count(allNode.begin(), allNode.end(), *it) > 1)		//至少有两条路过了同一个路口
		{
			result += fileAccessor.Jnc[*it];
			midNode.insert(*it);
		}
	}

	return result;
}


int Calculation::CalculateJnc(ShapeFileAccessor &fileAccessor, std::set<int> &inRoad)
{
	//经过转折的路口
	int result = 0;
	std::set<int> midNode;

	//遍历所有路口
	std::set<int> tmpNode;
	std::vector<int> allNode;
	for (auto it = inRoad.begin(); it != inRoad.end(); it++)
	{
		tmpNode.insert(fileAccessor.roadNode[*it][0]);
		tmpNode.insert(fileAccessor.roadNode[*it][1]);
		allNode.push_back(fileAccessor.roadNode[*it][0]);
		allNode.push_back(fileAccessor.roadNode[*it][1]);
	}

	//找出转折路口
	for (auto it = tmpNode.begin(); it != tmpNode.end(); it++)
	{
		if (count(allNode.begin(), allNode.end(), *it) > 1)		//至少有两条路过了同一个路口
		{
			result += fileAccessor.Jnc[*it];
			midNode.insert(*it);
		}
	}

	return result;
}

std::map<std::string, double> Calculation::CalculateWgt(std::map<std::string, std::map<int, double>> &weight, std::set<int> &inRoads)
{
	std::map<std::string, double> wgt;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++)
	{
		int road = *it2;
		//将所有inRoads集合里的路的权重全部加起来，存放在wgt里。
		for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
			wgt[wgt_it->first] += wgt_it->second[road];
		}
	}

	return wgt;
}

std::map<std::string, double> Calculation::CalculateWgtPart(std::map<std::string, std::map<int, double>> &weight, std::set<int> &inRoads, int startRoad, int endRoad, double rate) {
	std::map<std::string, double> wgt;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++)
	{
		int road = *it2;
		for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
			if (road != endRoad) {
				if (road == startRoad)
					wgt[wgt_it->first] += (0.5*wgt_it->second[road]);
				else
					wgt[wgt_it->first] += wgt_it->second[road];
			}
			else
				wgt[wgt_it->first] += (rate * wgt_it->second[road]);
		}
	}

	return wgt;
}

std::map<std::string, double> Calculation::CalculateWgtNet(ShapeFileAccessor &fileAccessor, std::map<std::string, std::map<int, double>> &weight, std::set<int> &inRoads, std::map<int, double> &partReachRoadsLength) {
	std::map<std::string, double> wgt;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++)
	{
		int road = *it2;
		for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
			if (partReachRoadsLength.count(road)) {
				double rate = partReachRoadsLength[*it2] / fileAccessor.Length[*it2];
				wgt[wgt_it->first] += (rate * wgt_it->second[road]);
			}
			else
				wgt[wgt_it->first] += wgt_it->second[road];
		}
	}

	return wgt;
}

std::map<std::string, double> CalculateWgtInline(std::map<std::string, std::map<int, double>> &weight, std::set<int> &inRoads)
{
	std::map<std::string, double> wgt;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++)
	{
		int road = *it2;
		for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
			wgt[wgt_it->first] += wgt_it->second[road];
		}
	}

	return wgt;
}

int calculateJnc(ShapeFileAccessor &fileAccessor, std::set<int> &inRoad)
{
	//经过转折的路口
	int result = 0;
	std::set<int> midNode;

	//遍历所有路口
	std::set<int> tmpNode;
	std::vector<int> allNode;
	for (auto it = inRoad.begin(); it != inRoad.end(); it++)
	{
		tmpNode.insert(fileAccessor.roadNode[*it][0]);
		tmpNode.insert(fileAccessor.roadNode[*it][1]);
		allNode.push_back(fileAccessor.roadNode[*it][0]);
		allNode.push_back(fileAccessor.roadNode[*it][1]);
	}

	//找出转折路口
	for (auto it = tmpNode.begin(); it != tmpNode.end(); it++)
	{
		if (count(allNode.begin(), allNode.end(), *it) > 1)		//至少有两条路过了同一个路口
		{
			result += fileAccessor.Jnc[*it];
			midNode.insert(*it);
		}
	}

	return result;
}

inline void getInRoads_MDR_partIn(std::set<int>  &lineRoads, std::map<int, std::vector<int>> &newRoadNode, std::map<std::string, int> &newNodeToRoad, int startroad, int endroad, int startNode, std::vector<vertex_descriptor> &parents, std::vector<double> &distances)
{
	//lineRoads.clear();
	std::set<int>().swap(lineRoads);
	//遍历得到所有经历的节点
	int endNode = 0;
	int endNode1 = newRoadNode[endroad][0];
	int endNode2 = newRoadNode[endroad][1];
	if (distances[endNode1] < distances[endNode2])
		endNode = endNode2;
	else
		endNode = endNode1;

	std::vector<int> crossNodes;
	crossNodes.push_back(endNode);
	int lastNode = int(parents[endNode]), nowNode;
	while (lastNode != startNode)
	{
		nowNode = lastNode;
		crossNodes.push_back(nowNode);
		lastNode = int(parents[nowNode]);
	}

	//获取所经历的道路
	lineRoads.insert(startroad);
	lineRoads.insert(endroad);
	for (int i = 0; i<int(crossNodes.size()) - 1; i++)
	{
		int node1 = crossNodes[i];
		int node2 = crossNodes[i + 1];
		int road = newNodeToRoad[IndexToKey(node1, node2)];
		lineRoads.insert(road);
	}

}

inline void getInRoads_MDR(std::set<int>  &lineRoads, std::map<int, std::vector<int>> &newRoadNode, std::map<std::string, int> &newNodeToRoad, int startroad, int endroad, int startNode, std::vector<vertex_descriptor> &parents, std::vector<double> &distances)
{
	//lineRoads.clear();
	std::set<int>().swap(lineRoads);
	//遍历得到所有经历的节点
	int endNode = 0;
	int endNode1 = newRoadNode[endroad][0];
	int endNode2 = newRoadNode[endroad][1];
	if (distances[endNode1] < distances[endNode2])
		endNode = endNode1;
	else
		endNode = endNode2;

	std::vector<int> crossNodes;
	crossNodes.push_back(endNode);
	int lastNode = int(parents[endNode]), nowNode;
	while (lastNode != startNode)
	{
		nowNode = lastNode;
		crossNodes.push_back(nowNode);
		lastNode = int(parents[nowNode]);
	}

	//获取所经历的道路
	lineRoads.insert(startroad);
	lineRoads.insert(endroad);
	for (int i = 0; i<int(crossNodes.size()) - 1; i++)
	{
		int node1 = crossNodes[i];
		int node2 = crossNodes[i + 1];
		int road = newNodeToRoad[IndexToKey(node1, node2)];
		lineRoads.insert(road);
	}

}

inline void getInRoads_MR2(std::vector<int> &lineRoads, std::set<int> &lineRoadsSet, ShapeFileAccessor &fileAccessor, int startroad, int endroad, int startNode, std::vector<vertex_descriptor> &parents, std::vector<double> &distances)
{
	//lineRoads.clear();
	std::vector<int>().swap(lineRoads);
	//lineRoadsSet.clear();
	std::set<int>().swap(lineRoadsSet);
	//把经历的道路建一个队列
	lineRoads.push_back(endroad);
	lineRoadsSet.insert(endroad);

	//遍历得到所有经历的节点
	int endNode = 0;
	int endNode1 = fileAccessor.roadNode[endroad][0];
	int endNode2 = fileAccessor.roadNode[endroad][1];
	if (distances[endNode1] < distances[endNode2])
		endNode = endNode1;
	else
		endNode = endNode2;

	std::vector<int> crossNodes;
	crossNodes.push_back(endNode);
	int lastNode = int(parents[endNode]), nowNode;
	while (lastNode != startNode)
	{
		nowNode = lastNode;
		crossNodes.push_back(nowNode);
		lastNode = int(parents[nowNode]);
	}

	//获取所经历的道路
	for (int i = 0; i<int(crossNodes.size()) - 1; i++)
	{
		int node1 = crossNodes[i];
		int node2 = crossNodes[i + 1];
		int road = fileAccessor.nodeToEdge[IndexToKey(node1, node2)];
		lineRoads.push_back(road);
		lineRoadsSet.insert(road);
	}

	lineRoads.push_back(startroad);
	lineRoadsSet.insert(startroad);

	//反向
	reverse(lineRoads.begin(), lineRoads.end());

	if (lineRoads[0] == lineRoads[1])
		lineRoads.pop_back();
}

inline void getInRoads_MR_partIn(std::set<int> &lineRoads, ShapeFileAccessor &fileAccessor, int startroad, int endroad, int startNode, std::vector<vertex_descriptor> &parents, std::vector<double> &distances, double limitLen)
{
	//把经历的道路建一个队列
	//lineRoads.clear();
	std::set<int>().swap(lineRoads);
	//遍历得到所有经历的节点
	int endNode = 0;
	int endNode1 = fileAccessor.roadNode[endroad][0];
	int endNode2 = fileAccessor.roadNode[endroad][1];
	if (distances[endNode1] < distances[endNode2])
		endNode = endNode2;		//取大的一支
	else
		endNode = endNode1;

	if (distances[endNode] >= limitLen)
		return;

	std::vector<int> crossNodes;
	crossNodes.push_back(endNode);
	int lastNode = int(parents[endNode]), nowNode;
	while (lastNode != startNode)
	{
		nowNode = lastNode;
		crossNodes.push_back(nowNode);
		lastNode = int(parents[nowNode]);
	}

	//获取所经历的道路
	lineRoads.insert(startroad);
	lineRoads.insert(endroad);
	for (int i = 0; i<int(crossNodes.size()) - 1; i++)
	{
		int node1 = crossNodes[i];
		int node2 = crossNodes[i + 1];
		int road = fileAccessor.nodeToEdge[IndexToKey(node1, node2)];
		lineRoads.insert(road);
	}
}

inline void getInRoads_MR(std::set<int> &lineRoads, ShapeFileAccessor &fileAccessor, int startroad, int endroad, int startNode, std::vector<vertex_descriptor> &parents, std::vector<double> &distances)
{
	//把经历的道路建一个队列
	//lineRoads.clear();
	std::set<int>().swap(lineRoads);
	//遍历得到所有经历的节点
	int endNode = 0;
	int endNode1 = fileAccessor.roadNode[endroad][0];
	int endNode2 = fileAccessor.roadNode[endroad][1];
	if (distances[endNode1] < distances[endNode2])
		endNode = endNode1;
	else
		endNode = endNode2;

	std::vector<int> crossNodes;
	crossNodes.push_back(endNode);
	int lastNode = int(parents[endNode]), nowNode;
	while (lastNode != startNode)
	{
		nowNode = lastNode;
		crossNodes.push_back(nowNode);
		lastNode = int(parents[nowNode]);
	}

	//获取所经历的道路
	lineRoads.insert(startroad);
	lineRoads.insert(endroad);
	for (int i = 0; i<int(crossNodes.size()) - 1; i++)
	{
		int node1 = crossNodes[i];
		int node2 = crossNodes[i + 1];
		int road = fileAccessor.nodeToEdge[IndexToKey(node1, node2)];
		lineRoads.insert(road);
	}
}

inline void getInRoads_DR2(std::vector<int> &lineRoads, std::set<int> &lineRoadsSet, ShapeFileAccessor &fileAccessor, int startroad, int endroad, std::vector<vertex_descriptor> &parents1,
	std::vector<double> &distances1, std::vector<vertex_descriptor> &parents2, std::vector<double> &distances2, int old_numEdges)
{
	//把经历的道路建一个队列
	//lineRoads.clear();
	std::vector<int>().swap(lineRoads);
	//lineRoadsSet.clear();
	std::set<int>().swap(lineRoadsSet);
	lineRoads.push_back(endroad);
	lineRoadsSet.insert(endroad);

	int reverseRoad = 0;
	if (startroad < old_numEdges)
		reverseRoad = startroad + old_numEdges;
	else
		reverseRoad = startroad - old_numEdges;

	int reverseRoad_end = 0;
	if (endroad < old_numEdges)
		reverseRoad_end = endroad + old_numEdges;
	else
		reverseRoad_end = endroad - old_numEdges;

	double dis1 = min(distances1[endroad], distances1[reverseRoad_end]);
	double dis2 = min(distances2[endroad], distances2[reverseRoad_end]);

	//遍历得到所有经历的道路
	if (dis1 < dis2)
	{
		int lastRoad, nowRoad;
		if (distances1[endroad] < distances1[reverseRoad_end])
			lastRoad = int(parents1[endroad]);
		else
			lastRoad = int(parents1[reverseRoad_end]);
		while (lastRoad != startroad && lastRoad != reverseRoad)
		{
			nowRoad = lastRoad;
			int realRoad = nowRoad;
			if (nowRoad >= old_numEdges)
				realRoad = nowRoad - old_numEdges;
			lastRoad = int(parents1[nowRoad]);
			lineRoads.push_back(realRoad);
			lineRoadsSet.insert(realRoad);
		}
	}
	else
	{
		int lastRoad, nowRoad;
		if (distances2[endroad] < distances2[reverseRoad_end])
			lastRoad = int(parents2[endroad]);
		else
			lastRoad = int(parents2[reverseRoad_end]);
		while (lastRoad != startroad && lastRoad != reverseRoad)
		{
			nowRoad = lastRoad;
			int realRoad = nowRoad;
			if (nowRoad >= old_numEdges)
				realRoad = nowRoad - old_numEdges;
			lastRoad = int(parents2[nowRoad]);
			lineRoads.push_back(realRoad);
			lineRoadsSet.insert(realRoad);
		}
	}

	lineRoads.push_back(startroad);
	lineRoadsSet.insert(startroad);

	//反向
	reverse(lineRoads.begin(), lineRoads.end());

	if (lineRoads[0] == lineRoads[1])
		lineRoads.pop_back();
}

inline void getInRoads_DR(std::set<int> &lineRoads, ShapeFileAccessor &fileAccessor, int startroad, int endroad, std::vector<vertex_descriptor> &parents1,
	std::vector<double> &distances1, std::vector<vertex_descriptor> &parents2, std::vector<double> &distances2, int old_numEdges)
{
	//把经历的道路建一个队列
	//lineRoads.clear();
	std::set<int>().swap(lineRoads);

	int reverseRoad = 0;
	if (startroad < old_numEdges)
		reverseRoad = startroad + old_numEdges;
	else
		reverseRoad = startroad - old_numEdges;

	int reverseRoad_end = 0;
	if (endroad < old_numEdges)
		reverseRoad_end = endroad + old_numEdges;
	else
		reverseRoad_end = endroad - old_numEdges;

	double dis1 = min(distances1[endroad], distances1[reverseRoad_end]);
	double dis2 = min(distances2[endroad], distances2[reverseRoad_end]);

	//遍历得到所有经历的道路
	if (dis1 < dis2)
	{
		int lastRoad, nowRoad;
		if (distances1[endroad] < distances1[reverseRoad_end])
			lastRoad = int(parents1[endroad]);
		else
			lastRoad = int(parents1[reverseRoad_end]);
		while (lastRoad != startroad && lastRoad != reverseRoad)
		{
			nowRoad = lastRoad;
			int realRoad = nowRoad;
			if (nowRoad >= old_numEdges)
				realRoad = nowRoad - old_numEdges;
			lineRoads.insert(realRoad);
			lastRoad = int(parents1[nowRoad]);
		}
	}
	else
	{
		int lastRoad, nowRoad;
		if (distances2[endroad] < distances2[reverseRoad_end])
			lastRoad = int(parents2[endroad]);
		else
			lastRoad = int(parents2[reverseRoad_end]);
		while (lastRoad != startroad && lastRoad != reverseRoad)
		{
			nowRoad = lastRoad;
			int realRoad = nowRoad;
			if (nowRoad >= old_numEdges)
				realRoad = nowRoad - old_numEdges;
			lineRoads.insert(realRoad);
			lastRoad = int(parents2[nowRoad]);
		}
	}
	lineRoads.insert(startroad);
	lineRoads.insert(endroad);

}

inline void getInRoads_DR_partIn(std::set<int> &lineRoads, ShapeFileAccessor &fileAccessor, int startroad, int endroad, int edge_id, std::vector<vertex_descriptor> &parents1,
	std::vector<double> &distances1, std::vector<vertex_descriptor> &parents2, std::vector<double> &distances2, int old_numEdges)
{
	//把经历的道路建一个队列
	//lineRoads.clear();
	std::set<int>().swap(lineRoads);

	int reverseRoad = 0;
	if (startroad < old_numEdges)
		reverseRoad = startroad + old_numEdges;
	else
		reverseRoad = startroad - old_numEdges;

	//遍历得到所有经历的道路
	if (distances1[edge_id] < distances2[edge_id]) {	//判断选用哪个进行路径回溯
		int lastRoad, nowRoad;
		lastRoad = int(parents1[edge_id]);
		while (lastRoad != startroad && lastRoad != reverseRoad)
		{
			nowRoad = lastRoad;
			int realRoad = nowRoad;
			if (nowRoad >= old_numEdges)
				realRoad = nowRoad - old_numEdges;
			lineRoads.insert(realRoad);
			lastRoad = int(parents1[nowRoad]);
		}
	}
	else {
		int lastRoad, nowRoad;
		lastRoad = int(parents2[edge_id]);
		while (lastRoad != startroad && lastRoad != reverseRoad)
		{
			nowRoad = lastRoad;
			int realRoad = nowRoad;
			if (nowRoad >= old_numEdges)
				realRoad = nowRoad - old_numEdges;
			lineRoads.insert(realRoad);
			lastRoad = int(parents2[nowRoad]);
		}
	}

	lineRoads.insert(startroad);
	lineRoads.insert(endroad);
}

inline void getInRoads_JncR2(std::vector<int> &lineRoads, std::set<int> &lineRoadsSet, ShapeFileAccessor &fileAccessor, int startroad, int endroad, std::vector<vertex_descriptor> &parents3, std::vector<double> &distances3)
{
	//把经历的道路建一个队列
	//lineRoads.clear();
	std::vector<int>().swap(lineRoads);
	//lineRoadsSet.clear();
	std::set<int>().swap(lineRoadsSet);
	lineRoads.push_back(endroad);
	lineRoadsSet.insert(endroad);

	int lastRoad = int(parents3[endroad]), nowRoad;
	while (lastRoad != startroad)
	{
		nowRoad = lastRoad;
		lineRoads.push_back(nowRoad);
		lineRoadsSet.insert(nowRoad);
		lastRoad = int(parents3[nowRoad]);
	}

	lineRoads.push_back(startroad);
	lineRoadsSet.insert(startroad);

	//反向
	reverse(lineRoads.begin(), lineRoads.end());

	if (lineRoads[0] == lineRoads[1])
		lineRoads.pop_back();
}

inline void getInRoads_JncR(std::set<int> &lineRoads, ShapeFileAccessor &fileAccessor, int startroad, int endroad, std::vector<vertex_descriptor> &parents3, std::vector<double> &distances3)
{
	//把经历的道路建一个队列
	lineRoads.clear();
	std::set<int>().swap(lineRoads);

	int lastRoad = int(parents3[endroad]), nowRoad;
	while (lastRoad != startroad)
	{
		nowRoad = lastRoad;
		lineRoads.insert(nowRoad);
		lastRoad = int(parents3[nowRoad]);
	}
	lineRoads.insert(startroad);
	lineRoads.insert(endroad);

}

inline double CalculateRelen(ShapeFileAccessor &fileAccessor, std::set<int> &inRoads, int startedge, int endedge)
{
	double result = 0;

	if (inRoads.size() == 1)
		return 0;

	for (auto it = inRoads.begin(); it != inRoads.end(); it++)
	{
		if (*it == startedge || *it == endedge)
		{
			result += 0.5*fileAccessor.Length[*it];
		}
		else
			result += fileAccessor.Length[*it];
	}

	return result;
}

inline double CalculateRelen_Net(ShapeFileAccessor &fileAccessor, std::set<int> &inRoads)
{
	double result = 0;
	for (auto it = inRoads.begin(); it != inRoads.end(); it++)
	{
		result += fileAccessor.Length[*it];
	}

	return result;
}

inline int CalculateDC(ShapeFileAccessor &fileAccessor, std::vector<int> &lineRoads, int oldnum)
{
	int dc = 0;
	for (int i = 0; i<int(lineRoads.size()) - 1; i++)
	{
		if (lineRoads[i] == lineRoads[i + 1])
			continue;

		int edge1_A = lineRoads[i];
		int edge1_B = lineRoads[i] + oldnum;
		int edge2_A = lineRoads[i + 1];
		int edge2_B = lineRoads[i + 1] + oldnum;
		std::string key1 = IndexToKey(edge1_A, edge2_A);
		std::string key2 = IndexToKey(edge1_A, edge2_B);
		std::string key3 = IndexToKey(edge1_B, edge2_A);
		std::string key4 = IndexToKey(edge1_B, edge2_B);
		if ((fileAccessor.AdjTurn.count(key1) > 0 && fileAccessor.AdjTurn[key1] > 0) ||
			(fileAccessor.AdjTurn.count(key2) > 0 && fileAccessor.AdjTurn[key2] > 0) ||
			(fileAccessor.AdjTurn.count(key3) > 0 && fileAccessor.AdjTurn[key3] > 0) ||
			(fileAccessor.AdjTurn.count(key4) > 0 && fileAccessor.AdjTurn[key4] > 0))
			++dc;
	}

	return dc;
}

inline int CalculateDCAll(ShapeFileAccessor &fileAccessor, std::vector<std::vector<int>> &lineRoadsAll, int oldnum)
{
	int dc = 0;
	std::set<std::string> st;
	for (auto it = lineRoadsAll.begin(); it != lineRoadsAll.end();it++) {
		for (int i = 0; i<int((*it).size()) - 1; i++)
		{
			if ((*it)[i] == (*it)[i + 1])
				continue;

			int edge1_A = (*it)[i];
			int edge1_B = (*it)[i] + oldnum;
			int edge2_A = (*it)[i + 1];
			int edge2_B = (*it)[i + 1] + oldnum;
			std::string key1 = IndexToKey(edge1_A, edge2_A);
			std::string key2 = IndexToKey(edge1_A, edge2_B);
			std::string key3 = IndexToKey(edge1_B, edge2_A);
			std::string key4 = IndexToKey(edge1_B, edge2_B);
			st.insert(key1);
			st.insert(key2);
			st.insert(key3);
			st.insert(key4);
		}
	}
	for(auto iter=st.begin();iter!=st.end();iter++){
		if (fileAccessor.AdjTurn.count(*iter) > 0 )
			++dc;
	}

	return dc;
}

inline void Generate_new_dict_real_min_dist(ShapeFileAccessor &fileAccessor, std::map<int, double> &new_dict_real_min_dist, std::map<int, double> &dict_min_dist,
	std::set<int> &inRoads, int startRoad, int old_numEdges, std::vector<vertex_descriptor> &new_parents1, 
	std::vector<double> &new_distances1, std::vector<vertex_descriptor> &new_parents2, std::vector<double> &new_distances2)
{
	if (inRoads.size() == 0)
		return;

	//根据MR_Limit所能到达的路径，重新构造有向图，计算新的最小转弯次数
	std::map<int, double>().swap(new_dict_real_min_dist);
	dict_min_dist.clear();

	//创建有向图		
	Graph_d edgGraph_subDR;
	GenerateDirectedGraph2(fileAccessor, inRoads, old_numEdges, edgGraph_subDR);

	//节点数目=最大节点id值
	int numVertices_DR = int(boost::num_vertices(edgGraph_subDR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	int startRoad2 = startRoad + old_numEdges;

	boost::dijkstra_shortest_paths(edgGraph_subDR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
	boost::dijkstra_shortest_paths(edgGraph_subDR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

	//计算到每个单向边的最短距离：有向图中每个节点就是有向边
	for (auto it = inRoads.begin(); it != inRoads.end(); it++) {
		int k = *it;
		int reverse_k = k + old_numEdges;
		dict_min_dist.insert(std::make_pair(k, min(distances1[k], distances2[k])));
		dict_min_dist.insert(std::make_pair(reverse_k, min(distances1[reverse_k], distances2[reverse_k])));
	}

	for (auto it = inRoads.begin(); it != inRoads.end(); it++){
		//找出有向边i对应的反向边id
		int k = *it;
		int reverse_k = k + old_numEdges;
		double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
		new_dict_real_min_dist.insert(std::make_pair(k, min_d));
	}

	//有向图赋值
	new_parents1.clear();
	new_parents1.insert(new_parents1.begin(), parents1.begin(), parents1.end());
	new_distances1.clear();
	new_distances1.insert(new_distances1.begin(), distances1.begin(), distances1.end());
	new_parents2.clear();
	new_parents2.insert(new_parents2.begin(), parents2.begin(), parents2.end());
	new_distances2.clear();
	new_distances2.insert(new_distances2.begin(), distances2.begin(), distances2.end());
}

void Generate_new_dict_real_min_dist_BFS(ShapeFileAccessor &fileAccessor, std::set<int> &inRoads, int startRoad, std::set<int> &allInRoads,
	std::map<int, double> &new_dict_real_min_dist, int old_numEdges, std::set<double> &DRLimitSet, std::map<double, std::set<int>> &validRoad) {

	if (inRoads.size() == 0)
		return;

	//清空旧数据
	new_dict_real_min_dist.clear();
	validRoad.clear();

	//根据MR_Limit所能到达的路径，重新构造有向图，计算新的最小转弯次数
	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;

	int total = fileAccessor.roadID.size();
	double max_DRLimit = INT_MAX;

	//准备数据结构
	std::queue<int> q;
	std::vector<double> dist(2 * total, 0x7fffffff);
	std::vector<int> parents(2 * total, 0x7fffffff);
	std::vector<bool> visited(2 * total, false);

	//初始化数据
	q.push(startRoad);
	dist[startRoad] = 0;

	//BFS计算到连接线段的最短距离
	while (!q.empty()) {
		int nowRoad = q.front();
		q.pop();

		if (visited[nowRoad])
			continue;
		visited[nowRoad] = true;

		if (nowRoad == 817)
			bool check = true;

		for (auto next_iter = fileAccessor.AdjRoadList[nowRoad].begin(); next_iter != fileAccessor.AdjRoadList[nowRoad].end(); next_iter++) {
			int connected_cursor = *next_iter;
			if (nowRoad % total == connected_cursor % total || inRoads.count(connected_cursor % total) == 0)
				continue;

			//可行：全覆盖->部分覆盖，全覆盖->全覆盖
			//不可行：部分覆盖->全覆盖，部分覆盖->部分覆盖
			int reverse_k = nowRoad;
			if (nowRoad < old_numEdges) reverse_k += old_numEdges;
			else reverse_k -= old_numEdges;

			//如果更新了已经reach的线条，需要重新遍历
			int turn = fileAccessor.AdjTurnMP[nowRoad][connected_cursor];
			if (dist[nowRoad] + turn < dist[connected_cursor]) {
				////不允许前往逆向的道路
				//if (connected_cursor%old_numEdges == parents[nowRoad] % old_numEdges)
				//	continue;
				visited[connected_cursor] = false;
				dist[connected_cursor] = dist[nowRoad] + turn;
				parents[connected_cursor] = nowRoad;
			}
			if (dist[connected_cursor] <= max_DRLimit)
				q.push(connected_cursor);
		}
	}

	//将起点设置为反向边计算最短路径
	int startRoad2 = startRoad + total;

	//准备数据结构
	std::queue<int> q2;
	std::vector<double> dist2(2 * total, 0x7fffffff);
	std::vector<int> parents2(2 * total, 0x7fffffff);
	std::vector<bool> visited2(2 * total, false);

	//初始化数据
	q2.push(startRoad2);
	dist2[startRoad2] = 0;

	//BFS计算到连接线段的最短距离
	while (!q2.empty()) {
		int nowRoad = q2.front();
		q2.pop();

		if (visited2[nowRoad])
			continue;
		visited2[nowRoad] = true;

		for (auto next_iter = fileAccessor.AdjRoadList[nowRoad].begin(); next_iter != fileAccessor.AdjRoadList[nowRoad].end(); next_iter++) {
			int connected_cursor = *next_iter;
			if (nowRoad % total == connected_cursor % total || inRoads.count(connected_cursor % total) == 0)
				continue;

			//可行：全覆盖->部分覆盖，全覆盖->全覆盖
			//不可行：部分覆盖->全覆盖，部分覆盖->部分覆盖
			int reverse_k = nowRoad;
			if (nowRoad < old_numEdges) reverse_k += old_numEdges;
			else reverse_k -= old_numEdges;

			//如果更新了已经reach的线条，需要重新遍历
			int turn = fileAccessor.AdjTurnMP[nowRoad][connected_cursor];
			if (dist2[nowRoad] + turn < dist2[connected_cursor]) {
				////不允许前往逆向的道路
				//if (connected_cursor%old_numEdges == parents2[nowRoad] % old_numEdges)
				//	continue;
				visited2[connected_cursor] = false;
				dist2[connected_cursor] = dist2[nowRoad] + turn;
				parents2[connected_cursor] = nowRoad;
			}
			if (dist2[connected_cursor] <= max_DRLimit)
				q2.push(connected_cursor);
		}
	}

	//计算到每个单向边的最短距离：应该将全覆盖和部分覆盖分开来来计算
	//全覆盖：key为road_id
	for (auto road_it = allInRoads.begin(); road_it != allInRoads.end(); road_it++) {
		int road_id = *road_it;
		int k = road_id;
		int reverse_k = k + old_numEdges;
		dict_min_dist.insert(std::make_pair(k, min(dist[k], dist2[k])));
		dict_min_dist.insert(std::make_pair(reverse_k, min(dist[reverse_k], dist2[reverse_k])));
		dict_real_min_dist[k] = min(dict_min_dist[k], dict_min_dist[reverse_k]);

	}

	new_dict_real_min_dist = dict_real_min_dist;
}

void Generate_new_dict_real_min_dist_partIn_BFS(ShapeFileAccessor &fileAccessor, std::set<int> &inRoads, int startRoad, std::set<int> &allInRoads, 
	std::map<int, std::map<int, double>> &partInRoadsNodeLen, int old_numEdges, std::set<double> &DRLimitSet, std::map<double, std::set<int>> &validRoad) {

	if (inRoads.size() == 0)
		return;

	//清空旧数据
	validRoad.clear();

	//根据MR_Limit所能到达的路径，重新构造有向图，计算新的最小转弯次数
	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;

	int total = fileAccessor.roadID.size();
	double max_DRLimit = INT_MAX;

	for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++)
	{
		if (*iter == -1) {
			max_DRLimit = INT_MAX;
			break;
		}
		max_DRLimit = *iter;
	}

	if (max_DRLimit == -1 && DRLimitSet.size() == 1) {
		validRoad[-1] = inRoads;
		return;
	}

	//准备数据结构
	std::queue<int> q;
	std::vector<double> dist(2 * total, 0x7fffffff);
	std::vector<int> parents(2 * total, 0x7fffffff);
	std::vector<bool> visited(2 * total, false);

	//初始化数据
	q.push(startRoad);
	dist[startRoad] = 0;

	//BFS计算到连接线段的最短距离
	while (!q.empty()) {
		int nowRoad = q.front();
		q.pop();

		if (visited[nowRoad])
			continue;
		visited[nowRoad] = true;

		if (nowRoad == 817)
			bool check = true;

		for (auto next_iter = fileAccessor.AdjRoadList[nowRoad].begin(); next_iter != fileAccessor.AdjRoadList[nowRoad].end(); next_iter++) {
			int connected_cursor = *next_iter;
			if (nowRoad % total == connected_cursor % total || inRoads.count(connected_cursor % total) == 0)
				continue;

			//可行：全覆盖->部分覆盖，全覆盖->全覆盖
			//不可行：部分覆盖->全覆盖，部分覆盖->部分覆盖
			int reverse_k = nowRoad;
			if (nowRoad < old_numEdges) reverse_k += old_numEdges;
			else reverse_k -= old_numEdges;
			if (partInRoadsNodeLen.count(nowRoad) || partInRoadsNodeLen.count(reverse_k))
				continue;

			//如果更新了已经reach的线条，需要重新遍历
			int turn = fileAccessor.AdjTurnMP[nowRoad][connected_cursor];
			if (dist[nowRoad] + turn < dist[connected_cursor]) {
				////不允许前往逆向的道路
				//if (connected_cursor%old_numEdges == parents[nowRoad] % old_numEdges)
				//	continue;
				visited[connected_cursor] = false;
				dist[connected_cursor] = dist[nowRoad] + turn;
				parents[connected_cursor] = nowRoad;
			}
			if (dist[connected_cursor] <= max_DRLimit)
				q.push(connected_cursor);
		}
	}

	//将起点设置为反向边计算最短路径
	int startRoad2 = startRoad + total;

	//准备数据结构
	std::queue<int> q2;
	std::vector<double> dist2(2 * total, 0x7fffffff);
	std::vector<int> parents2(2 * total, 0x7fffffff);
	std::vector<bool> visited2(2 * total, false);

	//初始化数据
	q2.push(startRoad2);
	dist2[startRoad2] = 0;

	//BFS计算到连接线段的最短距离
	while (!q2.empty()) {
		int nowRoad = q2.front();
		q2.pop();

		if (visited2[nowRoad])
			continue;
		visited2[nowRoad] = true;

		for (auto next_iter = fileAccessor.AdjRoadList[nowRoad].begin(); next_iter != fileAccessor.AdjRoadList[nowRoad].end(); next_iter++) {
			int connected_cursor = *next_iter;
			if (nowRoad % total == connected_cursor % total || inRoads.count(connected_cursor % total) == 0)
				continue;

			//可行：全覆盖->部分覆盖，全覆盖->全覆盖
			//不可行：部分覆盖->全覆盖，部分覆盖->部分覆盖
			int reverse_k = nowRoad;
			if (nowRoad < old_numEdges) reverse_k += old_numEdges;
			else reverse_k -= old_numEdges;
			if (partInRoadsNodeLen.count(nowRoad) || partInRoadsNodeLen.count(reverse_k))
				continue;

			//如果更新了已经reach的线条，需要重新遍历
			int turn = fileAccessor.AdjTurnMP[nowRoad][connected_cursor];
			if (dist2[nowRoad] + turn < dist2[connected_cursor]) {
				////不允许前往逆向的道路
				//if (connected_cursor%old_numEdges == parents2[nowRoad] % old_numEdges)
				//	continue;
				visited2[connected_cursor] = false;
				dist2[connected_cursor] = dist2[nowRoad] + turn;
				parents2[connected_cursor] = nowRoad;
			}
			if (dist2[connected_cursor] <= max_DRLimit)
				q2.push(connected_cursor);
		}
	}

	//计算到每个单向边的最短距离：应该将全覆盖和部分覆盖分开来来计算
	//全覆盖：key为road_id
	for (auto road_it = allInRoads.begin(); road_it != allInRoads.end(); road_it++) {
		int road_id = *road_it;
		int k = road_id;
		int reverse_k = k + old_numEdges;
		dict_min_dist.insert(std::make_pair(k, min(dist[k], dist2[k])));
		dict_min_dist.insert(std::make_pair(reverse_k, min(dist[reverse_k], dist2[reverse_k])));
		dict_real_min_dist[k] = min(dict_min_dist[k], dict_min_dist[reverse_k]);

		for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++) {
			double DRLimit = *iter;

			if (DRLimit == -1)
				validRoad[DRLimit].insert(k);
			if (dict_real_min_dist[k] <= DRLimit)
				validRoad[DRLimit].insert(k);
		}
	}
	//部分覆盖：key为node_id转换后的id，默认存储小id (注意：部分覆盖的线条，在通行方向上只有经过交点node出去的份，不可以进来)
	std::map<int, std::map<int, double>> new_partInRoadsNodeLen;
	for (auto road_it = partInRoadsNodeLen.begin(); road_it != partInRoadsNodeLen.end(); road_it++) {
		int road_id = road_it->first;
		for (auto node_it = road_it->second.begin(); node_it != road_it->second.end(); node_it++) {
			int node = node_it->first;
			int node1 = fileAccessor.roadNode[road_id][0];
			int node2 = fileAccessor.roadNode[road_id][1];
			int road_id = road_it->first;
			int edge_id = road_id;
			if (node == node2)
				edge_id = road_id + old_numEdges;
			//部分覆盖线条在方向上只有出去的份
			double dr_cost = min(dist[edge_id], dist2[edge_id]);
			
			for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++) {
				double DRLimit = *iter;

				if (DRLimit == -1 || dr_cost <= DRLimit) {
					validRoad[DRLimit].insert(road_id);
					new_partInRoadsNodeLen[road_id][node] = node_it->second;
				}	
			}
		}
	}

	//存储数据
	partInRoadsNodeLen.clear();
	partInRoadsNodeLen = new_partInRoadsNodeLen;
}

//带有部分覆盖的有向图构建与搜索
inline void Generate_new_dict_real_min_dist_partIn(ShapeFileAccessor &fileAccessor, std::map<int, double> &new_dict_real_min_dist, std::map<int, double> &dict_min_dist,
	std::set<int> &inRoads, int startRoad, std::set<int> &allInRoads, std::map<int, std::map<int, double>> &partInRoadsNodeLen, int old_numEdges, int old_nodeNums,
	std::vector<vertex_descriptor> &new_parents1, std::vector<double> &new_distances1, std::vector<vertex_descriptor> &new_parents2, std::vector<double> &new_distances2,
	std::map<int, std::map<int, int>> &partInNodeIDs, std::map<std::tuple<int, int, int>, int> &nodeToEdge, std::map<int, std::tuple<int, int, int>> &edgeToNode)
{
	if (inRoads.size() == 0)
		return;

	//根据MR_Limit所能到达的路径，重新构造有向图，计算新的最小转弯次数
	std::map<int, double>().swap(new_dict_real_min_dist);
	dict_min_dist.clear();
	partInNodeIDs.clear();
	nodeToEdge.clear();
	edgeToNode.clear();
	
	//创建有向图		
	Graph_d edgGraph_subDR;
	GenerateDirectedGraph3(fileAccessor, inRoads, allInRoads, partInRoadsNodeLen, old_numEdges, old_nodeNums, partInNodeIDs, nodeToEdge, edgeToNode, edgGraph_subDR);

	//节点数目=最大节点id值
	int numVertices_DR = int(boost::num_vertices(edgGraph_subDR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	int startRoad2 = startRoad + old_numEdges;

	boost::dijkstra_shortest_paths(edgGraph_subDR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
	boost::dijkstra_shortest_paths(edgGraph_subDR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

	//计算到每个单向边的最短距离：应该将全覆盖和部分覆盖分开来来计算
	//全覆盖：key为road_id
	for (auto road_it = allInRoads.begin(); road_it != allInRoads.end(); road_it++) {
		int road_id = *road_it;
		int k = road_id;
		int reverse_k = k + old_numEdges;
		dict_min_dist.insert(std::make_pair(k, min(distances1[k], distances2[k])));
		dict_min_dist.insert(std::make_pair(reverse_k, min(distances1[reverse_k], distances2[reverse_k])));
		new_dict_real_min_dist[k] = min(dict_min_dist[k], dict_min_dist[reverse_k]);
	}
	//部分覆盖：key为node_id转换后的id，默认存储小id (注意：部分覆盖的线条，在通行方向上只有经过交点node出去的份，不可以进来)
	for (auto road_it = partInNodeIDs.begin(); road_it != partInNodeIDs.end(); road_it++) {
		int road_id = road_it->first;
		for (auto node_it = road_it->second.begin(); node_it != road_it->second.end(); node_it++) {
			int node = node_it->first;
			int node1 = fileAccessor.roadNode[road_id][0];
			int node2 = fileAccessor.roadNode[road_id][1];
			if (node == node2)
				std::swap(node1, node2);
			//部分覆盖线条在方向上只有出去的份
			int edge_id = getEdgeID({ node, node1, node2 }, fileAccessor.AdjRoadList.size(), nodeToEdge, edgeToNode);
			dict_min_dist.insert(std::make_pair(edge_id, min(distances1[edge_id], distances2[edge_id])));
			new_dict_real_min_dist[edge_id] = dict_min_dist[edge_id];
		}
	}

	//有向图赋值
	new_parents1.clear();
	new_parents1.insert(new_parents1.begin(), parents1.begin(), parents1.end());
	new_distances1.clear();
	new_distances1.insert(new_distances1.begin(), distances1.begin(), distances1.end());
	new_parents2.clear();
	new_parents2.insert(new_parents2.begin(), parents2.begin(), parents2.end());
	new_distances2.clear();
	new_distances2.insert(new_distances2.begin(), distances2.begin(), distances2.end());
}

inline double DD(ShapeFileAccessor& fileAccessor, std::map<int, double>& dict_min_dist,std::map<int, double>& dict_real_min_dist, std::set<int>& validRoad,
	std::map<int,partInNode>& partInNodeIDs){
	std::map<int, int> ddMap;	//key????????value???・???
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
	for(int road_id: validRoad) {
		if (partInNodeIDs.count(road_id)){
			if (partInNodeIDs[road_id].leftLength != 0) {
				ddMap[dict_min_dist[road_id]]++;
			}
			if (partInNodeIDs[road_id].rightLength != 0) {
				ddMap[dict_min_dist[road_id]]++;
			}
		}
		else {
			ddMap[int(dict_real_min_dist[road_id])] ++;
		}
	}
	double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
	for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++)
	{
		M_ddSum1 += ((it3->first) * (it3->second));
		M_ddSum2 += it3->second;
	}
	if (M_ddSum2 == 0)
		M_dd = 0;
	else
		M_dd = M_ddSum1 / M_ddSum2;

	return M_dd;
}
inline double CalculateDD_PartIn(ShapeFileAccessor &fileAccessor, std::map<int, double> &dict_real_min_dist, std::set<int> &inRoads, 
	std::map<int, std::map<int, int>> &partInNodeIDs)
{
	std::map<int, int> ddMap;	//key转弯次数，value为道路数目
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++){
		int road_id = *it2;
		if (partInNodeIDs.count(road_id)) {
			int min_cost = INT_MAX;
			for (auto node_it = partInNodeIDs[road_id].begin(); node_it != partInNodeIDs[road_id].end(); node_it++) {
				int node = node_it->first;
				//部分覆盖线条在方向上只有出去的份
				int edge_id = partInNodeIDs[road_id][node];
				ddMap[dict_real_min_dist[edge_id]] += 1;
			}
		}
		else {
			ddMap[int(dict_real_min_dist[*it2])] += 1;
		}
	}
	double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
	for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++)
	{
		M_ddSum1 += ((it3->first)*(it3->second));
		M_ddSum2 += it3->second;
	}
	if (M_ddSum2 == 0)
		M_dd = 0;
	else
		M_dd = M_ddSum1 / M_ddSum2;

	return M_dd;
}

inline double CalculateRoadsDD(ShapeFileAccessor &fileAccessor, std::map<int, double> &dict_real_min_dist, std::set<int> &inRoads)
{
	std::map<int, int> ddMap;	//key转弯次数，value为道路数目
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++){
		ddMap[int(dict_real_min_dist[*it2])] += 1;
	}
	double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
	for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++)
	{
		M_ddSum1 += ((it3->first)*(it3->second));
		M_ddSum2 += it3->second;
	}
	M_dd = M_ddSum1 / M_ddSum2;

	return M_dd;
}

inline double DDL(ShapeFileAccessor& fileAccessor, std::map<int, double>& dict_min_dist, std::map<int, double>& dict_real_min_dist, std::set<int>& validRoad,
	std::map<int, partInNode>& partInNodeIDs) {
	std::map<int, int> ddlMap;	//key????????value???・????
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0 , M_ddlSum1 = 0, M_ddlSum2 = 0;
	for (int road_id : validRoad) {
		if (partInNodeIDs.count(road_id)) {
			if (partInNodeIDs[road_id].leftLength != 0) {
				ddlMap[dict_min_dist[road_id]]+= partInNodeIDs[road_id].leftLength;
			}
			if (partInNodeIDs[road_id].rightLength != 0) {
				ddlMap[dict_min_dist[road_id]] += partInNodeIDs[road_id].rightLength;
			}

		}
		else {
			ddlMap[int(dict_real_min_dist[road_id])]+=fileAccessor.Length[road_id];
		}
	}
	
	for (auto it3 = ddlMap.begin(); it3 != ddlMap.end(); it3++)
	{
		M_ddlSum1 += ((it3->first) * (it3->second));  //sum????????*・????
		M_ddlSum2 += it3->second; 
	}
	if (M_ddlSum2 == 0)
		M_ddl = 0;
	else
		M_ddl = M_ddlSum1 / M_ddlSum2;

	return M_ddl;
}

inline double CalculateDDL_PartIn(ShapeFileAccessor &fileAccessor, std::map<int, double> &dict_real_min_dist, std::set<int> &inRoads, 
	std::map<int, std::vector<double>> &partIn, int startRoad, int old_numEdges, std::map<int, std::map<int, int>> &partInNodeIDs, 
	std::map<int, std::map<int, double>> &parInNodeLen)
{
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0, len = 0;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++){
		int id = *it2;
		if (partIn.count(id) == 0) {
			len = fileAccessor.Length[*it2];
			M_ddlSum += dict_real_min_dist[id] * len;
			M_lenSum += len;
		}
		else {
			if (partInNodeIDs.count(id) == 0) {		//起点id不在行列
				continue;
			}
			for (auto node_it = partInNodeIDs[id].begin(); node_it != partInNodeIDs[id].end(); node_it++) {
				int node = node_it->first;
				//部分覆盖线条在方向上只有出去的份
				int edge_id = partInNodeIDs[id][node];
				double len = parInNodeLen[id][node];
				M_ddlSum += dict_real_min_dist[edge_id] * len;
				M_lenSum += len;
			}
		}
	}
	if (M_lenSum == 0)
		M_ddl = 0;
	else
		M_ddl = M_ddlSum / M_lenSum;

	return M_ddl;
}

inline double CalculateJncD_Part(ShapeFileAccessor &fileAccessor, std::vector<double> &Jnc_distance, 
	std::set<int> &inRoads, std::map<int, std::vector<double>> &partIn, int startRoad) {
	//计算通行范围内ddl
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0, len = 0;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++)
	{
		int id = *it2;
		if (partIn.count(id) == 0) {
			len = fileAccessor.Length[*it2];
		}
		else {
			len = 0;
			for (auto iter = partIn[id].begin(); iter != partIn[id].end(); iter++) {
				len += *iter;
			}
		}

		M_ddlSum += Jnc_distance[id] * len;
		M_lenSum += len;
	}
	if (M_lenSum == 0)
		M_ddl = 0;
	else
		M_ddl = M_ddlSum / M_lenSum;

	return M_ddl;
}

inline double CalculateJD_Part(ShapeFileAccessor &fileAccessor, std::vector<double> &Jnc_distance,
	std::set<int> &inRoads, std::map<int, std::vector<double>> &partIn, int startRoad)
{
	std::map<int, int> ddMap;	//key转弯次数，value为道路数目
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++) {
		ddMap[int(Jnc_distance[*it2])] += 1;
	}
	double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
	for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++)
	{
		M_ddSum1 += ((it3->first)*(it3->second));
		M_ddSum2 += it3->second;
	}
	if (M_ddSum2 == 0)
		M_dd = 0;
	else
		M_dd = M_ddSum1 / M_ddSum2;

	return M_dd;
}

inline double CalculateJD_All(ShapeFileAccessor &fileAccessor, std::vector<double> &Jnc_distance, std::set<int> &inRoads, int startRoad) {
	std::map<int, int> ddMap;	//key转弯次数，value为道路数目
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++) {
		ddMap[int(Jnc_distance[*it2])] += 1;
	}
	double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
	for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++)
	{
		M_ddSum1 += ((it3->first)*(it3->second));
		M_ddSum2 += it3->second;
	}
	if (M_ddSum2 == 0)
		M_dd = 0;
	else
		M_dd = M_ddSum1 / M_ddSum2;

	return M_dd;
}

inline double CalculateJncD_All(ShapeFileAccessor &fileAccessor, std::vector<double> &Jnc_distance, std::set<int> &inRoads, int startRoad) {
	//计算通行范围内ddl
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0, len = 0;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++)
	{
		int id = *it2;
		len = fileAccessor.Length[*it2];

		if (Jnc_distance[id] > 99999)
			bool check = true;

		M_ddlSum += Jnc_distance[id] * len;
		M_lenSum += len;
	}
	M_ddl = M_ddlSum / M_lenSum;

	return M_ddl;
}

inline double CalculateRoadsDDL(ShapeFileAccessor &fileAccessor, std::map<int, double> &dict_real_min_dist,
	std::set<int> &inRoads, std::map<int, std::vector<double>> &partIn, int startRoad, int old_numEdges)
{
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0, len = 0;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++)
	{
		int id = *it2;
		if (partIn.count(id) == 0){
			len = fileAccessor.Length[*it2];
		}
		else{
			len = 0;
			for (auto iter = partIn[id].begin(); iter != partIn[id].end(); iter++){
				len += *iter;
			}
		}

		M_ddlSum += dict_real_min_dist[id] * len;
		M_lenSum += len;
	}
	M_ddl = M_ddlSum / M_lenSum;

	return M_ddl;
}

std::map<std::string, double> CalculateWDD(ShapeFileAccessor &fileAccessor, std::map<std::string, std::map<int, double>> &weight, std::map<int, double> &dict_real_min_dist, std::set<int> &inRoads)
{
	std::map<int, std::set<int>> ddMap;	//key转弯次数，value为道路数目
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
	for (auto it2 = inRoads.begin(); it2 != inRoads.end(); it2++)
	{
		ddMap[int(dict_real_min_dist[*it2])].insert(*it2);
	}

	std::map<std::string, double> wdd;
	std::map<std::string, double> wdd_sum1;
	std::map<std::string, double> wdd_sum2;
	std::map<std::string, double> sub_wgt;
	for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++){
		sub_wgt = CalculateWgtInline(weight, it3->second);
		for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
			wdd_sum1[wgt_it->first] += ((it3->first)*(sub_wgt[wgt_it->first]));
		}
	}
	wdd_sum2 = CalculateWgtInline(weight, inRoads);

	for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
		if (wdd_sum2[wgt_it->first] == 0)
			wdd[wgt_it->first] = 0;
		else
			wdd[wgt_it->first] = wdd_sum1[wgt_it->first] / wdd_sum2[wgt_it->first];
	}

	return wdd;
}

//初始化参数
void Calculation::initPara(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr,
	bool ckMetricReach, bool ckDirectionalReach, bool ckJncR, bool ckJncDDL, bool ckJunctions, bool ckWgt,
	std::string txRadiiThreshold, std::string txRadiiThreshold2, std::string txAngleThreshold, std::string txDirectionalChanges,
	std::string txJunctionDegree, std::string txJunctionsLimit, std::string txAngleThreshold2, std::string txNewJnc, std::vector<std::string> &weightAttributesSet,
	bool ckNetreach, bool ckGeodesics, bool ckPath)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '\\')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv";
	dbfFilePath = dbfFilePath + ".dbf";


	//开关
	isMR = ckMetricReach;
	isDR = ckDirectionalReach;
	isJncR = ckJncR, isJncDDL = ckJncDDL;
	isJnc = ckJunctions, isWgt = ckWgt;
	isNet = ckNetreach, isGeo = ckGeodesics;
	outputPath = ckPath;

	//参数
	if (isMR)
	{
		MRLimitSet = getParaSet(txRadiiThreshold);
	}
	if (isDR)
	{
		MRLimitSet2 = getParaSet(txRadiiThreshold2);
		DRLimitSet = getParaSet(txDirectionalChanges);
	}
	angleLimitDR = getPara(txAngleThreshold);

	if (isJncR)
	{
		Jnc_maxNumSet = getParaSet(txJunctionsLimit);
	}
	Jnc_t_limit_JncR = getPara(txJunctionDegree);

	if (isJncDDL)
	{
		angleLimitJncDDL = getPara(txAngleThreshold2);
	}

	if (isJnc)
	{
		Jnc_t_limit_Jnc = getPara(txNewJnc);
	}
	if (isWgt)
	{
		getWeightData(infilepath, weightAttributesSet);
	}

	//准备好转向和交叉口数据：角度和交叉口只会取一组数据
	double angle = -1, Jnc_t = -1;
	if (isJncDDL)
		angle = angleLimitJncDDL;
	else
	{
		angle = angleLimitDR;
	}

	if (isJnc)
		Jnc_t = Jnc_t_limit_Jnc;
	else
	{
		Jnc_t = Jnc_t_limit_JncR;
	}

	if (searchLimitStr.find(".csv") != std::string::npos);
	else
		fileAccessor.calculateAngle(angle, Jnc_t);
}

void Calculation::init_MD_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold,
	bool ckJnc, bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isMR = true, isMD = true;
	isJnc = ckJnc, isWgt = ckWgt;

	//参数
	MRLimitSet = getParaSet(txRadiiThreshold);

	////测试用，待删除
	//fileAccessor.calculateAngle(INT_MAX, INT_MAX);

	if (isJnc)
	{
		Jnc_t_limit_Jnc = getPara(txNewJnc);
		fileAccessor.calculateAngle(INT_MAX, Jnc_t_limit_Jnc);
	}
	if (weightAttributesSet.size() > 0)
	{
		isWgt = true;
		getWeightData(infilepath, weightAttributesSet);
	}
}

void Calculation::init_MR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold,
	bool ckJnc, bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isMR = true;
	isJnc = ckJnc, isWgt = ckWgt;

	//参数
	MRLimitSet = getParaSet(txRadiiThreshold);

	////测试用，待删除
	//fileAccessor.calculateAngle(INT_MAX, INT_MAX);

	if (isJnc)
	{
		Jnc_t_limit_Jnc = getPara(txNewJnc);
		fileAccessor.calculateAngle(INT_MAX, Jnc_t_limit_Jnc);
	}
	if (weightAttributesSet.size()>0)
	{
		isWgt = true;
		getWeightData(infilepath, weightAttributesSet);
	}
}

//初始化参数和基础数据
void Calculation::init_subset_para(ShapeFileAccessor &fileAccessor, subsetPara &para) {
	//开关
	isMR = true;
	isDR = true;
	isJncR = true;

	//mr
	MRLimitSet.insert(para.MR_Para.mr_limit);
	//dr
	angleLimitDR = para.DR_Para.angle_limit;
	DRLimitSet.insert(para.DR_Para.dc_limit);
	//jncr
	Jnc_t_limit_JncR = para.JnR_Para.jnc_degree;
	Jnc_maxNumSet.insert(para.JnR_Para.jnc_limit);

	//准备基础数据
	fileAccessor.calculateAngle(angleLimitDR, Jnc_t_limit_JncR);
}

void Calculation::init_MDR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold2,
	std::string txAngleThreshold, std::string txDirectionalChanges, bool ckJnc, bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet) {
	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isDR = true;
	isMDR = true;
	isJnc = ckJnc, isWgt = ckWgt;

	//参数
	MRLimitSet2 = getParaSet(txRadiiThreshold2);
	angleLimitDR = getPara(txAngleThreshold);
	DRLimitSet = getParaSet(txDirectionalChanges);

	if (isJnc)
	{
		Jnc_t_limit_Jnc = getPara(txNewJnc);
		fileAccessor.calculateAngle(angleLimitDR, Jnc_t_limit_Jnc);
	}
	else
	{
		fileAccessor.calculateAngle(angleLimitDR, INT_MAX);
	}

	if (weightAttributesSet.size() > 0)
	{
		isWgt = true;
		getWeightData(infilepath, weightAttributesSet);
	}
}

void Calculation::init_DDL_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold2,
	std::string txAngleThreshold, std::string txDirectionalChanges, bool ckJnc, bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet) {
	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isDR = true;
	isDDL = true;
	isJnc = ckJnc, isWgt = ckWgt;

	//参数
	MRLimitSet2 = getParaSet(txRadiiThreshold2);
	angleLimitDR = getPara(txAngleThreshold);
	DRLimitSet = getParaSet(txDirectionalChanges);

	if (isJnc)
	{
		Jnc_t_limit_Jnc = getPara(txNewJnc);
		fileAccessor.calculateAngle(angleLimitDR, Jnc_t_limit_Jnc);
	}
	else
	{
		fileAccessor.calculateAngle(angleLimitDR, INT_MAX);
	}

	if (weightAttributesSet.size() > 0)
	{
		isWgt = true;
		getWeightData(infilepath, weightAttributesSet);
	}
}

void Calculation::init_DR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold2, std::string txAngleThreshold,
	std::string txDirectionalChanges, bool ckJnc, bool ckWgt, std::string txNewJnc, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isDR = true;
	isJnc = ckJnc, isWgt = ckWgt;

	//参数
	MRLimitSet2 = getParaSet(txRadiiThreshold2);
	angleLimitDR = getPara(txAngleThreshold);
	DRLimitSet = getParaSet(txDirectionalChanges);

	if (isJnc)
	{
		Jnc_t_limit_Jnc = getPara(txNewJnc);
		fileAccessor.calculateAngle(angleLimitDR, Jnc_t_limit_Jnc);
	}
	else
	{
		fileAccessor.calculateAngle(angleLimitDR, INT_MAX);
	}

	if (weightAttributesSet.size() > 0)
	{
		isWgt = true;
		getWeightData(infilepath, weightAttributesSet);
	}
}

void Calculation::init_JnD_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold,
	std::string txJunctionDegree, bool ckJnDD, std::string txAngleThreshold2, bool ckWgt, std::vector<std::string> &weightAttributesSet) {

	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isJncD = true;
	isJnc = true;
	isJncDDL = ckJnDD;
	isWgt = ckWgt;

	//参数
	MRLimitSet2 = getParaSet(txRadiiThreshold);
	Jnc_t_limit_JncR = getPara(txJunctionDegree);

	if (isJncDDL)
	{
		angleLimitJncDDL = getPara(txAngleThreshold2);
		fileAccessor.calculateAngle(angleLimitJncDDL, Jnc_t_limit_JncR);
	}
	else
	{
		fileAccessor.calculateAngle(INT_MAX, Jnc_t_limit_JncR);
	}

	if (weightAttributesSet.size() > 0)
	{
		isWgt = true;
		getWeightData(infilepath, weightAttributesSet);
	}
}

void Calculation::init_JnR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txJunctionDegree,
	std::string txJunctionsLimit, bool ckJnDD, std::string txAngleThreshold2, bool ckWgt, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isJncR = true;
	isJncDDL = ckJnDD;
	isWgt = ckWgt;

	//参数
	Jnc_t_limit_JncR = getPara(txJunctionDegree);
	Jnc_maxNumSet = getParaSet(txJunctionsLimit);

	if (isJncDDL)
	{
		angleLimitJncDDL = getPara(txAngleThreshold2);
		fileAccessor.calculateAngle(angleLimitJncDDL, Jnc_t_limit_JncR);
	}
	else
	{
		fileAccessor.calculateAngle(INT_MAX, Jnc_t_limit_JncR);
	}

	if (weightAttributesSet.size() > 0)
	{
		isWgt = true;
		getWeightData(infilepath, weightAttributesSet);
	}
}

void Calculation::init_Net_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, bool ckMR, std::string txRadiiThreshold,
	bool ckDR, std::string txRadiiThreshold2, std::string txAngleThreshold, std::string txDirectionalChanges, bool ckJnR,
	std::string txJunctionDegree, std::string txJunctionsLimit, std::string txNewJnc, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isNet = true;
	isMR = ckMR;
	isDR = ckDR;
	isJncR = ckJnR;

	//参数
	Jnc_t_limit_Jnc = getPara(txNewJnc);
	

	if (isMR)
	{
		newMRLimit = getValue(txRadiiThreshold);
		if (Jnc_t_limit_Jnc < INT_MAX - 10)
			fileAccessor.calculateAngle(INT_MAX, Jnc_t_limit_Jnc);
		getWeightData(infilepath, weightAttributesSet);
	}

	if (isDR)
	{
		newMRLimit = getValue(txRadiiThreshold2);
		angleLimitDR = getValue(txAngleThreshold);
		newDRLimit = getValue(txDirectionalChanges);

		fileAccessor.calculateAngle(angleLimitDR, Jnc_t_limit_Jnc);
		getWeightData(infilepath, weightAttributesSet);
	}

	if (isJncR)
	{
		Jnc_t_limit_JncR = getValue(txJunctionDegree);
		newJnc_maxNum = getValue(txJunctionsLimit);

		fileAccessor.calculateAngle(INT_MAX, Jnc_t_limit_JncR);
		getWeightData(infilepath, weightAttributesSet);
	}

}

void Calculation::init_dbf_path(std::string infileStr) {
	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";
}

void Calculation::init_Net_MR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold, std::string txNewJnc, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isNet = true;
	isMR = true;

	//准备数据
	newMRLimit = getValue(txRadiiThreshold);

	//参数
	if (txNewJnc.size() > 0) {
		isJnc = true;
		Jnc_t_limit_Jnc = getPara(txNewJnc);
		fileAccessor.calculateAngle(INT_MAX, Jnc_t_limit_Jnc);
	}
	if(weightAttributesSet.size() > 0) {
		isWgt = true;
		
		getWeightData(infilepath, weightAttributesSet);
	}
}

void Calculation::init_Net_DR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txRadiiThreshold2, std::string txAngleThreshold,
	std::string txDirectionalChanges, std::string txNewJnc, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isNet = true;
	isDR = true;

	//准备数据
	newMRLimit = getValue(txRadiiThreshold2);
	angleLimitDR = getValue(txAngleThreshold);
	newDRLimit = getValue(txDirectionalChanges);

	//参数
	if (txNewJnc.size() > 0) {
		isJnc = true;
		Jnc_t_limit_Jnc = getPara(txNewJnc);
		fileAccessor.calculateAngle(angleLimitDR, Jnc_t_limit_Jnc);
	}
	else {
		fileAccessor.calculateAngle(angleLimitDR, INT_MAX);
	}
	if(weightAttributesSet.size() > 0) {
		isWgt = true;
		
		getWeightData(infilepath, weightAttributesSet);
	}
}

void Calculation::init_Net_JnR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txJunctionDegree,
	std::string txJunctionsLimit, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isNet = true;
	isJncR = true;

	//准备数据
	Jnc_t_limit_JncR = getValue(txJunctionDegree);
	newJnc_maxNum = getValue(txJunctionsLimit);

	//参数
	isJnc = true;
	Jnc_t_limit_Jnc = Jnc_t_limit_JncR;
	fileAccessor.calculateAngle(INT_MAX, Jnc_t_limit_JncR);
	if(weightAttributesSet.size() > 0) {
		isWgt = true;
		getWeightData(infilepath, weightAttributesSet);
	}
}

void Calculation::init_Geo_MR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txAngleThreshold, std::string txNewJnc, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isGeo = true;
	isMR = true;

	//参数
	angleLimitDR = getValue(txAngleThreshold);
	if (angleLimitDR < 0) angleLimitDR = INT_MAX;
	Jnc_t_limit_Jnc = getPara(txNewJnc);
	if (Jnc_t_limit_Jnc < 0) Jnc_t_limit_Jnc = INT_MAX;
	

	//准备数据
	fileAccessor.calculateAngle(angleLimitDR, Jnc_t_limit_Jnc);
	getWeightData(infilepath, weightAttributesSet);
}

void Calculation::init_Geo_DR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txAngleThreshold, std::string txNewJnc, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isGeo = true;
	isDR = true;

	//参数
	angleLimitDR = getValue(txAngleThreshold);
	if (angleLimitDR < 0) angleLimitDR = INT_MAX;
	Jnc_t_limit_Jnc = getPara(txNewJnc);
	if (Jnc_t_limit_Jnc < 0) Jnc_t_limit_Jnc = INT_MAX;
	

	//准备数据
	fileAccessor.calculateAngle(angleLimitDR, Jnc_t_limit_Jnc);
	getWeightData(infilepath, weightAttributesSet);
}

void Calculation::init_Geo_JnR_para(ShapeFileAccessor &fileAccessor, Graph &g, std::string infileStr, std::string txAngleThreshold, std::string txNewJnc, std::vector<std::string> &weightAttributesSet)
{
	////距离无向图
	//m_g.clear();
	//m_g.copy_impl(g);

	//输入输出路径
	infilepath = infileStr;
	outfilepath = infilepath;
	for (int i = int(outfilepath.length()) - 1;; i--)
	{
		if (outfilepath[i] == '/')
			break;
		outfilepath.pop_back();
	}
	dbfFilePath = infileStr;
	int n = int(dbfFilePath.size());
	dbfFilePath.erase(n - 4, 4);
	csvFilePath = dbfFilePath + ".csv"; dbfFilePath = dbfFilePath + ".dbf";

	//开关
	isGeo = true;
	isJncR = true;

	//参数
	angleLimitDR = getValue(txAngleThreshold);
	if (angleLimitDR < 0) angleLimitDR = INT_MAX;
	Jnc_t_limit_Jnc = getPara(txNewJnc);
	if (Jnc_t_limit_Jnc < 0) Jnc_t_limit_Jnc = INT_MAX;
	

	//准备数据
	fileAccessor.calculateAngle(angleLimitDR, Jnc_t_limit_Jnc);
	getWeightData(infilepath, weightAttributesSet);
}


void Calculation::clearNGdata()
{
	//路径数据
	std::map<int, std::set<int>>().swap(reachRoad_all);
	std::map<int, std::map<int, double>>().swap(partInReachRoadLen);
	std::map<int, std::map<int, std::map<int, double>>>().swap(partInReachRoadNodeLen);
	std::map<int, std::set<int>>().swap(routeRoad_all);
	std::map<std::pair <int, int>, double>().swap(routeLen_all);
	std::map<std::pair <int, int>, std::vector<int>>().swap(routeRoad_allMap);
	std::map<int, std::map<int, std::vector<double>>>().swap(partInRoads_all);
	std::map<int, std::map<std::string, double>>().swap(GeoDataCount);

	//Netreach数据
	std::map<std::string, Netreach>().swap(NetreachData);	//有不受限制的三种：MR、DR、JncR
	//Geodesics数据
	std::map<std::string, Geodesics>().swap(GeodesicsData);	//有不收限制的两种：MR、JncR，和收限制的一种：DR
}

void Calculation::getNeedRoads(ShapeFileAccessor &fileAccessor, double limit)
{
	needRadius = limit;
	needRoads.clear();

	if (limit < 0) {
		return;
	}
	else
	{
		//计算mr
		std::map<int, std::set<int>>().swap(reachRoad_all);
		std::map<int, std::map<int, std::vector<double>>>().swap(partInRoads_all);

		//无向图复制
		Graph edgGraph;
		Graph *g = fileAccessor.ProcessShapeFile();
		edgGraph.copy_impl(*g);
		int numEdges = int(boost::num_edges(edgGraph));
		int numVertices = int(boost::num_vertices(edgGraph));

		//准备接数据
		std::vector<vertex_descriptor> parents(numVertices + 1);
		std::vector<double> distances(numVertices + 1);

		//数据结构
		std::set<int> inNode;
		std::set<int> inRoad;
		std::set<int> outRoad;
		std::set<int> lineRoads;
		std::map<int, std::vector<double>> partIn;

		//增加
		EdgeProperty ep1, ep2, ep3;

		for (auto it = FromIDVec.begin(); it != FromIDVec.end(); it++)	//对每条起始边
		{
			int startRoad = *it;

			//修改图：删一条，加两条			
			double dis = fileAccessor.Length[startRoad] / 2;
			int startNode = numVertices;
			int node1 = fileAccessor.roadNode[startRoad][0];
			int node2 = fileAccessor.roadNode[startRoad][1];

			//删除边-起始边
			boost::remove_edge(node1, node2, edgGraph);

			//添加边-中点到起始边两端
			ep1.m_base = dis;
			ep1.m_value = startRoad;
			boost::add_edge(startNode, node1, ep1, edgGraph);

			ep2.m_base = dis;
			ep2.m_value = numEdges;
			boost::add_edge(startNode, node2, ep2, edgGraph);

			boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

			std::set<int>().swap(inNode);
			std::set<int>().swap(inRoad);
			std::set<int>().swap(outRoad);
			std::map<int, std::vector<double>>().swap(partIn);

			double MRLimit = limit;

			//计算inNode：编号0-(顶点数目-1)
			for (int endNode = 0; endNode < numVertices; endNode++)
			{
				if (MRLimit > double(std::round(pow(10, 8)*distances[endNode])) / pow(10, 8))
					inNode.insert(endNode);
			}

			//判断是否能够冲出起始边
			if (2 * MRLimit >= fileAccessor.Length[startRoad])
			{
				//将起始边加入allInLength
				inRoad.insert(startRoad);

				//全覆盖
				for (auto it1 = fileAccessor.roadID.begin(); it1 != fileAccessor.roadID.end(); it1++)
				{
					int endRoad = *it1;
					if (endRoad == startRoad)	//跳过处理起始边
						continue;

					double roadLen = fileAccessor.Length[endRoad];
					int node1 = fileAccessor.roadNode[endRoad][0];		//endrode的端点1
					int node2 = fileAccessor.roadNode[endRoad][1];		//endrode的端点2

					// 如果是完全覆盖，则必须同时到达其两个端点
					if (inNode.count(node1) != 0 && inNode.count(node2) != 0)
					{
						double dist_res1 = MRLimit - distances[node1];  // 端点1方向越过距离
						double dist_res2 = MRLimit - distances[node2];  // 端点2方向越过距离
						double dist_res = dist_res1 + dist_res2;
						if (dist_res >= roadLen)   // 只有确定是完全越过，才会加入allIn
						{
							inRoad.insert(endRoad);
						}
					}
				}

				//部分覆盖
				for (auto it1 = fileAccessor.roadID.begin(); it1 != fileAccessor.roadID.end(); it1++)
				{
					int endRoad = *it1;
					// 跳过全部覆盖的边
					if (inRoad.count(endRoad) != 0)
						continue;

					int node1 = fileAccessor.roadNode[endRoad][0];		//endrode的端点1
					int node2 = fileAccessor.roadNode[endRoad][1];		//endrode的端点2

					// 如果是部分覆盖，只要有一个端点有越过
					if (inNode.count(node1) != 0 || inNode.count(node2) != 0)
					{
						outRoad.insert(endRoad);
					}
				}

				//outRoad=inRoad+partin的路
				outRoad.insert(inRoad.begin(), inRoad.end());
			}
			else
			{
				outRoad.insert(startRoad);
			}

			//收集可以到达的路径序列
			needRoads[startRoad] = outRoad;

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);
		}
	}
}

void Calculation::clearOldData()
{
	//MR开关
	isMR = false;
	isMD = false;
	//DR开关
	isDR = false;
	isDDL = false;
	//JncR开关
	isJncR = false, isJncDDL = false;
	isJncD = false;
	//Option functions开关
	isJnc = false, isWgt = false;
	//Visualization开关
	isNet = false, isGeo = false;

	fromIDSet.clear();

	//MR参数
	std::set<double>().swap(MRLimitSet);
	//DR参数
	std::set<double>().swap(MRLimitSet2);
	angleLimitDR = -1;
	std::set<double>().swap(DRLimitSet);
	//JncR参数
	Jnc_t_limit_JncR = -1;
	std::set<double>().swap(Jnc_maxNumSet);
	angleLimitJncDDL = -1;
	//Option functions参数
	Jnc_t_limit_Jnc = -1;
	std::map<std::string, std::map<int, double>>().swap(weight);
	//Visualization参数
	std::vector<int>().swap(FromIDVec);
	std::vector<int>().swap(ToIDVec);
	std::map<int, std::set<int>>().swap(FromToMap);
	isOneToOne = false;
	outputPath = false;
	//std::map<int, std::set<int>>().swap(reachRoad_all);
	//std::map<int, std::map<int, double>>().swap(partInReachRoadLen);
	//std::map<int, std::map<int, std::map<int, double>>>().swap(partInReachRoadNodeLen);
	//std::map<int, std::set<int>>().swap(routeRoad_all);
	std::map<std::pair <int, int>, double>().swap(routeLen_all);
	std::map<std::pair <int, int>, std::vector<int>>().swap(routeRoad_allMap);
	std::map<int, std::map<int, std::vector<double>>>().swap(partInRoads_all);
	std::map<int, std::map<std::string, double>>().swap(GeoDataCount);
	weight_all = 0;

	//输入排错
	err_beyond = false, err_noData = false;

	//计算输出，准备接数据

	//多线程参数
	order = 1, length = INT_MAX;
	isFinished = false;
	std::vector<int>().swap(subRoadVec);
	std::vector<int>().swap(subFromIDVec);

	//1：里程限制
	std::map<double, std::map<int, double>>().swap(MR_all);
	std::map<double, std::map<int, double>>().swap(meanMD_all);
	//可选
	std::map<double, std::map<int, double>>().swap(JncMR_all);
	std::map<double, std::map<std::string, std::map<int, double>>>().swap(wgtMR_all);

	//2：转向限制
	std::map<std::pair <double, double>, std::map<int, double>>().swap(DR_all);
	std::map<double, std::map<int, double>>().swap(DD_all);
	std::map<double, std::map<int, double>>().swap(DDL_all);
	std::map<double, std::map<std::string, std::map<int, double>>>().swap(WDD_all);
	//可选
	std::map<std::pair <double, double>, std::map<int, double>>().swap(JD_all);
	std::map<std::pair <double, double>, std::map<int, double>>().swap(JncDR_all);
	std::map<std::pair <double, double>, std::map<std::string, std::map<int, double>>>().swap(wgtDR_all);

	//3：交叉口限制
	std::map<double, std::map<int, double>>().swap(JncR_all);
	std::map<double, std::map<int, double>>().swap(JncDD_all);
	std::map<double, std::map<int, double>>().swap(JncDDL_all);
	std::map<double, std::map<std::string, std::map<int, double>>>().swap(JncWDD_all);
	//可选
	std::map<double, std::map<std::string, std::map<int, double>>>().swap(wgtJncR_all);

	////Netreach数据
	//std::map<std::string, Netreach>().swap(NetreachData);	//有不受限制的三种：MR、DR、JncR
	////Geodesics数据
	//std::map<std::string, Geodesics>().swap(GeodesicsData);	//有不收限制的两种：MR、JncR，和收限制的一种：DR

	//增加
	newMRLimit = 0, newDRLimit = 0, newJnc_maxNum = 0;
	r1 = 0, g1 = 0, b1 = 0;
	r2 = 0, g2 = 0, b2 = 0;
	thick1 = 0, thick2 = 0;
	isOutputVisualDataOver = false;

	//Step Depth
	isStepDepth = false;
	SD_MR = false, SD_DR = false, SD_JnR = false;
	stepDepth_Angle = 0;
	stepDepth_JunctionDegree = 0;
	std::map<int, std::set<double>>().swap(distanceAll);

	//进度条
	finishedCount = 0;

	//stepdepth半径限制
	needRadius = -1;
	std::map<int, std::set<int>>().swap(needRoads);

	std::map<std::string, std::map<int, double>>().swap(TempModifyData);

	stepDepthName = "";
}

bool Calculation::getWeightDataByName(std::string infilepath, std::string WgtLimitStr, std::map<int, double> &data) {
	if (WgtLimitStr.length() == 0)
		return false;

	data.clear();
	
	std::string dbfFile = infilepath;
	int n = int(dbfFile.size());
	dbfFile.erase(n - 4, 4);
	dbfFile = dbfFile + ".dbf";

	DBFHandle	hDBF;
	hDBF = DBFOpen(dbfFile.c_str(), "rb+");

	//查找该列所在索引是否存在
	int fieldIndex = DBFGetFieldIndex(hDBF, WgtLimitStr.c_str());
	if (fieldIndex == -1)	//字段不存在，返回false
	{
		DBFClose(hDBF);
		return false;
	}
	else
	{
		DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, fieldIndex, NULL, NULL, NULL);

		int record = DBFGetRecordCount(hDBF);
		double value = 0;
		if (fieldtype == FTInteger)
		{
			for (int i = 0; i < record; i++)
			{
				value = DBFReadIntegerAttribute(hDBF, i, fieldIndex);
				data[i] = value;
			}

			DBFClose(hDBF);
			return true;
		}
		else
		{
			if (fieldtype == FTDouble)
			{
				for (int i = 0; i < record; i++)
				{
					value = DBFReadDoubleAttribute(hDBF, i, fieldIndex);
					data[i] = value;
				}

				DBFClose(hDBF);
				return true;
			}
			else
			{
				DBFClose(hDBF);
				return false;
			}
		}
	}

	return true;
}

bool Calculation::getWeightData(std::string infilepath, std::vector<std::string> &weightAttributesSet) {
	weight.clear();

	if (weightAttributesSet.size() == 0)
		return false;

	std::string dbfFile = infilepath;
	int n = int(dbfFile.size());
	dbfFile.erase(n - 4, 4);
	dbfFile = dbfFile + ".dbf";

	DBFHandle	hDBF;
	hDBF = DBFOpen(dbfFile.c_str(), "rb+");

	for (std::string WgtLimitStr : weightAttributesSet) {
		//查找该列所在索引是否存在
		int fieldIndex = DBFGetFieldIndex(hDBF, WgtLimitStr.c_str());
		if (fieldIndex == -1)	//字段不存在，返回false
			continue;
		else{
			DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, fieldIndex, NULL, NULL, NULL);

			int record = DBFGetRecordCount(hDBF);
			double value = 0;
			if (fieldtype == FTInteger){
				for (int i = 0; i < record; i++){
					value = DBFReadIntegerAttribute(hDBF, i, fieldIndex);
					weight[WgtLimitStr].insert(std::make_pair(i, value));
				}
			}
			else{
				if (fieldtype == FTDouble){
					for (int i = 0; i < record; i++){
						value = DBFReadDoubleAttribute(hDBF, i, fieldIndex);
						weight[WgtLimitStr].insert(std::make_pair(i, value));
					}
				}
				else
					continue;
			}
		}
	}

	DBFClose(hDBF);
	return true;
}

//bool Calculation::getWeightData(std::string infilepath, std::string WgtLimitStr)
//{
//	weight.clear();
//	if (WgtLimitStr.length() == 0)
//		return false;
//
//	weight_all = 0;
//	std::string dbfFile = infilepath;
//	int n = int(dbfFile.size());
//	dbfFile.erase(n - 4, 4);
//	dbfFile = dbfFile + ".dbf";
//
//	DBFHandle	hDBF;
//	hDBF = DBFOpen(dbfFile.c_str(), "rb+");
//
//	//查找该列所在索引是否存在
//	int fieldIndex = DBFGetFieldIndex(hDBF, WgtLimitStr.c_str());
//	if (fieldIndex == -1)	//字段不存在，返回false
//	{
//		DBFClose(hDBF);
//		return false;
//	}
//	else
//	{
//		DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, fieldIndex, NULL, NULL, NULL);
//
//		int record = DBFGetRecordCount(hDBF);
//		double value = 0;
//		if (fieldtype == FTInteger)
//		{
//			for (int i = 0; i < record; i++)
//			{
//				value = DBFReadIntegerAttribute(hDBF, i, fieldIndex);
//				weight.insert(std::make_pair(i, value));
//				weight_all += value;
//			}
//
//			DBFClose(hDBF);
//			return true;
//		}
//		else
//		{
//			if (fieldtype == FTDouble)
//			{
//				for (int i = 0; i < record; i++)
//				{
//					value = DBFReadDoubleAttribute(hDBF, i, fieldIndex);
//					weight.insert(std::make_pair(i, value));
//					weight_all += value;
//				}
//
//				DBFClose(hDBF);
//				return true;
//			}
//			else
//			{
//				DBFClose(hDBF);
//				return false;
//			}
//		}
//	}
//
//	return true;
//}

void Calculation::setMultiPara_modify(ShapeFileAccessor &fileAccessor, int lastend, int ord, int len, int whichone)
{
	this->order = ord;
	this->length = len;

	//subRoadVec.clear();
	//subFromIDVec.clear();
	std::vector<int>().swap(subRoadVec);
	std::vector<int>().swap(subFromIDVec);

	//多线程调用起效
	int subCount = 0;
	if (whichone == 1)
	{
		for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++)
		{
			if (subCount >= lastend + length * (order - 1) && subCount < lastend + length * order)
				subRoadVec.push_back(*it);

			++subCount;
		}
	}

	if (whichone == 2)
	{
		for (auto it = FromIDVec.begin(); it != FromIDVec.end(); it++)
		{
			if (subCount >= lastend + length * (order - 1) && subCount < lastend + length * order)
				subFromIDVec.push_back(*it);

			++subCount;
		}
	}
}

void Calculation::setMultiParaByRand(std::set<int> start_roads, int whichone) {
	std::vector<int>().swap(subRoadVec);
	std::vector<int>().swap(subFromIDVec);

	if (whichone == 1){
		subRoadVec.insert(subRoadVec.begin(), start_roads.begin(), start_roads.end());
	}
	else if (whichone == 2){
		subFromIDVec.insert(subFromIDVec.begin(), start_roads.begin(), start_roads.end());
	}
}

void Calculation::setMultiPara(ShapeFileAccessor &fileAccessor, int ord, int len, int whichone)
{
	this->order = ord;
	this->length = len;

	//subRoadVec.clear();
	//subFromIDVec.clear();
	std::vector<int>().swap(subRoadVec);
	std::vector<int>().swap(subFromIDVec);

	//多线程调用起效
	int subCount = 0;
	if (whichone == 1)
	{
		for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++)
		{
			if (subCount >= length * (order - 1) && subCount < length*order)
				subRoadVec.push_back(*it);

			++subCount;
		}
	}

	if (whichone == 2)
	{
		if (this->isNet) {
			for (auto it = FromIDVec.begin(); it != FromIDVec.end(); it++)
			{
				if (subCount >= length * (order - 1) && subCount < length*order)
					subFromIDVec.push_back(*it);

				++subCount;
			}
		}
		else if (this->isGeo) {
			for (auto it = fromIDSet.begin(); it != fromIDSet.end(); it++)
			{
				if (subCount >= length * (order - 1) && subCount < length*order)
					subFromIDVec.push_back(*it);

				++subCount;
			}
		}
	}
}

void Calculation::MultiCalculate(ShapeFileAccessor &fileAccessor,long long pos)
{
		//设置用哪个CPU核心处理该线程			
	SetThreadAffinityMask(GetCurrentThread(), pos);
	// std::string threadIdString = "子线程id:" + std::to_string(GetCurrentThreadId()) + "  ";
	// OutputDebugString(threadIdString.c_str());
	// std::string output = "位于核心：" + std::to_string(GetCurrentProcessorNumber()) + "\n";
	// OutputDebugString(output.c_str());

	if (isMD)
		calculateMD(fileAccessor);
	else if (isMR)
		calculateMR(fileAccessor);
	else if (isDR) {
		if (isDDL)
			calculateDDLbyDij(fileAccessor);
			//calculateDDL(fileAccessor);
		else if (isMDR)
			calculateMDR(fileAccessor);
		else
			calculateDR(fileAccessor);
	}
	else if (isJncR)
		calculateJncR(fileAccessor);
	else if(isJncD)
		calculateJncD(fileAccessor);
}

void Calculation::MultiVisualize(ShapeFileAccessor &fileAccessor, long long pos)
{
	SetThreadAffinityMask(GetCurrentThread(), pos);
	//Step Depth计算
	if (isStepDepth)
	{
		//计算所有起点
		if (SD_MR)
		{
			newMRLimit = -1;
			Net_calculateMR(fileAccessor, newMRLimit);
			if (stepDepthName=="" && !subFromIDVec.empty()) {
				stepDepthName = "PD" + stepDepthStartRoad;
			}
		}
		if (SD_DR)
		{
			newDRLimit = -1, newMRLimit = -1;
			Net_calculateDR(fileAccessor, newDRLimit, newMRLimit);
			if (stepDepthName == "" && !subFromIDVec.empty()) {
				stepDepthName = "PD" + std::to_string((int)angleLimitDR) + "a" + stepDepthStartRoad;
			}
			
		}
		if (SD_JnR)
		{
			newJnc_maxNum = -1;
			Net_calculateJncR(fileAccessor, newJnc_maxNum);
			if (stepDepthName == "" && !subFromIDVec.empty()) {
				stepDepthName = "PD" + std::to_string((int)Jnc_t_limit_JncR) + "x" + stepDepthStartRoad;
			}
			
		}
		return;
	}

	//Net计算――――需要输入辅助参数
	if (isNet && isMR)
	{
		NGlimitStr = "MR";
		Net_calculateMR(fileAccessor, newMRLimit);
	}
	if (isNet && isDR)
	{
		NGlimitStr = "MDR";
		Net_calculateDR(fileAccessor, newDRLimit, newMRLimit);
	}
	if (isNet && isJncR)
	{
		NGlimitStr = "JncR";
		Net_calculateJncR(fileAccessor, newJnc_maxNum);
	}

	//Geo计算
	if (isGeo && isMR)
	{
		NGlimitStr = "MR";
		Geo_calculateMR(fileAccessor);
	}
	if (isGeo && isDR)
	{
		NGlimitStr = "DR";
		Geo_calculateDR(fileAccessor);
	}
	if (isGeo && isJncR)
	{
		NGlimitStr = "JncR";
		Geo_calculateJncR(fileAccessor);
	}
}

void Calculation::calculateMR_subset(Calculation *temp, ShapeFileAccessor &fileAccessor) {
	setFinished(false);

	//无向图复制
	Graph edgGraph;
	Graph *g = fileAccessor.ProcessShapeFile();
	edgGraph.copy_impl(*g);
	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);
	EdgeProperty ep1, ep2, ep3;
	std::vector<double> mr_distance;

	for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++)	//对每条起始边
	{
		extern bool NeedStop;
		if (NeedStop) {
			return;
		}

		int startRoad = *it;
		if (startRoad == 0)
			int a = startRoad + 1;

		//修改图：删一条，加两条			
		double dis = fileAccessor.Length[startRoad] / 2;
		int startNode = numVertices;
		int node1 = fileAccessor.roadNode[startRoad][0];
		int node2 = fileAccessor.roadNode[startRoad][1];

		//删除边-起始边
		boost::remove_edge(node1, node2, edgGraph);

		//添加边-中点到起始边两端
		ep1.m_base = dis;
		ep1.m_value = startRoad;
		boost::add_edge(startNode, node1, ep1, edgGraph);

		ep2.m_base = dis;
		ep2.m_value = numEdges;
		boost::add_edge(startNode, node2, ep2, edgGraph);

		boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

		//计算到每条终点线的距离
		mr_distance.clear();
		for (auto endRoad_it = fileAccessor.roadID.begin(); endRoad_it != fileAccessor.roadID.end(); endRoad_it++) {
			int endRoad = *endRoad_it;

			int endnode = 0;
			int endnode1 = fileAccessor.roadNode[endRoad][0];
			int endnode2 = fileAccessor.roadNode[endRoad][1];

			//获取短的分支			
			if (distances[endnode1] < distances[endnode2])
				endnode = endnode1;
			else
				endnode = endnode2;

			double len = distances[endnode] + 0.5 * fileAccessor.Length[endRoad];
			if (endRoad == startRoad)
				len = 0;
			mr_distance.push_back(len);
		}

		//计算平均mr
		temp->subset_3_mr_all[startRoad] = std::accumulate(mr_distance.begin(), mr_distance.end(), 0) / double(fileAccessor.roadID.size());

		//把图撤销修改：删两条，加一条
		boost::remove_edge(startNode, node1, edgGraph);
		boost::remove_edge(startNode, node2, edgGraph);

		ep3.m_base = fileAccessor.Length[startRoad];
		ep3.m_value = startRoad;
		boost::add_edge(node1, node2, ep3, edgGraph);

		addFinishedCount();
	}

	setFinished(true);
	return;
}

void Calculation::calculateDR_subset(Calculation *temp, ShapeFileAccessor &fileAccessor) {
	setFinished(false);

	//创建有向图		
	Graph_d edgGraph_DR;
	GenerateDirectedGraph(fileAccessor, edgGraph_DR);
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;
	std::map<int, std::set<int>> ddMap;

	extern bool NeedStop;
	for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;
		int startRoad2 = startRoad + old_numEdges;

		//两次有向图搜索
		boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
		boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

		dict_min_dist.clear();
		for (int i = 0; i < numVertices_DR; i++)
			dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

		//收集DR有效的道路队列
		dict_real_min_dist.clear();
		for (int k = 0; k < old_numEdges; k++) {
			//找出有向边i对应的反向边id
			int reverse_k = k + old_numEdges;
			double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
			dict_real_min_dist.insert(std::make_pair(k, min_d));
		}

		//计算dd、ddl
		ddMap.clear();
		double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
		for (auto it2 = fileAccessor.roadID.begin(); it2 != fileAccessor.roadID.end(); it2++) {
			ddMap[int(dict_real_min_dist[*it2])].insert(*it2);
			M_ddlSum += dict_real_min_dist[*it2] * fileAccessor.Length[*it2];
			M_lenSum += fileAccessor.Length[*it2];
		}
		double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
		for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++) {
			M_ddSum1 += ((it3->first)*(it3->second.size()));
			M_ddSum2 += it3->second.size();
		}
		M_dd = M_ddSum1 / M_ddSum2;
		M_ddl = M_ddlSum / M_lenSum;

		//存入数据
		temp->subset_3_dd_all[startRoad] = M_dd;
		temp->subset_3_ddl_all[startRoad] = M_ddl;

		addFinishedCount();
	}

	setFinished(true);
	return;
}

void Calculation::calculateJncR_subset(Calculation *temp, ShapeFileAccessor &fileAccessor) {
	setFinished(false);

	//创建交叉口的新图
	Graph edgGraph3;
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	GenerateUndirectedGraph_Jnc(fileAccessor, old_numEdges, edgGraph3);
	int numVertices3 = int(boost::num_vertices(edgGraph3));
	int numEdges3 = int(boost::num_edges(edgGraph3));

	std::vector<vertex_descriptor> parents3(numVertices3);
	std::vector<double> distances3(numVertices3);

	extern bool NeedStop;
	for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;

		boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

		//计算平均jnc
		temp->subset_3_jnc_all[startRoad] = std::accumulate(distances3.begin(), distances3.end(), 0) / double(fileAccessor.roadID.size());

		addFinishedCount();
	}

	setFinished(true);
	return;
}

void Calculation::calculateMD(ShapeFileAccessor &fileAccessor) {
	isFinished = false;

	if (subRoadVec.size() == 0) {
		isFinished = true;
		return;
	}

	int total = fileAccessor.roadID.size();

	std::map<int, partInNode> partInLength;
	std::set<int> outRoad;

	//std::ofstream tout("C:\\Users\\lin\\Desktop\\BFS.csv");
	for (auto cur_iter = subRoadVec.begin(); cur_iter != subRoadVec.end(); cur_iter++)	//对每条起始边
	{
		extern bool NeedStop;
		if (NeedStop) {
			return;
		}

		int startRoad = *cur_iter;

		//对所有MRLimit参数组一轮算完
		for (auto iter = MRLimitSet.begin(); iter != MRLimitSet.end(); iter++)
		{
			double MRLimit = *iter;

			if (MRLimit == -1)
			{
				std::vector<double> dist(total, 0x7fffffff);
				getMRDistBFS(fileAccessor, startRoad, -1, dist);

				//全部线段都能通行，所有道路的mr是一致的，但是meanMD是不同的
				double mr = 0, sumdLs = 0;
				for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++) {
					mr += fileAccessor.Length[*it];
					if (*it != startRoad) {
						//计算ds
						double ds = dist[*it] - 0.5*fileAccessor.Length[*it];
						sumdLs += fileAccessor.Length[*it] * ds;
					}
				}

				double Ls0 = pow(fileAccessor.Length[startRoad], 2) / 4;
				double sumLs = mr, meanmd = 0;
				if (sumLs > 0)
					meanmd = (Ls0 + sumdLs) / sumLs;

				//存储数据
				MR_all[MRLimit][startRoad] = mr;
				meanMD_all[MRLimit][startRoad] = meanmd;
			}
			else
			{
				//准备数据结构
				std::queue<int> q;
				std::vector<double> dist(total, 0x7fffffff);
				std::vector<bool> visited(total, false);
				partInLength.clear();
				outRoad.clear();

				//初始化数据
				q.push(startRoad);
				double rootseglength = fileAccessor.Length[startRoad];
				dist[startRoad] = rootseglength * 0.5;

				//BFS计算到连接线段的最短距离
				while (!q.empty()) {
					int nowRoad = q.front();
					q.pop();

					if (visited[nowRoad])
						continue;
					visited[nowRoad] = true;

					for (auto next_iter = fileAccessor.Road_to_roads[nowRoad].begin(); next_iter != fileAccessor.Road_to_roads[nowRoad].end(); next_iter++) {
						double length = fileAccessor.Length[*next_iter];
						int connected_cursor = *next_iter;
						if (connected_cursor == startRoad)
							continue;

						//如果更新了已经reach的线条，需要重新遍历
						if (dist[nowRoad] + length < dist[connected_cursor]) {
							visited[connected_cursor] = false;
							dist[connected_cursor] = dist[nowRoad] + length;
						}
						if (dist[connected_cursor] <= MRLimit)
							q.push(connected_cursor);
					}
				}

				//BFS收集全覆盖
				std::queue<int> qq;
				std::vector<bool> visited2(total, false);
				qq.push(startRoad);
				double mr = 0, Ls0 = 0, sumLs = 0, sumdLs = 0;
				//判断能否冲出起点线条
				if (0.5*rootseglength >= MRLimit) {
					mr = 2 * MRLimit;
					Ls0 += pow(2 * MRLimit, 2) / 4;
					sumLs += 2 * MRLimit;
					outRoad.insert(startRoad);
				}
				else {
					while (!qq.empty()) {
						int nowRoad = qq.front();
						qq.pop();

						if (visited2[nowRoad])
							continue;

						double len = fileAccessor.Length[nowRoad];
						mr += len;
						outRoad.insert(nowRoad);
						visited2[nowRoad] = true;

						if (nowRoad == startRoad)
						{
							//计算Ls0,sumLs
							Ls0 += pow(fileAccessor.Length[startRoad], 2) / 4;
							sumLs += fileAccessor.Length[startRoad];
						}
						else {
							//计算sumLs,sumdLs
							sumLs += fileAccessor.Length[nowRoad];
							sumdLs += fileAccessor.Length[nowRoad] * (dist[nowRoad] - 0.5*fileAccessor.Length[nowRoad]);
						}

						for (auto next_iter = fileAccessor.Road_to_roads[nowRoad].begin(); next_iter != fileAccessor.Road_to_roads[nowRoad].end(); next_iter++) {
							double length = fileAccessor.Length[*next_iter];
							int connected_cursor = *next_iter;
							if (connected_cursor == startRoad)
								continue;

							if (dist[connected_cursor] <= MRLimit)	//全覆盖
								qq.push(connected_cursor);
							else if (dist[nowRoad]< MRLimit && dist[connected_cursor] > MRLimit) {	//部分覆盖
								//找出覆盖节点方向
								int pre_node1 = fileAccessor.roadNode[nowRoad][0];
								int pre_node2 = fileAccessor.roadNode[nowRoad][1];
								int node1 = fileAccessor.roadNode[connected_cursor][0];
								int node2 = fileAccessor.roadNode[connected_cursor][1];
								int same_node = node1;
								if (pre_node1 == node2 || pre_node2 == node2)
									same_node = node2;
								//更新该方向覆盖距离
								if (same_node == node1) {
									if (MRLimit - dist[nowRoad] > partInLength[connected_cursor].leftLength) {
										partInLength[connected_cursor].leftPreRoad = nowRoad;
										partInLength[connected_cursor].leftLength = MRLimit - dist[nowRoad];
									}
								}
								else {
									if (MRLimit - dist[nowRoad] > partInLength[connected_cursor].rightLength) {
										partInLength[connected_cursor].rightPreRoad = nowRoad;
										partInLength[connected_cursor].rightLength = MRLimit - dist[nowRoad];
									}
								}
							}
						}
					}
				}

				//处理部分覆盖
				for (auto next_iter = partInLength.begin(); next_iter != partInLength.end(); next_iter++) {
					int road = next_iter->first;
					double len = min(next_iter->second.leftLength + next_iter->second.rightLength, fileAccessor.Length[road]);
					mr += len;
					outRoad.insert(road);
					//计算sumLs,sumdLs
					if (next_iter->second.leftLength + next_iter->second.rightLength < fileAccessor.Length[road]) {
						if (next_iter->second.leftLength > 0) {
							sumLs += next_iter->second.leftLength;
							sumdLs += next_iter->second.leftLength * (dist[next_iter->second.leftPreRoad] + 0.5*next_iter->second.leftLength);
						}
						if (next_iter->second.rightLength > 0) {
							sumLs += next_iter->second.rightLength;
							sumdLs += next_iter->second.rightLength * (dist[next_iter->second.rightPreRoad] + 0.5*next_iter->second.rightLength);
						}
					}
					else {
						double ds = dist[road] - 0.5*fileAccessor.Length[road];
						sumLs += fileAccessor.Length[road];
						sumdLs += fileAccessor.Length[road] * ds;
					}
				}

				double meanmd = 0;
				if (sumLs > 0)
					meanmd = (Ls0 + sumdLs) / sumLs;

				MR_all[MRLimit][startRoad] = mr;
				meanMD_all[MRLimit][startRoad] = meanmd;
			}

			//计算wgt
			if (isWgt)
			{
				std::map<std::string, double> wgtvalue = CalculateWgt(weight, outRoad);
				for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
					wgtMR_all[MRLimit][wgt_it->first][startRoad] = wgt_it->second;
				}
			}
		}

		finishedCount += 1;
	}

	isFinished = true;
}

//计算所有的MR、meanMD
void Calculation::calculateMR(ShapeFileAccessor &fileAccessor)
{
	calculateMRbyDij(fileAccessor);
	return;
}

//计算所有的MR、meanMD
void Calculation::calculateMRbyDij(ShapeFileAccessor& fileAccessor) {
	isFinished = false;
	if (subRoadVec.size() == 0) {
		isFinished = true;
		return;
	}
	int total_road = fileAccessor.roadID.size();

	std::map<int, partInNode> partInLength;
	std::set<int> outRoad;


	std::vector<double> dist(total_road, 0x7fffffff);
	MetricDistanceFunction<double> distFunc(fileAccessor);

	//收集数据用
	std::queue<int> qq;
	std::vector<bool> visited2(total_road, false);
	std::queue<int> q;
	std::vector<bool> visited(total_road, false);

	//计算最大的MRLimit
	double max_MRLimit = 0;
	for (auto i : MRLimitSet) {
		if (i > max_MRLimit) {
			max_MRLimit = i;
		}
		if (i == -1) {
			max_MRLimit = -1;
			break;
		}
	}

	for (auto cur_iter = subRoadVec.begin(); cur_iter != subRoadVec.end(); cur_iter++)	//对每条起始边
	{
		extern bool NeedStop;
		if (NeedStop) {
			return;
		}
		int startRoad = *cur_iter;
		getDistDijkstra(distFunc, startRoad, max_MRLimit, dist);
		partInLength.clear();
		outRoad.clear();
		std::set<int> validRoad; //记录每个MRLimit参数下特有的路。
		for (auto iter = MRLimitSet.begin(); iter != MRLimitSet.end(); iter++)
		{
			validRoad.clear();
			double MRLimit = *iter;
			double mr = 0, Ls0 = 0, sumLs = 0, sumdLs = 0;
			if (MRLimit == -1)
			{
				//全部线段都能通行，所有道路的mr是一致的，但是meanMD是不同的
				double mr = 0, sumdLs = 0;
				for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++) {
					mr += fileAccessor.Length[*it];
					if (*it != startRoad) {
						//计算ds
						double ds = dist[*it] - 0.5 * fileAccessor.Length[*it];
						sumdLs += fileAccessor.Length[*it] * ds;
					}
				}

				double Ls0 = pow(fileAccessor.Length[startRoad], 2) / 4;
				double sumLs = mr, meanmd = 0;
				if (sumLs > 0)
					meanmd = (Ls0 + sumdLs) / sumLs;

				//存储数据
				MR_all[MRLimit][startRoad] = mr;
				meanMD_all[MRLimit][startRoad] = meanmd;
			}
			else
			{
				double rootseglength = fileAccessor.Length[startRoad];
				if (0.5 * rootseglength >= MRLimit) { //不能冲出
					mr = 2 * MRLimit;
					Ls0 += pow(2 * MRLimit, 2) / 4; //线段的平均贡献距离?
					sumLs += 2 * MRLimit;
					outRoad.insert(startRoad);
				}
				else { //冲出
					qq.push(startRoad);
					while (!qq.empty()) {
						int nowRoad = qq.front();
						qq.pop();

						if (visited2[nowRoad])
							continue;

						double len = fileAccessor.Length[nowRoad];
						mr += len;
						outRoad.insert(nowRoad);
						visited2[nowRoad] = true;

						if (nowRoad == startRoad)
						{
							//计算Ls0,sumLs
							Ls0 += pow(fileAccessor.Length[startRoad], 2) / 4;
							sumLs += fileAccessor.Length[startRoad];
						}
						else {
							//计算sumLs,sumdLs
							sumLs += fileAccessor.Length[nowRoad];
							sumdLs += fileAccessor.Length[nowRoad] * (dist[nowRoad] - 0.5 * fileAccessor.Length[nowRoad]); //累计离这个路的距离*这个路的长度
						}

						for (auto next_iter = fileAccessor.Road_to_roads[nowRoad].begin(); next_iter != fileAccessor.Road_to_roads[nowRoad].end(); next_iter++) {
							double length = fileAccessor.Length[*next_iter];
							int connected_cursor = *next_iter;
							if (connected_cursor == startRoad)
								continue;

							if (dist[connected_cursor] <= MRLimit)	//全覆盖
								qq.push(connected_cursor);
							else if (dist[nowRoad]< MRLimit && dist[connected_cursor] > MRLimit) {	//部分覆盖
								//找出覆盖节点方向
								int pre_node1 = fileAccessor.roadNode[nowRoad][0];
								int pre_node2 = fileAccessor.roadNode[nowRoad][1];
								int node1 = fileAccessor.roadNode[connected_cursor][0];
								int node2 = fileAccessor.roadNode[connected_cursor][1];
								int same_node = node1;
								if (pre_node1 == node2 || pre_node2 == node2)
									same_node = node2;
								//更新该方向覆盖距离
								if (same_node == node1) {
									if (MRLimit - dist[nowRoad] > partInLength[connected_cursor].leftLength) {
										partInLength[connected_cursor].leftPreRoad = nowRoad;
										partInLength[connected_cursor].leftLength = MRLimit - dist[nowRoad];
									}
								}
								else {
									if (MRLimit - dist[nowRoad] > partInLength[connected_cursor].rightLength) {
										partInLength[connected_cursor].rightPreRoad = nowRoad;
										partInLength[connected_cursor].rightLength = MRLimit - dist[nowRoad];
									}
								}
							}
						}
					}
					//处理部分覆盖
					for (auto next_iter = partInLength.begin(); next_iter != partInLength.end(); next_iter++) {
						int road = next_iter->first;
						double len = min(next_iter->second.leftLength + next_iter->second.rightLength, fileAccessor.Length[road]);
						mr += len;
						outRoad.insert(road);
						//计算sumLs,sumdLs
						if (next_iter->second.leftLength + next_iter->second.rightLength < fileAccessor.Length[road]) {
							if (next_iter->second.leftLength > 0) {
								sumLs += next_iter->second.leftLength;
								sumdLs += next_iter->second.leftLength * (dist[next_iter->second.leftPreRoad] + 0.5 * next_iter->second.leftLength);
							}
							if (next_iter->second.rightLength > 0) {
								sumLs += next_iter->second.rightLength;
								sumdLs += next_iter->second.rightLength * (dist[next_iter->second.rightPreRoad] + 0.5 * next_iter->second.rightLength);
							}
						}
						else {
							double ds = dist[road] - 0.5 * fileAccessor.Length[road];
							sumLs += fileAccessor.Length[road];
							sumdLs += fileAccessor.Length[road] * ds;
						}
					}
				}
				double meanmd = 0;
				if (sumLs > 0)
					meanmd = (Ls0 + sumdLs) / sumLs;
				MR_all[MRLimit][startRoad] = mr;
				meanMD_all[MRLimit][startRoad] = meanmd;
			}

			//计算Jnc
			if (isJnc)
			{
				int jncMR = CalculateJnc(fileAccessor, outRoad);
				//tout << startRoad << "," << outRoad.size() << "," << jncMR << std::endl;
				JncMR_all[MRLimit][startRoad] = jncMR;
			}

			//计算wgt
			if (isWgt)
			{
				std::map<std::string, double> wgtvalue = CalculateWgt(weight, outRoad);
				for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
					wgtMR_all[MRLimit][wgt_it->first][startRoad] = wgt_it->second;
				}
			}
			//计算完一个MRLimit
			std::fill(visited.begin(), visited.end(), false);
			std::fill(visited2.begin(), visited2.end(), false);
		}
		//清空
		std::fill(dist.begin(), dist.end(), 0x7fffffff);
		finishedCount += 1;
	}

		isFinished = true;
}

void Calculation::Net_calculateMR(ShapeFileAccessor &fileAccessor, double subMRLimit)
{
	isFinished = false;

	if (subFromIDVec.size() == 0) {
		isFinished = true;
		return;
	}

	//清空数据
	//reachRoad_all.clear();
	//partInRoads_all.clear();

	//Netreach数据
	std::map<std::string, Netreach>().swap(NetreachData);	//有不受限制的三种：MR、DR、JncR
	net_file_name = getOutFilePath();
	net_str = "MR";

	std::map<int, std::set<int>>().swap(reachRoad_all);
	std::map<int, std::map<int, double>>().swap(partInReachRoadLen);
	std::map<int, std::map<int, std::map<int, double>>>().swap(partInReachRoadNodeLen);
	std::map<int, std::map<int, std::vector<double>>>().swap(partInRoads_all);

	//无向图复制
	Graph edgGraph;
	Graph *g = fileAccessor.ProcessShapeFile();
	edgGraph.copy_impl(*g);
	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);

	//数据结构
	std::set<int> inNode;
	std::set<int> inRoad;
	std::set<int> outRoad;
	std::set<int> lineRoads;
	std::map<int, std::vector<double>> partIn;

	//增加
	EdgeProperty ep1, ep2, ep3;

	extern bool NeedStop;
	for (auto it = subFromIDVec.begin(); it != subFromIDVec.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;

		//修改图：删一条，加两条			
		double dis = fileAccessor.Length[startRoad] / 2;
		int startNode = numVertices;
		int node1 = fileAccessor.roadNode[startRoad][0];
		int node2 = fileAccessor.roadNode[startRoad][1];

		//删除边-起始边
		boost::remove_edge(node1, node2, edgGraph);

		//添加边-中点到起始边两端
		ep1.m_base = dis;
		ep1.m_value = startRoad;
		boost::add_edge(startNode, node1, ep1, edgGraph);

		ep2.m_base = dis;
		ep2.m_value = numEdges;
		boost::add_edge(startNode, node2, ep2, edgGraph);

		boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

		if (isStepDepth)
		{
			for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++) {
				int endRoad = *it;

				int endnode = 0;
				int endnode1 = fileAccessor.roadNode[endRoad][0];
				int endnode2 = fileAccessor.roadNode[endRoad][1];

				//获取短的分支			
				if (distances[endnode1] < distances[endnode2])
					endnode = endnode1;
				else
					endnode = endnode2;

				double len = distances[endnode] + 0.5 * fileAccessor.Length[endRoad];
				if (endRoad == startRoad)
					len = 0;

				distanceAll[endRoad].insert(len);
			}

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);

			finishedCount += 1;
			continue;
		}

		//数据结构
		//inNode.clear();
		//inRoad.clear();		//全覆盖的边
		//outRoad.clear();	//所有通行的边
		//partIn.clear();

		std::set<int>().swap(inNode);
		std::set<int>().swap(inRoad);
		std::set<int>().swap(outRoad);
		std::map<int, std::vector<double>>().swap(partIn);

		double MRLimit = subMRLimit;

		//计算inNode：编号0-(顶点数目-1)
		for (int endNode = 0; endNode < numVertices; endNode++)
		{
			if (MRLimit > double(std::round(pow(10, 8)*distances[endNode])) / pow(10, 8))
				inNode.insert(endNode);
		}

		//判断是否能够冲出起始边
		if (2 * MRLimit >= fileAccessor.Length[startRoad])
		{
			//将起始边加入allInLength
			inRoad.insert(startRoad);

			//全覆盖
			for (auto it1 = fileAccessor.roadID.begin(); it1 != fileAccessor.roadID.end(); it1++)
			{
				int endRoad = *it1;
				if (endRoad == startRoad)	//跳过处理起始边
					continue;

				double roadLen = fileAccessor.Length[endRoad];
				int node1 = fileAccessor.roadNode[endRoad][0];		//endrode的端点1
				int node2 = fileAccessor.roadNode[endRoad][1];		//endrode的端点2

				// 如果是完全覆盖，则必须同时到达其两个端点
				if (inNode.count(node1) != 0 && inNode.count(node2) != 0)
				{
					double dist_res1 = MRLimit - distances[node1];  // 端点1方向越过距离
					double dist_res2 = MRLimit - distances[node2];  // 端点2方向越过距离
					double dist_res = dist_res1 + dist_res2;
					if (dist_res >= roadLen)   // 只有确定是完全越过，才会加入allIn
					{
						inRoad.insert(endRoad);
					}
				}
			}

			//部分覆盖
			for (auto it1 = fileAccessor.roadID.begin(); it1 != fileAccessor.roadID.end(); it1++)
			{
				int endRoad = *it1;
				// 跳过全部覆盖的边
				if (inRoad.count(endRoad) != 0)
					continue;

				int node1 = fileAccessor.roadNode[endRoad][0];		//endrode的端点1
				int node2 = fileAccessor.roadNode[endRoad][1];		//endrode的端点2

				// 如果是部分覆盖，只要有一个端点有越过
				if (inNode.count(node1) != 0 || inNode.count(node2) != 0)
				{
					// 检查是否已到达边缘的起点
					if (inNode.count(node1) != 0)
					{
						double dis_overflow = MRLimit - distances[node1];
						partIn[endRoad].push_back(dis_overflow);
						partInReachRoadNodeLen[startRoad][endRoad][node1] = dis_overflow;

						//计算坐标
						double x1 = fileAccessor.Route[endRoad][0];
						double y1 = fileAccessor.Route[endRoad][1];
						double x2 = fileAccessor.Route[endRoad][2];
						double y2 = fileAccessor.Route[endRoad][3];
						double rate = dis_overflow / fileAccessor.Length[endRoad];

						double x = x1 + (x2 - x1)*rate;
						double y = y1 + (y2 - y1)*rate;

						partInRoads_all[startRoad][endRoad].push_back(x1);
						partInRoads_all[startRoad][endRoad].push_back(y1);
						partInRoads_all[startRoad][endRoad].push_back(x);
						partInRoads_all[startRoad][endRoad].push_back(y);
					}
					if (inNode.count(node2) != 0)
					{
						double dis_overflow = MRLimit - distances[node2];
						partIn[endRoad].push_back(dis_overflow);
						partInReachRoadNodeLen[startRoad][endRoad][node2] = dis_overflow;

						//计算坐标
						double x2 = fileAccessor.Route[endRoad][0];
						double y2 = fileAccessor.Route[endRoad][1];
						double x1 = fileAccessor.Route[endRoad][2];
						double y1 = fileAccessor.Route[endRoad][3];
						double rate = dis_overflow / fileAccessor.Length[endRoad];

						double x = x1 + (x2 - x1)*rate;
						double y = y1 + (y2 - y1)*rate;

						partInRoads_all[startRoad][endRoad].push_back(x1);
						partInRoads_all[startRoad][endRoad].push_back(y1);
						partInRoads_all[startRoad][endRoad].push_back(x);
						partInRoads_all[startRoad][endRoad].push_back(y);
					}

					outRoad.insert(endRoad);
				}
			}

			//outRoad=inRoad+partin的路
			outRoad.insert(inRoad.begin(), inRoad.end());
		}
		else
		{
			outRoad.insert(startRoad);
			partIn[startRoad].push_back(2 * MRLimit);

			//计算坐标
			double x1 = fileAccessor.Route[startRoad][0];
			double y1 = fileAccessor.Route[startRoad][1];
			double x2 = fileAccessor.Route[startRoad][2];
			double y2 = fileAccessor.Route[startRoad][3];
			double x_mid = (x1 + x2) / 2;
			double y_mid = (y1 + y2) / 2;

			int node1 = fileAccessor.roadNode[startRoad][0];		//endrode的端点1
			int node2 = fileAccessor.roadNode[startRoad][1];		//endrode的端点2

			double rate = 2 * MRLimit / fileAccessor.Length[startRoad];

			double xn1 = fileAccessor.Route[startRoad][0];
			double yn1 = fileAccessor.Route[startRoad][1];
			double xn2 = fileAccessor.Route[startRoad][2];
			double yn2 = fileAccessor.Route[startRoad][3];
			double x_1 = x_mid + (xn1 - x_mid)*rate;
			double y_1 = y_mid + (yn1 - y_mid)*rate;
			double x_2 = x_mid + (xn2 - x_mid)*rate;
			double y_2 = y_mid + (yn2 - y_mid)*rate;

			partInRoads_all[startRoad][startRoad].push_back(x_1);
			partInRoads_all[startRoad][startRoad].push_back(y_1);
			partInRoads_all[startRoad][startRoad].push_back(x_2);
			partInRoads_all[startRoad][startRoad].push_back(y_2);
		}

		//计算Net所需数据
		reachRoad_all.insert(std::make_pair(startRoad, outRoad));
		for (auto it1 = outRoad.begin(); it1 != outRoad.end(); it1++)
		{
			if (NeedStop) {
				return;
			}

			int endRoad = *it1;

			int endnode = 0;
			int endnode1 = fileAccessor.roadNode[endRoad][0];
			int endnode2 = fileAccessor.roadNode[endRoad][1];

			//获取短的分支			
			if (distances[endnode1] < distances[endnode2])
				endnode = endnode1;
			else
				endnode = endnode2;

			if (partIn.count(endRoad) == 0)
			{
				//把经历的道路建一个队列
				getInRoads_MR(lineRoads, fileAccessor, startRoad, endRoad, startNode, parents, distances);

				NetreachData["MR"].From_OID.push_back(startRoad);
				NetreachData["MR"].To_OID.push_back(endRoad);
				NetreachData["MR"].Radius.push_back(MRLimit);
				NetreachData["MR"].DirChg.push_back(0);
				NetreachData["MR"].JncLmt.push_back(0);

				//根据获取到的道路队列，计算Relen、Jnc、ddl
				double len = distances[endnode] + fileAccessor.Length[endRoad];
				if (endRoad == startRoad)
					len = fileAccessor.Length[endRoad];
				
				double jnc = 0;
				std::map<std::string, double> wgtvalue;
				if (isJnc)
					jnc = CalculateJnc(fileAccessor, lineRoads);
				if (isWgt)
					wgtvalue = CalculateWgt(weight, lineRoads);

				NetreachData["MR"].Relen.push_back(len);
				if (isJnc)
					NetreachData["MR"].Jnc.push_back(jnc);
				if (isWgt)
					NetreachData["MR"].Wgt.push_back(wgtvalue);
			}
			else
			{
				if (partIn[endRoad].size() == 1) {	//只在一端部分覆盖，另一端无法通行
					//把经历的道路建一个队列
					getInRoads_MR(lineRoads, fileAccessor, startRoad, endRoad, startNode, parents, distances);

					NetreachData["MR"].From_OID.push_back(startRoad);
					NetreachData["MR"].To_OID.push_back(endRoad);
					NetreachData["MR"].Radius.push_back(MRLimit);
					NetreachData["MR"].DirChg.push_back(0);
					NetreachData["MR"].JncLmt.push_back(0);

					//根据获取到的道路队列，计算Relen、Jnc、ddl	
					double len = MRLimit;
					double jnc = 0;
					std::map<std::string, double> wgtvalue;
					if (isJnc)
						jnc = CalculateJnc(fileAccessor, lineRoads);
					if (isWgt)
						wgtvalue = CalculateWgtPart(weight, lineRoads, startRoad, endRoad, partIn[endRoad].back()/fileAccessor.Length[endRoad]);

					//收集部分通过路的通行距离，注意：可能存在起点能够从endRoad的两端进入，但是不接触
					partInReachRoadLen[startRoad][endRoad] = partIn[endRoad].back();

					NetreachData["MR"].Relen.push_back(len);
					if (isJnc)
						NetreachData["MR"].Jnc.push_back(jnc);
					if (isWgt)
						NetreachData["MR"].Wgt.push_back(wgtvalue);
				}
				else {	//两端都是部分覆盖，且不接触
					//需要区分从哪一端进入->找出起点到部分覆盖的两条路径，影响计算Relen、jnc、wgt
					for (auto flow = partInReachRoadNodeLen[startRoad][endRoad].begin(); flow != partInReachRoadNodeLen[startRoad][endRoad].end(); flow++) {
						lineRoads.clear();
						lineRoads.insert(startRoad);
						lineRoads.insert(endRoad);

						int same_node = flow->first;
						double flow_dis = flow->second;
						double rate = flow_dis / fileAccessor.Length[endRoad];

						//找出部分覆盖线段的前一条道路：连接same_node的道路中，路径最短的一条
						int start_node = parents[same_node];
						while (start_node != startNode) {
							int road= fileAccessor.nodeToEdge[IndexToKey(start_node, same_node)];
							lineRoads.insert(road);
							same_node = start_node;
							start_node = parents[start_node];
						}

						NetreachData["MR"].From_OID.push_back(startRoad);
						NetreachData["MR"].To_OID.push_back(endRoad);
						NetreachData["MR"].Radius.push_back(MRLimit);
						NetreachData["MR"].DirChg.push_back(0);
						NetreachData["MR"].JncLmt.push_back(0);

						double len = MRLimit;
						double jnc = 0;
						std::map<std::string, double> wgtvalue;
						if (isJnc)
							jnc = CalculateJnc(fileAccessor, lineRoads);
						if (isWgt)
							wgtvalue = CalculateWgtPart(weight, lineRoads,startRoad, endRoad, rate);

						//收集部分通过路的通行距离，注意：可能存在起点能够从endRoad的两端进入，但是不接触
						partInReachRoadLen[startRoad][endRoad] = std::accumulate(partIn[endRoad].begin(), partIn[endRoad].end(), 0);	//部分通过的总线长度

						NetreachData["MR"].Relen.push_back(len);
						if (isJnc)
							NetreachData["MR"].Jnc.push_back(jnc);
						if (isWgt)
							NetreachData["MR"].Wgt.push_back(wgtvalue);
					}
				}
				
			}
		}

		//把图撤销修改：删两条，加一条
		boost::remove_edge(startNode, node1, edgGraph);
		boost::remove_edge(startNode, node2, edgGraph);

		ep3.m_base = fileAccessor.Length[startRoad];
		ep3.m_value = startRoad;
		boost::add_edge(node1, node2, ep3, edgGraph);

		finishedCount += 1;
	}

	net_partInRoads_all.clear();
	net_partInRoads_all.insert(partInRoads_all.begin(), partInRoads_all.end());

	isFinished = true;
}


void Calculation::Geo_calculateMR(ShapeFileAccessor &fileAccessor)
{
	isFinished = false;

	if (subFromIDVec.size() == 0) {
		isFinished = true;
		return;
	}

	//清空数据
	//routeRoad_all.clear();
	//partInRoads_all.clear();

	//Geodesics数据
	std::map<std::string, Geodesics>().swap(GeodesicsData);	//有不收限制的两种：MR、JncR，和收限制的一种：DR
	geo_file_name = getOutFilePath();
	geo_str = "MR";

	std::map<int, std::set<int>>().swap(routeRoad_all);
	//std::map<int, std::map<int, std::vector<double>>>().swap(partInRoads_all);
	std::map<int, std::map<std::string, double>>().swap(GeoDataCount);

	std::map<std::pair <int, int>, double>().swap(routeLen_all);
	std::map<std::pair <int, int>, std::vector<int>>().swap(routeRoad_allMap);

	//创建有向图		
	Graph_d edgGraph_DR;
	if (angleLimitDR < INT_MAX - 10) {
		GenerateDirectedGraph(fileAccessor, edgGraph_DR);
	}
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;
	std::map<int, int> ddMap;
	std::map<double, std::set<int>> validRoad;

	//无向图复制
	Graph edgGraph;
	edgGraph.copy_impl(*g);
	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);

	//数据结构
	std::set<int> inNode;
	std::set<int> inRoad;
	std::set<int> outRoad;
	std::set<int> lineRoads;
	std::vector<int> routeRoads;
	std::vector<int> tmp_routeRoads;
	std::map<int, std::vector<double>> partIn;
	std::map<int, double> new_dict_real_min_dist;

	//对vector预配置内存空间
	//routeRoads.reserve(fileAccessor.Length.size() * sizeof(int) + 100);

	//增加
	EdgeProperty ep1, ep2, ep3;

	std::set<int> tmpFromSet;
	for (auto it = subFromIDVec.begin(); it != subFromIDVec.end(); it++)
	{
		tmpFromSet.insert(*it);
	}

	extern bool NeedStop;
	for (auto it = tmpFromSet.begin(); it != tmpFromSet.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;

		//修改图：删一条，加两条			
		double dis = fileAccessor.Length[startRoad] / 2;
		int startNode = numVertices;
		int node1 = fileAccessor.roadNode[startRoad][0];
		int node2 = fileAccessor.roadNode[startRoad][1];

		//删除边-起始边
		boost::remove_edge(node1, node2, edgGraph);

		//添加边-中点到起始边两端
		ep1.m_base = dis;
		ep1.m_value = startRoad;
		boost::add_edge(startNode, node1, ep1, edgGraph);

		ep2.m_base = dis;
		ep2.m_value = numEdges;
		boost::add_edge(startNode, node2, ep2, edgGraph);

		boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

		if (isOneToOne == false && angleLimitDR < INT_MAX - 10)
		{
			//创建有向图
			int startRoad2 = startRoad + old_numEdges;

			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

			//dict_min_dist.clear();
			std::map<int, double>().swap(dict_min_dist);
			for (int i = 0; i < numVertices_DR; i++)
				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

			//dict_real_min_dist.clear();
			std::map<int, double>().swap(dict_real_min_dist);
			for (int k = 0; k < old_numEdges; k++)
			{
				//找出有向边i对应的反向边id
				int reverse_k = k + old_numEdges;
				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				dict_real_min_dist.insert(std::make_pair(k, min_d));
			}
		}

		//准备好Geo数据
		if (isOneToOne == false)
		{
			if (int(ToIDVec.size()) > 0)
			{
				std::vector<std::vector<int>> tmp_routeRoadsAll;
				std::set<int> lineRoadsAll;
				for (auto iter = ToIDVec.begin(); iter != ToIDVec.end(); iter++)
				{
					int EndRoad = *iter;

					//必须把整条路全部算进来，否则在shp可视化文件上会数据不够
					getInRoads_MR2(routeRoads, lineRoads, fileAccessor, startRoad, EndRoad, startNode, parents, distances);

					routeRoad_all[startRoad].insert(lineRoads.begin(), lineRoads.end());

					//计算整体的dc、dd、ddl、wdd、jnc、wgt
					int dc = 0, jnc = 0;
					double dd = 0, ddl = 0;
					std::map<std::string, double> wdd;
					std::map<std::string, double> wgtvalue;

					if (angleLimitDR < INT_MAX - 10) {
						dc = CalculateDC(fileAccessor, routeRoads, old_numEdges);

						std::map<int, double>().swap(new_dict_real_min_dist);
						Generate_new_dict_real_min_dist(fileAccessor, new_dict_real_min_dist, dict_min_dist, lineRoads, startRoad, old_numEdges, parents1, distances1, parents2, distances2);

						dd = CalculateRoadsDD(fileAccessor, new_dict_real_min_dist, lineRoads);
						ddl = CalculateRoadsDDL(fileAccessor, new_dict_real_min_dist, lineRoads, partIn, startRoad, old_numEdges);
						wdd = CalculateWDD(fileAccessor, weight, new_dict_real_min_dist, lineRoads);
					}
					if (Jnc_t_limit_Jnc < INT_MAX - 10)
						jnc = CalculateJnc(fileAccessor, lineRoads);
					if (weight.size() > 0)
						wgtvalue = CalculateWgt(weight, lineRoads);

					if (angleLimitDR < INT_MAX - 10) {
						GeoDataCount[startRoad]["DC"] = dc;
						GeoDataCount[startRoad]["DD"] = dd;
						GeoDataCount[startRoad]["DDL"] = ddl;
						//GeoDataCount[startRoad]["WDD"] = wdd;
					}
					if (Jnc_t_limit_Jnc < INT_MAX - 10)
						GeoDataCount[startRoad]["Jnc"] = jnc;
					//if (angleLimitDR < INT_MAX - 10)
						//GeoDataCount[startRoad]["Wgt"] = wgt;

					//为Geodesics输出准备数据
					for (int pos = 0; pos < routeRoads.size(); pos++) {
						int endroad = routeRoads[pos];

						std::map<int, std::vector<double>>().swap(partIn);
						//加入起点的一半
						partIn[startRoad].push_back(0.5*fileAccessor.Length[startRoad]);
						//加入终点的一半
						partIn[endroad].push_back(0.5*fileAccessor.Length[endroad]);

						GeodesicsData["MR"].From_OID.push_back(startRoad);
						GeodesicsData["MR"].To_OID.push_back(endroad);

						//把经历的道路建一个队列
						tmp_routeRoads.clear();
						lineRoads.clear();
						tmp_routeRoads.insert(tmp_routeRoads.begin(), routeRoads.begin(), routeRoads.begin() + pos + 1);
						lineRoads.insert(tmp_routeRoads.begin(), tmp_routeRoads.end());
						
						//根据获取到的道路队列，计算dd、ddl、Jnc
						double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endroad);

						int dc = 0, jnc = 0;
						double dd = 0, ddl = 0;
						std::map<std::string, double> wdd;
						std::map<std::string, double> wgtvalue;

						if (angleLimitDR < INT_MAX - 10) {
							dc = CalculateDC(fileAccessor, tmp_routeRoads, old_numEdges);

							std::map<int, double>().swap(new_dict_real_min_dist);
							Generate_new_dict_real_min_dist(fileAccessor, new_dict_real_min_dist, dict_min_dist, lineRoads, startRoad, old_numEdges, parents1, distances1, parents2, distances2);

							dd = CalculateRoadsDD(fileAccessor, new_dict_real_min_dist, lineRoads);
							ddl = CalculateRoadsDDL(fileAccessor, new_dict_real_min_dist, lineRoads, partIn, startRoad, old_numEdges);
							wdd = CalculateWDD(fileAccessor, weight, new_dict_real_min_dist, lineRoads);
						}
						if (Jnc_t_limit_Jnc < INT_MAX - 10)
							jnc = CalculateJnc(fileAccessor, lineRoads);
						if (weight.size() > 0)
							wgtvalue = CalculateWgt(weight, lineRoads);

						GeodesicsData["MR"].PathLen.push_back(len);
						if (angleLimitDR < INT_MAX - 10) {
							GeodesicsData["MR"].DC.push_back(dc);
							GeodesicsData["MR"].DD.push_back(dd);
							GeodesicsData["MR"].DDL.push_back(ddl);
							GeodesicsData["MR"].WDD.push_back(wdd);
						}
						if (Jnc_t_limit_Jnc < INT_MAX - 10)
							GeodesicsData["MR"].Jnc.push_back(jnc);
						if (weight.size() > 0)
							GeodesicsData["MR"].Wgt.push_back(wgtvalue);
					}
				}
			}
			else
			{
				for (auto iter = fileAccessor.roadID.begin(); iter != fileAccessor.roadID.end(); iter++)
				{
					if (dict_real_min_dist[*iter] == 0)
					{
						int endroad = *iter;

						std::map<int, std::vector<double>>().swap(partIn);
						//加入起点的一半
						partIn[startRoad].push_back(0.5*fileAccessor.Length[startRoad]);
						//加入终点的一半
						partIn[endroad].push_back(0.5*fileAccessor.Length[endroad]);

						routeRoad_all[startRoad].insert(endroad);

						GeodesicsData["MR"].From_OID.push_back(startRoad);
						GeodesicsData["MR"].To_OID.push_back(endroad);

						//把经历的道路建一个队列
						getInRoads_MR2(tmp_routeRoads, lineRoads, fileAccessor, startRoad, endroad, startNode, parents, distances);

						std::map<int, double>().swap(new_dict_real_min_dist);
						Generate_new_dict_real_min_dist(fileAccessor, new_dict_real_min_dist, dict_min_dist, lineRoads, startRoad, old_numEdges, parents1, distances1, parents2, distances2);

						//根据获取到的道路队列，计算dd、ddl、Jnc
						double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endroad);
						int dc = CalculateDC(fileAccessor, tmp_routeRoads, old_numEdges);
						double dd = CalculateRoadsDD(fileAccessor, new_dict_real_min_dist, lineRoads);
						double ddl = CalculateRoadsDDL(fileAccessor, new_dict_real_min_dist, lineRoads, partIn, startRoad, old_numEdges);
						std::map<std::string, double> wdd = CalculateWDD(fileAccessor, weight, new_dict_real_min_dist, lineRoads);
						int jnc = CalculateJnc(fileAccessor, lineRoads);
						std::map<std::string, double> wgt = CalculateWgt(weight, lineRoads);

						GeodesicsData["MR"].PathLen.push_back(len);
						GeodesicsData["MR"].DC.push_back(dc);
						GeodesicsData["MR"].DD.push_back(dd);
						GeodesicsData["MR"].DDL.push_back(ddl);
						GeodesicsData["MR"].WDD.push_back(wdd);
						GeodesicsData["MR"].Jnc.push_back(jnc);
						GeodesicsData["MR"].Wgt.push_back(wgt);
					}
				}

				//计算整体的dc、dd、ddl、wdd、jnc、wgt
				int dc = 0;
				for (auto tt = fileAccessor.Dc.begin(); tt != fileAccessor.Dc.end(); tt++) {
					dc += tt->second;
				}

				double dd = CalculateRoadsDD(fileAccessor, dict_real_min_dist, fileAccessor.roadID);
				double ddl = CalculateRoadsDDL(fileAccessor, dict_real_min_dist, fileAccessor.roadID, partIn, startRoad, old_numEdges);
				std::map<std::string, double> wdd = CalculateWDD(fileAccessor, weight, dict_real_min_dist, fileAccessor.roadID);
				int jnc = 0;
				for (auto tt = fileAccessor.Jnc.begin(); tt != fileAccessor.Jnc.end(); tt++) {
					jnc += tt->second;
				}
				std::map<std::string, double> wgt = CalculateWgt(weight, fileAccessor.roadID);

				GeoDataCount[startRoad]["DC"] = dc;
				GeoDataCount[startRoad]["DD"] = dd;
				GeoDataCount[startRoad]["DDL"] = ddl;
				//GeoDataCount[startRoad]["WDD"] = wdd;
				GeoDataCount[startRoad]["Jnc"] = jnc;
				//GeoDataCount[startRoad]["Wgt"] = wgt;
			}
		}
		else
		{
			for (auto iter = FromToMap[startRoad].begin(); iter != FromToMap[startRoad].end(); iter++)
			{
				int EndRoad = *iter;

				int endnode = 0;
				int endnode1 = fileAccessor.roadNode[EndRoad][0];
				int endnode2 = fileAccessor.roadNode[EndRoad][1];
				//获取短的分支			
				if (distances[endnode1] < distances[endnode2])
					endnode = endnode1;
				else
					endnode = endnode2;
				double len = distances[endnode] + 0.5*fileAccessor.Length[EndRoad];
				if (EndRoad == startRoad)
					len = 0;
				routeLen_all.insert(std::make_pair(std::pair<int, int>(startRoad, EndRoad), len));

				if (outputPath)
				{
					//必须把整条路全部算进来，否则在shp可视化文件上会数据不够
					getInRoads_MR2(routeRoads, lineRoads, fileAccessor, startRoad, EndRoad, startNode, parents, distances);

					//保存路径ID序列等待输出
					routeRoad_allMap.insert(std::make_pair(std::pair<int, int>(startRoad, EndRoad), routeRoads));

					routeRoad_all[startRoad].insert(lineRoads.begin(), lineRoads.end());
				}

				////为Geodesics输出准备数据
				//for (auto it3 = routeRoads.begin(); it3 != routeRoads.end(); it3++)
				//{
				//	int endroad = *it3;

				//	GeodesicsData["MR"].From_OID.push_back(startRoad);
				//	GeodesicsData["MR"].To_OID.push_back(endroad);
				//	GeodesicsData["MR"].Radius.push_back(0);

				//	//把经历的道路建一个队列
				//	getInRoads_MR(lineRoads, fileAccessor, startRoad, endroad, startNode, parents, distances);

				//	//根据获取到的道路队列，计算dd、ddl、Jnc
				//	double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endroad);
				//	double dd = CalculateRoadsDD(fileAccessor, dict_real_min_dist, lineRoads);
				//	double ddl = CalculateRoadsDDL(fileAccessor, dict_real_min_dist, lineRoads);
				//	int jnc = CalculateJnc(fileAccessor, lineRoads);
				//	double wgt = CalculateWgt(weight, lineRoads);

				//	//保存最短路径长度
				//	if (endroad == EndRoad)
				//		routeLen_all.insert(std::make_pair(std::pair<int, int>(startRoad, EndRoad), len));

				//	GeodesicsData["MR"].PathLen.push_back(len);
				//	GeodesicsData["MR"].DD.push_back(dd);
				//	GeodesicsData["MR"].DDL.push_back(ddl);
				//	GeodesicsData["MR"].Jnc.push_back(jnc);
				//	GeodesicsData["MR"].Wgt.push_back(wgt);
				//}
			}
		}

		//把图撤销修改：删两条，加一条
		boost::remove_edge(startNode, node1, edgGraph);
		boost::remove_edge(startNode, node2, edgGraph);

		ep3.m_base = fileAccessor.Length[startRoad];
		ep3.m_value = startRoad;
		boost::add_edge(node1, node2, ep3, edgGraph);

		finishedCount += 1;
	}

	isFinished = true;
}


inline std::map<std::string, std::map<int, double>> getMap(ShapeFileAccessor& fileAccessor, int startRoad, std::set<int> validRoads, std::map<int, partInNode> partInLength, std::map<std::string, std::map<int, double>> weight, std::vector<bool> type) {
	std::map<std::string, std::map<int, double>> resultMap;
	struct CompareSecond {
		bool operator()(const std::pair<int, int>& p1, const std::pair<int, int>& p2) {
			return p1.second > p2.second;
		}
	};
	std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, CompareSecond> q;
	std::vector<bool> visited(fileAccessor.roadID.size() * 2, false);
	q.push(std::make_pair(startRoad, 0));
	int inverse_startRoad= (startRoad + fileAccessor.roadID.size()) % (fileAccessor.roadID.size() * 2);
	for (int connectedRoad : fileAccessor.AdjRoadList[startRoad]) {
		if (connectedRoad == inverse_startRoad)continue;
		if (validRoads.count(connectedRoad % fileAccessor.roadID.size()) > 0 && !visited[connectedRoad]) {
			q.push(std::make_pair(connectedRoad, fileAccessor.AdjTurnMP[startRoad][connectedRoad]));

		}
	}
	for (int connectedRoad : fileAccessor.AdjRoadList[inverse_startRoad]) {
		if (connectedRoad == startRoad)continue;
		if (validRoads.count(connectedRoad % fileAccessor.roadID.size()) > 0 && !visited[connectedRoad]) {
			q.push(std::make_pair(connectedRoad, fileAccessor.AdjTurnMP[inverse_startRoad][connectedRoad]));
		}
	}

	while (!q.empty()) {
		std::pair<int, int> nowRoadAndTurnCount = q.top();
		int nowRoad = nowRoadAndTurnCount.first;
		int inverse_nowRoad = (nowRoad + fileAccessor.roadID.size()) % (fileAccessor.roadID.size() * 2);
		int turnCount = nowRoadAndTurnCount.second;
		q.pop();

		if (!visited[nowRoad]) {
			if (partInLength.count(nowRoad)==0) {
				visited[inverse_nowRoad] = true;
			}
			visited[nowRoad] = true;

			// dd
			if (type[0]) {
				resultMap["dd"][turnCount]++;
			}

			// ddl
			if (type[1]) {
				if (partInLength.count(nowRoad)) {
					resultMap["ddl"][turnCount] += partInLength[nowRoad].leftLength + partInLength[nowRoad].rightLength;
				}
				else {
					resultMap["ddl"][turnCount] += fileAccessor.Length[nowRoad];
				}
			}

			// wdd
			if (type[2]) {
				for (const auto& weightPair : weight) {
					const std::string& weightName = weightPair.first;
					const std::map<int, double>& weightMap = weightPair.second;
					resultMap[weightName][turnCount] += weightMap.find(nowRoad)->second;
				}
			}

			for (int connectedRoad : fileAccessor.AdjRoadList[nowRoad]) {
				if (connectedRoad == inverse_nowRoad)continue;
				if (validRoads.count(connectedRoad%fileAccessor.roadID.size()) > 0 && !visited[connectedRoad]) {
					q.push(std::make_pair(connectedRoad, turnCount + fileAccessor.AdjTurnMP[nowRoad][connectedRoad]));
					
				}
			}
		}
	}
	return resultMap;
}

void Calculation::calculateDDLbyDij(ShapeFileAccessor& fileAccessor) {
	isFinished = false;

	if (subRoadVec.size() == 0) {
		isFinished = true;
		return;
	}

	int numVertices_DR = fileAccessor.roadID.size() * 2;

	std::vector<double> distances1(numVertices_DR, numVertices_DR + 1);
	std::vector<double> distances2(numVertices_DR, numVertices_DR + 1);
	DirectionDistanceFunction<int> directiondistfunc(fileAccessor);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist; 
	std::map<int, double> new_dict_real_min_dist; 
	std::map<int, double> ddMap; 
	std::map<int, double> ddlMap; 
	std::map<int, double> wddMap; 
	std::set<int> inRoad; 

	std::map<int, int> newMapNodes;
	std::map<int, std::vector<int>> newRoadNode;
	std::map<std::string, int> newNodeToRoad;
	std::map<int, std::vector<double>> partIn; 

	int numVertices_MD = fileAccessor.roadID.size(); 


	std::vector<double> distances0(numVertices_MD, INT_MAX);

	MetricDistanceFunction<double> metricdistfunc(fileAccessor);

	std::set<int> inNode; 
	std::set<int> outRoad; 
	std::vector<double> allIn;
	std::vector<double> partIn2;
	std::map<int, partInNode> partInLength;
	std::map<int, std::map<int, std::map<int, int>>> mdr_partInReachRoadNodeCoor;


	std::set<double> last_allInRoads;		


	std::vector<vertex_descriptor> parents(numVertices_MD + 1);
	std::vector<double> distances(numVertices_MD + 1);
	std::vector<int> tmp{ 0,0 };

	double MAX_MRLimit = -1;
	for (auto MRLimit : MRLimitSet2) {
		if (MRLimit > MAX_MRLimit)MAX_MRLimit = MRLimit;
		if (MRLimit == -1) {
			MAX_MRLimit = -1; break;
		}
	}

	extern bool NeedStop;

	for (auto startRoad : subRoadVec) {
		std::vector<double>(numVertices_MD, INT_MAX).swap(distances0);
		getDistDijkstra(metricdistfunc, startRoad, MAX_MRLimit, distances0);
		if (MRLimitSet2.empty()) {
			outRoad.insert(fileAccessor.roadID.begin(), fileAccessor.roadID.end());
		}
		else {
			for (auto MRLimit : MRLimitSet2) {
				if (MRLimit == -1) outRoad.insert(fileAccessor.roadID.begin(), fileAccessor.roadID.end());
				else {
					std::set<int>().swap(outRoad);
					std::set<int>().swap(inRoad);
					/*std::map<int, std::vector<double>>().swap(partIn);*/
					std::map<int, partInNode>().swap(partInLength);
					std::queue<int> q;
					std::vector<bool> visited(fileAccessor.roadID.size());
					//the startRoad can be totally cover. 
					if (2 * MRLimit < fileAccessor.Length[startRoad]) {
						outRoad.insert(startRoad); 
					}
					else {
						q.push(startRoad);
						while (!q.empty()) {
							int nowRoad = q.front();
							q.pop();
							if (visited[nowRoad])
								continue;
							double len = fileAccessor.Length[nowRoad];
							outRoad.insert(nowRoad);
							visited[nowRoad] = true;
							for (auto next_iter = fileAccessor.Road_to_roads[nowRoad].begin(); next_iter != fileAccessor.Road_to_roads[nowRoad].end(); next_iter++) {
								double length = fileAccessor.Length[*next_iter];
								int connected_cursor = *next_iter;
								if (connected_cursor == startRoad)
									continue;
								if (distances0[connected_cursor] <= MRLimit) // connected_cursor can be totally cover	
									q.push(connected_cursor);
								else if (distances0[nowRoad]< MRLimit && distances0[connected_cursor] > MRLimit) {	//can't
									outRoad.insert(connected_cursor);
									int pre_node1 = fileAccessor.roadNode[nowRoad][0];
									int pre_node2 = fileAccessor.roadNode[nowRoad][1];
									int node1 = fileAccessor.roadNode[connected_cursor][0];
									int node2 = fileAccessor.roadNode[connected_cursor][1];
									int same_node = node1;
									if (pre_node1 == node2 || pre_node2 == node2)
										same_node = node2;
									if (same_node == node1) {
										if (MRLimit - distances0[nowRoad] > partInLength[connected_cursor].leftLength) {
											partInLength[connected_cursor].leftPreRoad = nowRoad;
											partInLength[connected_cursor].leftLength = MRLimit - distances0[nowRoad];
										}
									}
									else {
										if (MRLimit - distances0[nowRoad] > partInLength[connected_cursor].rightLength) {
											partInLength[connected_cursor].rightPreRoad = nowRoad;
											partInLength[connected_cursor].rightLength = MRLimit - distances0[nowRoad];
										}
									}
								}
							}
						}

					}
				}


				if (isWgt) {
					std::map<std::string,std::map<int, double>> map = getMap(fileAccessor, startRoad, outRoad, partInLength, weight, std::vector<bool>(3, true));
					ddMap = map["dd"];
					ddlMap = map["ddl"];
					
				
					double weight_turn_sum = 0, weight_sum = 0;
					for (auto it = weight.begin(); it != weight.end(); it++) {
						wddMap = map[it->first];
						for (const auto& pair : wddMap) {
							weight_turn_sum += pair.first * pair.second;
							weight_sum += pair.second;
						}
						WDD_all[MRLimit][it->first][startRoad] = weight_turn_sum / weight_sum;
					}
					
				}
				else {
					std::vector<bool> type = { true,true,false };
					std::map<std::string, std::map<int, double>> map = getMap(fileAccessor, startRoad, outRoad, partInLength, weight, type);
					ddMap = map["dd"];
					ddlMap = map["ddl"];
				}
				double road_count = 0, turn_count = 0;
				for (const auto& pair : ddMap) {
					turn_count += pair.first * pair.second;
					road_count += pair.second;
				}
				DD_all[MRLimit][startRoad] = turn_count / road_count;

				double length_turn_sum = 0, length_sum = 0;
				for (const auto& pair : ddlMap) {
					length_turn_sum += pair.first * pair.second;
					length_sum += pair.second;
				}
				DDL_all[MRLimit][startRoad] = length_turn_sum / length_sum;
				
				//std::vector<double>(numVertices_DR, INT_MAX).swap(distances1);
				//std::vector<double>(numVertices_DR, INT_MAX).swap(distances2);
				//PartDirectionDistanceFunction<int> partdirectiondistfunc(fileAccessor, outRoad);
				//getDistBFS(partdirectiondistfunc, startRoad, -1, distances1);
				//getDistBFS(partdirectiondistfunc, startRoad + fileAccessor.roadID.size(), -1, distances2);
				//std::map<int, double>().swap(dict_min_dist);
				//for (int i = 0; i < numVertices_DR; i++)
				//	dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));
				//std::map<int, double>().swap(dict_real_min_dist);
				//for (int k = 0; k < fileAccessor.roadID.size(); k++) {
				//	int reverse_k = k + fileAccessor.roadID.size();
				//	double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				//	dict_real_min_dist.insert(std::make_pair(k, min_d));
				//}

				//double M_dd = DD(fileAccessor, dict_min_dist, dict_real_min_dist, outRoad, partInLength);
				//double M_ddl = DDL(fileAccessor, dict_min_dist, dict_real_min_dist, outRoad, partInLength);
				//DD_all[MRLimit][startRoad] = M_dd;
				//DDL_all[MRLimit][startRoad] = M_ddl;

				//if (isWgt)
				//{
				//	double M_ddl = DDL(fileAccessor, dict_min_dist, dict_real_min_dist, outRoad, partInLength);
				//	for (auto wgt_it = wdd.begin(); wgt_it != wdd.end(); wgt_it++) {
				//		WDD_all[MRLimit][wgt_it->first][startRoad] = wgt_it->second;
				//	}

				//	std::map<std::string, double> wdd;
				//	std::map<std::string, double> wdd_sum1;
				//	std::map<std::string, double> wdd_sum2;
				//	std::map<std::string, double> sub_wgt;
				//	std::set<int> valid_roads;

				//	for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++) {
				//		sub_wgt = CalculateWgtInline(weight, it3->second);
				//		for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
				//			wdd_sum1[wgt_it->first] += ((it3->first) * (sub_wgt[wgt_it->first]));
				//		}
				//		valid_roads.insert(it3->second.begin(), it3->second.end());
				//	}
				//	wdd_sum2 = CalculateWgtInline(weight, valid_roads);

				//	for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
				//		if (wdd_sum2[wgt_it->first] == 0)
				//			wdd[wgt_it->first] = 0;
				//		else
				//			wdd[wgt_it->first] = wdd_sum1[wgt_it->first] / wdd_sum2[wgt_it->first];
				//	}

				//	for (auto wgt_it = wdd.begin(); wgt_it != wdd.end(); wgt_it++) {
				//		WDD_all[MRLimit][wgt_it->first][startRoad] = wgt_it->second;
				//	}

				//}

			}
		}
		

		finishedCount += 1;
		addFinishedCount();
	}

	isFinished = true;
}


void Calculation::calculateDDL(ShapeFileAccessor &fileAccessor) {
	isFinished = false;

	if (subRoadVec.size() == 0) {
		isFinished = true;
		return;
	}

	//创建有向图		
	Graph_d edgGraph_DR;
	GenerateDirectedGraph(fileAccessor, edgGraph_DR);
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;
	std::map<int, double> new_dict_real_min_dist;
	std::map<int, std::set<int>> ddMap;
	std::set<int> inRoad;
	std::map<double, std::set<int>> validRoad;
	//有效路准备转无向图需要的数据
	std::map<int, int> newMapNodes;
	std::map<int, std::vector<int>> newRoadNode;
	std::map<std::string, int> newNodeToRoad;
	std::map<int, std::vector<double>> partIn;

	//预备无向图数据
	Graph edgGraph_MD;
	edgGraph_MD.copy_impl(*g);
	int numEdges_MD = int(boost::num_edges(edgGraph_MD));
	int numVertices_MD = int(boost::num_vertices(edgGraph_MD));

	//准备接数据
	std::vector<vertex_descriptor> parents0(numVertices_MD + 1);
	std::vector<double> distances0(numVertices_MD + 1);
	std::set<int> inNode;
	std::set<int> outRoad;
	std::vector<double> allIn;
	std::vector<double> partIn2;
	std::map<int, std::map<int, std::map<int, int>>> mdr_partInReachRoadNodeCoor;

	//增加
	std::set<double> last_allInRoads;		//前一个半径限制下的全覆盖路径ID序列

	//对vector预配置内存空间
	//allIn.reserve(subRoadVec.size() * sizeof(double) + 100);
	//partIn2.reserve(subRoadVec.size() * sizeof(double) + 100);

	//准备接道路数据
	std::vector<vertex_descriptor> parents(numVertices_MD + 1);
	std::vector<double> distances(numVertices_MD + 1);
	std::vector<int> tmp{ 0,0 };

	//增加
	EdgeProperty ep1, ep2, ep3;
	Graph subGraph;

	extern bool NeedStop;
	for (auto it = subRoadVec.begin(); it != subRoadVec.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;
		int startRoad2 = startRoad + old_numEdges;

		//传入n组队列，输出n*m组终极有效队列
		if (int(MRLimitSet2.size()) == 0)		//无里程限制：direction reach
		{
			//两次有向图搜索
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

			std::map<int, double>().swap(dict_min_dist);
			for (int i = 0; i < numVertices_DR; i++)
				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

			//收集DR有效的道路队列
			std::map<double, std::set<int>>().swap(validRoad);
			std::map<int, double>().swap(dict_real_min_dist);
			for (int k = 0; k < old_numEdges; k++) {
				//找出有向边i对应的反向边id
				int reverse_k = k + old_numEdges;
				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				dict_real_min_dist.insert(std::make_pair(k, min_d));
			}

			//计算dd、ddl
			std::map<int, std::set<int>>().swap(ddMap);
			double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
			for (auto it2 = fileAccessor.roadID.begin(); it2 != fileAccessor.roadID.end(); it2++){
				ddMap[int(dict_real_min_dist[*it2])].insert(*it2);
				M_ddlSum += dict_real_min_dist[*it2] * fileAccessor.Length[*it2];
				M_lenSum += fileAccessor.Length[*it2];
			}
			double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
			for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++){
				M_ddSum1 += ((it3->first)*(it3->second.size()));
				M_ddSum2 += it3->second.size();
			}
			M_dd = M_ddSum1 / M_ddSum2;
			M_ddl = M_ddlSum / M_lenSum;

			//数据存储
			DD_all[-1][startRoad] = M_dd;
			DDL_all[-1][startRoad] = M_ddl;

			if (isWgt) {
				std::map<std::string, double> wdd;
				std::map<std::string, double> wdd_sum1;
				std::map<std::string, double> wdd_sum2;
				std::map<std::string, double> sub_wgt;

				for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++) {
					sub_wgt = CalculateWgtInline(weight, it3->second);
					for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
						wdd_sum1[wgt_it->first] += ((it3->first)*(sub_wgt[wgt_it->first]));
					}
				}
				wdd_sum2 = CalculateWgtInline(weight, fileAccessor.roadID);

				for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
					if (wdd_sum2[wgt_it->first] == 0)
						wdd[wgt_it->first] = 0;
					else
						wdd[wgt_it->first] = wdd_sum1[wgt_it->first] / wdd_sum2[wgt_it->first];
				}

				//数据存储
				for (auto wgt_it = wdd.begin(); wgt_it != wdd.end(); wgt_it++) {
					WDD_all[-1][wgt_it->first][startRoad] = wgt_it->second;
				}

			}

		}
		else	 //转向限制+里程限制：combined reach
		{
			double dis = fileAccessor.Length[startRoad] / 2;
			int startNode = numVertices_MD;
			int MD_node1 = fileAccessor.roadNode[startRoad][0];
			int MD_node2 = fileAccessor.roadNode[startRoad][1];

			//删除边-起始边
			boost::remove_edge(MD_node1, MD_node2, edgGraph_MD);

			//添加边-中点到起始边两端
			ep1.m_base = dis;
			ep1.m_value = startRoad;
			boost::add_edge(startNode, MD_node1, ep1, edgGraph_MD);

			ep2.m_base = dis;
			ep2.m_value = numEdges_MD;
			boost::add_edge(startNode, MD_node2, ep2, edgGraph_MD);

			boost::dijkstra_shortest_paths(edgGraph_MD, startNode, boost::predecessor_map(&parents0[0]).distance_map(&distances0[0]));

			//对所有MRLimit参数组一轮算完
			for (auto iter = MRLimitSet2.begin(); iter != MRLimitSet2.end(); iter++)
			{
				double MRLimit = *iter;

				std::set<int>().swap(inNode);
				std::set<int>().swap(inRoad);
				std::set<int>().swap(outRoad);
				std::set<double>().swap(last_allInRoads);
				std::map<int, std::vector<double>>().swap(partIn);
				partInReachRoadNodeLen.clear();

				if (MRLimit == -1) {
					outRoad.insert(fileAccessor.roadID.begin(), fileAccessor.roadID.end());
				}
				else {
					//计算inNode：编号0-(顶点数目-1)
					for (int endNode = 0; endNode < numVertices_MD; endNode++)
					{
						double tmpdis = distances0[endNode];
						double td = double(std::round(pow(10, 8)*tmpdis)) / double(pow(10, 8));
						if (MRLimit > td)
							inNode.insert(endNode);
					}

					//判断是否能够冲出起始边
					if (2 * MRLimit >= fileAccessor.Length[startRoad])
					{
						//将起始边加入allInLength
						inRoad.insert(startRoad);

						//全覆盖
						for (auto it1 = fileAccessor.roadID.begin(); it1 != fileAccessor.roadID.end(); it1++)
						{
							int endRoad = *it1;
							if (endRoad == startRoad)	//跳过处理起始边
								continue;

							double roadLen = fileAccessor.Length[endRoad];
							int node1 = fileAccessor.roadNode[endRoad][0];		//endrode的端点1
							int node2 = fileAccessor.roadNode[endRoad][1];		//endrode的端点2

							// 如果是完全覆盖，则必须同时到达其两个端点
							if (inNode.count(node1) != 0 && inNode.count(node2) != 0)
							{
								double dist_res1 = MRLimit - distances0[node1];  // 端点1方向越过距离
								double dist_res2 = MRLimit - distances0[node2];  // 端点2方向越过距离
								double dist_res = dist_res1 + dist_res2;
								if (dist_res >= roadLen)   // 只有确定是完全越过，才会加入allIn
								{
									inRoad.insert(endRoad);
								}
							}
						}

						//部分覆盖
						for (auto it1 = fileAccessor.roadID.begin(); it1 != fileAccessor.roadID.end(); it1++)
						{
							int endRoad = *it1;
							// 跳过全部覆盖的边
							if (inRoad.count(endRoad) != 0)
								continue;

							int node1 = fileAccessor.roadNode[endRoad][0];		//endrode的端点1
							int node2 = fileAccessor.roadNode[endRoad][1];		//endrode的端点2

							// 如果是部分覆盖，只要有一个端点有越过
							if (inNode.count(node1) != 0 || inNode.count(node2) != 0)
							{
								// 检查是否已到达边缘的起点
								if (inNode.count(node1) != 0)
								{
									double dis_overflow = MRLimit - distances0[node1];
									partIn[endRoad].push_back(dis_overflow);
									partInReachRoadNodeLen[startRoad][endRoad][node1] = dis_overflow;
									mdr_partInReachRoadNodeCoor[startRoad][endRoad][node1] = partInRoads_all[startRoad][endRoad].size();

									//计算坐标
									double x1 = fileAccessor.Route[endRoad][0];
									double y1 = fileAccessor.Route[endRoad][1];
									double x2 = fileAccessor.Route[endRoad][2];
									double y2 = fileAccessor.Route[endRoad][3];
									double rate = dis_overflow / fileAccessor.Length[endRoad];

									double x = x1 + (x2 - x1)*rate;
									double y = y1 + (y2 - y1)*rate;

									partInRoads_all[startRoad][endRoad].push_back(x1);
									partInRoads_all[startRoad][endRoad].push_back(y1);
									partInRoads_all[startRoad][endRoad].push_back(x);
									partInRoads_all[startRoad][endRoad].push_back(y);
								}
								if (inNode.count(node2) != 0)
								{
									double dis_overflow = MRLimit - distances0[node2];
									partIn[endRoad].push_back(dis_overflow);
									partInReachRoadNodeLen[startRoad][endRoad][node2] = dis_overflow;
									mdr_partInReachRoadNodeCoor[startRoad][endRoad][node2] = partInRoads_all[startRoad][endRoad].size();

									//计算坐标
									double x2 = fileAccessor.Route[endRoad][0];
									double y2 = fileAccessor.Route[endRoad][1];
									double x1 = fileAccessor.Route[endRoad][2];
									double y1 = fileAccessor.Route[endRoad][3];
									double rate = dis_overflow / fileAccessor.Length[endRoad];

									double x = x1 + (x2 - x1)*rate;
									double y = y1 + (y2 - y1)*rate;

									partInRoads_all[startRoad][endRoad].push_back(x1);
									partInRoads_all[startRoad][endRoad].push_back(y1);
									partInRoads_all[startRoad][endRoad].push_back(x);
									partInRoads_all[startRoad][endRoad].push_back(y);
								}

								outRoad.insert(endRoad);
							}
						}

						//outRoad=inRoad+partin的路
						outRoad.insert(inRoad.begin(), inRoad.end());
					}
					else
					{
						outRoad.insert(startRoad);
						partIn[startRoad].push_back(2 * MRLimit);

						//计算坐标
						double x1 = fileAccessor.Route[startRoad][0];
						double y1 = fileAccessor.Route[startRoad][1];
						double x2 = fileAccessor.Route[startRoad][2];
						double y2 = fileAccessor.Route[startRoad][3];
						double x_mid = (x1 + x2) / 2;
						double y_mid = (y1 + y2) / 2;

						int node1 = fileAccessor.roadNode[startRoad][0];		//endrode的端点1
						int node2 = fileAccessor.roadNode[startRoad][1];		//endrode的端点2

						double rate = 2 * MRLimit / fileAccessor.Length[startRoad];

						double xn1 = fileAccessor.Route[startRoad][0];
						double yn1 = fileAccessor.Route[startRoad][1];
						double xn2 = fileAccessor.Route[startRoad][2];
						double yn2 = fileAccessor.Route[startRoad][3];
						double x_1 = x_mid + (xn1 - x_mid)*rate;
						double y_1 = y_mid + (yn1 - y_mid)*rate;
						double x_2 = x_mid + (xn2 - x_mid)*rate;
						double y_2 = y_mid + (yn2 - y_mid)*rate;

						partInRoads_all[startRoad][startRoad].push_back(x_1);
						partInRoads_all[startRoad][startRoad].push_back(y_1);
						partInRoads_all[startRoad][startRoad].push_back(x_2);
						partInRoads_all[startRoad][startRoad].push_back(y_2);
					}
				}

				std::map<int, double>().swap(new_dict_real_min_dist);
				double M_dd = 0, M_ddl = 0;
				if (MRLimit == -1) {		//无需重构有向图				
					boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
					boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

					std::map<int, double>().swap(dict_min_dist);
					for (int i = 0; i < numVertices_DR; i++)
						dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

					//收集DR有效的道路队列
					std::map<double, std::set<int>>().swap(validRoad);
					std::map<int, double>().swap(dict_real_min_dist);
					for (int k = 0; k < old_numEdges; k++) {
						//找出有向边i对应的反向边id
						int reverse_k = k + old_numEdges;
						double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
						dict_real_min_dist.insert(std::make_pair(k, min_d));

					}
					new_dict_real_min_dist.insert(dict_real_min_dist.begin(), dict_real_min_dist.end());

					ddMap.clear();
					for (auto it2 = fileAccessor.roadID.begin(); it2 != fileAccessor.roadID.end(); it2++) {
						ddMap[int(dict_real_min_dist[*it2])].insert(*it2);
					}

					//计算dd、ddl
					M_dd = CalculateRoadsDD(fileAccessor, new_dict_real_min_dist, outRoad);
					M_ddl = CalculateRoadsDDL(fileAccessor, new_dict_real_min_dist, outRoad, partIn, startRoad, old_numEdges);
					DD_all[MRLimit][startRoad] = M_dd;
					DDL_all[MRLimit][startRoad] = M_ddl;

				}
				else {
					if (outRoad.size() == fileAccessor.roadID.size()) {
						//两次有向图搜索
						boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
						boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

						std::map<int, double>().swap(dict_min_dist);
						for (int i = 0; i < numVertices_DR; i++)
							dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

						//收集DR有效的道路队列
						std::map<double, std::set<int>>().swap(validRoad);
						std::map<int, double>().swap(dict_real_min_dist);
						for (int k = 0; k < old_numEdges; k++) {
							//找出有向边i对应的反向边id
							int reverse_k = k + old_numEdges;
							double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
							dict_real_min_dist.insert(std::make_pair(k, min_d));

						}
						new_dict_real_min_dist.insert(dict_real_min_dist.begin(), dict_real_min_dist.end());

						ddMap.clear();
						for (auto it2 = fileAccessor.roadID.begin(); it2 != fileAccessor.roadID.end(); it2++) {
							ddMap[int(new_dict_real_min_dist[*it2])].insert(*it2);
						}

						//计算dd、ddl
						M_dd = CalculateRoadsDD(fileAccessor, new_dict_real_min_dist, outRoad);
						M_ddl = CalculateRoadsDDL(fileAccessor, new_dict_real_min_dist, outRoad, partIn, startRoad, old_numEdges);
						DD_all[MRLimit][startRoad] = M_dd;
						DDL_all[MRLimit][startRoad] = M_ddl;

					}
					else {	//需要重构有向图	
						std::map<int, std::map<int, int>> partInNodeIDs;
						std::map<std::tuple<int, int, int>, int> nodeToEdge;
						std::map<int, std::tuple<int, int, int>> edgeToNode;

						if (outRoad.size() == 1) {
							new_dict_real_min_dist.clear();
							new_dict_real_min_dist[startRoad] = 0;
						}
						else {
							Generate_new_dict_real_min_dist_partIn(fileAccessor, new_dict_real_min_dist, dict_min_dist, outRoad, startRoad, inRoad, partInReachRoadNodeLen[startRoad],
								old_numEdges, 2 * old_numEdges, parents1, distances1, parents2, distances2, partInNodeIDs, nodeToEdge, edgeToNode);
						}

						//计算dd、ddl
						M_dd = CalculateDD_PartIn(fileAccessor, new_dict_real_min_dist, outRoad, partInNodeIDs);
						M_ddl = CalculateDDL_PartIn(fileAccessor, new_dict_real_min_dist, outRoad, partIn, startRoad, old_numEdges, partInNodeIDs, partInReachRoadNodeLen[startRoad]);
						DD_all[MRLimit][startRoad] = M_dd;
						DDL_all[MRLimit][startRoad] = M_ddl;

						ddMap.clear();
						for (auto it2 = outRoad.begin(); it2 != outRoad.end(); it2++) {
							int road_id = *it2;
							if (partInNodeIDs.count(road_id)) {
								int min_cost = INT_MAX;
								for (auto node_it = partInNodeIDs[road_id].begin(); node_it != partInNodeIDs[road_id].end(); node_it++) {
									int node = node_it->first;
									int edge_id = node_it->second;
									min_cost = min(min_cost, new_dict_real_min_dist[edge_id]);
								}
								ddMap[min_cost].insert(road_id);
							}
							else {
								ddMap[int(new_dict_real_min_dist[road_id])].insert(road_id);
							}
						}
					}
				}

				//计算wdd
				if (isWgt)
				{
					std::map<std::string, double> wdd;
					std::map<std::string, double> wdd_sum1;
					std::map<std::string, double> wdd_sum2;
					std::map<std::string, double> sub_wgt;
					std::set<int> valid_roads;

					for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++) {
						sub_wgt = CalculateWgtInline(weight, it3->second);
						for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
							wdd_sum1[wgt_it->first] += ((it3->first)*(sub_wgt[wgt_it->first]));
						}
						valid_roads.insert(it3->second.begin(), it3->second.end());
					}
					wdd_sum2 = CalculateWgtInline(weight, valid_roads);

					for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
						if (wdd_sum2[wgt_it->first] == 0)
							wdd[wgt_it->first] = 0;
						else
							wdd[wgt_it->first] = wdd_sum1[wgt_it->first] / wdd_sum2[wgt_it->first];
					}

					//数据存储
					for (auto wgt_it = wdd.begin(); wgt_it != wdd.end(); wgt_it++) {
						WDD_all[MRLimit][wgt_it->first][startRoad] = wgt_it->second;
					}
					
				}

				//把图撤销修改：删两条，加一条
				boost::remove_edge(startNode, MD_node1, edgGraph_MD);
				boost::remove_edge(startNode, MD_node2, edgGraph_MD);

				ep3.m_base = fileAccessor.Length[startRoad];
				ep3.m_value = startRoad;
				boost::add_edge(MD_node1, MD_node2, ep3, edgGraph_MD);
			}
		}

		finishedCount += 1;
		addFinishedCount();
	}

	isFinished = true;
}

//void Calculation::calculateMDR(ShapeFileAccessor &fileAccessor) {
//	isFinished = false;
//
//	if (subRoadVec.size() == 0) {
//		isFinished = true;
//		return;
//	}
//
//	//创建有向图		
//	Graph_d edgGraph_DR;
//	GenerateDirectedGraph(fileAccessor, edgGraph_DR);
//	Graph *g = fileAccessor.ProcessShapeFile();
//	int old_numEdges = int(boost::num_edges(*g));
//	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));
//
//	std::vector<vertex_descriptor> parents1(numVertices_DR);
//	std::vector<double> distances1(numVertices_DR);
//	std::vector<vertex_descriptor> parents2(numVertices_DR);
//	std::vector<double> distances2(numVertices_DR);
//
//	std::map<int, double> dict_min_dist;
//	std::map<int, double> dict_real_min_dist;
//	std::map<int, double> new_dict_real_min_dist;
//	std::set<int> inRoad;
//	std::map<double, std::set<int>> validRoad;
//	//有效路准备转无向图需要的数据
//	std::map<int, int> newMapNodes;
//	std::map<int, std::vector<int>> newRoadNode;
//	std::map<std::string, int> newNodeToRoad;
//	std::map<int, std::vector<double>> partIn;
//
//	//预备无向图数据
//	Graph edgGraph_MD;
//	edgGraph_MD.copy_impl(*g);
//	int numEdges_MD = int(boost::num_edges(edgGraph_MD));
//	int numVertices_MD = int(boost::num_vertices(edgGraph_MD));
//
//	//准备接数据
//	std::vector<vertex_descriptor> parents0(numVertices_MD + 1);
//	std::vector<double> distances0(numVertices_MD + 1);
//	std::set<int> inNode;
//	std::set<int> outRoad;
//	std::vector<double> allIn;
//	std::vector<double> partIn2;
//	std::map<int, std::map<int, std::map<int, int>>> mdr_partInReachRoadNodeCoor;
//
//	//增加
//	std::set<double> last_allInRoads;		//前一个半径限制下的全覆盖路径ID序列
//
//	//对vector预配置内存空间
//	allIn.reserve(subRoadVec.size() * sizeof(double) + 100);
//	partIn2.reserve(subRoadVec.size() * sizeof(double) + 100);
//
//	//准备接道路数据
//	std::vector<vertex_descriptor> parents(numVertices_MD + 1);
//	std::vector<double> distances(numVertices_MD + 1);
//	std::vector<int> tmp{ 0,0 };
//
//	//增加
//	Graph subGraph;
//
//	extern bool NeedStop;
//	for (auto it = subRoadVec.begin(); it != subRoadVec.end(); it++)	//对每条起始边
//	{
//		if (NeedStop) {
//			return;
//		}
//
//		int startRoad = *it;
//		int startRoad2 = startRoad + old_numEdges;
//
//		//传入n组队列，输出n*m组终极有效队列
//		if (int(MRLimitSet2.size()) == 0)		//无里程限制：direction reach
//		{
//			//两次有向图搜索
//			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
//			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));
//
//			std::map<int, double>().swap(dict_min_dist);
//			for (int i = 0; i < numVertices_DR; i++)
//				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));
//
//			//收集DR有效的道路队列
//			std::map<double, std::set<int>>().swap(validRoad);
//			std::map<int, double>().swap(dict_real_min_dist);
//			for (int k = 0; k < old_numEdges; k++) {
//				//找出有向边i对应的反向边id
//				int reverse_k = k + old_numEdges;
//				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
//				dict_real_min_dist.insert(std::make_pair(k, min_d));
//
//				for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++) {
//					double DRLimit = *iter;
//
//					if (DRLimit == -1)
//						validRoad[DRLimit].insert(k);
//					if (dict_real_min_dist[k] <= DRLimit)
//						validRoad[DRLimit].insert(k);
//				}
//			}
//
//
//			//计算dr
//			for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
//				double DRLimit = *dr_iter;
//
//				double dr = 0;
//				for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++) {
//					dr += fileAccessor.Length[*it2];
//				}
//
//				//数据存储
//				DR_all[std::pair<double, double>(DRLimit, -1)][startRoad] = dr;
//
//				//计算Jnc
//				if (isJnc) {
//					int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
//					JncDR_all[std::pair<double, double>(DRLimit, -1)][startRoad] = jncDR;
//				}
//
//				//计算wgt
//				if (isWgt) {
//					double wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
//					wgtDR_all[std::pair<double, double>(DRLimit, -1)][startRoad] = wgtvalue;
//				}
//			}
//		}
//		else	 //转向限制+里程限制：combined reach
//		{
//			//对所有MRLimit参数组一轮算完
//			for (auto iter = MRLimitSet2.begin(); iter != MRLimitSet2.end(); iter++)
//			{
//				double MRLimit = *iter;
//
//				std::map<int, double>().swap(new_dict_real_min_dist);
//				double M_dd = 0, M_ddl = 0;
//				if (MRLimit == -1) {		//无需重构有向图				
//					boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
//					boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));
//
//					std::map<int, double>().swap(dict_min_dist);
//					for (int i = 0; i < numVertices_DR; i++)
//						dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));
//
//					//收集DR有效的道路队列
//					std::map<double, std::set<int>>().swap(validRoad);
//					std::map<int, double>().swap(dict_real_min_dist);
//					for (int k = 0; k < old_numEdges; k++) {
//						//找出有向边i对应的反向边id
//						int reverse_k = k + old_numEdges;
//						double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
//						dict_real_min_dist.insert(std::make_pair(k, min_d));
//
//						for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
//							double DRLimit = *dr_iter;
//
//							if (DRLimit == -1)
//								validRoad[DRLimit].insert(k);
//							if (dict_real_min_dist[k] <= DRLimit)
//								validRoad[DRLimit].insert(k);
//						}
//					}
//					new_dict_real_min_dist.insert(dict_real_min_dist.begin(), dict_real_min_dist.end());
//
//					//计算mdr
//					for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
//						double DRLimit = *dr_iter;
//
//						//计算mdr
//						double dr = 0;
//						for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++) {
//							dr += fileAccessor.Length[*it2];
//						}
//
//						//数据存储
//						DR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = dr;
//
//						//计算Jnc
//						if (isJnc) {
//							int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
//							JncDR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = jncDR;
//						}
//
//						//计算wgt
//						if (isWgt) {
//							double wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
//							wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = wgtvalue;
//						}
//					}
//				}
//				else {	//先做mr搜索
//					CalculateRoadsMRbyBFS(fileAccessor, distances, MRLimit, startRoad, outRoad, partIn, inRoad, partInRoads_all, partInReachRoadNodeLen,
//						mdr_partInReachRoadNodeCoor, partInRoads_all);
//
//					if (outRoad.size() == fileAccessor.roadID.size()) {
//						//两次有向图搜索
//						boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
//						boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));
//
//						std::map<int, double>().swap(dict_min_dist);
//						for (int i = 0; i < numVertices_DR; i++)
//							dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));
//
//						//收集DR有效的道路队列
//						std::map<double, std::set<int>>().swap(validRoad);
//						std::map<int, double>().swap(dict_real_min_dist);
//						for (int k = 0; k < old_numEdges; k++) {
//							//找出有向边i对应的反向边id
//							int reverse_k = k + old_numEdges;
//							double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
//							dict_real_min_dist.insert(std::make_pair(k, min_d));
//
//							for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
//								double DRLimit = *dr_iter;
//
//								if (DRLimit == -1)
//									validRoad[DRLimit].insert(k);
//								if (dict_real_min_dist[k] <= DRLimit)
//									validRoad[DRLimit].insert(k);
//							}
//						}
//						new_dict_real_min_dist.insert(dict_real_min_dist.begin(), dict_real_min_dist.end());
//
//						//计算mdr
//						for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
//							double DRLimit = *dr_iter;
//
//							//计算mdr
//							double dr = 0;
//							for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++) {
//								dr += fileAccessor.Length[*it2];
//							}
//
//							//数据存储
//							DR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = dr;
//
//							//计算Jnc
//							if (isJnc) {
//								int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
//								JncDR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = jncDR;
//							}
//
//							//计算wgt
//							if (isWgt) {
//								double wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
//								wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = wgtvalue;
//							}
//						}
//					}
//					else {	//需要重构有向图	
//						std::map<int, std::map<int, int>> partInNodeIDs;
//						std::map<std::tuple<int, int, int>, int> nodeToEdge;
//						std::map<int, std::tuple<int, int, int>> edgeToNode;
//
//						if (outRoad.size() == 1) {
//							new_dict_real_min_dist.clear();
//							new_dict_real_min_dist[startRoad] = 0;
//						}
//						else {
//							Generate_new_dict_real_min_dist_partIn(fileAccessor, new_dict_real_min_dist, dict_min_dist, outRoad, startRoad, inRoad, partInReachRoadNodeLen[startRoad],
//								old_numEdges, 2 * old_numEdges, parents1, distances1, parents2, distances2, partInNodeIDs, nodeToEdge, edgeToNode);
//						}
//
//						if (startRoad == 612)
//							bool check = true;
//
//						//收集DR有效的道路队列
//						std::map<double, std::set<int>>().swap(validRoad);
//						for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
//							double DRLimit = *dr_iter;
//
//							if (DRLimit == -1) {
//								validRoad[DRLimit] = outRoad;
//							}
//							else {
//								//收集DR有效的道路队列
//								std::map<int, std::map<int, int>> validPartInRoads;
//								for (auto it2 = outRoad.begin(); it2 != outRoad.end(); it2++) {
//									int road_id = *it2;
//									if (partInReachRoadNodeLen[startRoad].count(road_id)) {
//										if (partInNodeIDs.count(road_id) == 0)
//											continue;
//										for (auto node_it = partInNodeIDs[road_id].begin(); node_it != partInNodeIDs[road_id].end(); node_it++) {
//											int node = node_it->first;
//											//部分覆盖线条在方向上只有出去的份
//											int edge_id = partInNodeIDs[road_id][node];
//											if (new_dict_real_min_dist[edge_id] <= DRLimit) {
//												validRoad[DRLimit].insert(road_id);
//												validPartInRoads[road_id][node] = edge_id;
//											}
//										}
//									}
//									else {	//全覆盖
//										if (new_dict_real_min_dist[road_id] <= DRLimit)
//											validRoad[DRLimit].insert(road_id);
//									}
//								}
//								outRoad.clear();
//								outRoad.insert(validRoad[DRLimit].begin(), validRoad[DRLimit].end());
//
//								//更新部分覆盖的线条
//								std::map<int, std::vector<double>> tmp_partInRoadsCoor;
//								std::map<int, std::map<int, double>> tmp_partInNodeLen;
//								for (auto road_it = validPartInRoads.begin(); road_it != validPartInRoads.end(); road_it++) {
//									int road_id = road_it->first;
//									for (auto node_it = road_it->second.begin(); node_it != road_it->second.end(); node_it++) {
//										int node = node_it->first;
//										int pos = mdr_partInReachRoadNodeCoor[startRoad][road_id][node];
//										tmp_partInNodeLen[road_id][node] = partInReachRoadNodeLen[startRoad][road_id][node];
//										tmp_partInRoadsCoor[road_id].insert(tmp_partInRoadsCoor[road_id].end(),
//											partInRoads_all[startRoad][road_id].begin() + pos, partInRoads_all[startRoad][road_id].begin() + pos + 4);
//									}
//								}
//								partInRoads_all[startRoad].clear();
//								partInRoads_all[startRoad] = tmp_partInRoadsCoor;
//								partInReachRoadNodeLen[startRoad].clear();
//								partInReachRoadNodeLen[startRoad] = tmp_partInNodeLen;
//							}
//
//							//计算mdr
//							double dr = 0;
//							for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++) {
//								int road_id = *it2;
//								if (validRoad[DRLimit].size() == 1) {
//									dr += 2 * MRLimit;
//									continue;
//								}
//								
//								if (partInReachRoadNodeLen[startRoad].count(road_id)) {
//									for (auto node_it = partInReachRoadNodeLen[startRoad][road_id].begin(); node_it != partInReachRoadNodeLen[startRoad][road_id].end(); node_it++) {
//										int node = node_it->first;
//										double disflow = node_it->second;
//										dr += disflow;
//									}
//								}
//								else {
//									dr += fileAccessor.Length[road_id];
//								}
//							}
//							DR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = dr;
//
//							if (dr < 145.6)
//								bool check = true;
//
//							//计算Jnc
//							if (isJnc) {
//								int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
//								JncDR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = jncDR;
//							}
//
//							//计算wgt
//							if (isWgt) {
//								double wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
//								wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = wgtvalue;
//							}
//						}
//
//					}
//				}
//
//			}
//		}
//
//		finishedCount += 1;
//		addFinishedCount();
//	}
//
//	isFinished = true;
//}


void Calculation::calculateMDR(ShapeFileAccessor &fileAccessor) {
	isFinished = false;

	if (subRoadVec.size() == 0) {
		isFinished = true;
		return;
	}

	//创建有向图		
	Graph_d edgGraph_DR;
	GenerateDirectedGraph(fileAccessor, edgGraph_DR);
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;
	std::map<int, double> new_dict_real_min_dist;
	std::set<int> inRoad;
	std::map<double, std::set<int>> validRoad;
	//有效路准备转无向图需要的数据
	std::map<int, int> newMapNodes;
	std::map<int, std::vector<int>> newRoadNode;
	std::map<std::string, int> newNodeToRoad;
	std::map<int, std::vector<double>> partIn;

	//预备无向图数据
	Graph edgGraph_MD;
	edgGraph_MD.copy_impl(*g);
	int numEdges_MD = int(boost::num_edges(edgGraph_MD));
	int numVertices_MD = int(boost::num_vertices(edgGraph_MD));

	//准备接数据
	std::vector<vertex_descriptor> parents0(numVertices_MD + 1);
	std::vector<double> distances0(numVertices_MD + 1);
	std::set<int> inNode;
	std::set<int> outRoad;
	std::vector<double> allIn;
	std::vector<double> partIn2;
	std::map<int, std::map<int, std::map<int, int>>> mdr_partInReachRoadNodeCoor;

	//增加
	std::set<double> last_allInRoads;		//前一个半径限制下的全覆盖路径ID序列

	//准备接道路数据
	std::vector<vertex_descriptor> parents(numVertices_MD + 1);
	std::vector<double> distances(numVertices_MD + 1);
	std::vector<int> tmp{ 0,0 };

	//增加
	Graph subGraph;

	extern bool NeedStop;
	for (auto it = subRoadVec.begin(); it != subRoadVec.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;
		int startRoad2 = startRoad + old_numEdges;

		//传入n组队列，输出n*m组终极有效队列
		if (int(MRLimitSet2.size()) == 0)		//无里程限制：direction reach
		{
			CalculateRoadsDRbyBFS(fileAccessor, startRoad, DRLimitSet, dict_real_min_dist, validRoad);

			//计算dr
			for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
				double DRLimit = *dr_iter;

				double dr = 0;
				for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++) {
					dr += fileAccessor.Length[*it2];
				}

				//数据存储
				DR_all[std::pair<double, double>(DRLimit, -1)][startRoad] = dr;

				//计算Jnc
				if (isJnc) {
					int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
					JncDR_all[std::pair<double, double>(DRLimit, -1)][startRoad] = jncDR;
				}

				//计算wgt
				if (isWgt) {
					std::map<std::string, double> wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
					for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
						wgtDR_all[std::pair<double, double>(DRLimit, -1)][wgt_it->first][startRoad] = wgt_it->second;
					}
					
				}
			}
		}
		else	 //转向限制+里程限制：combined reach
		{
			//对所有MRLimit参数组一轮算完
			for (auto iter = MRLimitSet2.begin(); iter != MRLimitSet2.end(); iter++)
			{
				double MRLimit = *iter;

				std::map<int, double>().swap(new_dict_real_min_dist);
				double M_dd = 0, M_ddl = 0;
				if (MRLimit == -1) {		//无需重构有向图				

					CalculateRoadsDRbyBFS(fileAccessor, startRoad, DRLimitSet, dict_real_min_dist, validRoad);

					//计算mdr
					for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
						double DRLimit = *dr_iter;

						//计算mdr
						double dr = 0;
						for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++) {
							dr += fileAccessor.Length[*it2];
						}

						//数据存储
						DR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = dr;

						//计算Jnc
						if (isJnc) {
							int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
							JncDR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = jncDR;
						}

						//计算wgt
						if (isWgt) {
							std::map<std::string, double> wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
							for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
								wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][wgt_it->first][startRoad] = wgt_it->second;
							}
							
						}
					}
				}
				else {	//先做mr搜索
					CalculateRoadsMRbyBFS(fileAccessor, distances, MRLimit, startRoad, outRoad, partIn, inRoad, partInRoads_all, partInReachRoadNodeLen,
						mdr_partInReachRoadNodeCoor, partInRoads_all);

					if (outRoad.size() == fileAccessor.roadID.size()) {

						CalculateRoadsDRbyBFS(fileAccessor, startRoad, DRLimitSet, dict_real_min_dist, validRoad);

						//计算mdr
						for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
							double DRLimit = *dr_iter;

							//计算mdr
							double dr = 0;
							for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++) {
								dr += fileAccessor.Length[*it2];
							}

							//数据存储
							DR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = dr;

							//计算Jnc
							if (isJnc) {
								int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
								JncDR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = jncDR;
							}

							//计算wgt
							if (isWgt) {
								std::map<std::string, double> wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
								for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
									wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][wgt_it->first][startRoad] = wgt_it->second;
								}
							}
						}
					}
					else {	//需要重构有向图	
						if (outRoad.size() == 1) {
							for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
								double DRLimit = *dr_iter;
								validRoad[DRLimit] = outRoad;
							}
						}
						else {
							Generate_new_dict_real_min_dist_partIn_BFS(fileAccessor, outRoad, startRoad, inRoad, partInReachRoadNodeLen[startRoad], old_numEdges, DRLimitSet, validRoad);
						}

						for (auto dr_iter = DRLimitSet.begin(); dr_iter != DRLimitSet.end(); dr_iter++) {
							double DRLimit = *dr_iter;

							//计算mdr
							double dr = 0;
							for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++) {
								int road_id = *it2;
								if (validRoad[DRLimit].size() == 1) {
									dr += 2 * MRLimit;
									continue;
								}
								
								if (partInReachRoadNodeLen[startRoad].count(road_id)) {
									for (auto node_it = partInReachRoadNodeLen[startRoad][road_id].begin(); node_it != partInReachRoadNodeLen[startRoad][road_id].end(); node_it++) {
										int node = node_it->first;
										double disflow = node_it->second;
										dr += disflow;
									}
								}
								else {
									dr += fileAccessor.Length[road_id];
								}
							}
							DR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = dr;

							if (dr < 391)
								bool check = true;

							//计算Jnc
							if (isJnc) {
								int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
								JncDR_all[std::pair<double, double>(DRLimit, MRLimit)][startRoad] = jncDR;
							}

							//计算wgt
							if (isWgt) {
								std::map<std::string, double> wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
								for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
									wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][wgt_it->first][startRoad] = wgt_it->second;
								}
							}
						}

					}
				}

			}
		}

		finishedCount += 1;
		addFinishedCount();
	}

	isFinished = true;
}

//xlj 2023-10-09 添加calculateDRbyDijkstra
void Calculation::calculateDRbyDijkstraDeprecated(ShapeFileAccessor& fileAccessor) {
	isFinished = false;
	int count_repeat_in_q = 0;
	if (subRoadVec.size() == 0) {
		isFinished = true;
		return;
	}

	int total = fileAccessor.roadID.size();

	//数据结构
	std::map<double, std::set<int>> validRoad;
	std::map<int, std::set<int>> ddMap;

	double max_DRLimit = INT_MAX;
	for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++)
	{
		max_DRLimit = *iter;
	}

	for (auto cur_iter = subRoadVec.begin(); cur_iter != subRoadVec.end(); cur_iter++)	//对每条起始边
	{
		extern bool NeedStop;
		if (NeedStop) {
			return;
		}
		//无向路的id，因此startRoad代表一个方向，startRoad2代表另外一个方向。
		int startRoad = *cur_iter;

		//准备数据结构
		std::priority_queue<int,std::vector<int>,std::greater<int>> q; //优先队列，使得下一次的访问节点为最近的节点。
		std::vector<double> dist(2 * total, 0x7fffffff); //记录距离
		//初始化数据
		q.push(startRoad);
		dist[startRoad] = 0;
		while (!q.empty()) {
			int nowRoad = q.top();
			q.pop();
			//遍历nowRoad的所有邻居
			for (auto next_iter = fileAccessor.AdjRoadList[nowRoad].begin(); next_iter != fileAccessor.AdjRoadList[nowRoad].end(); next_iter++) {
				int connected_cursor = *next_iter;
				//排除掉nowRoad的反向路
				if (nowRoad % total == connected_cursor % total)
					continue;
				//如果更新了已经reach的线条，需要重新遍历
				int turn = fileAccessor.AdjTurnMP[nowRoad][connected_cursor];
				//debug消息
				//std::string output = "startRoad:"+std::to_string(startRoad)+"\n"+"nowRoad:" + std::to_string(nowRoad) + "\n" + "dist[nowRoad]:" + std::to_string(dist[nowRoad]) + "\n" + "turn:" + std::to_string(turn) + "\n" + "dist[connected_cursor]:" + std::to_string(dist[connected_cursor]) + "\n";
				//OutputDebugString(output.c_str());
				if (dist[nowRoad] + turn < dist[connected_cursor]) {
					//debug消息
					//std::string output = "DIJ更新dist的次数" + std::to_string(count_repeat_in_q++) + "\n";
					//OutputDebugString(output.c_str());
					dist[connected_cursor] = dist[nowRoad] + turn;
					//只有dist更新了，且在max_DRLimit之内的，才需要加入优先队列。
					if (dist[connected_cursor] <= max_DRLimit)
						q.push(connected_cursor);
				}
				
			}
		}
		//OutputDebugString("dist");
		//for (auto iter_dist : dist) {
		//	OutputDebugString((std::to_string(iter_dist) + "\n").c_str());
		//}
		//将起点设置为反向边计算最短路径
		int startRoad2 = startRoad + total;

		//准备数据结构
		std::priority_queue<int, std::vector<int>, std::greater<int>> q2; //优先队列，使得下一次的访问节点为最近的节点。
		std::vector<double> dist2(2 * total, 0x7fffffff); //记录距离
		//初始化数据
		q2.push(startRoad2);
		dist2[startRoad2] = 0;
		while (!q2.empty()) {
			int nowRoad = q2.top();
			q2.pop();
			//遍历nowRoad的所有邻居
			for (auto next_iter = fileAccessor.AdjRoadList[nowRoad].begin(); next_iter != fileAccessor.AdjRoadList[nowRoad].end(); next_iter++) {
				int connected_cursor = *next_iter;
				//排除掉nowRoad的反向路
				if (nowRoad % total == connected_cursor % total)
					continue;
				//如果更新了已经reach的线条，需要重新遍历
				int turn = fileAccessor.AdjTurnMP[nowRoad][connected_cursor];
				if (dist2[nowRoad] + turn < dist2[connected_cursor]) {
					//debug消息
					//std::string output = "DIJ更新dist的次数" + std::to_string(count_repeat_in_q++) + "\n";
					//OutputDebugString(output.c_str());
					dist2[connected_cursor] = dist2[nowRoad] + turn;
					//只有dist更新了，且在max_DRLimit之内的，才需要加入优先队列。
					if (dist2[connected_cursor] <= max_DRLimit)
						q2.push(connected_cursor);
				}

			}
		}

		//计算真正的最短路径
		validRoad.clear();
		for (int k = 0; k < 2 * total; k++) {
			dist[k] = min(dist[k], dist2[k]);
		}
		std::vector<double> dict_real_min_dist(total, INT_MAX);
		for (int k = 0; k < total; k++)
		{
			//找出有向边i对应的反向边id
			int reverse_k = k + total;
			double min_d = min(dist[k], dist[reverse_k]);
			dict_real_min_dist[k] = min_d;
			//这里的dist是每条边到起始边的距离，那么validRoad就是DRlimit下对于起始边的有效边。
			double DRLimit_before = -1;
			for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++) {
				double DRLimit_current = *iter;
				if (DRLimit_current == -1)
					validRoad[DRLimit_current].insert(k);
				//2023-10-10 XieLingjie 修改：validRoad只加入比起前一个参数增加的路
				if (dict_real_min_dist[k] > DRLimit_before && dict_real_min_dist[k] <= DRLimit_current) {
					validRoad[DRLimit_current].insert(k);
					break;
				}
				//不属于上一个区间
				DRLimit_before = DRLimit_current;
			}
		}

		//计算dr
		//2023-10-10 XieLingjie 修改：记录前一个参数下的dr，当前参数的dr=当前validRoad的和+上一个参数的dr
		double dr_before = 0;
		double jncDR_before = 0;
		std::map<std::string, double> wgtvalue_before;
		//初始化wgtvalue_before
		for (const auto& pair : weight) {
			const std::string& key = pair.first;
			wgtvalue_before[key] = 0;
		}

		for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++)
		{
			double DRLimit = *iter;
			double dr = 0;
			for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++)
			{
				//reach计算的是可达的路的总长度
				dr += fileAccessor.Length[*it2];
			}

			//2023-10-10 XieLingjie 修改：+上一个参数的dr,接着记录当前的dr为dr_before，为下一轮做准备
			//数据存储，DR参数、起始边对应一个dr。
			DR_all[std::pair<double, double>(DRLimit, -1)][startRoad] = dr + dr_before;
			dr_before += dr;

			//计算Jnc
			if (isJnc)
			{
				int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
				//2023-10-10 XieLingjie 修改：+上一个参数的jncDR
				JncDR_all[std::pair<double, double>(DRLimit, -1)][startRoad] = jncDR + jncDR_before;
				jncDR_before += jncDR;
			}

			//计算wgt
			if (isWgt)
			{
				std::map<std::string, double> wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
				for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
					//2023-10-10 XieLingjie 修改：+上一个参数的wgtvalue[key]
					wgtDR_all[std::pair<double, double>(DRLimit, -1)][wgt_it->first][startRoad] = wgt_it->second + wgtvalue_before[wgt_it->first];
					wgtvalue_before[wgt_it->first] += wgt_it->second;
				}

			}

			if (startRoad == 1)
				int check = 1;
		}

		finishedCount += 1;
		addFinishedCount();
	}

	isFinished = true;
}
void Calculation::calculateDR(ShapeFileAccessor &fileAccessor) {
	isFinished = false;
	int count_repeat_in_q = 0;
	if (subRoadVec.size() == 0) {
		isFinished = true;
		return;
	}

	int total = fileAccessor.roadID.size();
	DirectionDistanceFunction<int> directiondistfunc(fileAccessor);

	//数据结构
	std::map<double, std::set<int>> validRoad;
	std::map<int, std::set<int>> ddMap;

	double max_DRLimit = INT_MAX;
	for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++)
	{
		max_DRLimit = *iter;
	}

	for (auto cur_iter = subRoadVec.begin(); cur_iter != subRoadVec.end(); cur_iter++)	//对每条起始边
	{
		extern bool NeedStop;
		if (NeedStop) {
			return;
		}
		//无向路的id，因此startRoad代表一个方向，startRoad2代表另外一个方向。
		int startRoad = *cur_iter;

		//准备数据结构
		std::queue<int> q;
		std::vector<double> dist(2 * total, 0x7fffffff);
		std::vector<bool> visited(2 * total, false);

		getDistDijkstra(directiondistfunc, startRoad, max_DRLimit, dist);

		int startRoad2 = startRoad + total;

		//准备数据结构
		std::queue<int> q2;
		std::vector<double> dist2(2 * total, 0x7fffffff);
		std::vector<bool> visited2(2 * total, false);

		getDistDijkstra(directiondistfunc, startRoad2, max_DRLimit, dist2);
		
		//计算真正的最短路径
		validRoad.clear();
		for (int k = 0; k < 2 * total; k++) {
			dist[k] = min(dist[k], dist2[k]);
		}
		std::vector<double> dict_real_min_dist(total, INT_MAX);
		for (int k = 0; k < total; k++)
		{
			//找出有向边i对应的反向边id
			int reverse_k = k + total;
			double min_d = min(dist[k], dist[reverse_k]);
			dict_real_min_dist[k] = min_d;
			//这里的dist是每条边到起始边的距离，那么validRoad就是DRlimit下对于起始边的有效边。
			double DRLimit_before = -1;
			for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++){
				double DRLimit_current = *iter;
				if (DRLimit_current == -1)
					validRoad[DRLimit_current].insert(k);
				//2023-10-10 XieLingjie 修改：validRoad只加入比起前一个参数增加的路
				if (dict_real_min_dist[k] > DRLimit_before && dict_real_min_dist[k] <= DRLimit_current) {
					validRoad[DRLimit_current].insert(k);
					break;
				}
				//不属于上一个区间
				DRLimit_before = DRLimit_current;
			}
		}

		//计算dr
		//2023-10-10 XieLingjie 修改：记录前一个参数下的dr，当前参数的dr=当前validRoad的和+上一个参数的dr
		double dr_before=0;
		double jncDR_before = 0;
		std::map<std::string, double> wgtvalue_before;
		//初始化wgtvalue_before
		for (const auto& pair : weight) {
			const std::string& key = pair.first;
			wgtvalue_before[key] = 0;
		}

		for (auto iter = DRLimitSet.begin(); iter != DRLimitSet.end(); iter++)
		{
			double DRLimit = *iter;
			double dr = 0;
			for (auto it2 = validRoad[DRLimit].begin(); it2 != validRoad[DRLimit].end(); it2++)
			{
				//reach计算的是可达的路的总长度
				dr += fileAccessor.Length[*it2];
			}

			//2023-10-10 XieLingjie 修改：+上一个参数的dr,接着记录当前的dr为dr_before，为下一轮做准备
			//数据存储，DR参数、起始边对应一个dr。
			DR_all[std::pair<double, double>(DRLimit, -1)][startRoad] = dr + dr_before;
			dr_before += dr;

			//计算Jnc
			if (isJnc)
			{
				int jncDR = CalculateJnc(fileAccessor, validRoad[DRLimit]);
				//2023-10-10 XieLingjie 修改：+上一个参数的jncDR
				JncDR_all[std::pair<double, double>(DRLimit, -1)][startRoad] = jncDR + jncDR_before;
				jncDR_before += jncDR;
			}

			//计算wgt
			if (isWgt)
			{
				std::map<std::string, double> wgtvalue = CalculateWgt(weight, validRoad[DRLimit]);
				for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
					//2023-10-10 XieLingjie 修改：+上一个参数的wgtvalue[key]
					wgtDR_all[std::pair<double, double>(DRLimit, -1)][wgt_it->first][startRoad] = wgt_it->second + wgtvalue_before[wgt_it->first];
					wgtvalue_before[wgt_it->first] += wgt_it->second;
				}

			}

			if (startRoad == 1)
				int check = 1;
		}

		finishedCount += 1;
		addFinishedCount();
	}

	isFinished = true;
}

void Calculation::Net_calculateDR(ShapeFileAccessor &fileAccessor, double subDRLimit, double subMRLimit)
{
	isFinished = false;

	if (subFromIDVec.size() == 0) {
		isFinished = true;
		return;
	}

	//清空数据
	//reachRoad_all.clear();
	//partInRoads_all.clear();

	//Netreach数据
	std::map<std::string, Netreach>().swap(NetreachData);	//有不受限制的三种：MR、DR、JncR
	net_file_name = getOutFilePath();
	net_str = "DR";

	std::map<int, std::set<int>>().swap(reachRoad_all);
	std::map<int, std::map<int, double>>().swap(partInReachRoadLen);
	std::map<int, std::map<int, std::map<int, double>>>().swap(partInReachRoadNodeLen);
	std::map<int, std::map<int, std::vector<double>>>().swap(partInRoads_all);

	//创建有向图		
	Graph_d edgGraph_DR;
	GenerateDirectedGraph(fileAccessor, edgGraph_DR);
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;
	std::map<int, int> ddMap;
	std::set<int> inRoad;
	std::set<int> validRoad;
	std::set<int> lineRoads;

	//预备无向图数据
	Graph edgGraph_MD;
	edgGraph_MD.copy_impl(*g);
	int numEdges_MD = int(boost::num_edges(edgGraph_MD));
	int numVertices_MD = int(boost::num_vertices(edgGraph_MD));

	//准备接数据
	std::set<int> inNode;
	std::set<int> outRoad;
	std::map<int, std::vector<double>> partIn;

	//增加
	std::vector<int> tmp{ 0,0 };
	Graph subGraph;
	EdgeProperty ep1, ep2;

	//准备接道路数据
	std::vector<vertex_descriptor> parents0(numVertices_MD + 1);
	std::vector<double> distances0(numVertices_MD + 1);
	std::map<int, double> new_dict_real_min_dist;
	std::map<int, std::map<int, std::map<int, int>>> mdr_partInReachRoadNodeCoor;

	extern bool NeedStop;
	for (auto it = subFromIDVec.begin(); it != subFromIDVec.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;
		int startRoad2 = startRoad + old_numEdges;

		//传入n组队列，输出n*m组终极有效队列
		if (subMRLimit == -1)		//无里程限制
		{
			//两次有向图搜索
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

			std::map<int, double>().swap(dict_min_dist);
			for (int i = 0; i < numVertices_DR; i++)
				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

			//收集DR有效的道路队列
			validRoad.clear();
			std::map<int, double>().swap(dict_real_min_dist);
			for (int k = 0; k < old_numEdges; k++) {
				//找出有向边i对应的反向边id
				int reverse_k = k + old_numEdges;
				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				dict_real_min_dist.insert(std::make_pair(k, min_d));

				if (subDRLimit == -1)
					validRoad.insert(k);
				else if (dict_real_min_dist[k] <= subDRLimit)
					validRoad.insert(k);
			}

			if (isStepDepth)
			{
				for (int endRoad = 0; endRoad < old_numEdges; endRoad++) {
					distanceAll[endRoad].insert(dict_real_min_dist[endRoad]);
				}

				finishedCount += 1;
				continue;
			}

			reachRoad_all.insert(std::make_pair(startRoad, validRoad));
			for (auto it1 = validRoad.begin(); it1 != validRoad.end(); it1++)
			{
				if (NeedStop) {
					return;
				}

				int endRoad = *it1;

				NetreachData["MDR"].From_OID.push_back(startRoad);
				NetreachData["MDR"].To_OID.push_back(endRoad);
				NetreachData["MDR"].Radius.push_back(subMRLimit);
				NetreachData["MDR"].DirChg.push_back(subDRLimit);
				NetreachData["MDR"].JncLmt.push_back(0);

				//把经历的道路建一个队列
				getInRoads_DR(lineRoads, fileAccessor, startRoad, endRoad, parents1, distances1, parents2, distances2, old_numEdges);

				//根据获取到的道路队列，计算Relen、Jnc、ddl
				double len = CalculateRelen_Net(fileAccessor, lineRoads) - 0.5*fileAccessor.Length[startRoad];
				if (endRoad == startRoad)
					len = fileAccessor.Length[endRoad];
				
				double jnc = 0;
				std::map<std::string, double> wgtvalue;
				if (isJnc)
					jnc = CalculateJnc(fileAccessor, lineRoads);
				if (isWgt)
					wgtvalue = CalculateWgt(weight, lineRoads);

				NetreachData["MDR"].Relen.push_back(len);
				if (isJnc)
					NetreachData["MDR"].Jnc.push_back(jnc);
				if (isWgt)
					NetreachData["MDR"].Wgt.push_back(wgtvalue);
			}
		}
		else	 //转向限制+里程限制
		{
			//删除边-起始边
			double dis = fileAccessor.Length[startRoad] / 2;
			int startNode = numVertices_MD;
			int MD_node1 = fileAccessor.roadNode[startRoad][0];
			int MD_node2 = fileAccessor.roadNode[startRoad][1];
			boost::remove_edge(MD_node1, MD_node2, edgGraph_MD);

			//添加边-中点到起始边两端
			ep1.m_base = dis;
			ep1.m_value = startRoad;
			boost::add_edge(startNode, MD_node1, ep1, edgGraph_MD);

			ep2.m_base = dis;
			ep2.m_value = numEdges_MD;
			boost::add_edge(startNode, MD_node2, ep2, edgGraph_MD);

			boost::dijkstra_shortest_paths(edgGraph_MD, startNode, boost::predecessor_map(&parents0[0]).distance_map(&distances0[0]));

			std::set<int>().swap(inNode);
			std::set<int>().swap(inRoad);
			std::set<int>().swap(outRoad);
			std::map<int, std::vector<double>>().swap(partIn);
			partInReachRoadNodeLen.clear();

			//计算inNode：编号0-(顶点数目-1)
			for (int endNode = 0; endNode < numVertices_MD; endNode++){
				double tmpdis = distances0[endNode];
				double td = double(std::round(pow(10, 8)*tmpdis)) / double(pow(10, 8));
				if (subMRLimit > td)
					inNode.insert(endNode);
			}

			//判断是否能够冲出起始边
			if (2 * subMRLimit >= fileAccessor.Length[startRoad])
			{
				//将起始边加入allInLength
				inRoad.insert(startRoad);

				//全覆盖
				for (auto it1 = fileAccessor.roadID.begin(); it1 != fileAccessor.roadID.end(); it1++){
					int endRoad = *it1;
					if (endRoad == startRoad)	//跳过处理起始边
						continue;

					double roadLen = fileAccessor.Length[endRoad];
					int sub_md_node1 = fileAccessor.roadNode[endRoad][0];		//endrode的端点1
					int sub_md_node2 = fileAccessor.roadNode[endRoad][1];		//endrode的端点2

					// 如果是完全覆盖，则必须同时到达其两个端点
					if (inNode.count(sub_md_node1) != 0 && inNode.count(sub_md_node2) != 0)
					{
						double dist_res1 = subMRLimit - distances0[sub_md_node1];  // 端点1方向越过距离
						double dist_res2 = subMRLimit - distances0[sub_md_node2];  // 端点2方向越过距离
						double dist_res = dist_res1 + dist_res2;
						if (dist_res >= roadLen)   // 只有确定是完全越过，才会加入allIn
						{
							inRoad.insert(endRoad);
						}
					}
				}

				//部分覆盖
				for (auto it1 = fileAccessor.roadID.begin(); it1 != fileAccessor.roadID.end(); it1++){
					int endRoad = *it1;
					// 跳过全部覆盖的边
					if (inRoad.count(endRoad) != 0)
						continue;

					int sub_md_node1 = fileAccessor.roadNode[endRoad][0];		//endrode的端点1
					int sub_md_node2 = fileAccessor.roadNode[endRoad][1];		//endrode的端点2

					// 如果是部分覆盖，只要有一个端点有越过
					if (inNode.count(sub_md_node1) != 0 || inNode.count(sub_md_node2) != 0)
					{
						// 检查是否已到达边缘的起点
						if (inNode.count(sub_md_node1) != 0)
						{
							double dis_overflow = subMRLimit - distances0[sub_md_node1];
							partIn[endRoad].push_back(dis_overflow);
							partInReachRoadNodeLen[startRoad][endRoad][sub_md_node1] = dis_overflow;
							mdr_partInReachRoadNodeCoor[startRoad][endRoad][sub_md_node1] = partInRoads_all[startRoad][endRoad].size();

							//计算坐标
							double x1 = fileAccessor.Route[endRoad][0];
							double y1 = fileAccessor.Route[endRoad][1];
							double x2 = fileAccessor.Route[endRoad][2];
							double y2 = fileAccessor.Route[endRoad][3];
							double rate = dis_overflow / fileAccessor.Length[endRoad];

							double x = x1 + (x2 - x1)*rate;
							double y = y1 + (y2 - y1)*rate;

							partInRoads_all[startRoad][endRoad].push_back(x1);
							partInRoads_all[startRoad][endRoad].push_back(y1);
							partInRoads_all[startRoad][endRoad].push_back(x);
							partInRoads_all[startRoad][endRoad].push_back(y);
						}
						if (inNode.count(sub_md_node2) != 0)
						{
							double dis_overflow = subMRLimit - distances0[sub_md_node2];
							partIn[endRoad].push_back(dis_overflow);
							partInReachRoadNodeLen[startRoad][endRoad][sub_md_node2] = dis_overflow;
							mdr_partInReachRoadNodeCoor[startRoad][endRoad][sub_md_node2] = partInRoads_all[startRoad][endRoad].size();

							//计算坐标
							double x2 = fileAccessor.Route[endRoad][0];
							double y2 = fileAccessor.Route[endRoad][1];
							double x1 = fileAccessor.Route[endRoad][2];
							double y1 = fileAccessor.Route[endRoad][3];
							double rate = dis_overflow / fileAccessor.Length[endRoad];

							double x = x1 + (x2 - x1)*rate;
							double y = y1 + (y2 - y1)*rate;

							partInRoads_all[startRoad][endRoad].push_back(x1);
							partInRoads_all[startRoad][endRoad].push_back(y1);
							partInRoads_all[startRoad][endRoad].push_back(x);
							partInRoads_all[startRoad][endRoad].push_back(y);
						}

						outRoad.insert(endRoad);
					}
				}

				outRoad.insert(inRoad.begin(), inRoad.end());
			}
			else
			{
				outRoad.insert(startRoad);
				partIn[startRoad].push_back(2 * subMRLimit);

				//计算坐标
				double x1 = fileAccessor.Route[startRoad][0];
				double y1 = fileAccessor.Route[startRoad][1];
				double x2 = fileAccessor.Route[startRoad][2];
				double y2 = fileAccessor.Route[startRoad][3];
				double x_mid = (x1 + x2) / 2;
				double y_mid = (y1 + y2) / 2;

				int node1 = fileAccessor.roadNode[startRoad][0];		//endrode的端点1
				int node2 = fileAccessor.roadNode[startRoad][1];		//endrode的端点2

				double rate = 2 * subMRLimit / fileAccessor.Length[startRoad];

				double xn1 = fileAccessor.Route[startRoad][0];
				double yn1 = fileAccessor.Route[startRoad][1];
				double xn2 = fileAccessor.Route[startRoad][2];
				double yn2 = fileAccessor.Route[startRoad][3];
				double x_1 = x_mid + (xn1 - x_mid)*rate;
				double y_1 = y_mid + (yn1 - y_mid)*rate;
				double x_2 = x_mid + (xn2 - x_mid)*rate;
				double y_2 = y_mid + (yn2 - y_mid)*rate;

				partInRoads_all[startRoad][startRoad].push_back(x_1);
				partInRoads_all[startRoad][startRoad].push_back(y_1);
				partInRoads_all[startRoad][startRoad].push_back(x_2);
				partInRoads_all[startRoad][startRoad].push_back(y_2);
			}

			//准备数据结构
			std::map<int, std::map<int, int>> validPartInRoads;

			if (partInRoads_all[startRoad].size() == 1 && partInRoads_all[startRoad].count(startRoad)) {
				//do nothing
			}
			else {
				//开始dr搜索
				new_dict_real_min_dist.clear();
				//Generate_new_dict_real_min_dist(fileAccessor, new_dict_real_min_dist, dict_min_dist, outRoad, startRoad, old_numEdges, parents1, distances1, parents2, distances2);

				std::map<int, std::map<int, int>> partInNodeIDs;
				std::map<std::tuple<int, int, int>, int> nodeToEdge;
				std::map<int, std::tuple<int, int, int>> edgeToNode;
				Generate_new_dict_real_min_dist_partIn(fileAccessor, new_dict_real_min_dist, dict_min_dist, outRoad, startRoad, inRoad, partInReachRoadNodeLen[startRoad],
					old_numEdges, 2 * old_numEdges, parents1, distances1, parents2, distances2, partInNodeIDs, nodeToEdge, edgeToNode);

				//收集DR有效的道路队列
				if (subDRLimit != -1) {
					validRoad.clear();
					for (auto it2 = outRoad.begin(); it2 != outRoad.end(); it2++) {
						int road_id = *it2;
						if (partInReachRoadNodeLen[startRoad].count(road_id)) {
							if (partInNodeIDs.count(road_id) == 0)
								continue;
							for (auto node_it = partInNodeIDs[road_id].begin(); node_it != partInNodeIDs[road_id].end(); node_it++) {
								int node = node_it->first;
								//部分覆盖线条在方向上只有出去的份
								int edge_id = node_it->second;
								if (new_dict_real_min_dist[edge_id] <= subDRLimit) {
									validRoad.insert(road_id);
									validPartInRoads[road_id][node] = edge_id;
								}
							}
						}
						else {	//全覆盖
							if (new_dict_real_min_dist[road_id] <= subDRLimit)
								validRoad.insert(road_id);
						}
					}
					outRoad.clear();
					outRoad.insert(validRoad.begin(), validRoad.end());
				}
				//更新部分覆盖的线条
				std::map<int, std::vector<double>> tmp_partInRoadsCoor;
				std::map<int, std::map<int, double>> tmp_partInNodeLen;
				for (auto road_it = validPartInRoads.begin(); road_it != validPartInRoads.end(); road_it++) {
					int road_id = road_it->first;
					for (auto node_it = road_it->second.begin(); node_it != road_it->second.end(); node_it++) {
						int node = node_it->first;
						int pos = mdr_partInReachRoadNodeCoor[startRoad][road_id][node];
						tmp_partInNodeLen[road_id][node] = partInReachRoadNodeLen[startRoad][road_id][node];
						tmp_partInRoadsCoor[road_id].insert(tmp_partInRoadsCoor[road_id].end(),
							partInRoads_all[startRoad][road_id].begin() + pos, partInRoads_all[startRoad][road_id].begin() + pos + 4);
					}
				}
				partInRoads_all[startRoad].clear();
				partInRoads_all[startRoad] = tmp_partInRoadsCoor;
				partInReachRoadNodeLen[startRoad].clear();
				partInReachRoadNodeLen[startRoad] = tmp_partInNodeLen;
			}

			//可达线路
			reachRoad_all.insert(std::make_pair(startRoad, outRoad));

			for (auto it1 = outRoad.begin(); it1 != outRoad.end(); it1++)
			{
				if (NeedStop) {
					return;
				}

				int endRoad = *it1;

				int endnode = 0;
				int endnode1 = fileAccessor.roadNode[endRoad][0];		//endrode的新端点1
				int endnode2 = fileAccessor.roadNode[endRoad][1];		//endrode的新端点2

				//获取短的分支			
				if (distances0[endnode1] < distances0[endnode2])
					endnode = endnode1;
				else
					endnode = endnode2;

				if (partInRoads_all[startRoad].count(endRoad) == 0)  //到这条路是全覆盖通行
				{
					NetreachData["MDR"].From_OID.push_back(startRoad);
					NetreachData["MDR"].To_OID.push_back(endRoad);
					NetreachData["MDR"].Radius.push_back(subMRLimit);
					NetreachData["MDR"].DirChg.push_back(subDRLimit);
					NetreachData["MDR"].JncLmt.push_back(0);

					//把经历的道路建一个队列
					getInRoads_DR(lineRoads, fileAccessor, startRoad, endRoad, parents1, distances1, parents2, distances2, old_numEdges);

					//根据获取到的道路队列，计算Relen
					double len = CalculateRelen_Net(fileAccessor, lineRoads) - 0.5*fileAccessor.Length[startRoad];
					if (endRoad == startRoad)
						len = fileAccessor.Length[endRoad];
					
					double jnc = 0;
					std::map<std::string, double> wgtvalue;
					if (isJnc)
						jnc = CalculateJnc(fileAccessor, lineRoads);
					if (isWgt)
						wgtvalue = CalculateWgt(weight, lineRoads);

					NetreachData["MDR"].Relen.push_back(len);
					if (isJnc)
						NetreachData["MDR"].Jnc.push_back(jnc);
					if (isWgt)
						NetreachData["MDR"].Wgt.push_back(wgtvalue);
				}
				else{
					//需要区分从哪一端进入->找出起点到部分覆盖的两条路径，影响计算Relen、jnc、wgt
					if (endRoad == startRoad) {
						NetreachData["MDR"].From_OID.push_back(startRoad);
						NetreachData["MDR"].To_OID.push_back(endRoad);
						NetreachData["MDR"].Radius.push_back(subMRLimit);
						NetreachData["MDR"].DirChg.push_back(subDRLimit);
						NetreachData["MDR"].JncLmt.push_back(0);

						lineRoads.clear();
						lineRoads.insert(startRoad);
						double rate = subMRLimit / fileAccessor.Length[endRoad];

						double len = subMRLimit;
						double jnc = 0;
						std::map<std::string, double> wgtvalue;
						if (isJnc)
							jnc = CalculateJnc(fileAccessor, lineRoads);
						if (isWgt)
							wgtvalue = CalculateWgtPart(weight, lineRoads, startRoad, endRoad, rate);

						//收集部分通过路的通行距离，注意：可能存在起点能够从endRoad的两端进入，但是不接触
						partInReachRoadLen[startRoad][endRoad] = std::accumulate(partIn[endRoad].begin(), partIn[endRoad].end(), 0);	//部分通过的总线长度

						NetreachData["MDR"].Relen.push_back(len);
						if (isJnc)
							NetreachData["MDR"].Jnc.push_back(jnc);
						if (isWgt)
							NetreachData["MDR"].Wgt.push_back(wgtvalue);

						continue;
					}

					for (auto flow = partInReachRoadNodeLen[startRoad][endRoad].begin(); flow != partInReachRoadNodeLen[startRoad][endRoad].end(); flow++) {
						//转弯路径可达
						NetreachData["MDR"].From_OID.push_back(startRoad);
						NetreachData["MDR"].To_OID.push_back(endRoad);
						NetreachData["MDR"].Radius.push_back(subMRLimit);
						NetreachData["MDR"].DirChg.push_back(subDRLimit);
						NetreachData["MDR"].JncLmt.push_back(0);

						//把经历的道路建一个队列
						int node = flow->first;
						int edge_id = validPartInRoads[endRoad][node];
						double rate = flow->second / fileAccessor.Length[endRoad];
						getInRoads_DR_partIn(lineRoads, fileAccessor, startRoad, endRoad, edge_id, parents1, distances1, parents2, distances2, old_numEdges);

						double len = subMRLimit;
						double jnc = 0;
						std::map<std::string, double> wgtvalue;
						if (isJnc)
							jnc = CalculateJnc(fileAccessor, lineRoads);
						if (isWgt)
							wgtvalue = CalculateWgtPart(weight, lineRoads, startRoad, endRoad, rate);

						//收集部分通过路的通行距离，注意：可能存在起点能够从endRoad的两端进入，但是不接触
						partInReachRoadLen[startRoad][endRoad] = std::accumulate(partIn[endRoad].begin(), partIn[endRoad].end(), 0);	//部分通过的总线长度

						NetreachData["MDR"].Relen.push_back(len);
						if (isJnc)
							NetreachData["MDR"].Jnc.push_back(jnc);
						if (isWgt)
							NetreachData["MDR"].Wgt.push_back(wgtvalue);
					}
				}

			}
		}

		finishedCount += 1;
	}

	net_partInRoads_all.clear();
	net_partInRoads_all.insert(partInRoads_all.begin(), partInRoads_all.end());

	isFinished = true;
}

void SearchDRPathMR(ShapeFileAccessor &fileAccessor, std::set<int> &validRoads, std::set<int> &lineRoads, int startRoad, int EndRoad) {
	//清空旧数据
	lineRoads.clear();

	std::map<int, int> newMapNodes;
	std::map<int, std::vector<int>> newRoadNode;
	std::map<std::string, int> newNodeToRoad;
	std::vector<int> tmp{ 0,0 };

	for (auto it1 = validRoads.begin(); it1 != validRoads.end(); it1++)
	{
		int edge = *it1;

		newRoadNode.insert(std::make_pair(edge, tmp));

		for (int j = 0; j < 2; j++)
		{
			int key = fileAccessor.roadNode[edge][j];
			auto iter = newMapNodes.find(key);

			if (iter != newMapNodes.end()) {
				newRoadNode[edge][j] = newMapNodes[key];
			}
			else {
				int nodeNum = int(newMapNodes.size());
				newMapNodes.insert(std::pair<int, int>(key, nodeNum));
				newRoadNode[edge][j] = nodeNum;
			}
		}

		int sub_node1 = newRoadNode[edge][0];
		int sub_node2 = newRoadNode[edge][1];
		newNodeToRoad.insert(std::make_pair(IndexToKey(sub_node1, sub_node2), edge));
		newNodeToRoad.insert(std::make_pair(IndexToKey(sub_node2, sub_node1), edge));
	}

	EdgeProperty ep1, ep2, ep3;
	Graph subGraph;
	GenerateUndirectedGraph(fileAccessor, validRoads, newRoadNode, subGraph);

	int numEdges0 = int(boost::num_edges(subGraph));
	int numVertices0 = int(boost::num_vertices(subGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices0 + 1);
	std::vector<double> distances(numVertices0 + 1);

	//修改图：删一条，加两条			
	double dis = fileAccessor.Length[startRoad] / 2;
	int startNode = numVertices0;
	int sub_node1 = newRoadNode[startRoad][0];
	int sub_node2 = newRoadNode[startRoad][1];

	//删除边-起始边
	boost::remove_edge(sub_node1, sub_node2, subGraph);

	//添加边-中点到起始边两端
	ep1.m_base = dis;
	ep1.m_value = startRoad;
	boost::add_edge(startNode, sub_node1, ep1, subGraph);

	ep2.m_base = dis;
	ep2.m_value = numEdges0;
	boost::add_edge(startNode, sub_node2, ep2, subGraph);

	boost::dijkstra_shortest_paths(subGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

	//把经历的道路建一个队列
	std::vector<int> tmp_routeRoads;
	getInRoads_MDR(lineRoads, newRoadNode, newNodeToRoad, startRoad, EndRoad, startNode, parents, distances);
}

int dr_last_size = 0;
int dr_failed_count = 0;
int max_failed_count = 1000;

void SearchDRPath(ShapeFileAccessor &fileAccessor, std::map<int, double> &dict_real_min_dist, std::map<int, std::set<int>> &parents,
	std::set<std::vector<int>> &pathAll, std::vector<int> &path, int nowRoad, int startRoad) {
	if (nowRoad == startRoad) {
		std::vector<int> true_path(path);
		std::reverse(true_path.begin(), true_path.end());
		pathAll.insert(true_path);
		return;
	}

	//搜索范围过大，强制退出
	if (pathAll.size() > 1000)
		return;

	if (path.size() < dr_last_size)
		++dr_failed_count;
	dr_last_size = path.size();

	if (dr_failed_count > max_failed_count)
		return;

	if (nowRoad == 0)
		int check = 1;

	for (auto it = fileAccessor.Road_to_roads[nowRoad].begin(); it != fileAccessor.Road_to_roads[nowRoad].end(); it++) {
		int pre_road = *it;
		auto tt = std::find(path.begin(), path.end(), pre_road);
		if (tt != path.end())
			continue;

		//不允许掉头：特点是pre_road->nowRoad、nowRoad->nextRoad，它们的中转节点一致
		int same_node1 = INT_MIN, same_node2 = INT_MAX;
		int node1, node2, node3, node4;
		node3 = fileAccessor.roadNode[nowRoad][0];
		node4 = fileAccessor.roadNode[nowRoad][1];

		//nextRoad是nowRoad在path要过去的下一条道路，path搜索是尾插法，倒数第二是nextRoad
		if (path.size() > 1) {
			int nextRoad = path[path.size() - 2];
			node1 = fileAccessor.roadNode[nextRoad][0];
			node2 = fileAccessor.roadNode[nextRoad][1];
			same_node2 = node1;
			if (node2 == node3 || node2 == node4)
				same_node2 = node2;
		}

		//找出道路连接的节点号
		node1 = fileAccessor.roadNode[pre_road][0];
		node2 = fileAccessor.roadNode[pre_road][1];
		same_node1 = node1;
		if (node2 == node3 || node2 == node4)
			same_node1 = node2;

		//新的连接点必须不同
		if (same_node1 == same_node2)
			continue;

		//不允许搜索JnR时候，进入pre_road的前一条路是nowRoad，否则形成pre_road<->nowRoad死循环
		if (parents[pre_road].size() == 1 && *parents[pre_road].begin()== nowRoad) {
			continue;
		}

		//判断是不是合法的排在nowRoad前面的路：(1)next_road无转弯进入nowRoad;（2）next_road经过一个转弯进入nowRoad
		if (dict_real_min_dist[pre_road] == dict_real_min_dist[nowRoad] && fileAccessor.AdjTurnTP[pre_road][nowRoad] == 0) {
			path.push_back(pre_road);
			SearchDRPath(fileAccessor, dict_real_min_dist, parents, pathAll, path, pre_road, startRoad);
			path.pop_back();
		}
		else if (dict_real_min_dist[pre_road] == dict_real_min_dist[nowRoad] - 1 && fileAccessor.AdjTurnTP[pre_road][nowRoad] == 1) {
			path.push_back(pre_road);
			SearchDRPath(fileAccessor, dict_real_min_dist, parents, pathAll, path, pre_road, startRoad);
			path.pop_back();
		}
		else {	//剩下的是不合法情况，说明已经无路可走，这条path关闭
			//do nothing
		}
	}

}


void Calculation::Geo_calculateDR(ShapeFileAccessor &fileAccessor)
{
	isFinished = false;

	if (subFromIDVec.size() == 0) {
		isFinished = true;
		return;
	}

	//清空数据
	/*routeRoad_all.clear();
	partInRoads_all.clear();*/

	//Geodesics数据
	std::map<std::string, Geodesics>().swap(GeodesicsData);	//有不收限制的两种：MR、JncR，和收限制的一种：DR
	geo_file_name = getOutFilePath();
	geo_str = "DR";

	std::map<int, std::set<int>>().swap(routeRoad_all);
	//std::map<int, std::map<int, std::vector<double>>>().swap(partInRoads_all);
	std::map<int, std::map<std::string, double>>().swap(GeoDataCount);

	std::map<std::pair <int, int>, double>().swap(routeLen_all);
	std::map<std::pair <int, int>, std::vector<int>>().swap(routeRoad_allMap);

	//创建有向图		
	Graph_d edgGraph_DR;
	GenerateDirectedGraph(fileAccessor, edgGraph_DR);
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;
	std::map<int, int> ddMap;

	//无向图复制
	Graph edgGraph;
	edgGraph.copy_impl(*g);
	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);

	//增加
	EdgeProperty ep1, ep2, ep3;

	//记录转弯的祖先
	std::map<int, std::set<int>> parents_dr;
	std::map<int, std::set<int>> parents_DR;

	//准备接数据
	std::set<int> lineRoads;
	std::vector<int> routeRoads;
	std::vector<int> tmp_routeRoads;
	std::map<int, std::vector<double>> partIn;
	std::map<int, double> new_dict_real_min_dist;

	//对vector预配置内存空间
	//routeRoads.reserve(fileAccessor.Length.size() * sizeof(int) + 100);

	std::set<int> tmpFromSet;
	for (auto it = subFromIDVec.begin(); it != subFromIDVec.end(); it++)
	{
		tmpFromSet.insert(*it);
	}

	extern bool NeedStop;
	for (auto it = tmpFromSet.begin(); it != tmpFromSet.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;
		int startRoad2 = startRoad + old_numEdges;

		boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
		boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

		std::vector<int> tmp_dict_real_min_dist;
		if (isOneToOne == false)
		{
			//清空旧数据
			parents_dr.clear();
			parents_DR.clear();

			//dict_min_dist.clear();
			std::map<int, double>().swap(dict_min_dist);
			for (int i = 0; i < numVertices_DR; i++) {
				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));
				if (distances1[i] < distances2[i]) {
					parents_dr[i].insert(parents1[i] >= old_numEdges ? parents1[i] - old_numEdges : parents1[i]);
				}
				else if (distances1[i] > distances2[i]) {
					parents_dr[i].insert(parents2[i] >= old_numEdges ? parents2[i] - old_numEdges : parents2[i]);
				}
				else {
					parents_dr[i].insert(parents1[i] >= old_numEdges ? parents1[i] - old_numEdges : parents1[i]);
					parents_dr[i].insert(parents2[i] >= old_numEdges ? parents2[i] - old_numEdges : parents2[i]);
				}
			}

			//dict_real_min_dist.clear();
			std::map<int, double>().swap(dict_real_min_dist);
			for (int k = 0; k < old_numEdges; k++)
			{
				//找出有向边i对应的反向边id
				int reverse_k = k + old_numEdges;
				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				dict_real_min_dist.insert(std::make_pair(k, min_d));
				tmp_dict_real_min_dist.push_back(min_d);

				//记录祖先
				if (dict_min_dist[k] < dict_min_dist[reverse_k]) {
					parents_DR[k] = parents_dr[k];
				}
				else if (dict_min_dist[k] > dict_min_dist[reverse_k]) {
					parents_DR[k] = parents_dr[reverse_k];
				}
				else {
					parents_DR[k].insert(parents_dr[k].begin(), parents_dr[k].end());
					parents_DR[k].insert(parents_dr[reverse_k].begin(), parents_dr[reverse_k].end());
				}
			}
		}

		//准备Geo数据
		if (isOneToOne == false)
		{
			if (int(ToIDVec.size()) > 0)
			{
				std::vector<std::vector<int>> tmp_routeRoadsAll;
				std::set<int> lineRoadsAll;
				for (auto iter = ToIDVec.begin(); iter != ToIDVec.end(); iter++)
				{
					int EndRoad = *iter;

					//收集最小代价T下能到达的线条
					std::set<std::vector<int>> pathAll;
					std::vector<int> path = { EndRoad };

					int cost_sum = std::accumulate(tmp_dict_real_min_dist.begin(), tmp_dict_real_min_dist.end(), 0);
					if (cost_sum == 0) {	//如果JnC代价均为0，直接计算mr
						//修改图：删一条，加两条			
						double dis = fileAccessor.Length[startRoad] / 2;
						int startNode = numVertices;
						int node1 = fileAccessor.roadNode[startRoad][0];
						int node2 = fileAccessor.roadNode[startRoad][1];

						//删除边-起始边
						boost::remove_edge(node1, node2, edgGraph);

						//添加边-中点到起始边两端
						ep1.m_base = dis;
						ep1.m_value = startRoad;
						boost::add_edge(startNode, node1, ep1, edgGraph);

						ep2.m_base = dis;
						ep2.m_value = numEdges;
						boost::add_edge(startNode, node2, ep2, edgGraph);

						boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

						//把startRoad到EndRoad的最短路径找出来
						getInRoads_MR2(routeRoads, lineRoads, fileAccessor, startRoad, EndRoad, startNode, parents, distances);

						//把图撤销修改：删两条，加一条
						boost::remove_edge(startNode, node1, edgGraph);
						boost::remove_edge(startNode, node2, edgGraph);

						ep3.m_base = fileAccessor.Length[startRoad];
						ep3.m_value = startRoad;
						boost::add_edge(node1, node2, ep3, edgGraph);
					}
					else {	
						dr_last_size = 0;
						dr_failed_count = 0;
						SearchDRPath(fileAccessor, dict_real_min_dist, parents_DR, pathAll, path, EndRoad, startRoad);
						//判断有没有多条最短路径
						bool isMulti = false;
						routeRoads.clear();
						if (pathAll.size() > 1) {
							double min_len = 0x7fffffff;
							for (auto tt = pathAll.begin(); tt != pathAll.end(); tt++) {
								double len = 0;
								for (int k = 0; k < (*tt).size(); k++) len += fileAccessor.Length[(*tt)[k]];
								if (len < min_len) {
									min_len = len;
									routeRoads = *tt;
								}
							}
							isMulti = true;
						}
						else if (pathAll.size() == 1){
							auto tt = pathAll.begin();
							routeRoads = *tt;
						}
						else {
							//把经历的道路建一个队列
							getInRoads_DR2(routeRoads, lineRoads, fileAccessor, startRoad, EndRoad, parents1, distances1, parents2, distances2, old_numEdges);
						}

						lineRoads.insert(routeRoads.begin(), routeRoads.end());

						////收集代价小于终点线的道路
						//std::set<int> valid_roads;
						//double dc_limit = dict_real_min_dist[EndRoad];
						//for (int k = 0; k < old_numEdges; k++) {
						//	if (dict_real_min_dist[k] <= dc_limit)
						//		valid_roads.insert(k);
						//}

						////重新做mr搜索出一条到终点线的最短路径
						//SearchDRPathMR(fileAccessor, valid_roads, lineRoads, startRoad, EndRoad);
					}
					
					routeRoad_all[startRoad].insert(lineRoads.begin(), lineRoads.end());

					//计算整体的dc、dd、ddl、wdd、jnc、wgt
					int dc = 0, jnc = 0;
					double dd = 0, ddl = 0;
					std::map<std::string, double> wdd;
					std::map<std::string, double> wgtvalue;

					dc = CalculateDC(fileAccessor, routeRoads, old_numEdges);
					dd = CalculateRoadsDD(fileAccessor, dict_real_min_dist, lineRoads);
					ddl = CalculateRoadsDDL(fileAccessor, dict_real_min_dist, lineRoads, partIn, startRoad, old_numEdges);
					wdd = CalculateWDD(fileAccessor, weight, dict_real_min_dist, lineRoads);

					if (Jnc_t_limit_Jnc < INT_MAX - 10)
						jnc = CalculateJnc(fileAccessor, lineRoads);
					if (weight.size() > 0)
						wgtvalue = CalculateWgt(weight, lineRoads);

					GeoDataCount[startRoad]["DC"] = dc;
					GeoDataCount[startRoad]["DD"] = dd;
					GeoDataCount[startRoad]["DDL"] = ddl;
					//GeoDataCount[startRoad]["WDD"] = wdd;
					GeoDataCount[startRoad]["Jnc"] = jnc;
					//GeoDataCount[startRoad]["Wgt"] = wgt;

					//为Geodesics输出准备数据
					for (int pos=0;pos< routeRoads.size();pos++){
						int endRoad = routeRoads[pos];

						std::map<int, std::vector<double>>().swap(partIn);
						//加入起点的一半
						partIn[startRoad].push_back(0.5*fileAccessor.Length[startRoad]);
						//加入终点的一半
						partIn[endRoad].push_back(0.5*fileAccessor.Length[endRoad]);

						GeodesicsData["DR"].From_OID.push_back(startRoad);
						GeodesicsData["DR"].To_OID.push_back(endRoad);

						//把经历的道路建一个队列
						tmp_routeRoads.clear();
						lineRoads.clear();
						tmp_routeRoads.insert(tmp_routeRoads.begin(), routeRoads.begin(), routeRoads.begin() + pos + 1 );
						lineRoads.insert(tmp_routeRoads.begin(), tmp_routeRoads.end());

						std::map<std::string, double> wdd;
						std::map<std::string, double> wgtvalue;

						//根据获取到的道路队列，计算Relen、Jnc、ddl
						double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endRoad);
						int dc = CalculateDC(fileAccessor, tmp_routeRoads, old_numEdges);
						double dd = CalculateRoadsDD(fileAccessor, dict_real_min_dist, lineRoads);
						double ddl = CalculateRoadsDDL(fileAccessor, dict_real_min_dist, lineRoads, partIn, startRoad, old_numEdges);
						wdd = CalculateWDD(fileAccessor, weight, dict_real_min_dist, lineRoads);
						
						int jnc = 0;
						double wgt = 0;

						if (Jnc_t_limit_Jnc < INT_MAX - 10)
							jnc = CalculateJnc(fileAccessor, lineRoads);
						if (weight.size() > 0)
							wgtvalue = CalculateWgt(weight, lineRoads);

						GeodesicsData["DR"].PathLen.push_back(len);
						GeodesicsData["DR"].DC.push_back(dc);
						GeodesicsData["DR"].DD.push_back(dd);
						GeodesicsData["DR"].DDL.push_back(ddl);
						GeodesicsData["DR"].WDD.push_back(wdd);
						if (Jnc_t_limit_Jnc < INT_MAX - 10)
							GeodesicsData["DR"].Jnc.push_back(jnc);
						if (weight.size() > 0)
							GeodesicsData["DR"].Wgt.push_back(wgtvalue);
					}
				}
			}
			else
			{
				for (auto iter = fileAccessor.roadID.begin(); iter != fileAccessor.roadID.end(); iter++)
				{
					if (dict_real_min_dist[*iter] == 0)
					{
						int endroad = *iter;

						std::map<int, std::vector<double>>().swap(partIn);
						//加入起点的一半
						partIn[startRoad].push_back(0.5*fileAccessor.Length[startRoad]);
						//加入终点的一半
						partIn[endroad].push_back(0.5*fileAccessor.Length[endroad]);

						routeRoad_all[startRoad].insert(endroad);

						GeodesicsData["DR"].From_OID.push_back(startRoad);
						GeodesicsData["DR"].To_OID.push_back(endroad);

						//把经历的道路建一个队列
						getInRoads_DR2(tmp_routeRoads, lineRoads, fileAccessor, startRoad, endroad, parents1, distances1, parents2, distances2, old_numEdges);

						std::map<int, double>().swap(new_dict_real_min_dist);
						Generate_new_dict_real_min_dist(fileAccessor, new_dict_real_min_dist, dict_min_dist, lineRoads, startRoad, old_numEdges, parents1, distances1, parents2, distances2);

						//根据获取到的道路队列，计算Relen、Jnc、ddl
						double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endroad);
						int dc = CalculateDC(fileAccessor, tmp_routeRoads, old_numEdges);
						double dd = CalculateRoadsDD(fileAccessor, new_dict_real_min_dist, lineRoads);
						double ddl = CalculateRoadsDDL(fileAccessor, new_dict_real_min_dist, lineRoads, partIn, startRoad, old_numEdges);
						std::map<std::string, double> wdd = CalculateWDD(fileAccessor, weight, new_dict_real_min_dist, lineRoads);
						int jnc = CalculateJnc(fileAccessor, lineRoads);
						std::map<std::string, double> wgt = CalculateWgt(weight, lineRoads);

						GeodesicsData["DR"].PathLen.push_back(len);
						GeodesicsData["DR"].DC.push_back(dc);
						GeodesicsData["DR"].DD.push_back(dd);
						GeodesicsData["DR"].DDL.push_back(ddl);
						GeodesicsData["DR"].WDD.push_back(wdd);
						GeodesicsData["DR"].Jnc.push_back(jnc);
						GeodesicsData["DR"].Wgt.push_back(wgt);
					}
				}

				//计算整体的dc、dd、ddl、wdd、jnc、wgt
				int dc = 0;
				for (auto tt = fileAccessor.Dc.begin(); tt != fileAccessor.Dc.end(); tt++) {
					dc += tt->second;
				}
				double dd = CalculateRoadsDD(fileAccessor, dict_real_min_dist, fileAccessor.roadID);
				double ddl = CalculateRoadsDDL(fileAccessor, dict_real_min_dist, fileAccessor.roadID, partIn, startRoad, old_numEdges);
				std::map<std::string, double> wdd = CalculateWDD(fileAccessor, weight, dict_real_min_dist, fileAccessor.roadID);
				int jnc = 0;
				for (auto tt = fileAccessor.Jnc.begin(); tt != fileAccessor.Jnc.end(); tt++) {
					jnc += tt->second;
				}
				std::map<std::string, double> wgt = CalculateWgt(weight, fileAccessor.roadID);

				GeoDataCount[startRoad]["DC"] = dc;
				GeoDataCount[startRoad]["DD"] = dd;
				GeoDataCount[startRoad]["DDL"] = ddl;
				//GeoDataCount[startRoad]["WDD"] = wdd;
				GeoDataCount[startRoad]["Jnc"] = jnc;
				//GeoDataCount[startRoad]["Wgt"] = wgt;
			}
		}
		else
		{
			for (auto iter = FromToMap[startRoad].begin(); iter != FromToMap[startRoad].end(); iter++)
			{
				int EndRoad = *iter;

				getInRoads_DR2(routeRoads, lineRoads, fileAccessor, startRoad, EndRoad, parents1, distances1, parents2, distances2, old_numEdges);

				double len = CalculateRelen(fileAccessor, lineRoads, startRoad, EndRoad);
				routeLen_all.insert(std::make_pair(std::pair<int, int>(startRoad, EndRoad), len));

				if (outputPath)
				{
					//保存路径ID序列等待输出
					routeRoad_allMap.insert(std::make_pair(std::pair<int, int>(startRoad, EndRoad), routeRoads));

					routeRoad_all[startRoad].insert(lineRoads.begin(), lineRoads.end());

				}

				////为Geodesics输出准备数据
				//for (auto it3 = routeRoads.begin(); it3 != routeRoads.end(); it3++)
				//{
				//	int endRoad = *it3;

				//	GeodesicsData["DR"].From_OID.push_back(startRoad);
				//	GeodesicsData["DR"].To_OID.push_back(endRoad);
				//	GeodesicsData["DR"].Radius.push_back(0);

				//	//把经历的道路建一个队列
				//	getInRoads_DR(lineRoads, fileAccessor, startRoad, endRoad, parents1, distances1, parents2, distances2, old_numEdges);

				//	//根据获取到的道路队列，计算Relen、Jnc、ddl
				//	double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endRoad);
				//	double dd = CalculateRoadsDD(fileAccessor, dict_real_min_dist, lineRoads);
				//	double ddl = CalculateRoadsDDL(fileAccessor, dict_real_min_dist, lineRoads);
				//	int jnc = CalculateJnc(fileAccessor, lineRoads);
				//	double wgt = CalculateWgt(weight, lineRoads);

				//	//保存最短路径长度
				//	if (endRoad == EndRoad)
				//		routeLen_all.insert(std::make_pair(std::pair<int, int>(startRoad, EndRoad), len));

				//	GeodesicsData["DR"].PathLen.push_back(len);
				//	GeodesicsData["DR"].DD.push_back(dd);
				//	GeodesicsData["DR"].DDL.push_back(ddl);
				//	GeodesicsData["DR"].Jnc.push_back(jnc);
				//	GeodesicsData["DR"].Wgt.push_back(wgt);
				//}
			}
		}

		finishedCount += 1;
	}

	isFinished = true;
}

//计算所有的JncR、JncDDL
void Calculation::calculateJncR(ShapeFileAccessor &fileAccessor)
{
	//待测试
	calculateJncRbyBFS(fileAccessor);
	return;

}

void Calculation::calculateJncD(ShapeFileAccessor &fileAccessor) {
	isFinished = false;

	if (subRoadVec.size() == 0) {
		isFinished = true;
		return;
	}

	int total = fileAccessor.roadID.size();

	//预备无向图数据
	Graph *g = fileAccessor.ProcessShapeFile();
	int numEdges_MD = int(boost::num_edges(*g));
	int numVertices_MD = int(boost::num_vertices(*g));

	//交叉口无向图
	Graph edgGraph3;
	int old_numEdges = int(boost::num_edges(*g));
	GenerateUndirectedGraph_Jnc(fileAccessor, old_numEdges, edgGraph3);
	int numVertices3 = int(boost::num_vertices(edgGraph3));
	int numEdges3 = int(boost::num_edges(edgGraph3));
	std::vector<vertex_descriptor> parents3(numVertices3);
	std::vector<double> distances3(numVertices3);

	//准备接数据
	std::set<int> outRoad;
	std::set<int> inRoad;
	std::vector<double> allIn;
	std::map<int, std::vector<double>> partIn;
	std::map<int, std::map<int, std::map<int, int>>> mdr_partInReachRoadNodeCoor;

	//准备接道路数据
	std::vector<vertex_descriptor> parents(numVertices_MD + 1);
	std::vector<double> distances(numVertices_MD + 1);

	//增加
	Graph subGraph;

	for (auto cur_iter = subRoadVec.begin(); cur_iter != subRoadVec.end(); cur_iter++)	//对每条起始边
	{
		extern bool NeedStop;
		if (NeedStop) {
			return;
		}

		int startRoad = *cur_iter;

		//计算里程限制下的通行路径
		if (int(MRLimitSet2.size()) == 0) {		//无里程限制
			outRoad = fileAccessor.roadID;

			//计算到每条终点线的交叉口代价
			boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

			//计算通行路径的交叉口代价
			//double jncDR = CalculateJnc(fileAccessor, outRoad);
			double jD = CalculateJD_All(fileAccessor, distances3, fileAccessor.roadID, startRoad);
			JncDR_all[std::pair<double, double>(-1, -1)][startRoad] = jD;

			double jncDR = CalculateJncD_All(fileAccessor, distances3, fileAccessor.roadID, startRoad);
			JD_all[std::pair<double, double>(-1, -1)][startRoad] = jD;

			//计算wgt
			if (isWgt) {
				std::map<std::string, double> wgtvalue = CalculateWgt(weight, outRoad);
				for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
					wgtDR_all[std::pair<double, double>(-1, -1)][wgt_it->first][startRoad] = wgt_it->second;
				}
				
			}
		}
		else {
			//对所有MRLimit参数组一轮算完
			for (auto iter = MRLimitSet2.begin(); iter != MRLimitSet2.end(); iter++)
			{
				double MRLimit = *iter;

				double jD = 0, jncDR = 0;

				if (MRLimit == -1) {
					outRoad = fileAccessor.roadID;

					//计算到每条终点线的交叉口代价
					boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

					//计算通行路径的交叉口代价
					jncDR = CalculateJncD_All(fileAccessor, distances3, fileAccessor.roadID, startRoad);

					jD = CalculateJD_All(fileAccessor, distances3, fileAccessor.roadID, startRoad);
				}
				else {
					CalculateRoadsMRbyBFS(fileAccessor, distances, MRLimit, startRoad, outRoad, partIn, inRoad, partInRoads_all, partInReachRoadNodeLen,
						mdr_partInReachRoadNodeCoor, partInRoads_all);

					if (outRoad.size() == 1) {
						jncDR = 0;
						jD = 0;
					}
					else {
						//交叉口无向图
						Graph edgGraph4;
						GenerateUndirectedGraph_Jnc_Part(fileAccessor, outRoad, old_numEdges, edgGraph4);
						int numVertices4 = int(boost::num_vertices(edgGraph3));
						int numEdges4 = int(boost::num_edges(edgGraph4));
						std::vector<vertex_descriptor> parents4(numVertices4);
						std::vector<double> distances4(numVertices4);

						//计算到每条终点线的交叉口代价
						boost::dijkstra_shortest_paths(edgGraph4, startRoad, boost::predecessor_map(&parents4[0]).distance_map(&distances4[0]));

						//计算通行路径的交叉口代价
						jncDR = CalculateJncD_Part(fileAccessor, distances4, outRoad, partIn, startRoad);
						jD = CalculateJD_Part(fileAccessor, distances4, outRoad, partIn, startRoad);
					}
					
				}

				//计算通行路径的交叉口代价
				JncDR_all[std::pair<double, double>(-1, MRLimit)][startRoad] = jncDR;
				JD_all[std::pair<double, double>(-1, MRLimit)][startRoad] = jD;

				//计算wgt
				if (isWgt) {
					std::map<std::string, double> wgtvalue = CalculateWgt(weight, outRoad);
					for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
						wgtDR_all[std::pair<double, double>(-1, MRLimit)][wgt_it->first][startRoad] = wgt_it->second;
					}

				}
			}
		}

		finishedCount += 1;
		addFinishedCount();
	}

	isFinished = true;
}

//计算所有的JncR、JncDDL
void Calculation::calculateJncRbyBFS(ShapeFileAccessor &fileAccessor) {
	isFinished = false;

	if (subRoadVec.size() == 0) {
		isFinished = true;
		return;
	}

	int total = fileAccessor.roadID.size();
	std::set<int> outRoad;;

	std::map<double, double> Jnc_ddlSum, Jnc_lenSum, Jnc_ddl;
	std::map<double, double> Jnc_dd, Jnc_ddSum1, Jnc_ddSum2;
	std::map<double, std::map<std::string, double>> Jnc_wdd, Jnc_wddSum1, Jnc_wddSum2;
	std::map<double, std::map<int, std::set<int>>> ddMap;

	for (auto cur_iter = subRoadVec.begin(); cur_iter != subRoadVec.end(); cur_iter++)	//对每条起始边
	{
		extern bool NeedStop;
		if (NeedStop) {
			return;
		}

		int startRoad = *cur_iter;

		for (auto iter = Jnc_maxNumSet.begin(); iter != Jnc_maxNumSet.end(); iter++)
		{
			double Jnc_maxNum = *iter;

			//准备数据结构
			std::queue<int> q;
			std::vector<double> dist(total, 0x7fffffff);
			std::vector<bool> visited(total, false);
			
			//初始化数据
			q.push(startRoad);
			double rootseglength = fileAccessor.Length[startRoad];
			dist[startRoad] = 0;

			//BFS计算到连接线段的最短距离
			while (!q.empty()) {
				int nowRoad = q.front();
				q.pop();

				if (visited[nowRoad])
					continue;
				visited[nowRoad] = true;

				for (auto next_iter = fileAccessor.Road_to_roads[nowRoad].begin(); next_iter != fileAccessor.Road_to_roads[nowRoad].end(); next_iter++) {
					int connected_cursor = *next_iter;
					int same_node1 = INT_MAX;
					int node1, node2, node3, node4;
					node1 = fileAccessor.roadNode[nowRoad][0];
					node2 = fileAccessor.roadNode[nowRoad][1];
					node3 = fileAccessor.roadNode[connected_cursor][0];
					node4 = fileAccessor.roadNode[connected_cursor][1];
					same_node1 = node1;
					if (node2 == node3 || node2 == node4)
						same_node1 = node2;
					int length = fileAccessor.Jnc[same_node1];
					
					if (connected_cursor == startRoad)
						continue;

					//如果更新了已经reach的线条，需要重新遍历
					if (dist[nowRoad] + length < dist[connected_cursor]) {
						visited[connected_cursor] = false;
						dist[connected_cursor] = dist[nowRoad] + length;
					}
					if (dist[connected_cursor] <= Jnc_maxNum)
						q.push(connected_cursor);
				}
			}

			//BFS收集全覆盖
			std::queue<int> qq;
			std::vector<bool> visited2(total, false);
			qq.push(startRoad);
			double mr = 0;
			outRoad.clear();

			while (!qq.empty()) {
				int nowRoad = qq.front();
				qq.pop();

				if (visited2[nowRoad])
					continue;

				double len = fileAccessor.Length[nowRoad];
				mr += len;
				outRoad.insert(nowRoad);
				visited2[nowRoad] = true;

				for (auto next_iter = fileAccessor.Road_to_roads[nowRoad].begin(); next_iter != fileAccessor.Road_to_roads[nowRoad].end(); next_iter++) {
					double length = fileAccessor.Length[*next_iter];
					int connected_cursor = *next_iter;
					if (connected_cursor == startRoad)
						continue;

					if (dist[connected_cursor] <= Jnc_maxNum) {
						qq.push(connected_cursor);
					}	
				}
			}
			JncR_all[Jnc_maxNum][startRoad] = mr;

			if (isJncDDL) {
				//根据通行线条，重新构建有向图
				std::set<double> DRLimitSet;
				std::map<int, double> dict_real_min_dist;
				std::map<double, std::set<int>> validRoad;

				if (outRoad.size() == fileAccessor.roadID.size()) {
					CalculateRoadsDRbyBFS(fileAccessor, startRoad, DRLimitSet, dict_real_min_dist, validRoad);
				}
				else {
					Generate_new_dict_real_min_dist_BFS(fileAccessor, outRoad, startRoad, outRoad, dict_real_min_dist, fileAccessor.roadID.size(), DRLimitSet, validRoad);
				}

				//初始化
				Jnc_ddlSum[Jnc_maxNum] = 0, Jnc_lenSum, Jnc_ddl[Jnc_maxNum] = 0;
				Jnc_dd[Jnc_maxNum] = 0, Jnc_ddSum1[Jnc_maxNum] = 0, Jnc_ddSum2[Jnc_maxNum] = 0;
				Jnc_wdd[Jnc_maxNum].clear(), Jnc_wddSum1[Jnc_maxNum].clear(), Jnc_wddSum2[Jnc_maxNum].clear();

				for (auto road_it= outRoad.begin();road_it!= outRoad.end();road_it++)
				{
					int endRoad = *road_it;
					ddMap[Jnc_maxNum][int(dict_real_min_dist[endRoad])].insert(endRoad);
					Jnc_ddlSum[Jnc_maxNum] += dict_real_min_dist[endRoad] * fileAccessor.Length[endRoad];
					Jnc_lenSum[Jnc_maxNum] += fileAccessor.Length[endRoad];
				}

				Jnc_ddl[Jnc_maxNum] = Jnc_ddlSum[Jnc_maxNum] / Jnc_lenSum[Jnc_maxNum];

				for (auto it3 = ddMap[Jnc_maxNum].begin(); it3 != ddMap[Jnc_maxNum].end(); it3++)
				{
					Jnc_ddSum1[Jnc_maxNum] += ((it3->first)*(it3->second.size()));
					Jnc_ddSum2[Jnc_maxNum] += it3->second.size();
				}
				Jnc_dd[Jnc_maxNum] = Jnc_ddSum1[Jnc_maxNum] / Jnc_ddSum2[Jnc_maxNum];

				if (isWgt)
				{
					for (auto it3 = ddMap[Jnc_maxNum].begin(); it3 != ddMap[Jnc_maxNum].end(); it3++)
					{
						std::map<std::string, double> sub_wgt = CalculateWgt(weight, it3->second);
						for (auto wgt_it = sub_wgt.begin(); wgt_it != sub_wgt.end(); wgt_it++) {
							Jnc_wddSum1[Jnc_maxNum][wgt_it->first] += ((it3->first)*(wgt_it->second));
						}
					}
					Jnc_wddSum2[Jnc_maxNum] = CalculateWgt(weight, outRoad);

					for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
						if (Jnc_wddSum2[Jnc_maxNum][wgt_it->first] == 0)
							Jnc_wdd[Jnc_maxNum][wgt_it->first] = 0;
						else
							Jnc_wdd[Jnc_maxNum][wgt_it->first] = Jnc_wddSum1[Jnc_maxNum][wgt_it->first] / Jnc_wddSum2[Jnc_maxNum][wgt_it->first];
					}
					
				}

				JncDD_all[Jnc_maxNum][startRoad] = Jnc_dd[Jnc_maxNum];
				JncDDL_all[Jnc_maxNum][startRoad] = Jnc_ddl[Jnc_maxNum];

				if (isWgt)
				{
					for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
						JncWDD_all[Jnc_maxNum][wgt_it->first][startRoad] = Jnc_wdd[Jnc_maxNum][wgt_it->first];
					}
					
				}
			}

			//计算wgt
			if (isWgt)
			{
				std::map<std::string, double> wgtvalue = CalculateWgt(weight, outRoad);
				for (auto wgt_it = wgtvalue.begin(); wgt_it != wgtvalue.end(); wgt_it++) {
					wgtJncR_all[Jnc_maxNum][wgt_it->first][startRoad] = wgt_it->second;
				}
				
			}
		}

		finishedCount += 1;
	}

	isFinished = true;
}

void Calculation::Net_calculateJncR(ShapeFileAccessor &fileAccessor, double subJnc_maxNum)
{
	isFinished = false;

	//logOut << GetNowTime() << ", " << "----------------- Net_calculateJncR start ------------------\n";
	//logOut << GetNowTime() << ", " << "Jnc_maxNum:" << subJnc_maxNum << ", Jnc_t_limit_Jnc:" << Jnc_t_limit_JncR << std::endl;

	if (subFromIDVec.size() == 0) {
		//logOut << GetNowTime() << ", " << "----------------- Net_calculateJncR Over ------------------\n";
		isFinished = true;
		return;
	}

	//清空数据
	//reachRoad_all.clear();
	//partInRoads_all.clear();

	//Netreach数据
	std::map<std::string, Netreach>().swap(NetreachData);	//有不受限制的三种：MR、DR、JncR
	net_file_name = getOutFilePath();
	net_str = "JnR";

	std::map<int, std::set<int>>().swap(reachRoad_all);
	std::map<int, std::map<int, double>>().swap(partInReachRoadLen);
	std::map<int, std::map<int, std::map<int, double>>>().swap(partInReachRoadNodeLen);
	std::map<int, std::map<int, std::vector<double>>>().swap(partInRoads_all);

	//创建交叉口的新图
	Graph edgGraph3;
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	GenerateUndirectedGraph_Jnc(fileAccessor, old_numEdges, edgGraph3);
	int numVertices3 = int(boost::num_vertices(edgGraph3));
	int numEdges3 = int(boost::num_edges(edgGraph3));

	std::vector<vertex_descriptor> parents3(numVertices3);
	std::vector<double> distances3(numVertices3);

	std::set<int> validRoad;
	std::set<int> lineRoads;

	extern bool NeedStop;
	for (auto it = subFromIDVec.begin(); it != subFromIDVec.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;

		//logOut << GetNowTime() << ", " << "startRoad:" << startRoad << std::endl;

		boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

		if (isStepDepth)
		{
			for (int endRoad = 0; endRoad < numVertices3; endRoad++){
				distanceAll[endRoad].insert(distances3[endRoad]);
			}

			finishedCount += 1;
			continue;
		}

		double Jnc_maxNum = subJnc_maxNum;

		//validRoad.clear();
		std::set<int>().swap(validRoad);
		for (int endRoad = 0; endRoad < numVertices3; endRoad++)
		{
			if (distances3[endRoad] <= Jnc_maxNum)
			{
				validRoad.insert(endRoad);
			}
		}

		//计算Net所需数据
		reachRoad_all[startRoad] = validRoad;
		//logOut << GetNowTime() << ", " << "reachRoad_all size:" << validRoad.size() << std::endl;
		for (auto it1 = validRoad.begin(); it1 != validRoad.end(); it1++)
		{
			if (NeedStop) {
				return;
			}

			int endRoad = *it1;

			NetreachData["JncR"].From_OID.push_back(startRoad);
			NetreachData["JncR"].To_OID.push_back(endRoad);
			NetreachData["JncR"].Radius.push_back(0);
			NetreachData["JncR"].DirChg.push_back(0);
			NetreachData["JncR"].JncLmt.push_back(Jnc_maxNum);

			//把经历的道路建一个队列
			getInRoads_JncR(lineRoads, fileAccessor, startRoad, endRoad, parents3, distances3);

			//根据获取到的道路队列，计算Relen、Jnc、wgt
			double len = CalculateRelen_Net(fileAccessor, lineRoads);
			double jnc = 0;
			std::map<std::string, double> wgtvalue;
			if (isJnc)
				jnc = CalculateJnc(fileAccessor, lineRoads);
			if (isWgt)
				wgtvalue = CalculateWgt(weight, lineRoads);

			NetreachData["JncR"].Relen.push_back(len);
			if (isJnc)
				NetreachData["JncR"].Jnc.push_back(jnc);
			if (isWgt)
				NetreachData["JncR"].Wgt.push_back(wgtvalue);
		}

		finishedCount += 1;
	}

	net_partInRoads_all.clear();
	net_partInRoads_all.insert(partInRoads_all.begin(), partInRoads_all.end());

	//logOut << GetNowTime() << ", " << "----------------- Net_calculateJncR Over ------------------\n";
	isFinished = true;
	return;
}

int jnr_last_size = 0;
int jnr_failed_count = 0;

void SearchJnRPath(ShapeFileAccessor &fileAccessor, std::vector<double> &dict_real_min_dist, std::vector<vertex_descriptor> &parents, 
	std::set<std::vector<int>> &pathAll, std::vector<int> &path, int nowRoad, int startRoad) {
	if (nowRoad == startRoad) {
		std::vector<int> true_path(path);
		std::reverse(true_path.begin(), true_path.end());
		pathAll.insert(true_path);
		return;
	}

	//搜索范围过大，强制退出
	if (pathAll.size() > 1000)
		return;

	if (path.size() < jnr_last_size)
		++jnr_failed_count;
	jnr_last_size = path.size();

	if (jnr_failed_count > max_failed_count)
		return;

	//从尾向头搜索
	for (auto it = fileAccessor.Road_to_roads[nowRoad].begin(); it != fileAccessor.Road_to_roads[nowRoad].end(); it++) {
		int pre_road = *it;
		auto tt = std::find(path.begin(), path.end(), pre_road);
		if (tt != path.end())	//不允许path出现重复road
			continue;

		//不允许掉头：特点是pre_road->nowRoad、nowRoad->nextRoad，它们的中转节点一致
		int same_node1 = INT_MIN, same_node2 = INT_MAX;
		int node1, node2, node3, node4;
		node3 = fileAccessor.roadNode[nowRoad][0];
		node4 = fileAccessor.roadNode[nowRoad][1];

		//nextRoad是nowRoad在path要过去的下一条道路，path搜索是尾插法，倒数第二是nextRoad
		if (path.size() > 1) {
			int nextRoad = path[path.size()-2];
			node1 = fileAccessor.roadNode[nextRoad][0];
			node2 = fileAccessor.roadNode[nextRoad][1];
			same_node2 = node1;
			if (node2 == node3 || node2 == node4)
				same_node2 = node2;
		}
		
		//找出道路连接的节点号
		node1 = fileAccessor.roadNode[pre_road][0];
		node2 = fileAccessor.roadNode[pre_road][1];
		same_node1 = node1;
		if (node2 == node3 || node2 == node4)
			same_node1 = node2;
		
		//新的连接点必须不同
		if (same_node1 == same_node2)
			continue;

		//不允许搜索JnR时候，进入pre_road的前一条路是nowRoad，否则形成pre_road<->nowRoad死循环
		if (parents[pre_road] == nowRoad)
			continue;

		//判断是不是合法的排在nowRoad前面的路：(1)pre_road无转弯进入nowRoad;（2）pre_road经过一个转弯进入nowRoad
		if (dict_real_min_dist[pre_road] == dict_real_min_dist[nowRoad] && fileAccessor.Jnc[same_node1] == 0) {
			path.push_back(pre_road);
			SearchJnRPath(fileAccessor, dict_real_min_dist, parents, pathAll, path, pre_road, startRoad);
			path.pop_back();
		}
		else if (dict_real_min_dist[pre_road] == dict_real_min_dist[nowRoad] - 1 && fileAccessor.Jnc[same_node1] == 1) {
			path.push_back(pre_road);
			SearchJnRPath(fileAccessor, dict_real_min_dist, parents, pathAll, path, pre_road, startRoad);
			path.pop_back();
		}
		else {	//剩下的是不合法情况，说明已经无路可走，这条path关闭
			//do nothing
		}
	}
}

void Calculation::Geo_calculateJncR(ShapeFileAccessor &fileAccessor)
{
	isFinished = false;

	if (subFromIDVec.size() == 0) {
		isFinished = true;
		return;
	}

	//清空数据
	/*routeRoad_all.clear();
	partInRoads_all.clear();*/

	//Geodesics数据
	std::map<std::string, Geodesics>().swap(GeodesicsData);	//有不收限制的两种：MR、JncR，和收限制的一种：DR
	geo_file_name = getOutFilePath();
	geo_str = "JnR";

	std::map<int, std::set<int>>().swap(routeRoad_all);
	//std::map<int, std::map<int, std::vector<double>>>().swap(partInRoads_all);
	std::map<int, std::map<std::string, double>>().swap(GeoDataCount);

	std::map<std::pair <int, int>, double>().swap(routeLen_all);
	std::map<std::pair <int, int>, std::vector<int>>().swap(routeRoad_allMap);

	//创建有向图		
	Graph_d edgGraph_DR;
	if (angleLimitDR < INT_MAX - 10) {
		GenerateDirectedGraph(fileAccessor, edgGraph_DR);
	}
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;
	std::map<int, int> ddMap;

	//创建交叉口的新图
	Graph edgGraph3;
	GenerateUndirectedGraph_Jnc(fileAccessor, old_numEdges, edgGraph3);
	int numVertices3 = int(boost::num_vertices(edgGraph3));
	int numEdges3 = int(boost::num_edges(edgGraph3));

	std::vector<vertex_descriptor> parents3(numVertices3);
	std::vector<double> distances3(numVertices3);

	std::set<int> validRoad;
	std::set<int> lineRoads;
	std::vector<int> routeRoads;
	std::vector<int> tmp_routeRoads;
	std::map<int, std::vector<double>> partIn;

	std::map<int, double> new_dict_real_min_dist;

	//无向图复制
	Graph edgGraph;
	edgGraph.copy_impl(*g);
	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);

	//增加
	EdgeProperty ep1, ep2, ep3;

	//对vector预配置内存空间
	//routeRoads.reserve(fileAccessor.Length.size() * sizeof(int) + 100);

	std::set<int> tmpFromSet;
	for (auto it = subFromIDVec.begin(); it != subFromIDVec.end(); it++)
	{
		tmpFromSet.insert(*it);
	}

	extern bool NeedStop;
	for (auto it = tmpFromSet.begin(); it != tmpFromSet.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;

		boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

		if (isOneToOne == false && angleLimitDR < INT_MAX - 10)
		{
			//创建有向图
			int startRoad2 = startRoad + old_numEdges;

			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

			//dict_min_dist.clear();
			std::map<int, double>().swap(dict_min_dist);
			for (int i = 0; i < numVertices_DR; i++)
				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

			//dict_real_min_dist.clear();
			std::map<int, double>().swap(dict_real_min_dist);
			for (int k = 0; k < old_numEdges; k++)
			{
				//找出有向边i对应的反向边id
				int reverse_k = k + old_numEdges;
				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				dict_real_min_dist.insert(std::make_pair(k, min_d));
			}
		}

		//准备Geo数据
		if (isOneToOne == false)
		{
			if (int(ToIDVec.size()) > 0)
			{
				std::vector<std::vector<int>> tmp_routeRoadsAll;
				std::set<int> lineRoadsAll;
				for (auto iter = ToIDVec.begin(); iter != ToIDVec.end(); iter++)
				{
					int EndRoad = *iter;

					//收集最小代价T下能到达的线条
					std::set<std::vector<int>> pathAll;
					std::vector<int> path = { EndRoad };
					
					int cost_sum = std::accumulate(distances3.begin(), distances3.end(), 0);
					if (cost_sum == 0) {	//如果JnC代价均为0，直接计算mr
						//修改图：删一条，加两条			
						double dis = fileAccessor.Length[startRoad] / 2;
						int startNode = numVertices;
						int node1 = fileAccessor.roadNode[startRoad][0];
						int node2 = fileAccessor.roadNode[startRoad][1];

						//删除边-起始边
						boost::remove_edge(node1, node2, edgGraph);

						//添加边-中点到起始边两端
						ep1.m_base = dis;
						ep1.m_value = startRoad;
						boost::add_edge(startNode, node1, ep1, edgGraph);

						ep2.m_base = dis;
						ep2.m_value = numEdges;
						boost::add_edge(startNode, node2, ep2, edgGraph);

						boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

						//把startRoad到EndRoad的最短路径找出来
						getInRoads_MR2(routeRoads, lineRoads, fileAccessor, startRoad, EndRoad, startNode, parents, distances);

						//把图撤销修改：删两条，加一条
						boost::remove_edge(startNode, node1, edgGraph);
						boost::remove_edge(startNode, node2, edgGraph);

						ep3.m_base = fileAccessor.Length[startRoad];
						ep3.m_value = startRoad;
						boost::add_edge(node1, node2, ep3, edgGraph);
					}
					else {
						jnr_last_size = 0;
						jnr_failed_count = 0;
						SearchJnRPath(fileAccessor, distances3, parents3, pathAll, path, EndRoad, startRoad);

						//判断有没有多条最短路径
						bool isMulti = false;
						routeRoads.clear();
						if (pathAll.size() > 1) {
							double min_len = 0x7fffffff;
							for (auto tt = pathAll.begin(); tt != pathAll.end(); tt++) {
								double len = 0;
								for (int k = 0; k < (*tt).size(); k++) len += fileAccessor.Length[(*tt)[k]];
								if (len < min_len) {
									min_len = len;
									routeRoads = *tt;
								}
							}
							isMulti = true;
						}
						else if (pathAll.size() == 1) {
							auto tt = pathAll.begin();
							routeRoads = *tt;
						}
						else {
							//把经历的道路建一个队列
							getInRoads_JncR2(routeRoads, lineRoads, fileAccessor, startRoad, EndRoad, parents3, distances3);
						}
						lineRoads.insert(routeRoads.begin(), routeRoads.end());

						////收集代价小于终点线的道路
						//std::set<int> valid_roads;
						//double dc_limit = distances3[EndRoad];
						//for (int k = 0; k < old_numEdges; k++) {
						//	if (dict_real_min_dist[k] <= dc_limit)
						//		valid_roads.insert(k);
						//}

						////重新做mr搜索出一条到终点线的最短路径
						//SearchDRPathMR(fileAccessor, valid_roads, lineRoads, startRoad, EndRoad);
					}
					
					routeRoad_all[startRoad].insert(lineRoads.begin(), lineRoads.end());

					//计算整体的dc、dd、ddl、wdd、jnc、wgt
					int dc = 0, jnc = 0;
					double dd = 0, ddl = 0;

					if (angleLimitDR < INT_MAX - 10) {
						dc = CalculateDC(fileAccessor, routeRoads, old_numEdges);

						std::map<int, double>().swap(new_dict_real_min_dist);
						Generate_new_dict_real_min_dist(fileAccessor, new_dict_real_min_dist, dict_min_dist, lineRoads, startRoad, old_numEdges, parents1, distances1, parents2, distances2);

						dd = CalculateRoadsDD(fileAccessor, new_dict_real_min_dist, lineRoads);
						ddl = CalculateRoadsDDL(fileAccessor, new_dict_real_min_dist, lineRoads, partIn, startRoad, old_numEdges);
						//wdd = CalculateWDD(fileAccessor, weight, new_dict_real_min_dist, lineRoads);
					}
					if (Jnc_t_limit_Jnc < INT_MAX - 10)
						jnc = CalculateJnc(fileAccessor, lineRoads);
					if (weight.size() > 0)
						//wgt = CalculateWgt(weight, lineRoads);

					if (angleLimitDR < INT_MAX - 10) {
						GeoDataCount[startRoad]["DC"] = dc;
						GeoDataCount[startRoad]["DD"] = dd;
						GeoDataCount[startRoad]["DDL"] = ddl;
						//GeoDataCount[startRoad]["WDD"] = wdd;
					}
					if (Jnc_t_limit_Jnc < INT_MAX - 10)
						GeoDataCount[startRoad]["Jnc"] = jnc;
					//if (weight.size() > 0)
						//GeoDataCount[startRoad]["Wgt"] = wgt;

					//为Geodesics输出准备数据
					for (int pos = 0; pos < routeRoads.size(); pos++) {
						int endRoad = routeRoads[pos];

						std::map<int, std::vector<double>>().swap(partIn);
						//加入起点的一半
						partIn[startRoad].push_back(0.5*fileAccessor.Length[startRoad]);
						//加入终点的一半
						partIn[endRoad].push_back(0.5*fileAccessor.Length[endRoad]);

						GeodesicsData["JncR"].From_OID.push_back(startRoad);
						GeodesicsData["JncR"].To_OID.push_back(endRoad);

						//把经历的道路建一个队列
						tmp_routeRoads.clear();
						lineRoads.clear();
						tmp_routeRoads.insert(tmp_routeRoads.begin(), routeRoads.begin(), routeRoads.begin() + pos + 1);
						lineRoads.insert(tmp_routeRoads.begin(), tmp_routeRoads.end());

						//根据获取到的道路队列，计算Relen、Jnc、ddl
						double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endRoad);

						int dc = 0, jnc = 0;
						double dd = 0, ddl = 0;
						std::map<std::string, double> wdd;
						std::map<std::string, double> wgt;

						if (angleLimitDR < INT_MAX - 10) {
							dc = CalculateDC(fileAccessor, tmp_routeRoads, old_numEdges);

							std::map<int, double>().swap(new_dict_real_min_dist);
							Generate_new_dict_real_min_dist(fileAccessor, new_dict_real_min_dist, dict_min_dist,lineRoads, startRoad, old_numEdges, parents1, distances1, parents2, distances2);

							dd = CalculateRoadsDD(fileAccessor, new_dict_real_min_dist, lineRoads);
							ddl = CalculateRoadsDDL(fileAccessor, new_dict_real_min_dist, lineRoads, partIn, startRoad, old_numEdges);
							wdd = CalculateWDD(fileAccessor, weight, new_dict_real_min_dist, lineRoads);
						}
						if (Jnc_t_limit_Jnc < INT_MAX - 10)
							jnc = CalculateJnc(fileAccessor, lineRoads);
						if (weight.size() > 0)
							wgt = CalculateWgt(weight, lineRoads);

						GeodesicsData["JncR"].PathLen.push_back(len);
						if (angleLimitDR < INT_MAX - 10) {
							GeodesicsData["JncR"].DC.push_back(dc);
							GeodesicsData["JncR"].DD.push_back(dd);
							GeodesicsData["JncR"].DDL.push_back(ddl);
							GeodesicsData["JncR"].WDD.push_back(wdd);
						}
						if (Jnc_t_limit_Jnc < INT_MAX - 10)
							GeodesicsData["JncR"].Jnc.push_back(jnc);
						if (weight.size() > 0)
							GeodesicsData["JncR"].Wgt.push_back(wgt);
					}
				}
			}
			else
			{
				for (auto iter = fileAccessor.roadID.begin(); iter != fileAccessor.roadID.end(); iter++)
				{
					if (dict_real_min_dist[*iter] == 0)
					{
						int endroad = *iter;

						std::map<int, std::vector<double>>().swap(partIn);
						//加入起点的一半
						partIn[startRoad].push_back(0.5*fileAccessor.Length[startRoad]);
						//加入终点的一半
						partIn[endroad].push_back(0.5*fileAccessor.Length[endroad]);

						routeRoad_all[startRoad].insert(endroad);

						GeodesicsData["JncR"].From_OID.push_back(startRoad);
						GeodesicsData["JncR"].To_OID.push_back(endroad);

						//把经历的道路建一个队列
						getInRoads_JncR2(tmp_routeRoads, lineRoads, fileAccessor, startRoad, endroad, parents3, distances3);

						std::map<int, double>().swap(new_dict_real_min_dist);
						Generate_new_dict_real_min_dist(fileAccessor, new_dict_real_min_dist, dict_min_dist,lineRoads, startRoad, old_numEdges, parents1, distances1, parents2, distances2);

						//根据获取到的道路队列，计算Relen、Jnc、ddl
						double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endroad);
						int dc = CalculateDC(fileAccessor, tmp_routeRoads, old_numEdges);
						double dd = CalculateRoadsDD(fileAccessor, new_dict_real_min_dist, lineRoads);
						double ddl = CalculateRoadsDDL(fileAccessor, new_dict_real_min_dist, lineRoads, partIn, startRoad, old_numEdges);
						std::map<std::string, double> wdd = CalculateWDD(fileAccessor, weight, new_dict_real_min_dist, lineRoads);
						int jnc = CalculateJnc(fileAccessor, lineRoads);
						std::map<std::string, double> wgt = CalculateWgt(weight, lineRoads);

						GeodesicsData["JncR"].PathLen.push_back(len);
						GeodesicsData["JncR"].DC.push_back(dc);
						GeodesicsData["JncR"].DD.push_back(dd);
						GeodesicsData["JncR"].DDL.push_back(ddl);
						GeodesicsData["JncR"].WDD.push_back(wdd);
						GeodesicsData["JncR"].Jnc.push_back(jnc);
						GeodesicsData["JncR"].Wgt.push_back(wgt);
					}
				}

				//计算整体的dc、dd、ddl、wdd、jnc、wgt
				int dc = 0;
				for (auto tt = fileAccessor.Dc.begin(); tt != fileAccessor.Dc.end(); tt++) {
					dc += tt->second;
				}
				double dd = CalculateRoadsDD(fileAccessor, dict_real_min_dist, fileAccessor.roadID);
				double ddl = CalculateRoadsDDL(fileAccessor, dict_real_min_dist, fileAccessor.roadID, partIn, startRoad, old_numEdges);
				//double wdd = CalculateWDD(fileAccessor, weight, dict_real_min_dist, fileAccessor.roadID);
				int jnc = 0;
				for (auto tt = fileAccessor.Jnc.begin(); tt != fileAccessor.Jnc.end(); tt++) {
					jnc += tt->second;
				}
				//double wgt = CalculateWgt(weight, fileAccessor.roadID);

				GeoDataCount[startRoad]["DC"] = dc;
				GeoDataCount[startRoad]["DD"] = dd;
				GeoDataCount[startRoad]["DDL"] = ddl;
				//GeoDataCount[startRoad]["WDD"] = wdd;
				GeoDataCount[startRoad]["Jnc"] = jnc;
				//GeoDataCount[startRoad]["Wgt"] = wgt;
			}
		}
		else
		{
			for (auto iter = FromToMap[startRoad].begin(); iter != FromToMap[startRoad].end(); iter++)
			{
				int EndRoad = *iter;

				getInRoads_JncR2(routeRoads, lineRoads, fileAccessor, startRoad, EndRoad, parents3, distances3);

				double len = CalculateRelen(fileAccessor, lineRoads, startRoad, EndRoad);
				routeLen_all.insert(std::make_pair(std::pair<int, int>(startRoad, EndRoad), len));

				if (outputPath)
				{
					//保存路径ID序列等待输出
					routeRoad_allMap.insert(std::make_pair(std::pair<int, int>(startRoad, EndRoad), routeRoads));

					routeRoad_all[startRoad].insert(lineRoads.begin(), lineRoads.end());

				}

				////为Geodesics输出准备数据
				//for (auto it3 = routeRoads.begin(); it3 != routeRoads.end(); it3++)
				//{
				//	int endRoad = *it3;

				//	GeodesicsData["JncR"].From_OID.push_back(startRoad);
				//	GeodesicsData["JncR"].To_OID.push_back(endRoad);
				//	GeodesicsData["JncR"].Radius.push_back(0);

				//	//把经历的道路建一个队列
				//	getInRoads_JncR(lineRoads, fileAccessor, startRoad, endRoad, parents3, distances3);

				//	//根据获取到的道路队列，计算Relen、Jnc、ddl
				//	double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endRoad);
				//	double dd = CalculateRoadsDD(fileAccessor, dict_real_min_dist, lineRoads);
				//	double ddl = CalculateRoadsDDL(fileAccessor, dict_real_min_dist, lineRoads);
				//	int jnc = CalculateJnc(fileAccessor, lineRoads);
				//	double wgt = CalculateWgt(weight, lineRoads);

				//	//保存最短路径长度
				//	if (endRoad == EndRoad)
				//		routeLen_all.insert(std::make_pair(std::pair<int, int>(startRoad, EndRoad), len));

				//	GeodesicsData["JncR"].PathLen.push_back(len);
				//	GeodesicsData["JncR"].DD.push_back(dd);
				//	GeodesicsData["JncR"].DDL.push_back(ddl);
				//	GeodesicsData["JncR"].Jnc.push_back(jnc);
				//	GeodesicsData["JncR"].Wgt.push_back(wgt);
				//}
			}
		}


		finishedCount += 1;
	}

	isFinished = true;
}

void Calculation::modifyDBF2(std::map<int, int> &Result, std::string outPath, std::string fieldName)
{
	DBFHandle	hDBF;
	hDBF = DBFOpen(outPath.c_str(), "rb+");

	if (hDBF == NULL)
		return;

	if (int(Result.size()) == 0)
		return;

	//查找该列所在索引是否存在
	int fieldIndex = DBFGetFieldIndex(hDBF, fieldName.c_str());
	if (fieldIndex == -1)
	{
		//检查field名称是否超过10个字符，若超出，则斩除超出部分的字符
		if (fieldName.length() > 10)
			fieldName = fieldName.substr(0, 10);

		//添加表列
		DBFAddField(hDBF, fieldName.c_str(), FTInteger, 20, 6);
		DBFClose(hDBF);
		hDBF = DBFOpen(outPath.c_str(), "rb+");
		fieldIndex = DBFGetFieldIndex(hDBF, fieldName.c_str());
	}

	//添加数据
	for (auto it = Result.begin(); it != Result.end(); it++)
	{
		DBFWriteIntegerAttribute(hDBF, it->first, fieldIndex, it->second);
	}

	DBFClose(hDBF);
}

void Calculation::RenameField(std::string dbfFilePath, std::string old_name, std::string new_name) {
	DBFHandle	hDBF;
	hDBF = DBFOpen(dbfFilePath.c_str(), "rb+");

	if (hDBF == NULL)
		return;

	//查找该列所在索引是否存在
	int fieldIndex = DBFGetFieldIndex(hDBF, old_name.c_str());
	if (fieldIndex != -1) {
		DBFAlterFieldDefn(hDBF, fieldIndex, new_name.c_str(), 'F', 20, 6);
	}
		
	DBFClose(hDBF);
	

}

void Calculation::AddField(std::string dbfFilePath, std::string new_name) {
	DBFHandle	hDBF;
	hDBF = DBFOpen(dbfFilePath.c_str(), "rb+");

	if (hDBF == NULL)
		return;

	//添加表列
	if (new_name.size() <= 10) {
		DBFAddField(hDBF, new_name.c_str(), FTDouble, 20, 4);
	}
	DBFClose(hDBF);
}

void Calculation::DeleteField(std::string dbfFilePath, std::string old_name) {
	DBFHandle	hDBF;
	hDBF = DBFOpen(dbfFilePath.c_str(), "rb+");

	if (hDBF == NULL)
		return;

	//查找该列所在索引是否存在
	int fieldIndex = DBFGetFieldIndex(hDBF, old_name.c_str());
	if (fieldIndex != -1) {
		DBFDeleteField(hDBF, fieldIndex);
	}

	DBFClose(hDBF);
}

void Calculation::UpdateField(std::string dbfFilePath, std::string old_name, std::map<int, double> &data) {
	DBFHandle	hDBF;
	hDBF = DBFOpen(dbfFilePath.c_str(), "rb+");

	if (hDBF == NULL)
		return;

	if (int(data.size()) == 0)
		return;

	//查找该列所在索引是否存在
	int fieldIndex = DBFGetFieldIndex(hDBF, old_name.c_str());
	if (fieldIndex == -1)
		return;

	//添加数据
	for (auto it = data.begin(); it != data.end(); it++)
	{
		DBFWriteDoubleAttribute(hDBF, it->first, fieldIndex, it->second);
	}

	DBFClose(hDBF);
}

void Calculation::CSVRenameField(std::string csvFilePath, std::string old_name, std::string new_name) {
	CSVProcess& csvProcessor = CSVProcess::getInstance();
	if (!csvProcessor.open(csvFilePath)) {
		std::cerr << "Error: Unable to open CSV file at " << csvFilePath << std::endl;
		return;
	}
	csvProcessor.renameField(old_name, new_name);
	csvProcessor.close();

}

void Calculation::CSVAddField(std::string csvFilePath, std::string new_name) {
	CSVProcess& csvProcessor = CSVProcess::getInstance();
	if (!csvProcessor.open(csvFilePath)) {
		std::cerr << "Error: Unable to open CSV file at " << csvFilePath << std::endl;
		return;
	}
	csvProcessor.addField(new_name);
	csvProcessor.close();
}

void Calculation::CSVDeleteField(std::string csvFilePath, std::string old_name) {
	CSVProcess& csvProcessor = CSVProcess::getInstance();
	if (!csvProcessor.open(csvFilePath)) {
		std::cerr << "Error: Unable to open CSV file at " << csvFilePath << std::endl;
		return;
	}
	csvProcessor.deleteField(old_name);
	csvProcessor.close();
}

void Calculation::CSVUpdateField(std::string csvFilePath, std::string old_name, std::map<int, double>& data) {
	CSVProcess& csvProcessor = CSVProcess::getInstance();
	if (!csvProcessor.open(csvFilePath)) {
		std::cerr << "Error: Unable to open CSV file at " << csvFilePath << std::endl;
		return;
	}
	csvProcessor.updateField(old_name,data);
	csvProcessor.close();
}

void Calculation::modifyCSV(const std::map<int, double>& result, const std::string& outPath, const std::string& fieldName) {
	CSVProcess& csvProcessor = CSVProcess::getInstance();

	if (!csvProcessor.open(outPath)) {
		std::cerr << "Error: Unable to open CSV file at " << outPath << std::endl;
		return;
	}

	// 检查字段是否存在
	int fieldIndex = csvProcessor.getFieldIndex(fieldName);
	if (fieldIndex == -1) {
		// 添加字段如果不存在
		csvProcessor.addField(fieldName);
	}

	// 写入数据
	csvProcessor.updateField( fieldName, result);
	csvProcessor.close();
}

void Calculation::modifyDBF(std::map<int, double> &Result, std::string outPath, std::string fieldName, std::string fullName)
{
	if (fullName == "") {
		fullName = fieldName;
	}
	DBFHandle	hDBF;
	auto path = outPath.c_str();
	hDBF = DBFOpen(path, "rb+");

	if (hDBF == NULL)
		return;

	if (int(Result.size()) == 0)
		return;

	TempModifyData[fullName] = Result;

	//查找该列所在索引是否存在
	int fieldIndex = DBFGetFieldIndex(hDBF, fieldName.c_str());
	if (fieldIndex == -1)
	{
		//检查field名称是否超过10个字符，若超出，则斩除超出部分的字符
		if (fieldName.length() > 10)
			fieldName = fieldName.substr(0, 10);

		//添加表列
		if (fieldName == "PathCount")
			DBFAddField(hDBF, fieldName.c_str(), FTInteger, 20, 0);
		else
			DBFAddField(hDBF, fieldName.c_str(), FTDouble, 20, 4);
		DBFClose(hDBF);
		hDBF = DBFOpen(outPath.c_str(), "rb+");
		fieldIndex = DBFGetFieldIndex(hDBF, fieldName.c_str());
	}

	//添加数据
	for (auto it = Result.begin(); it != Result.end(); it++)
	{
		DBFWriteDoubleAttribute(hDBF, it->first, fieldIndex, it->second);
	}

	DBFClose(hDBF);
}

void Calculation::OutputData(ShapeFileAccessor &fileAccessor)
{
	std::string fieldName;
	std::string fieldName2;
	std::string str1, str2, str3;

	std::set<std::string> fieldnames;

	//增加输出seglength
	fieldName = "seglength";
	modifyDBF(fileAccessor.Length, dbfFilePath, fieldName);
	//modifyCSV(fileAccessor.Length, csvFilePath, fieldName);
	if (isMD)
	{
		for (auto iter = MRLimitSet.begin(); iter != MRLimitSet.end(); iter++)
		{
			double MRLimit = *iter;
			if (MRLimit == -1)
				str1 = "n";
			else
			{
				str1 = std::to_string(int(MRLimit));
				if (int(MRLimit) % 1000 == 0)
				{
					int s = int(MRLimit) / 1000;
					str1 = std::to_string(s) + "k";
				}
			}

			//输出meanMD
			fieldName2 = "mMD" + str1;
			modifyDBF(meanMD_all[MRLimit], dbfFilePath, fieldName2);
			//modifyCSV(meanMD_all[MRLimit], csvFilePath, fieldName2);
		}
	}
	else if (isMR)
	{
		for (auto iter = MRLimitSet.begin(); iter != MRLimitSet.end(); iter++)
		{
			double MRLimit = *iter;
			if (MRLimit == -1)
				str1 = "n";
			else{
				str1 = std::to_string(int(MRLimit));
				if (int(MRLimit) % 1000 == 0)
				{
					int s = int(MRLimit) / 1000;
					str1 = std::to_string(s) + "k";
				}
			}
			//输出MR
			fieldName = "R" + str1;
			modifyDBF(MR_all[MRLimit], dbfFilePath, fieldName);
			//modifyCSV(MR_all[MRLimit], csvFilePath, fieldName);
			//OutputDebugString(("isWgt:" + std::to_string(isWgt)).c_str());
			if (isWgt){
				for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
					std::string wgtstr = wgt_it->first;
					fieldName = "R" + str1 + "W" + wgtstr;

					//DBF裁剪了字段，需要判断重复
					std::string subfieldName = "R" + str1 + "W";
					if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 10 - subfieldName.size());
					int pos = 1;
					while (fieldnames.count(subfieldName)) {
						subfieldName = "R" + str1 + "W";
						if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 9 - subfieldName.size()) + std::to_string(pos);
						++pos;
					}
					fieldnames.insert(subfieldName);


					modifyDBF(wgtMR_all[MRLimit][wgtstr], dbfFilePath, subfieldName,fieldName);
					//modifyCSV(wgtMR_all[MRLimit][wgtstr], csvFilePath, fieldName);
				}
			}
			else if (isJnc){
				std::string txJD = std::to_string(int(Jnc_t_limit_Jnc)) + "x";
				fieldName = "R" + str1 + "C" + txJD;
				modifyDBF(JncMR_all[MRLimit], dbfFilePath, fieldName);
				//modifyCSV(JncMR_all[MRLimit], csvFilePath, fieldName);
			}
			else{//输出meanMD
				fieldName2 = "mMD" + str1;
				modifyDBF(meanMD_all[MRLimit], dbfFilePath, fieldName2);
				//modifyCSV(meanMD_all[MRLimit], csvFilePath, fieldName2);
			}
		}
	}

	if (isDR)
	{
		if (int(MRLimitSet2.size()) == 0)
		{
			double MRLimit = -1;
			std::string txAngle = std::to_string(int(angleLimitDR)) + "a";

			//输出DD、DDL
			if (isDDL) {
				fieldName = "D" + txAngle;
				modifyDBF(DD_all[MRLimit], dbfFilePath, fieldName);
				//modifyCSV(DD_all[MRLimit], csvFilePath, fieldName);
				fieldName = "DL" + txAngle;
				modifyDBF(DDL_all[MRLimit], dbfFilePath, fieldName);
				//modifyCSV(DDL_all[MRLimit], csvFilePath, fieldName);

				if (isWgt)
				{					
					for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
						std::string wgtstr = wgt_it->first;
						fieldName = "D" + txAngle + "W" + wgtstr;

						//DBF裁剪了字段，需要判断重复
						std::string subfieldName = "D" + txAngle ;
						if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 10 - subfieldName.size());
						int pos = 1;
						while (fieldnames.count(subfieldName)) {
							subfieldName = "D" + txAngle;
							if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 9 - subfieldName.size()) + std::to_string(pos);
							++pos;
						}
						fieldnames.insert(subfieldName);
						
						modifyDBF(WDD_all[MRLimit][wgtstr], dbfFilePath, subfieldName, fieldName);
						//modifyCSV(WDD_all[MRLimit][wgtstr], csvFilePath, fieldName);
					}
				}
			}

			//输出DR
			for (auto it1 = DRLimitSet.begin(); it1 != DRLimitSet.end(); it1++)
			{
				double DRLimit = *it1;
				double MRLimit = -1;

				if (MRLimit == -1)
					str1 = "n";
				else
				{
					str1 = std::to_string(int(MRLimit));
					if (int(MRLimit) % 1000 == 0)
					{
						int s = int(MRLimit) / 1000;
						str1 = std::to_string(s) + "k";
					}
				}

				if (DRLimit == -1)
					str2 = "n";
				else
					str2 = std::to_string(int(DRLimit));

				fieldName = "R" + str2 + "d" + txAngle;
				modifyDBF(DR_all[std::pair<double, double>(DRLimit, MRLimit)], dbfFilePath, fieldName);
				//modifyCSV(DR_all[std::pair<double, double>(DRLimit, MRLimit)], csvFilePath, fieldName);

				if (isJnc)
				{
					std::string txJD = std::to_string(int(Jnc_t_limit_Jnc)) + "x";
					fieldName = "R"  + str2 + "d" + txAngle + "C" + txJD;
					modifyDBF(JncDR_all[std::pair<double, double>(DRLimit, MRLimit)], dbfFilePath, fieldName);
					//modifyCSV(JncDR_all[std::pair<double, double>(DRLimit, MRLimit)], csvFilePath, fieldName);
				}

				if (isWgt)
				{
					for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
						std::string wgtstr = wgt_it->first;
						fieldName = "R" + str2 + "d" + txAngle + "W" + wgtstr;

						//DBF裁剪了字段，需要判断重复
						std::string subfieldName = "R" + str2 + "d" + txAngle + "W";
						if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 10 - subfieldName.size());
						int pos = 1;
						while (fieldnames.count(subfieldName)) {
							subfieldName = "R" + str2 + "d" + txAngle + "W";
							if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 9 - subfieldName.size()) + std::to_string(pos);
							++pos;
						}
						fieldnames.insert(subfieldName);
						
						modifyDBF(wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][wgtstr], dbfFilePath, subfieldName, fieldName);
						//modifyCSV(wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][wgtstr], csvFilePath, fieldName);
					}
				}
			}
		}
		else
		{
			std::string txAngle = std::to_string(int(angleLimitDR)) + "a";

			//输出DD、DDL
			for (auto it = MRLimitSet2.begin(); it != MRLimitSet2.end(); it++)
			{
				double MRLimit = *it;


				if (MRLimit == -1)
					str1 = "n";
				else
				{
					str1 = std::to_string(int(MRLimit));
					if (int(MRLimit) % 1000 == 0)
					{
						int s = int(MRLimit) / 1000;
						str1 = std::to_string(s) + "k";
					}
				}

				if (isDDL) {
					fieldName = "D" + txAngle + str1;
					modifyDBF(DD_all[MRLimit], dbfFilePath, fieldName);
					//modifyCSV(DD_all[MRLimit], csvFilePath, fieldName);
					fieldName = "DL" + txAngle + str1;
					modifyDBF(DDL_all[MRLimit], dbfFilePath, fieldName);
					//modifyCSV(DDL_all[MRLimit], csvFilePath, fieldName);

					if (isWgt)
					{
						for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
							std::string wgtstr = wgt_it->first;
							fieldName = "D" + txAngle + str1 + "W" +wgtstr;
							//DBF裁剪了字段，需要判断重复
							std::string subfieldName = "R" + str2 + "d" + str1 ;
							if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 10 - subfieldName.size());
							int pos = 1;
							while (fieldnames.count(subfieldName)) {
								subfieldName = "R" + str2 + "d" + str1;
								if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 9 - subfieldName.size()) + std::to_string(pos);
								++pos;
							}
							fieldnames.insert(subfieldName);
							
							modifyDBF(WDD_all[MRLimit][wgtstr], dbfFilePath, subfieldName, fieldName);
							//modifyCSV(WDD_all[MRLimit][wgtstr], csvFilePath, fieldName);
						}
					}
				}
			}

			//输出DR
			if (isDDL == false) {
				for (auto it1 = DRLimitSet.begin(); it1 != DRLimitSet.end(); it1++)
				{
					for (auto it2 = MRLimitSet2.begin(); it2 != MRLimitSet2.end(); it2++)
					{
						double DRLimit = *it1;
						double MRLimit = *it2;

						if (MRLimit == -1)
							str1 = "n";
						else
						{
							str1 = std::to_string(int(MRLimit));
							if (int(MRLimit) % 1000 == 0)
							{
								int s = int(MRLimit) / 1000;
								str1 = std::to_string(s) + "k";
							}
						}

						if (DRLimit == -1)
							str2 = "n";
						else
							str2 = std::to_string(int(DRLimit));

						fieldName = "R" + str2 + "d" + txAngle + str1;
						modifyDBF(DR_all[std::pair<double, double>(DRLimit, MRLimit)], dbfFilePath, fieldName);
						//modifyCSV(DR_all[std::pair<double, double>(DRLimit, MRLimit)], csvFilePath, fieldName);

						if (isJnc)
						{
							std::string txJD = std::to_string(int(Jnc_t_limit_Jnc)) + "x";
							fieldName = "R"  + str2 + "d" + txAngle + str1 + "C" + txJD;
							modifyDBF(JncDR_all[std::pair<double, double>(DRLimit, MRLimit)], dbfFilePath, fieldName);
							//modifyCSV(JncDR_all[std::pair<double, double>(DRLimit, MRLimit)], csvFilePath, fieldName);
						}

						if (isWgt)
						{
							for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
								std::string wgtstr = wgt_it->first;
								fieldName = "R" + str2 + "d" + txAngle + str1 + "W" + wgtstr;

								std::string subfieldName="R" + str2 + "d" + txAngle + str1 + "W";
								if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 10 - subfieldName.size());
								int pos = 1;
								while (fieldnames.count(subfieldName)) {
									subfieldName = "R" + str2 + "d" + txAngle + str1 + "W";
									if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 9 - subfieldName.size()) + std::to_string(pos);
									++pos;
								}
								fieldnames.insert(subfieldName);
								modifyDBF(wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][wgtstr], dbfFilePath, subfieldName, fieldName);
								//modifyCSV(wgtDR_all[std::pair<double, double>(DRLimit, MRLimit)][wgtstr], csvFilePath, fieldName);
							}
						}
					}
				}
			}
			
		}
	}

	if (isJncR) 
	{
		for (auto iter = Jnc_maxNumSet.begin(); iter != Jnc_maxNumSet.end(); iter++)
		{
			double Jnc_maxNum = *iter;
			if (Jnc_maxNum == -1)
				str3 = "n";
			else
				str3 = std::to_string(int(Jnc_maxNum));

			std::string txJD = std::to_string(int(Jnc_t_limit_JncR)) + "x";

			fieldName = "R" + str3 + "j" + txJD;
			modifyDBF(JncR_all[Jnc_maxNum], dbfFilePath, fieldName);
			//modifyCSV(JncR_all[Jnc_maxNum], csvFilePath, fieldName);

			if (isWgt)
			{
				for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
					std::string wgtstr = wgt_it->first;
					fieldName = "R" + str3 + "j" + txJD + "W" + wgtstr;

					std::string subfieldName = "R" + str3 + "j" + txJD + "W";
					if (fieldName.size() < 10) subfieldName += wgtstr.substr(0, 10 - subfieldName.size());
					int pos = 1;
					while (fieldnames.count(subfieldName)) {
						subfieldName = "R" + str3 + "j" + txJD +"W";
						if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 9 - subfieldName.size()) + std::to_string(pos);
						++pos;
					}
					fieldnames.insert(subfieldName);
					modifyDBF(wgtJncR_all[Jnc_maxNum][wgtstr], dbfFilePath, subfieldName, fieldName);
					//modifyCSV(wgtJncR_all[Jnc_maxNum][wgtstr], csvFilePath, fieldName);
				}
			}
		}
	}

	if (isJncD) {
		if (MRLimitSet2.size() == 0) {
			double MRLimit = -1;
			str1 = "";
			std::string txJD = std::to_string(int(Jnc_t_limit_JncR)) + "x";
			if (isJnc)
			{
				std::string J_fieldName = "DL" + txJD + str1;
				modifyDBF(JncDR_all[std::pair<double, double>(-1, MRLimit)], dbfFilePath, fieldName);
				//modifyCSV(JncDR_all[std::pair<double, double>(-1, MRLimit)], csvFilePath, fieldName);

				J_fieldName = "D" + txJD + str1;
				modifyDBF(JD_all[std::pair<double, double>(-1, MRLimit)], dbfFilePath, fieldName);
				//modifyCSV(JD_all[std::pair<double, double>(-1, MRLimit)], csvFilePath, fieldName);
			}

			else if (isWgt)
			{
				for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
					std::string wgtstr = wgt_it->first;
					fieldName = "R" + str1 + "W" + wgtstr;

					std::string subfieldName = "R" + str1 + "W";
					if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 10 - subfieldName.size());
					int pos = 1;
					while (fieldnames.count(subfieldName)) {
						subfieldName = "R" + str1 + "W";
						if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 9 - subfieldName.size()) + std::to_string(pos);
						++pos;
					}
					fieldnames.insert(subfieldName);
					modifyDBF(wgtDR_all[std::pair<double, double>(-1, MRLimit)][wgtstr], dbfFilePath, subfieldName, fieldName);
					//modifyCSV(wgtDR_all[std::pair<double, double>(-1, MRLimit)][wgtstr], csvFilePath, fieldName);
				}
			}
		}

		for (auto it2 = MRLimitSet2.begin(); it2 != MRLimitSet2.end(); it2++)
		{
			double MRLimit = *it2;

			if (MRLimit == -1)
				str1 = "n";
			else
			{
				str1 = std::to_string(int(MRLimit));
				if (int(MRLimit) % 1000 == 0)
				{
					int s = int(MRLimit) / 1000;
					str1 = std::to_string(s) + "k";
				}
			}

			std::string txJD = std::to_string(int(Jnc_t_limit_JncR)) + "x";

			if (isJnc){
				fieldName = "DL" + txJD + str1 ;
				modifyDBF(JncDR_all[std::pair<double, double>(-1, MRLimit)], dbfFilePath, fieldName);
				//modifyCSV(JncDR_all[std::pair<double, double>(-1, MRLimit)], csvFilePath, fieldName);

				fieldName = "D" + txJD + str1;
				modifyDBF(JD_all[std::pair<double, double>(-1, MRLimit)], dbfFilePath, fieldName);
				//modifyCSV(JD_all[std::pair<double, double>(-1, MRLimit)], csvFilePath, fieldName);
			}

			if (isWgt)
			{
				for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
					std::string wgtstr = wgt_it->first;
					fieldName = "R" + str1 + "W" + wgtstr;

					std::string subfieldName = "R" + str1 + "W";
					if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 10 - subfieldName.size());
					int pos = 1;
					while (fieldnames.count(subfieldName)) {
						subfieldName = "R" + str1 + "W";
						if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 9 - subfieldName.size()) + std::to_string(pos);
						++pos;
					}
					fieldnames.insert(subfieldName);
					modifyDBF(wgtDR_all[std::pair<double, double>(-1, MRLimit)][wgtstr], dbfFilePath, subfieldName, fieldName);
					//modifyCSV(wgtDR_all[std::pair<double, double>(-1, MRLimit)][wgtstr], csvFilePath, fieldName);
				}
			}
		}

	}

	if (isJncDDL)
	{
		std::string txJD = std::to_string(int(Jnc_t_limit_JncR)) + "x";

		for (auto iter = Jnc_maxNumSet.begin(); iter != Jnc_maxNumSet.end(); iter++)
		{
			double Jnc_maxNum = *iter;
			if (Jnc_maxNum == -1)
				str3 = "n";
			else
				str3 = std::to_string(int(Jnc_maxNum));

			fieldName2 = "D" + str3 + "j" + txJD;
			modifyDBF(JncDD_all[Jnc_maxNum], dbfFilePath, fieldName2);
			//modifyCSV(JncDD_all[Jnc_maxNum], csvFilePath, fieldName2);
			fieldName2 = "DL" + str3 + "j" + txJD;
			modifyDBF(JncDDL_all[Jnc_maxNum], dbfFilePath, fieldName2);
			//modifyCSV(JncDDL_all[Jnc_maxNum], csvFilePath, fieldName2);

			if (isWgt)
			{
				for (auto wgt_it = weight.begin(); wgt_it != weight.end(); wgt_it++) {
					std::string wgtstr = wgt_it->first;
					fieldName = "D" + str3 + "j" + txJD + "W" + wgtstr;

					std::string subfieldName = "D" + str3 + "j" + txJD + "W";
					if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 10 - subfieldName.size());
					int pos = 1;
					while (fieldnames.count(subfieldName)) {
						subfieldName = "D" + str3 + "j" + txJD + "W";
						if (subfieldName.size() < 10) subfieldName += wgtstr.substr(0, 9 - subfieldName.size()) + std::to_string(pos);
						++pos;
					}
					fieldnames.insert(subfieldName);
					modifyDBF(JncWDD_all[Jnc_maxNum][wgtstr], dbfFilePath, subfieldName, fieldName);
					//modifyCSV(JncWDD_all[Jnc_maxNum][wgtstr], csvFilePath, fieldName);
				}
			}
		}
	}

}

std::wstring StringToWString(const std::string& str)
{
	int num = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	wchar_t *wide = new wchar_t[num];
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide, num);
	std::wstring w_str(wide);
	delete[] wide;
	return w_str;
}

std::string WStringToString(const std::wstring &wstr)
{
	std::string str;
	int nLen = (int)wstr.length();
	str.resize(nLen, ' ');
	int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), nLen, (LPSTR)str.c_str(), nLen, NULL, NULL);
	if (nResult == 0)
	{
		return "";
	}
	return str;
}

inline std::string getFromToStr(std::string searchLimitStr) {
	std::string res;
	std::string::size_type position= searchLimitStr.find("-");
	if (position == searchLimitStr.npos) {  //如果没找到，返回一个特别的标志c++中用npos表示
		res = "from " + searchLimitStr;
	}
	else {
		res = "from " + searchLimitStr.substr(0, position) + "-to " + searchLimitStr.substr(position + 1, searchLimitStr.length());
	}
	return res;
}

std::string Calculation::getOutFilePath()
{
	std::string outFilePath;

	//Net绘制――――需要输入辅助参数
	if (isNet && isMR)
	{
		outFilePath = "Reach_M_" + std::to_string(int(newMRLimit)) + "_" + getFromToStr(searchLimitStr);
	}
	if (isNet && isDR)
	{
		outFilePath = "Reach_C_" + std::to_string(int(angleLimitDR)) + "a" + std::to_string(int(newDRLimit)) + "d" + std::to_string(int(newMRLimit)) + "_" + getFromToStr(searchLimitStr);
		if (newMRLimit == -1)
			outFilePath = "Reach_D_" + std::to_string(int(angleLimitDR)) + "a" + std::to_string(int(newDRLimit)) + "d_" + getFromToStr(searchLimitStr);
	}
	if (isNet && isJncR)
	{
		outFilePath = "Reach_J_" + std::to_string(int(Jnc_t_limit_JncR)) + "t" + std::to_string(int(newJnc_maxNum)) + "j_" + getFromToStr(searchLimitStr);
	}

	//Geo绘制
	if (isGeo && isMR)
	{
		outFilePath = "Path_M_" + getFromToStr(searchLimitStr);
	}
	if (isGeo && isDR)
	{
		outFilePath = "Path_D_" + std::to_string(int(angleLimitDR)) + "a_" + getFromToStr(searchLimitStr);

	}
	if (isGeo && isJncR)
	{
		outFilePath = "Path_J_" + std::to_string(int(Jnc_t_limit_Jnc)) + "t_" + getFromToStr(searchLimitStr);
	}

	return outFilePath;
}

std::string Calculation::getOutFilePath2()
{
	std::string outFilePath;

	//Net绘制――――需要输入辅助参数
	if (isNet && isMR)
	{
		outFilePath = "Reach_M_" + std::to_string(int(newMRLimit));
	}
	if (isNet && isDR)
	{
		outFilePath = "Reach_C_" + std::to_string(int(angleLimitDR)) + "a" + std::to_string(int(newDRLimit)) + "d" + std::to_string(int(newMRLimit));
		if (newMRLimit == -1)
			outFilePath = "Reach_D_" + std::to_string(int(angleLimitDR)) + "a" + std::to_string(int(newDRLimit)) + "d";
	}
	if (isNet && isJncR)
	{
		outFilePath = "Reach_J_" + std::to_string(int(Jnc_t_limit_JncR)) + "t" + std::to_string(int(newJnc_maxNum)) + "j";
	}

	//Geo绘制
	if (isGeo && isMR)
	{
		outFilePath = "Path_M";
	}
	if (isGeo && isDR)
	{
		outFilePath = "Path_D_" + std::to_string(int(angleLimitDR)) + "a";

	}
	if (isGeo && isJncR)
	{
		outFilePath = "Path_J_" + std::to_string(int(Jnc_t_limit_Jnc)) + "t";
	}

	return outFilePath;
}

void Calculation::outputStepDepth(ShapeFileAccessor &fileAccessor, std::string stepDepthName)
{
	//从所有起点中选择最优起点
	std::map<int, double> minStartAll;

	double value = -1;
	for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++)
	{
		int endRoad = *it;
		if (this->distanceAll.count(endRoad) > 0) {
			auto iter = this->distanceAll[endRoad].begin();
			value = *iter;
		}
		else {
			value = -1;
		}		

		minStartAll.insert(std::make_pair(endRoad, value));
	}

	//增加表列Step Depth，修改回原shp文件
	this->modifyDBF(minStartAll, this->dbfFilePath, stepDepthName);
}

void Calculation::outputPathCount(std::map<int, double> &data, std::string fieldname) {
	//增加表列PathCount，修改回原shp文件
	this->modifyDBF(data, this->dbfFilePath, fieldname);
}

//输出所有可视化分析文件：Net和Geo分析
void Calculation::OutputVisualData(ShapeFileAccessor &fileAccessor)
{
	isOutputVisualDataOver = false;

	//计算
	std::string shpFileName;
	std::string dbfFileName;

	//Net绘制――――需要输入辅助参数
	if (isNet && isMR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputNetreach(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//创建文件夹
		std::string folderPath = outfilepath + getOutFilePath2();

		//bool flag = CreateDirectory((LPCWSTR)(StringToWString(folderPath)).c_str(), NULL);
		bool flag = CreateDirectory(folderPath.c_str(), NULL);

		//输出一幅所有的道路图
		shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		outputNetAll(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = folderPath + "/" + getOutFilePath2();
		dbfFileName = folderPath + "/" + getOutFilePath2();
		outputNetSub(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);
	}
	if (isNet && isDR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputNetreach(SHPT_ARC, "MDR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//创建文件夹
		std::string folderPath = outfilepath + getOutFilePath2();

		//bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		bool flag = CreateDirectory(folderPath.c_str(), NULL);

		//输出一幅所有的道路图
		shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		outputNetAll(SHPT_ARC, "MDR", fileAccessor.Route, shpFileName, dbfFileName);
		
		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = folderPath + "/" + getOutFilePath2();
		dbfFileName = folderPath + "/" + getOutFilePath2();
		outputNetSub(SHPT_ARC, "MDR", fileAccessor.Route, shpFileName, dbfFileName);

	}
	if (isNet && isJncR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputNetreach(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//创建文件夹
		std::string folderPath = outfilepath + getOutFilePath2();

		//bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		bool flag = CreateDirectory(folderPath.c_str(), NULL);

		//输出一幅所有的道路图
		shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		dbfFileName = folderPath + "\\" + getOutFilePath2() + "_All.dbf";
		outputNetAll(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = folderPath + "/" + getOutFilePath2();
		dbfFileName = folderPath + "/" + getOutFilePath2();
		outputNetSub(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

	}

	//Geo绘制
	if (isGeo && isMR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputGeodesics(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//创建文件夹
		std::string folderPath = outfilepath + getOutFilePath2();

		//bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		bool flag = CreateDirectory(folderPath.c_str(), NULL);

		//输出一幅所有的道路图
		shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		outputGeoAll(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = folderPath + "/" + getOutFilePath2();
		dbfFileName = folderPath + "/" + getOutFilePath2();
		outputGeoSub(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

	}
	if (isGeo && isDR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputGeodesics(SHPT_ARC, "DR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//创建文件夹
		std::string folderPath = outfilepath + getOutFilePath2();

		//bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		bool flag = CreateDirectory(folderPath.c_str(), NULL);

		//输出一幅所有的道路图
		shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		outputGeoAll(SHPT_ARC, "DR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = folderPath + "/" + getOutFilePath2();
		dbfFileName = folderPath + "/" + getOutFilePath2();
		outputGeoSub(SHPT_ARC, "DR", fileAccessor.Route, shpFileName, dbfFileName);

	}
	if (isGeo && isJncR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputGeodesics(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//创建文件夹
		std::string folderPath = outfilepath + getOutFilePath2();

		//bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		bool flag = CreateDirectory(folderPath.c_str(), NULL);

		//输出一幅所有的道路图
		shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		outputGeoAll(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = folderPath + "/" + getOutFilePath2();
		dbfFileName = folderPath + "/" + getOutFilePath2();
		outputGeoSub(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

	}

	isOutputVisualDataOver = true;
}

void Calculation::OutputVisualData(ShapeFileAccessor &fileAccessor, std::string outfilepath) {
	isOutputVisualDataOver = false;

	//计算
	std::string shpFileName;
	std::string dbfFileName;

	bool flag = CreateDirectory(outfilepath.c_str(), NULL);

	//Net绘制――――需要输入辅助参数
	//Net绘制――――需要输入辅助参数
	if (isNet && isMR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputNetreach(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		////创建文件夹
		//std::string folderPath = outfilepath + getOutFilePath2();

		////bool flag = CreateDirectory((LPCWSTR)(StringToWString(folderPath)).c_str(), NULL);
		//bool flag = CreateDirectory(folderPath.c_str(), NULL);

		////输出一幅所有的道路图
		//shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		//dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		//outputNetAll(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + getOutFilePath2();
		dbfFileName = outfilepath + "/" + getOutFilePath2();
		outputNetSub(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);
	}
	if (isNet && isDR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputNetreach(SHPT_ARC, "MDR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		////创建文件夹
		//std::string folderPath = outfilepath + getOutFilePath2();

		////bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		//bool flag = CreateDirectory(folderPath.c_str(), NULL);

		////输出一幅所有的道路图
		//shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		//dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		//outputNetAll(SHPT_ARC, "MDR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + getOutFilePath2();
		dbfFileName = outfilepath + "/" + getOutFilePath2();
		outputNetSub(SHPT_ARC, "MDR", fileAccessor.Route, shpFileName, dbfFileName);

	}
	if (isNet && isJncR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputNetreach(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		////创建文件夹
		//std::string folderPath = outfilepath + getOutFilePath2();

		////bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		//bool flag = CreateDirectory(folderPath.c_str(), NULL);

		////输出一幅所有的道路图
		//shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		//dbfFileName = folderPath + "\\" + getOutFilePath2() + "_All.dbf";
		//outputNetAll(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + getOutFilePath2();
		dbfFileName = outfilepath + "/" + getOutFilePath2();
		outputNetSub(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

	}

	//Geo绘制
	if (isGeo && isMR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputGeodesics(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		////创建文件夹
		//std::string folderPath = outfilepath + getOutFilePath2();

		////bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		//bool flag = CreateDirectory(folderPath.c_str(), NULL);

		////输出一幅所有的道路图
		//shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		//dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		//outputGeoAll(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + getOutFilePath2();
		dbfFileName = outfilepath + "/" + getOutFilePath2();
		outputGeoSub(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

	}
	if (isGeo && isDR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputGeodesics(SHPT_ARC, "DR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		////创建文件夹
		//std::string folderPath = outfilepath + getOutFilePath2();

		////bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		//bool flag = CreateDirectory(folderPath.c_str(), NULL);

		////输出一幅所有的道路图
		//shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		//dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		//outputGeoAll(SHPT_ARC, "DR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + getOutFilePath2();
		dbfFileName = outfilepath + "/" + getOutFilePath2();
		outputGeoSub(SHPT_ARC, "DR", fileAccessor.Route, shpFileName, dbfFileName);

	}
	if (isGeo && isJncR)
	{
		//输出shp文件
		shpFileName = outfilepath + getOutFilePath() + ".shp";
		dbfFileName = outfilepath + getOutFilePath() + ".dbf";
		outputGeodesics(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		////创建文件夹
		//std::string folderPath = outfilepath + getOutFilePath2();

		////bool flag = CreateDirectory((LPCSTR)(StringToWString(folderPath)).c_str(), NULL);
		//bool flag = CreateDirectory(folderPath.c_str(), NULL);

		////输出一幅所有的道路图
		//shpFileName = folderPath + "/" + getOutFilePath2() + "_All.shp";
		//dbfFileName = folderPath + "/" + getOutFilePath2() + "_All.dbf";
		//outputGeoAll(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + getOutFilePath2();
		dbfFileName = outfilepath + "/" + getOutFilePath2();
		outputGeoSub(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

	}

	isOutputVisualDataOver = true;
}

void Calculation::OutputVisualData(ShapeFileAccessor &fileAccessor, std::string outfilepath, std::string filename, std::string net_geo, std::string mr_dr_jnr) {
	isOutputVisualDataOver = false;

	//计算
	std::string shpFileName;
	std::string dbfFileName;

	bool flag = CreateDirectory(outfilepath.c_str(), NULL);

	//Net绘制――――需要输入辅助参数
	if (net_geo=="Net" && mr_dr_jnr=="MR")
	{
		//输出shp文件
		shpFileName = outfilepath + filename + ".shp";
		dbfFileName = outfilepath + filename + ".dbf";
		outputNetreach(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + "Reach_M";
		dbfFileName = outfilepath + "/" + "Reach_M";
		//outputNetSub(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);
	}
	if (net_geo == "Net" && mr_dr_jnr == "DR")
	{
		//输出shp文件
		shpFileName = outfilepath + filename + ".shp";
		dbfFileName = outfilepath + filename + ".dbf";
		outputNetreach(SHPT_ARC, "MDR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		std::string subname = "Reach_D";
		std::string::size_type idx = filename.find("Reach_C");
		if (idx != std::string::npos)//存在
			subname = "Reach_C";

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + subname;
		dbfFileName = outfilepath + "/" + subname;
		//outputNetSub(SHPT_ARC, "MDR", fileAccessor.Route, shpFileName, dbfFileName);

	}
	if (net_geo == "Net" && mr_dr_jnr == "JnR")
	{
		//输出shp文件
		shpFileName = outfilepath + filename + ".shp";
		dbfFileName = outfilepath + filename + ".dbf";
		outputNetreach(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + "Reach_J";
		dbfFileName = outfilepath + "/" + "Reach_J";
		//outputNetSub(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);
	}

	//Geo绘制
	if (net_geo == "Geo" && mr_dr_jnr == "MR")
	{
		//输出shp文件
		shpFileName = outfilepath + filename + ".shp";
		dbfFileName = outfilepath + filename + ".dbf";
		outputGeodesics(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + "Path_M";
		dbfFileName = outfilepath + "/" + "Path_M";
		//outputGeoSub(SHPT_ARC, "MR", fileAccessor.Route, shpFileName, dbfFileName);

	}
	if (net_geo == "Geo" && mr_dr_jnr == "DR")
	{
		//输出shp文件
		shpFileName = outfilepath + filename + ".shp";
		dbfFileName = outfilepath + filename + ".dbf";
		outputGeodesics(SHPT_ARC, "DR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + "Path_D";
		dbfFileName = outfilepath + "/" + "Path_D";
		//outputGeoSub(SHPT_ARC, "DR", fileAccessor.Route, shpFileName, dbfFileName);

	}
	if (net_geo == "Geo" && mr_dr_jnr == "JnR")
	{
		//输出shp文件
		shpFileName = outfilepath + filename + ".shp";
		dbfFileName = outfilepath + filename + ".dbf";
		outputGeodesics(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

		outShpFileName = shpFileName;

		//输出多幅子图――――用唯一起点边ID标定
		shpFileName = outfilepath + "/" + "Path_J";
		dbfFileName = outfilepath + "/" + "Path_J";
		//outputGeoSub(SHPT_ARC, "JncR", fileAccessor.Route, shpFileName, dbfFileName);

	}

	isOutputVisualDataOver = true;
}

bool Calculation::findLimitData(std::string dbfPath, std::string fieldName, std::map<int, double> &limitData)
{
	DBFHandle	hDBF;
	hDBF = DBFOpen(dbfPath.c_str(), "rb+");

	if (hDBF == NULL)
		return false;

	//查找该列所在索引是否存在
	int fieldIndex = DBFGetFieldIndex(hDBF, fieldName.c_str());
	if (fieldIndex == -1)
	{
		DBFClose(hDBF);
		return false;
	}

	//添加数据
	DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, fieldIndex, NULL, NULL, NULL);

	int record = DBFGetRecordCount(hDBF);
	double value = 0;
	if (fieldtype == FTInteger)
	{
		for (int i = 0; i < record; i++)
		{
			value = DBFReadIntegerAttribute(hDBF, i, fieldIndex);
			limitData.insert(std::make_pair(i, value));
		}

		DBFClose(hDBF);
		return true;
	}
	else
	{
		if (fieldtype == FTDouble)
		{
			for (int i = 0; i < record; i++)
			{
				value = DBFReadIntegerAttribute(hDBF, i, fieldIndex);
				limitData.insert(std::make_pair(i, value));
			}

			DBFClose(hDBF);
			return true;
		}
		else
		{
			DBFClose(hDBF);
			return false;
		}
	}

	DBFClose(hDBF);
	return false;
}

void Calculation::getFromToID(ShapeFileAccessor &fileAccessor, std::string fromidStr)
{
	//清空原数据
	std::vector<int>().swap(FromIDVec);
	std::vector<int>().swap(ToIDVec);
	std::map<int, std::set<int>>().swap(FromToMap);
	isOneToOne = false;

	std::string str, tmp;
	std::stringstream input(fromidStr);

	//测试用，待删除
	//std::ofstream out("error_from_to.txt");

	//如果输入的csv文件
	if (fromidStr.find(".csv") != std::string::npos)
	{
		isOneToOne = true;

		std::ifstream inFile(fromidStr, std::ios::in);		// 读文件
		std::string lineStr;
		int count = 0;

		fromIDSet.clear();
		while (getline(inFile, lineStr))
		{
			++count;
			if (count == 1)
				continue;

			std::stringstream ss(lineStr);
			std::string str;

			int rawCount = 0, a = 0, b = 0;
			while (getline(ss, str, ','))	// 按照逗号分隔,读取一行
			{
				++rawCount;

				if (rawCount == 1)
				{
					a = atoi(str.c_str());
					if (a < 0 || a >= fileAccessor.roadID.size()) {
						//out << count << ",from," << a << ",";
						continue;
					}
				}

				if (rawCount == 2)
				{
					b = atoi(str.c_str());
					if (b < 0 || b >= fileAccessor.roadID.size()) {
						//out << count << ",to," << b << std::endl;
						continue;
					}	
					fromIDSet.insert(a);
					FromIDVec.push_back(a);
					ToIDVec.push_back(b);
					FromToMap[a].insert(b);
					break;
				}
			}
			//out << FromToMap.size() << std::endl;
			//out.close();
		}
	}
	else
	{
		//如果输入的指定的ID号
		int isfirst = 0;
		while (std::getline(input, str, ';'))
		{
			if (isfirst == 0)
			{
				getSearchID(fileAccessor, str, FromIDVec);
				++isfirst;

				fromIDSet.clear();
				fromIDSet.insert(FromIDVec.begin(), FromIDVec.end());
			}
			else
			{
				getSearchID(fileAccessor, str, ToIDVec);
			}
		}
	}

}

std::string Calculation::getSearchIDByAttribute(ShapeFileAccessor &fileAccessor, std::string searchidStr, std::vector<int> &validRoad) {
	std::vector<int>().swap(validRoad);

	if (searchidStr.find("(") == std::string::npos && searchidStr.find(")") == std::string::npos) {
		//获取限定条件字符串
		int nPos1 = 0;
		int nPos2 = searchidStr.length();

		if (searchidStr.find("<=") != std::string::npos) {
			nPos2 = searchidStr.find("<=");
		}
		else if (searchidStr.find(">=") != std::string::npos) {
			nPos2 = searchidStr.find(">=");
		}
		else if (searchidStr.find(">") != std::string::npos) {
			nPos2 = searchidStr.find(">");
		}
		else if (searchidStr.find("<") != std::string::npos) {
			nPos2 = searchidStr.find("<");
		}
		else if (searchidStr.find("=") != std::string::npos) {
			nPos2 = searchidStr.find("=");
		}

		if (nPos2 == searchidStr.length()) {
			return "请检查是否有输入条件运算符，支持<=,>=,<,>,=";
		}

		std::string limitStr = searchidStr.substr(nPos1, nPos2 - nPos1);

		//特殊处理
		if (limitStr == "ID") {
			limitStr = "FID";
		}

		//判断条件字符串是否合法
		std::map<int, double> limitData;
		if (findLimitData(dbfFilePath, limitStr, limitData) == false)
		{
			return "填入的属性字段" + limitStr + "在dbf文件中无对应数据，请检查";
		}

		//根据限制条件找出合法的道路
		getLimitID(fileAccessor, limitData, searchidStr, validRoad);

		return "";
	}

	////判断是否有"(",")"
	//if (searchidStr.find("(") == std::string::npos && searchidStr.find(")") == std::string::npos) {
	//	return "对于双条件搜索，属性字段请用括号限定，字段左侧和右侧可以添加条件运算符，正确示例：59.4<=(seglength)<=1200";
	//}

	//获取限定条件字符串
	int nPos1 = int(searchidStr.find("("));
	int nPos2 = int(searchidStr.find(")"));
	std::string limitStr = searchidStr.substr(nPos1 + 1, nPos2 - nPos1 - 1);

	//特殊处理
	if (limitStr == "ID") {
		limitStr = "FID";
	}
	
	//判断条件字符串是否合法
	std::map<int, double> limitData;
	if (findLimitData(dbfFilePath, limitStr, limitData) == false)
	{
		return "填入的属性字段" + limitStr + "在dbf文件中无对应数据，请检查";
	}

	std::string str1="", str2="";
	int pos1 = 0, pos2 = 0;
	std::vector<int> validRoad1, validRoad2;

	//左侧条件
	if (nPos1 > 0){
		std::string left_str = searchidStr.substr(0, nPos1);
		if (left_str.find("<=") != std::string::npos) {
			pos1 = left_str.find("<=");
			str1 = "<=";
			//根据限制条件找出合法的道路
			getLimitID(fileAccessor, limitData, ">=" + left_str.substr(0, pos1), validRoad1);
		}
		else if (left_str.find(">=") != std::string::npos) {
			pos1 = left_str.find(">=");
			str1 = ">=";
			//根据限制条件找出合法的道路
			getLimitID(fileAccessor, limitData, ">" + left_str.substr(0, pos1), validRoad1);
		}
		else if (left_str.find(">") != std::string::npos) {
			pos1 = left_str.find(">");
			str1 = ">";
			//根据限制条件找出合法的道路
			getLimitID(fileAccessor, limitData, ">" + left_str.substr(0, pos1), validRoad1);
		}
		else if (left_str.find("<") != std::string::npos) {
			pos1 = left_str.find("<");
			str1 = "<";
			//根据限制条件找出合法的道路
			getLimitID(fileAccessor, limitData, ">" + left_str.substr(0, pos1), validRoad1);
		}
		else if (left_str.find("=") != std::string::npos) {
			pos1 = left_str.find("=");
			str1 = "=";
			//根据限制条件找出合法的道路
			getLimitID(fileAccessor, limitData, ">" + left_str.substr(0, pos1), validRoad1);
		}
	}
	
	//右侧条件
	std::string right_str = searchidStr.substr(nPos2+1, searchidStr.length());
	//根据限制条件找出合法的道路
	getLimitID(fileAccessor, limitData, right_str, validRoad2);
	
	//判断是否需要计算交集
	if (validRoad1.size() == 0) {
		validRoad = validRoad2;
	}
	else {
		set_intersection(validRoad1.begin(), validRoad1.end(), validRoad2.begin(), validRoad2.end(), inserter(validRoad, validRoad.begin()));
	}

	return "";
}

void Calculation::getSearchID(ShapeFileAccessor &fileAccessor, std::string searchidStr, std::vector<int> &validRoad)
{
	//清空数据
	//validRoad.clear();
	
	std::vector<int>().swap(validRoad);

	if (searchidStr.find("(") == std::string::npos && searchidStr.find(")") == std::string::npos)
	{
		std::string tmp;
		std::string number("0123456789");
		std::stringstream ssStr(searchidStr);
		while (std::getline(ssStr, tmp, ','))
			validRoad.push_back(stoi(tmp.substr(tmp.find_first_of(number))));

		Graph *g = fileAccessor.ProcessShapeFile();
		int edgeMax = int(boost::num_edges(*g));		//边数目
		for (auto it = validRoad.begin(); it != validRoad.end(); it++)
		{
			if (*it >= edgeMax)
				err_beyond = true;
		}
	}
	else
	{
		//判断约束类型，传入约束数据，输出FromIDVec
		if (searchidStr.find("length") != std::string::npos)
		{
			getLimitID(fileAccessor, fileAccessor.Length, searchidStr, validRoad);
			return;
		}

		//获取限定条件字符串
		int nPos1 = int(searchidStr.find("("));
		int nPos2 = int(searchidStr.find(")"));
		std::string limitStr = searchidStr.substr(nPos1 + 1, nPos2 - nPos1 - 1);

		//打开dbf文件去找括号内限定的条件，找不到就没有
		std::map<int, double> limitData;
		if (findLimitData(dbfFilePath, limitStr, limitData) == false)
		{
			err_noData = true;
			return;
		}

		//根据限制条件找出合法的道路
		getLimitID(fileAccessor, limitData, searchidStr, validRoad);
		return;

	}

}

void Calculation::getLimitID(ShapeFileAccessor& fileAccessor, std::map<int, double>& roadData, std::string str, std::vector<int>& validRoad) {
	// 清空数据
	validRoad.clear();

	// 定义操作符和相应的比较函数
	std::vector<std::pair<std::string, std::function<bool(double, double)>>>
		conditions = {
			{"<=", [](double a, double b) { return a <= b; }},
			{">=", [](double a, double b) { return a >= b; }},
			{"<", [](double a, double b) { return a < b; }},
			{">", [](double a, double b) { return a > b; }},
			{"=", [](double a, double b) { return a == b; }}
	};

	// 找到第一个匹配的条件
	for (const auto& condition : conditions) {
		if (str.find(condition.first) != std::string::npos) {
			std::string tmpstr = str.substr(str.find(condition.first));
			double Limit = stof(tmpstr.substr(tmpstr.find_first_of("0123456789")));

			// 遍历 roadData 并应用条件
			for (const auto& it : roadData) {
				if (condition.second(it.second, Limit)) {
					validRoad.push_back(it.first);
				}
			}
			return; // 找到条件后返回
		}
	}

	// 如果没有条件被匹配
	std::stringstream ssStr(str);
	std::string tmp;
	while (std::getline(ssStr, tmp, ',')) {
		validRoad.push_back(stoi(tmp.substr(tmp.find_first_of("0123456789"))));
	}
}

void Calculation::outputNetSub(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath)
{
	std::string newShpPath;
	std::string newDbfPath;

	int currentPos = 0;
	for (auto it = reachRoad_all.begin(); it != reachRoad_all.end(); it++)
	{
		int startRoad = it->first;

		std::string outpath = outShpPath + "_from " + std::to_string(startRoad);
		bool flag = CreateDirectory(outpath.c_str(), NULL);

		int last_pos = 0;
		for (int i = outpath.size() - 1; i >= 0; i--) {
			if (outpath[i] == '/') {
				last_pos = i;
				break;
			}
		}
		std::string name_prefix = outpath.substr(last_pos + 1, outpath.length());

		newShpPath = outpath + "/" + name_prefix + ".shp";
		newDbfPath = outpath + "/" + name_prefix + ".dbf";

		//创建shp文件
		SHPHandle	hSHPHandle;
		SHPObject	*psShape;
		double	x[2], y[2], z[2], m[2];
		hSHPHandle = SHPCreate(newShpPath.c_str(), nSHPType);

		//创建dbf文件
		DBFHandle	hDBF;

		//创建Dbf文件		
		hDBF = DBFCreate(newDbfPath.c_str());

		//添加表列
		DBFAddField(hDBF, "From_OID", FTInteger, 20, 0);
		DBFAddField(hDBF, "To_OID", FTInteger, 20, 0);
		DBFAddField(hDBF, "Radius", FTDouble, 20, 4);
		DBFAddField(hDBF, "DirChg", FTDouble, 20, 4);
		DBFAddField(hDBF, "JncLmt", FTInteger, 20, 0);
		DBFAddField(hDBF, "Relen", FTDouble, 20, 4);

		int now_count = 6;
		int jnc_idx = now_count, wgt_idx = now_count;
		std::map<std::string, int> wgt_idx_all;
		if (isJnc) {
			DBFAddField(hDBF, "Jnc", FTInteger, 20, 0);
			++now_count;
			jnc_idx = now_count - 1;
		}
		if (isWgt) {
			for (auto wgt_it = NetreachData[str].Wgt[0].begin(); wgt_it != NetreachData[str].Wgt[0].end(); wgt_it++) {
				std::string fieldname = "W" + wgt_it->first.substr(0, 9);
				DBFAddField(hDBF, fieldname.c_str(), FTDouble, 20, 4);
				++now_count;
				wgt_idx = now_count - 1;
				wgt_idx_all[fieldname] = wgt_idx;
			}
			
		}

		int record = 0;
		for (int i = 0; i < int(NetreachData[str].To_OID.size()); )
		{
			if (NetreachData[str].From_OID[i] == startRoad)
			{
				int endEdge = NetreachData[str].To_OID[i];
				int startEdge = startRoad;

				//添加shp数据
				if (net_partInRoads_all[startEdge].count(endEdge) == 0)
				{
					for (int k = 0, j = 0; k < 2; k++)
					{
						x[k] = roadPoints[endEdge][j];
						y[k] = roadPoints[endEdge][j + 1];
						z[k] = 0;
						m[k] = 0;
						j += 2;
					}

					psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
						2, x, y, z, m);
					SHPWriteObject(hSHPHandle, -1, psShape);
					SHPDestroyObject(psShape);

					//添加对应的图形数据
					DBFWriteDoubleAttribute(hDBF, record, 0, NetreachData[str].From_OID[i]);
					DBFWriteDoubleAttribute(hDBF, record, 1, NetreachData[str].To_OID[i]);
					DBFWriteDoubleAttribute(hDBF, record, 2, NetreachData[str].Radius[i]);
					DBFWriteDoubleAttribute(hDBF, record, 3, NetreachData[str].DirChg[i]);
					DBFWriteDoubleAttribute(hDBF, record, 4, NetreachData[str].JncLmt[i]);
					DBFWriteDoubleAttribute(hDBF, record, 5, NetreachData[str].Relen[i]);
					if (isJnc)
						DBFWriteDoubleAttribute(hDBF, record, jnc_idx, NetreachData[str].Jnc[i]);
					if (isWgt) {
						for (auto wgt_it = NetreachData[str].Wgt[0].begin(); wgt_it != NetreachData[str].Wgt[0].end(); wgt_it++) {
							std::string fieldname = "W" + wgt_it->first.substr(0, 9);
							DBFWriteDoubleAttribute(hDBF, record, wgt_idx_all[fieldname], NetreachData[str].Wgt[i][wgt_it->first]);
						}
					}
						
					++record;
					++i;
				}
				else
				{
					x[0] = net_partInRoads_all[startEdge][endEdge][0];
					y[0] = net_partInRoads_all[startEdge][endEdge][1];
					x[1] = net_partInRoads_all[startEdge][endEdge][2];
					y[1] = net_partInRoads_all[startEdge][endEdge][3];

					for (int k = 0; k < 2; k++)
					{
						z[k] = 0;
						m[k] = 0;
					}

					psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
						2, x, y, z, m);
					SHPWriteObject(hSHPHandle, -1, psShape);
					SHPDestroyObject(psShape);

					//添加对应的图形数据
					DBFWriteDoubleAttribute(hDBF, record, 0, NetreachData[str].From_OID[i]);
					DBFWriteDoubleAttribute(hDBF, record, 1, NetreachData[str].To_OID[i]);
					DBFWriteDoubleAttribute(hDBF, record, 2, NetreachData[str].Radius[i]);
					DBFWriteDoubleAttribute(hDBF, record, 3, NetreachData[str].DirChg[i]);
					DBFWriteDoubleAttribute(hDBF, record, 4, NetreachData[str].JncLmt[i]);
					DBFWriteDoubleAttribute(hDBF, record, 5, NetreachData[str].Relen[i]);
					if (isJnc)
						DBFWriteDoubleAttribute(hDBF, record, jnc_idx, NetreachData[str].Jnc[i]);
					if (isWgt) {
						for (auto wgt_it = NetreachData[str].Wgt[0].begin(); wgt_it != NetreachData[str].Wgt[0].end(); wgt_it++) {
							std::string fieldname = "W" + wgt_it->first.substr(0, 9);
							DBFWriteDoubleAttribute(hDBF, record, wgt_idx_all[fieldname], NetreachData[str].Wgt[i][wgt_it->first]);
						}
					}
					++record;
					++i;

					if (net_partInRoads_all[startEdge][endEdge].size() > 4)
					{
						x[0] = net_partInRoads_all[startEdge][endEdge][4];
						y[0] = net_partInRoads_all[startEdge][endEdge][5];
						x[1] = net_partInRoads_all[startEdge][endEdge][6];
						y[1] = net_partInRoads_all[startEdge][endEdge][7];

						for (int k = 0; k < 2; k++)
						{
							z[k] = 0;
							m[k] = 0;
						}

						psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
							2, x, y, z, m);
						SHPWriteObject(hSHPHandle, -1, psShape);
						SHPDestroyObject(psShape);

						//添加对应的图形数据
						DBFWriteDoubleAttribute(hDBF, record, 0, NetreachData[str].From_OID[i]);
						DBFWriteDoubleAttribute(hDBF, record, 1, NetreachData[str].To_OID[i]);
						DBFWriteDoubleAttribute(hDBF, record, 2, NetreachData[str].Radius[i]);
						DBFWriteDoubleAttribute(hDBF, record, 3, NetreachData[str].DirChg[i]);
						DBFWriteDoubleAttribute(hDBF, record, 4, NetreachData[str].JncLmt[i]);
						DBFWriteDoubleAttribute(hDBF, record, 5, NetreachData[str].Relen[i]);
						if (isJnc)
							DBFWriteDoubleAttribute(hDBF, record, jnc_idx, NetreachData[str].Jnc[i]);
						if (isWgt) {
							for (auto wgt_it = NetreachData[str].Wgt[0].begin(); wgt_it != NetreachData[str].Wgt[0].end(); wgt_it++) {
								std::string fieldname = "W" + wgt_it->first.substr(0, 9);
								DBFWriteDoubleAttribute(hDBF, record, wgt_idx_all[fieldname], NetreachData[str].Wgt[i][wgt_it->first]);
							}
						}
						++record;
						++i;
					}
				}
			}
			else
				++i;
		}
		SHPClose(hSHPHandle);
		DBFClose(hDBF);

		currentPos += int(it->second.size());
	}

}

void Calculation::outputSubsetReach(ShapeFileAccessor &fileAccessor, std::set<int> &outRoad, std::string outShpPath)
{
	if (int(outRoad.size()) == 0)
		return;

	//创建shp文件
	SHPHandle	hSHPHandle;
	SHPObject	*psShape;
	double	x[2], y[2], z[2], m[2];
	int nSHPType = SHPT_ARC;
	hSHPHandle = SHPCreate(outShpPath.c_str(), nSHPType);

	//创建dbf文件
	DBFHandle	hDBF;
	std::string outDbfPath = outShpPath.substr(0, outShpPath.size() - 4) + ".dbf";
	hDBF = DBFCreate(outDbfPath.c_str());

	//添加表列
	DBFAddField(hDBF, "ID", FTInteger, 20, 0);
	DBFAddField(hDBF, "seglength", FTDouble, 20, 4);

	int record = 0;
	for (auto it=outRoad.begin();it!=outRoad.end();it++){
		int road_id = *it;

		//添加shp数据
		for (int k = 0, j = 0; k < 2; k++)
		{
			x[k] = fileAccessor.Route[road_id][j];
			y[k] = fileAccessor.Route[road_id][j + 1];
			z[k] = 0;
			m[k] = 0;
			j += 2;
		}

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
			2, x, y, z, m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);

		//添加对应的图形数据
		DBFWriteDoubleAttribute(hDBF, record, 0, road_id);
		DBFWriteDoubleAttribute(hDBF, record, 1, fileAccessor.Length[road_id]);
		++record;
	}

	SHPClose(hSHPHandle);

	DBFClose(hDBF);
}

void Calculation::ouputSubsetPartReach(ShapeFileAccessor &fileAccessor, std::set<int> &outRoad,
	std::map<std::string, std::map<int, double>> &attributes, std::string outShpPath) {
	if (outRoad.size() == 0)
		return;

	//创建shp文件
	SHPHandle	hSHPHandle;
	SHPObject	*psShape;
	double	x[2], y[2], z[2], m[2];
	int nSHPType = SHPT_ARC;
	hSHPHandle = SHPCreate(outShpPath.c_str(), nSHPType);

	//创建dbf文件
	DBFHandle	hDBF;
	std::string outDbfPath = outShpPath.substr(0, outShpPath.size() - 4) + ".dbf";
	hDBF = DBFCreate(outDbfPath.c_str());

	//添加表列
	DBFAddField(hDBF, "ID", FTInteger, 20, 0);
	for (auto it = attributes.begin(); it != attributes.end(); it++) {
		DBFAddField(hDBF, it->first.c_str(), FTDouble, 20, 4);
	}

	int record = 0;
	for (auto it = outRoad.begin(); it != outRoad.end(); it++) {
		int road_id = *it;

		//写入shp文件
		for (int k = 0, j = 0; k < 2; k++)
		{
			x[k] = fileAccessor.Route[road_id][j];
			y[k] = fileAccessor.Route[road_id][j + 1];
			z[k] = 0;
			m[k] = 0;
			j += 2;
		}

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
			2, x, y, z, m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);

		//写入dbf文件
		DBFWriteDoubleAttribute(hDBF, record, 0, road_id);
		int field_idx = 1;
		for (auto it = attributes.begin(); it != attributes.end(); it++) {
			DBFWriteDoubleAttribute(hDBF, record, field_idx, attributes[it->first][road_id]);
			++field_idx;
		}
		++record;
	}

	SHPClose(hSHPHandle);

	DBFClose(hDBF);
}

void Calculation::ouputSubsetPartReachAll(ShapeFileAccessor &fileAccessor, std::map<int, std::set<int>> &SubsetIDs,
	std::map<int, std::map<std::string, std::map<int, double>>> &attributes, std::string outShpPath) {
	if (SubsetIDs.size() == 0)
		return;

	//创建shp文件
	SHPHandle	hSHPHandle;
	SHPObject	*psShape;
	double	x[2], y[2], z[2], m[2];
	int nSHPType = SHPT_ARC;
	hSHPHandle = SHPCreate(outShpPath.c_str(), nSHPType);

	//创建dbf文件
	DBFHandle	hDBF;
	std::string outDbfPath = outShpPath.substr(0, outShpPath.size() - 4) + ".dbf";
	hDBF = DBFCreate(outDbfPath.c_str());

	//添加表列
	DBFAddField(hDBF, "ID", FTInteger, 20, 0);
	auto it = attributes.begin(); 
	for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
		DBFAddField(hDBF, iter->first.c_str(), FTDouble, 20, 4);
	}

	int record = 0;
	for (auto pos_it = SubsetIDs.begin(); pos_it != SubsetIDs.end(); pos_it++) {
		int pos_idx = pos_it->first;
		for (auto it = SubsetIDs[pos_idx].begin(); it != SubsetIDs[pos_idx].end(); it++) {
			int road_id = *it;

			//写入shp文件
			for (int k = 0, j = 0; k < 2; k++)
			{
				x[k] = fileAccessor.Route[road_id][j];
				y[k] = fileAccessor.Route[road_id][j + 1];
				z[k] = 0;
				m[k] = 0;
				j += 2;
			}

			psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
				2, x, y, z, m);
			SHPWriteObject(hSHPHandle, -1, psShape);
			SHPDestroyObject(psShape);

			//写入dbf文件
			DBFWriteDoubleAttribute(hDBF, record, 0, road_id);
			int field_idx = 1;
			for (auto it = attributes[pos_idx].begin(); it != attributes[pos_idx].end(); it++) {
				DBFWriteDoubleAttribute(hDBF, record, field_idx, attributes[pos_idx][it->first][road_id]);
				++field_idx;
			}
			++record;
		}
	}
	

	SHPClose(hSHPHandle);

	DBFClose(hDBF);
}

void Calculation::outputSubsetReachMR(ShapeFileAccessor &fileAccessor, std::set<int> &mr_reachAllInRoads_total, 
	std::map<int, std::vector<double>> &mr_partInRoads_total, std::string outShpPath) {
	if (int(mr_reachAllInRoads_total.size()) == 0 && mr_partInRoads_total.size()==0)
		return;

	//创建shp文件
	SHPHandle	hSHPHandle;
	SHPObject	*psShape;
	double	x[2], y[2], z[2], m[2];
	int nSHPType = SHPT_ARC;
	hSHPHandle = SHPCreate(outShpPath.c_str(), nSHPType);

	//创建dbf文件
	DBFHandle	hDBF;
	std::string outDbfPath = outShpPath.substr(0, outShpPath.size() - 4) + ".dbf";
	hDBF = DBFCreate(outDbfPath.c_str());

	//添加表列
	DBFAddField(hDBF, "ID", FTInteger, 20, 0);
	DBFAddField(hDBF, "seglength", FTDouble, 20, 4);

	int record = 0;
	
	//全覆盖线条
	for (auto it = mr_reachAllInRoads_total.begin(); it != mr_reachAllInRoads_total.end(); it++) {
		int road_id = *it;

		//添加shp数据
		for (int k = 0, j = 0; k < 2; k++)
		{
			x[k] = fileAccessor.Route[road_id][j];
			y[k] = fileAccessor.Route[road_id][j + 1];
			z[k] = 0;
			m[k] = 0;
			j += 2;
		}

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
			2, x, y, z, m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);

		//添加对应的图形数据
		DBFWriteDoubleAttribute(hDBF, record, 0, road_id);
		DBFWriteDoubleAttribute(hDBF, record, 1, fileAccessor.Length[road_id]);
		++record;
	}

	//部分覆盖
	for (auto it = mr_partInRoads_total.begin(); it != mr_partInRoads_total.end(); it++) {
		int road_id = it->first;

		x[0] = it->second[0];
		y[0] = it->second[1];
		x[1] = it->second[2];
		y[1] = it->second[3];

		for (int k = 0; k < 2; k++)
		{
			z[k] = 0;
			m[k] = 0;
		}

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
			2, x, y, z, m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);

		//添加对应的图形数据
		DBFWriteDoubleAttribute(hDBF, record, 0, road_id);
		DBFWriteDoubleAttribute(hDBF, record, 1, fileAccessor.Length[road_id]);
		++record;

		if (it->second.size() > 4)
		{
			x[0] = it->second[4];
			y[0] = it->second[5];
			x[1] = it->second[6];
			y[1] = it->second[7];

			for (int k = 0; k < 2; k++)
			{
				z[k] = 0;
				m[k] = 0;
			}

			psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
				2, x, y, z, m);
			SHPWriteObject(hSHPHandle, -1, psShape);
			SHPDestroyObject(psShape);

			//添加对应的图形数据
			DBFWriteDoubleAttribute(hDBF, record, 0, road_id);
			DBFWriteDoubleAttribute(hDBF, record, 1, fileAccessor.Length[road_id]);
			++record;
		}
	}

	SHPClose(hSHPHandle);

	DBFClose(hDBF);
}

void Calculation::outputNetreach(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath)
{
	if (int(NetreachData[str].To_OID.size()) == 0)
		return;

	//创建shp文件
	SHPHandle	hSHPHandle;
	SHPObject	*psShape;
	double	x[2], y[2], z[2], m[2];
	hSHPHandle = SHPCreate(outShpPath.c_str(), nSHPType);

	//创建dbf文件
	DBFHandle	hDBF;
	int record = 0;

	//创建Dbf文件		
	hDBF = DBFCreate(outDbfPath.c_str());

	//添加表列
	//DBFAddField(hDBF, "From_OID", FTInteger, 20, 0);
	DBFAddField(hDBF, "ID", FTInteger, 20, 0);
	//DBFAddField(hDBF, "Radius", FTDouble, 20, 4);
	//DBFAddField(hDBF, "DirChg", FTDouble, 20, 4);
	//DBFAddField(hDBF, "JncLmt", FTInteger, 20, 0);
	DBFAddField(hDBF, "Relen", FTDouble, 20, 4);

	int now_count = 2;
	int jnc_idx = now_count, wgt_idx = now_count;
	std::map<std::string, int> wgt_idx_all;
	if (isJnc) {
		DBFAddField(hDBF, "Jnc", FTInteger, 20, 0);
		++now_count;
		jnc_idx = now_count - 1;
	}
	if (isWgt) {
		for (auto wgt_it = NetreachData[str].Wgt[0].begin(); wgt_it != NetreachData[str].Wgt[0].end(); wgt_it++) {
			std::string fieldname = "W" + wgt_it->first.substr(0, 9);
			DBFAddField(hDBF, fieldname.c_str(), FTDouble, 20, 4);
			++now_count;
			wgt_idx = now_count - 1;
			wgt_idx_all[fieldname] = wgt_idx;
		}

	}
		

	for (int i = 0; i < int(NetreachData[str].To_OID.size()); )
	{
		int startEdge = NetreachData[str].From_OID[i];
		int endEdge = NetreachData[str].To_OID[i];

		//添加shp数据
		if (net_partInRoads_all[startEdge].count(endEdge) == 0)
		{
			for (int k = 0, j = 0; k < 2; k++)
			{
				x[k] = roadPoints[endEdge][j];
				y[k] = roadPoints[endEdge][j + 1];
				z[k] = 0;
				m[k] = 0;
				j += 2;
			}

			psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
				2, x, y, z, m);
			SHPWriteObject(hSHPHandle, -1, psShape);
			SHPDestroyObject(psShape);

			//添加对应的图形数据
			//DBFWriteDoubleAttribute(hDBF, record, 0, NetreachData[str].From_OID[i]);
			DBFWriteDoubleAttribute(hDBF, record, 0, NetreachData[str].To_OID[i]);
			/*DBFWriteDoubleAttribute(hDBF, record, 2, NetreachData[str].Radius[i]);
			DBFWriteDoubleAttribute(hDBF, record, 3, NetreachData[str].DirChg[i]);
			DBFWriteDoubleAttribute(hDBF, record, 4, NetreachData[str].JncLmt[i]);*/
			DBFWriteDoubleAttribute(hDBF, record, 1, NetreachData[str].Relen[i]);
			if (isJnc)
				DBFWriteDoubleAttribute(hDBF, record, jnc_idx, NetreachData[str].Jnc[i]);
			if (isWgt) {
				for (auto wgt_it = NetreachData[str].Wgt[0].begin(); wgt_it != NetreachData[str].Wgt[0].end(); wgt_it++) {
					std::string fieldname = "W" + wgt_it->first.substr(0, 9);
					DBFWriteDoubleAttribute(hDBF, record, wgt_idx_all[fieldname], NetreachData[str].Wgt[i][wgt_it->first]);
				}
			}
			++record;
			++i;
		}
		else
		{
			x[0] = net_partInRoads_all[startEdge][endEdge][0];
			y[0] = net_partInRoads_all[startEdge][endEdge][1];
			x[1] = net_partInRoads_all[startEdge][endEdge][2];
			y[1] = net_partInRoads_all[startEdge][endEdge][3];

			for (int k = 0; k < 2; k++)
			{
				z[k] = 0;
				m[k] = 0;
			}

			psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
				2, x, y, z, m);
			SHPWriteObject(hSHPHandle, -1, psShape);
			SHPDestroyObject(psShape);

			//添加对应的图形数据
			//DBFWriteDoubleAttribute(hDBF, record, 0, NetreachData[str].From_OID[i]);
			DBFWriteDoubleAttribute(hDBF, record, 0, NetreachData[str].To_OID[i]);
			//DBFWriteDoubleAttribute(hDBF, record, 2, NetreachData[str].Radius[i]);
			//DBFWriteDoubleAttribute(hDBF, record, 3, NetreachData[str].DirChg[i]);
			//DBFWriteDoubleAttribute(hDBF, record, 4, NetreachData[str].JncLmt[i]);
			DBFWriteDoubleAttribute(hDBF, record, 1, NetreachData[str].Relen[i]);
			if (isJnc)
				DBFWriteDoubleAttribute(hDBF, record, jnc_idx, NetreachData[str].Jnc[i]);
			if (isWgt) {
				for (auto wgt_it = NetreachData[str].Wgt[0].begin(); wgt_it != NetreachData[str].Wgt[0].end(); wgt_it++) {
					std::string fieldname = "W" + wgt_it->first.substr(0, 9);
					DBFWriteDoubleAttribute(hDBF, record, wgt_idx_all[fieldname], NetreachData[str].Wgt[i][wgt_it->first]);
				}
			}
			++record;
			++i;

			if (net_partInRoads_all[startEdge][endEdge].size() > 4)
			{
				x[0] = net_partInRoads_all[startEdge][endEdge][4];
				y[0] = net_partInRoads_all[startEdge][endEdge][5];
				x[1] = net_partInRoads_all[startEdge][endEdge][6];
				y[1] = net_partInRoads_all[startEdge][endEdge][7];

				for (int k = 0; k < 2; k++)
				{
					z[k] = 0;
					m[k] = 0;
				}

				psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
					2, x, y, z, m);
				SHPWriteObject(hSHPHandle, -1, psShape);
				SHPDestroyObject(psShape);

				//添加对应的图形数据
				//DBFWriteDoubleAttribute(hDBF, record, 0, NetreachData[str].From_OID[i]);
				DBFWriteDoubleAttribute(hDBF, record, 0, NetreachData[str].To_OID[i]);
				/*DBFWriteDoubleAttribute(hDBF, record, 2, NetreachData[str].Radius[i]);
				DBFWriteDoubleAttribute(hDBF, record, 3, NetreachData[str].DirChg[i]);
				DBFWriteDoubleAttribute(hDBF, record, 4, NetreachData[str].JncLmt[i]);*/
				DBFWriteDoubleAttribute(hDBF, record, 1, NetreachData[str].Relen[i]);
				if (isJnc)
					DBFWriteDoubleAttribute(hDBF, record, jnc_idx, NetreachData[str].Jnc[i]);
				if (isWgt) {
					for (auto wgt_it = NetreachData[str].Wgt[0].begin(); wgt_it != NetreachData[str].Wgt[0].end(); wgt_it++) {
						std::string fieldname = "W" + wgt_it->first.substr(0, 9);
						DBFWriteDoubleAttribute(hDBF, record, wgt_idx_all[fieldname], NetreachData[str].Wgt[i][wgt_it->first]);
					}
				}
				++record;
				++i;
			}
		}
	}

	SHPClose(hSHPHandle);

	DBFClose(hDBF);
}

void Calculation::outputNetAll(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath)
{
	if (int(NetreachData[str].To_OID.size()) == 0)
		return;

	//创建shp文件
	SHPHandle	hSHPHandle;
	SHPObject	*psShape;
	double	x[2], y[2], z[2], m[2];
	hSHPHandle = SHPCreate(outShpPath.c_str(), nSHPType);

	//创建dbf文件
	DBFHandle	hDBF;

	//创建Dbf文件		
	hDBF = DBFCreate(outDbfPath.c_str());

	//添加表列
	DBFAddField(hDBF, "To_OID", FTInteger, 20, 0);

	//添加可以到达To_ID边的所有起点边Field
	int maxNum = int(reachRoad_all.size());
	for (int i = 0; i < int(reachRoad_all.size()); i++)
	{
		std::string tmpStr = "From_ID_" + std::to_string(i + 1);
		DBFAddField(hDBF, tmpStr.c_str(), FTInteger, 20, 0);
	}

	//准备好可以到达To_ID边的所有起点边数据
	std::map<int, std::set<int> > toFromAll;
	for (auto it = reachRoad_all.begin(); it != reachRoad_all.end(); it++)
	{
		int net_startroad = it->first;
		for (auto it2 = reachRoad_all[net_startroad].begin(); it2 != reachRoad_all[net_startroad].end(); it2++)
		{
			int net_endroad = *it2;
			toFromAll[net_endroad].insert(net_startroad);
		}
	}

	//写入数据
	int record = 0;
	for (auto it = toFromAll.begin(); it != toFromAll.end(); it++)
	{
		int endEdge = it->first;

		//添加shp数据
		for (int i = 0, j = 0; i < 2; i++)
		{
			x[i] = roadPoints[endEdge][j];
			y[i] = roadPoints[endEdge][j + 1];
			z[i] = 0;
			m[i] = 0;
			j += 2;
		}

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL, 2, x, y, z, m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);

		//添加dbf的到达边数据
		DBFWriteDoubleAttribute(hDBF, record, 0, endEdge);

		//添加dbf的from数据
		int row = 1;
		for (auto it2 = it->second.begin(); it2 != it->second.end();)
		{
			DBFWriteDoubleAttribute(hDBF, record, row, *it2);
			it2++;
			++row;
			while (it2 == it->second.end() && row <= maxNum)
			{
				DBFWriteDoubleAttribute(hDBF, record, row, -1);
				++row;
			}
		}

		++record;
	}

	SHPClose(hSHPHandle);

	DBFClose(hDBF);
}

void Calculation::outputGeoSub(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath)
{
	std::string newShpPath;
	std::string newDbfPath;

	int currentPos = 0;
	for (auto it = routeRoad_all.begin(); it != routeRoad_all.end(); it++)
	{
		int startRoad = it->first;

		std::string outpath = outShpPath + "_from " + std::to_string(startRoad);
		for (int k = 0; k < this->ToIDVec.size();k++) {
			if (k == 0) {
				outpath += "-to " + std::to_string(this->ToIDVec[k]);
			}
			else {
				outpath += "," + std::to_string(this->ToIDVec[k]);
			}
		}
		bool flag = CreateDirectory(outpath.c_str(), NULL);

		int last_pos = 0;
		for (int i = outpath.size() - 1; i >= 0; i--) {
			if (outpath[i] == '/') {
				last_pos = i;
				break;
			}
		}
		std::string name_prefix = outpath.substr(last_pos + 1, outpath.length());

		newShpPath = outpath + "/" + name_prefix + ".shp";
		newDbfPath = outpath + "/" + name_prefix + ".dbf";

		//创建shp文件
		SHPHandle	hSHPHandle;
		SHPObject	*psShape;
		double	x[2], y[2], z[2], m[2];
		hSHPHandle = SHPCreate(newShpPath.c_str(), nSHPType);

		//创建dbf文件
		DBFHandle	hDBF;

		//创建Dbf文件		
		hDBF = DBFCreate(newDbfPath.c_str());

		//添加表列
		std::map<std::string, int> idxMp;
		DBFAddField(hDBF, "From_OID", FTInteger, 20, 0);
		idxMp["From_OID"] = 0;
		DBFAddField(hDBF, "To_OID", FTInteger, 20, 0);
		idxMp["To_OID"] = 1;
		int idxCount = 2;
		if (angleLimitDR < INT_MAX - 10) {
			DBFAddField(hDBF, "DC", FTInteger, 20, 0);
			DBFAddField(hDBF, "DD", FTDouble, 20, 4);
			DBFAddField(hDBF, "DDL", FTDouble, 20, 4);
			//DBFAddField(hDBF, "WDD", FTDouble, 20, 4);

			idxMp["DC"] = 2;
			idxMp["DD"] = 3;
			idxMp["DDL"] = 4;
			//idxMp["WDD"] = 5;
			idxCount += 3;
		}
		DBFAddField(hDBF, "PathLen", FTDouble, 20, 4);
		idxMp["PathLen"] = idxCount;
		idxCount += 1;

		for (auto wgt_it = GeodesicsData[str].WDD[0].begin(); wgt_it != GeodesicsData[str].WDD[0].end(); wgt_it++) {
			std::string fieldname = "WD" + wgt_it->first.substr(0, 8);
			DBFAddField(hDBF, fieldname.c_str(), FTDouble, 20, 4);
			idxMp[fieldname] = idxCount;
			idxCount += 1;
		}

		if (Jnc_t_limit_Jnc < INT_MAX - 10) {
			DBFAddField(hDBF, "Jnc", FTInteger, 20, 0);
			idxMp["Jnc"] = idxCount;
			idxCount += 1;
		}
		if (weight.size() > 0) {
			DBFAddField(hDBF, "Wgt", FTDouble, 20, 4);
			idxMp["Wgt"] = idxCount;
		}
		if (weight.size() > 0) {
			for (auto wgt_it = GeodesicsData[str].Wgt[0].begin(); wgt_it != GeodesicsData[str].Wgt[0].end(); wgt_it++) {
				std::string fieldname = "W" + wgt_it->first.substr(0, 9);
				DBFAddField(hDBF, fieldname.c_str(), FTDouble, 20, 4);
				idxMp[fieldname] = idxCount;
				idxCount += 1;
			}
		}

		int record = 0;
		for (int i = 0; i < int(GeodesicsData[str].To_OID.size()); i++)
		{
			if (GeodesicsData[str].From_OID[i] == startRoad)
			{
				int endEdge = GeodesicsData[str].To_OID[i];

				//添加shp数据
				for (int i = 0, j = 0; i < 2; i++)
				{
					x[i] = roadPoints[endEdge][j];
					y[i] = roadPoints[endEdge][j + 1];
					z[i] = 0;
					m[i] = 0;
					j += 2;
				}

				psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
					2, x, y, z, m);
				SHPWriteObject(hSHPHandle, -1, psShape);
				SHPDestroyObject(psShape);

				//添加对应的图形数据
				DBFWriteDoubleAttribute(hDBF, record, idxMp["From_OID"], GeodesicsData[str].From_OID[record]);
				DBFWriteDoubleAttribute(hDBF, record, idxMp["To_OID"], GeodesicsData[str].To_OID[record]);
				if (angleLimitDR < INT_MAX - 10) {
					DBFWriteDoubleAttribute(hDBF, record, idxMp["DC"], GeodesicsData[str].DC[record]);
					DBFWriteDoubleAttribute(hDBF, record, idxMp["DD"], GeodesicsData[str].DD[record]);
					DBFWriteDoubleAttribute(hDBF, record, idxMp["DDL"], GeodesicsData[str].DDL[record]);
					for (auto wgt_it = GeodesicsData[str].WDD[0].begin(); wgt_it != GeodesicsData[str].WDD[0].end(); wgt_it++) {
						std::string fieldname = "WD" + wgt_it->first.substr(0, 8);
						DBFWriteDoubleAttribute(hDBF, record, idxMp[fieldname], GeodesicsData[str].WDD[record][wgt_it->first]);
					}
				}
				DBFWriteDoubleAttribute(hDBF, record, idxMp["PathLen"], GeodesicsData[str].PathLen[record]);
				if (Jnc_t_limit_Jnc < INT_MAX - 10)
					DBFWriteDoubleAttribute(hDBF, record, idxMp["Jnc"], GeodesicsData[str].Jnc[record]);
				if (weight.size() > 0) {
					for (auto wgt_it = GeodesicsData[str].Wgt[0].begin(); wgt_it != GeodesicsData[str].Wgt[0].end(); wgt_it++) {
						std::string fieldname = "W" + wgt_it->first.substr(0, 9);
						DBFWriteDoubleAttribute(hDBF, record, idxMp[fieldname], NetreachData[str].Wgt[record][wgt_it->first]);
					}
				}

				++record;
			}
		}
		SHPClose(hSHPHandle);
		DBFClose(hDBF);

		currentPos += int(it->second.size());
	}
}

void Calculation::outputGeoAll(int nSHPType, std::string str, std::map<int, std::vector<double>> &roadPoints, std::string outShpPath, std::string outDbfPath)
{
	if (int(GeodesicsData[str].To_OID.size()) == 0)
		return;

	//创建shp文件
	SHPHandle	hSHPHandle;
	SHPObject	*psShape;
	double	x[2], y[2], z[2], m[2];
	hSHPHandle = SHPCreate(outShpPath.c_str(), nSHPType);

	//创建dbf文件
	DBFHandle	hDBF;

	//创建Dbf文件		
	hDBF = DBFCreate(outDbfPath.c_str());

	//添加表列
	DBFAddField(hDBF, "To_OID", FTInteger, 20, 0);

	//添加可以到达To_ID边的所有起点边Field
	int maxNum = int(routeRoad_all.size());
	for (int i = 0; i < int(routeRoad_all.size()); i++)
	{
		std::string tmpStr = "From_ID_" + std::to_string(i + 1);
		DBFAddField(hDBF, tmpStr.c_str(), FTInteger, 20, 0);
	}

	//准备好可以到达To_ID边的所有起点边数据
	std::map<int, std::set<int> > toFromAll;
	for (auto it = routeRoad_all.begin(); it != routeRoad_all.end(); it++)
	{
		int geo_startroad = it->first;
		for (auto it2 = routeRoad_all[geo_startroad].begin(); it2 != routeRoad_all[geo_startroad].end(); it2++)
		{
			int geo_endroad = *it2;
			toFromAll[geo_endroad].insert(geo_startroad);
		}
	}

	//写入数据
	int record = 0;
	for (auto it = toFromAll.begin(); it != toFromAll.end(); it++)
	{
		int endEdge = it->first;

		//添加shp数据
		for (int i = 0, j = 0; i < 2; i++)
		{
			x[i] = roadPoints[endEdge][j];
			y[i] = roadPoints[endEdge][j + 1];
			z[i] = 0;
			m[i] = 0;
			j += 2;
		}

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
			2, x, y, z, m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);

		//添加dbf的到达边数据
		DBFWriteDoubleAttribute(hDBF, record, 0, endEdge);

		//添加dbf的from数据
		int row = 1;
		for (auto it2 = it->second.begin(); it2 != it->second.end();)
		{
			DBFWriteDoubleAttribute(hDBF, record, row, *it2);
			it2++;
			++row;
			while (it2 == it->second.end() && row <= maxNum)
			{
				DBFWriteDoubleAttribute(hDBF, record, row, -1);
				++row;
			}
		}

		++record;
	}

	SHPClose(hSHPHandle);

	DBFClose(hDBF);
}

void Calculation::outputGeodesics(int nSHPType, std::string str, std::map<int, std::vector<double>>& roadPoints, std::string outShpPath, std::string outDbfPath)
{
	if (GeodesicsData[str].To_OID.empty())
		return;

	//创建shp文件
	SHPHandle	hSHPHandle;
	SHPObject* psShape;
	double	x[2], y[2], z[2], m[2];
	hSHPHandle = SHPCreate(outShpPath.c_str(), nSHPType);

	//创建dbf文件
	DBFHandle	hDBF;
	int record = 0;

	//创建Dbf文件		
	hDBF = DBFCreate(outDbfPath.c_str());

	if (hDBF == NULL)
		return;

	//添加表列
	std::map<std::string, int> idxMp;
	DBFAddField(hDBF, "ID", FTInteger, 20, 0);
	idxMp["ID"] = 0;
	int idxCount = 2;
	if (angleLimitDR < INT_MAX - 10) {
		DBFAddField(hDBF, "DC", FTInteger, 20, 0);
		DBFAddField(hDBF, "DD", FTDouble, 20, 4);
		DBFAddField(hDBF, "DDL", FTDouble, 20, 4);

		idxMp["DC"] = 1;
		idxMp["DD"] = 2;
		idxMp["DDL"] = 3;
		idxCount += 3;
	}
	DBFAddField(hDBF, "PathLen", FTDouble, 20, 4);
	idxMp["PathLen"] = idxCount;
	idxCount += 1;

	// Check if WDD is not empty
	if (!GeodesicsData[str].WDD.empty()) {
		for (auto wgt_it = GeodesicsData[str].WDD[0].begin(); wgt_it != GeodesicsData[str].WDD[0].end(); wgt_it++) {
			std::string fieldname = "WD" + wgt_it->first.substr(0, 8);
			DBFAddField(hDBF, fieldname.c_str(), FTDouble, 20, 4);
			idxMp[fieldname] = idxCount;
			idxCount += 1;
		}
	}

	if (Jnc_t_limit_Jnc < INT_MAX - 10) {
		DBFAddField(hDBF, "Jnc", FTInteger, 20, 0);
		idxMp["Jnc"] = idxCount;
		idxCount += 1;
	}
	if (!weight.empty()) {
		DBFAddField(hDBF, "Wgt", FTDouble, 20, 4);
		idxMp["Wgt"] = idxCount;
	}
	if (!weight.empty()) {
		for (auto wgt_it = GeodesicsData[str].Wgt[0].begin(); wgt_it != GeodesicsData[str].Wgt[0].end(); wgt_it++) {
			std::string fieldname = "W" + wgt_it->first.substr(0, 9);
			DBFAddField(hDBF, fieldname.c_str(), FTDouble, 20, 4);
			idxMp[fieldname] = idxCount;
			idxCount += 1;
		}
	}

	for (int num = 0; num < int(GeodesicsData[str].To_OID.size()); num++)
	{
		int startEdge = GeodesicsData[str].From_OID[num];
		int endEdge = GeodesicsData[str].To_OID[num];

		// Check if endEdge exists in roadPoints and has enough points
		if (roadPoints.find(endEdge) == roadPoints.end() || roadPoints[endEdge].size() < 4) {
			continue; // Skip this entry if it doesn't have enough data
		}

		//添加shp数据
		for (int i = 0, j = 0; i < 2; i++)
		{
			x[i] = roadPoints[endEdge][j];
			y[i] = roadPoints[endEdge][j + 1];
			z[i] = 0;
			m[i] = 0;
			j += 2;
		}

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
			2, x, y, z, m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);

		//添加对应的图形数据
		record = num;
		DBFWriteDoubleAttribute(hDBF, record, idxMp["ID"], GeodesicsData[str].To_OID[record]);
		if (angleLimitDR < INT_MAX - 10) {
			DBFWriteDoubleAttribute(hDBF, record, idxMp["DC"], GeodesicsData[str].DC[record]);
			DBFWriteDoubleAttribute(hDBF, record, idxMp["DD"], GeodesicsData[str].DD[record]);
			DBFWriteDoubleAttribute(hDBF, record, idxMp["DDL"], GeodesicsData[str].DDL[record]);
			if (!GeodesicsData[str].WDD.empty()) {
				for (auto wgt_it = GeodesicsData[str].WDD[0].begin(); wgt_it != GeodesicsData[str].WDD[0].end(); wgt_it++) {
					std::string fieldname = "WD" + wgt_it->first.substr(0, 8);
					DBFWriteDoubleAttribute(hDBF, record, idxMp[fieldname], GeodesicsData[str].WDD[record][wgt_it->first]);
				}
			}
		}
		DBFWriteDoubleAttribute(hDBF, record, idxMp["PathLen"], GeodesicsData[str].PathLen[record]);
		if (Jnc_t_limit_Jnc < INT_MAX - 10)
			DBFWriteDoubleAttribute(hDBF, record, idxMp["Jnc"], GeodesicsData[str].Jnc[record]);
		if (!weight.empty()) {
			for (auto wgt_it = GeodesicsData[str].Wgt[0].begin(); wgt_it != GeodesicsData[str].Wgt[0].end(); wgt_it++) {
				std::string fieldname = "W" + wgt_it->first.substr(0, 9);
				DBFWriteDoubleAttribute(hDBF, record, idxMp[fieldname], NetreachData[str].Wgt[record][wgt_it->first]);
			}
		}
	}

	SHPClose(hSHPHandle);

	DBFClose(hDBF);
}

void Calculation::resetFinishedState() {
	subset_finishedCount = 0;
	FinishedVec.clear();
}

void Calculation::addFinishedCount() {
	count_lock.lock();
	++subset_finishedCount;
	count_lock.unlock();
}

double Calculation::getFinishedRate() {
	count_lock.lock();
	double rate = 0;
	if (needProcessCount != 0)
		rate = double(subset_finishedCount) / double(needProcessCount);
	if (rate == 1) {	//等待汇总结束
		while (true) {
			Sleep(5);
			if (IsSubsetsAllFinished())
				break;
		}
	}
	count_lock.unlock();
	return rate;
}

void Calculation::setFinished(bool is_finished) {
	subset_lock.lock();
	if (is_finished==false) {	//新增一个flag
		FinishedVec.push_back(false);
	}
	else {	//找到第一个不为true的设置为true
		for (int i = 0; i < FinishedVec.size(); i++) {
			if (FinishedVec[i] == false) {
				FinishedVec[i] = true;
				break;
			}
		}
	}
	subset_lock.unlock();
}

bool Calculation::IsSubsetsAllFinished() {
	subset_lock.lock();
	bool all_over = true;
	if (FinishedVec.size() == 0)
		all_over = false;
	for (bool state : FinishedVec) {
		if (!state) {
			all_over = false;
			break;
		}
	}
	subset_lock.unlock();
	return all_over;
}

/************************************************整体子集线分析********************************************************/
void Calculation::calculate_subset_3(ShapeFileAccessor &fileAccessor, subsetPara &para) {
	//初始换状态集
	FinishedVec.clear();
	subset_finishedCount = 0;
	int all_num = 0;
	if (para.isMR) ++all_num;
	if (para.isDR) ++all_num;
	if (para.isJnR) ++all_num;

	needProcessCount = all_num * fileAccessor.roadID.size();

	//清空旧数据
	std::map<int, double>().swap(subset_3_mr_all);
	std::map<int, double>().swap(subset_3_dd_all);
	std::map<int, double>().swap(subset_3_ddl_all);
	std::map<int, double>().swap(subset_3_jnc_all);

	//初始化CA，准备好基础数据
	this->init_subset_para(fileAccessor, para);

	//拉起三个运算线程，分别计算不同定义下的最短路径
	if (para.isMR) {
		std::thread thrd_1(&Calculation::calculateMR_subset, this, fileAccessor);	//不受限制，计算每个起点线的平均mr
		thrd_1.detach();
	}

	if (para.isDR) {
		std::thread thrd_2(&Calculation::calculateDR_subset, this, fileAccessor);	//不受限制，计算每个起点线的dd、ddl
		thrd_2.detach();
	}

	if (para.isJnR) {
		std::thread thrd_3(&Calculation::calculateJncR_subset, this, fileAccessor);	//不受限制，计算每个起点线的平均jnc
		thrd_3.detach();
	}

	//汇总数据，存入results
	std::thread thrd_4(&Calculation::ouput_subset_3, this, fileAccessor);
	thrd_4.detach();
}

void Calculation::ouput_subset_3(Calculation *temp, ShapeFileAccessor &fileAccessor) {
	while (true) {
		Sleep(50);	//没必要频繁查询占用CPU算力

		if (temp->IsSubsetsAllFinished())
			break;
	}

	//收集计算综合指标
	double meanMD = 0, meanDD = 0, meanDDL = 0, meanJnC = 0;
	for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++) {
		int road_id = *it;

		meanMD += temp->subset_3_mr_all[road_id];
		meanDD += temp->subset_3_dd_all[road_id];
		meanDDL += temp->subset_3_ddl_all[road_id];
		meanJnC += temp->subset_3_jnc_all[road_id];
	}
	temp->results.result_3.meanMD = meanMD / double(fileAccessor.roadID.size());
	temp->results.result_3.meanDD = meanDD / double(fileAccessor.roadID.size());
	temp->results.result_3.meanDDL = meanDDL / double(fileAccessor.roadID.size());
	temp->results.result_3.meanJnCD = meanJnC / double(fileAccessor.roadID.size());

	//为csv准备数据
	temp->results.result_3.mean_mr = temp->subset_3_mr_all;
	temp->results.result_3.dd = temp->subset_3_dd_all;
	temp->results.result_3.ddl = temp->subset_3_ddl_all;
	temp->results.result_3.mean_jnr = temp->subset_3_jnc_all;
}

/************************************************单组子集线分析 Start********************************************************/
void Calculation::calculate_subset_1(ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs) {
	//初始换状态集
	FinishedVec.clear();
	subset_finishedCount = 0;
	needProcessCount = 3 * subsetIDs.size();

	std::thread thrd_1(&Calculation::get_nearest_distance_to_subset, this, fileAccessor, para, subsetIDs);
	thrd_1.detach();

	std::thread thrd_2(&Calculation::get_subset_collective_reach, this, fileAccessor, para, subsetIDs);
	thrd_2.detach();

	std::thread thrd_3(&Calculation::get_members_within_subset, this, fileAccessor, para, subsetIDs);
	thrd_3.detach();
}

/*
项目1的计算是以子集内的线为终点，在一条线到多个终点的多个距离中的取其中的最小值。计算结果存储单独的shp文件（图形＋数据表）
MeanDD: Mean DD to the nearest member of subset;
MeanDDL: Mean DDL to the nearest member of subset;
MeanMD: Mean Metric Distance to the nearest member subset;
MeanJncD: Mean Junction Distance to the nearest member subset;
注意：MeanXXX表示XXX取平均，是一个统计值
*/
void Calculation::get_nearest_distance_to_subset(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs) {
	//相当于计算Step_MR：多个起点出发，找到每个线条作为终点时的最近起点是哪个

	setFinished(false);

	//无向图复制
	Graph edgGraph;
	Graph *g = fileAccessor.ProcessShapeFile();
	edgGraph.copy_impl(*g);
	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);

	//数据结构
	std::map<int, std::set<double>> mr_distanceAll;
	std::map<int, std::set<double>> dr_distanceAll;
	std::map<int, std::set<double>> jnr_distanceAll;

	//增加
	EdgeProperty ep1, ep2, ep3;

	//创建有向图		
	Graph_d edgGraph_DR;
	GenerateDirectedGraph(fileAccessor, edgGraph_DR);
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;

	//创建交叉口的新图
	Graph edgGraph3;
	GenerateUndirectedGraph_Jnc(fileAccessor, old_numEdges, edgGraph3);
	int numVertices3 = int(boost::num_vertices(edgGraph3));
	int numEdges3 = int(boost::num_edges(edgGraph3));

	std::vector<vertex_descriptor> parents3(numVertices3);
	std::vector<double> distances3(numVertices3);

	extern bool NeedStop;
	for (auto startRoad_it = subsetIDs.begin(); startRoad_it != subsetIDs.end(); startRoad_it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *startRoad_it;

		/*************************************Step_MR********************************************/
		if (para.isMR) {
			//修改图：删一条，加两条			
			double dis = fileAccessor.Length[startRoad] / 2;
			int startNode = numVertices;
			int node1 = fileAccessor.roadNode[startRoad][0];
			int node2 = fileAccessor.roadNode[startRoad][1];

			//删除边-起始边
			boost::remove_edge(node1, node2, edgGraph);

			//添加边-中点到起始边两端
			ep1.m_base = dis;
			ep1.m_value = startRoad;
			boost::add_edge(startNode, node1, ep1, edgGraph);

			ep2.m_base = dis;
			ep2.m_value = numEdges;
			boost::add_edge(startNode, node2, ep2, edgGraph);

			boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

			for (auto endRoad_it = fileAccessor.roadID.begin(); endRoad_it != fileAccessor.roadID.end(); endRoad_it++) {
				int endRoad = *endRoad_it;

				int endnode = 0;
				int endnode1 = fileAccessor.roadNode[endRoad][0];
				int endnode2 = fileAccessor.roadNode[endRoad][1];

				//获取短的分支			
				if (distances[endnode1] < distances[endnode2])
					endnode = endnode1;
				else
					endnode = endnode2;

				double len = distances[endnode] + 0.5 * fileAccessor.Length[endRoad];
				if (endRoad == startRoad)
					len = 0;

				mr_distanceAll[endRoad].insert(len);
			}

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);
		}

		/*************************************Step_DR********************************************/
		int startRoad2 = startRoad + old_numEdges;

		if (para.isDR) {
			//两次有向图搜索
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

			std::map<int, double>().swap(dict_min_dist);
			for (int i = 0; i < numVertices_DR; i++)
				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

			//收集DR有效的道路队列
			dict_real_min_dist.clear();
			for (int k = 0; k < old_numEdges; k++) {
				//找出有向边i对应的反向边id
				int reverse_k = k + old_numEdges;
				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				dict_real_min_dist.insert(std::make_pair(k, min_d));
			}

			for (int endRoad = 0; endRoad < old_numEdges; endRoad++) {
				dr_distanceAll[endRoad].insert(dict_real_min_dist[endRoad]);
			}
		}

		/*************************************Step_JnR********************************************/
		if (para.isJnR) {
			boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

			for (int endRoad = 0; endRoad < numVertices3; endRoad++) {
				jnr_distanceAll[endRoad].insert(distances3[endRoad]);
			}
		}
		

		addFinishedCount();
	}

	//收集数据，计算最短
	std::map<int, double> mr_minStartAll;
	std::map<int, double> dr_minStartAll;
	std::map<int, double> jnr_minStartAll;
	for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++) {

		if (subsetIDs.count(*it))
			continue;

		int endRoad = *it;

		if (para.isMR) {
			auto mr_iter = mr_distanceAll[endRoad].begin();
			mr_minStartAll.insert(std::make_pair(endRoad, *mr_iter));
		}

		if (para.isDR) {
			auto dr_iter = dr_distanceAll[endRoad].begin();
			dr_minStartAll.insert(std::make_pair(endRoad, *dr_iter));
		}
		
		if (para.isJnR) {
			auto jnr_iter = jnr_distanceAll[endRoad].begin();
			jnr_minStartAll.insert(std::make_pair(endRoad, *jnr_iter));
		}
		
	}

	//为csv准备数据
	temp->results.result_1.step_mr = mr_minStartAll;
	temp->results.result_1.step_dr = dr_minStartAll;
	temp->results.result_1.step_jnr = jnr_minStartAll;

	//根据最短路径计算综合指标
	double sum_mr = 0, sum_jnc = 0;
	std::map<int, std::set<int>> ddMap;
	double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
	double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
	if (para.isMR || para.isDR) {
		for (auto it = fileAccessor.roadID.begin(); it != fileAccessor.roadID.end(); it++) {

			if (subsetIDs.count(*it))
				continue;

			int road_id = *it;
			sum_mr += mr_minStartAll[road_id];
			sum_jnc += jnr_minStartAll[road_id];

			ddMap[int(dr_minStartAll[road_id])].insert(road_id);
			M_ddlSum += dr_minStartAll[road_id] * fileAccessor.Length[road_id];
			M_lenSum += fileAccessor.Length[road_id];
		}
		
		if (para.isDR) {
			for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++) {
				M_ddSum1 += ((it3->first)*(it3->second.size()));
				M_ddSum2 += it3->second.size();
			}
			M_dd = M_ddSum1 / M_ddSum2;
			M_ddl = M_ddlSum / M_lenSum;
		}
	}
	
	double mean_mr = sum_mr / double(fileAccessor.roadID.size() - subsetIDs.size());
	double mean_jnc = sum_jnc / double(fileAccessor.roadID.size() - subsetIDs.size());

	//写入result
	temp->results.result_1.meanDD_1 = M_dd;
	temp->results.result_1.meanDDL_1 = M_ddl;
	temp->results.result_1.meanMD_1 = mean_mr;
	temp->results.result_1.meanJnCD_1 = mean_jnc;

	setFinished(true);
	return;
}

inline void GetValidRoadsByMR(ShapeFileAccessor &fileAccessor, std::map<int, std::vector<int>> &newRoadNode, std::set<int> &allRoads, int numVertices, std::vector<double> &distances, double MRLimit, int startRoad, std::set<int> &outRoad,
	std::map<int, std::vector<double>> &mr_partIn, std::map<int, std::set<int>> &mr_reachAllInRoads, std::map<int, std::map<int, std::vector<double>>> &mr_partInRoads_all,
	std::map<int, std::map<int, std::map<int, double>>> &mr_partInReachRoadNodeLen, std::map<int, std::map<int, std::map<int, int>>> &mr_partInReachRoadNodeCoor) {

	//输入：fileAccessor, numVertices, distance, MRLimit, startRoad
	//输出：mr_reachAllInRoads, partIn, mr_partInReachRoadNodeLen, mr_partInRoads_all, outRoad

	//搜索reach范围
	std::set<int> inNode;
	std::set<int> inRoad;
	std::set<int>().swap(outRoad);
	std::map<int, std::vector<double>>().swap(mr_partIn);
	

	//计算inNode：编号0-(顶点数目-1)
	for (int endNode = 0; endNode < numVertices; endNode++)
	{
		if (MRLimit > double(std::round(pow(10, 8)*distances[endNode])) / pow(10, 8))
			inNode.insert(endNode);
	}

	//判断是否能够冲出起始边
	if (2 * MRLimit >= fileAccessor.Length[startRoad])
	{
		//将起始边加入allInLength
		inRoad.insert(startRoad);

		//全覆盖
		for (auto it1 = allRoads.begin(); it1 != allRoads.end(); it1++)
		{
			int endRoad = *it1;
			if (endRoad == startRoad)	//跳过处理起始边
				continue;

			double roadLen = fileAccessor.Length[endRoad];
			int node1 = newRoadNode[endRoad][0];		//endrode的端点1
			int node2 = newRoadNode[endRoad][1];		//endrode的端点2

			// 如果是完全覆盖，则必须同时到达其两个端点
			if (inNode.count(node1) != 0 && inNode.count(node2) != 0)
			{
				double dist_res1 = MRLimit - distances[node1];  // 端点1方向越过距离
				double dist_res2 = MRLimit - distances[node2];  // 端点2方向越过距离
				double dist_res = dist_res1 + dist_res2;
				if (dist_res >= roadLen)   // 只有确定是完全越过，才会加入allIn
				{
					inRoad.insert(endRoad);
				}
			}
		}

		//部分覆盖
		for (auto it1 = allRoads.begin(); it1 != allRoads.end(); it1++)
		{
			int endRoad = *it1;
			// 跳过全部覆盖的边
			if (inRoad.count(endRoad) != 0)
				continue;

			int node1 = newRoadNode[endRoad][0];		//endrode的端点1
			int node2 = newRoadNode[endRoad][1];		//endrode的端点2

			// 如果是部分覆盖，只要有一个端点有越过
			if (inNode.count(node1) != 0 || inNode.count(node2) != 0)
			{
				// 检查是否已到达边缘的起点
				if (inNode.count(node1) != 0)
				{
					double dis_overflow = MRLimit - distances[node1];
					mr_partIn[endRoad].push_back(dis_overflow);
					mr_partInReachRoadNodeLen[startRoad][endRoad][node1] = dis_overflow;
					mr_partInReachRoadNodeCoor[startRoad][endRoad][node1] = mr_partInRoads_all[startRoad][endRoad].size();

					//计算坐标
					double x1 = fileAccessor.Route[endRoad][0];
					double y1 = fileAccessor.Route[endRoad][1];
					double x2 = fileAccessor.Route[endRoad][2];
					double y2 = fileAccessor.Route[endRoad][3];
					double rate = dis_overflow / fileAccessor.Length[endRoad];

					double x = x1 + (x2 - x1)*rate;
					double y = y1 + (y2 - y1)*rate;

					mr_partInRoads_all[startRoad][endRoad].push_back(x1);
					mr_partInRoads_all[startRoad][endRoad].push_back(y1);
					mr_partInRoads_all[startRoad][endRoad].push_back(x);
					mr_partInRoads_all[startRoad][endRoad].push_back(y);
				}
				if (inNode.count(node2) != 0)
				{
					double dis_overflow = MRLimit - distances[node2];
					mr_partIn[endRoad].push_back(dis_overflow);
					mr_partInReachRoadNodeLen[startRoad][endRoad][node2] = dis_overflow;
					mr_partInReachRoadNodeCoor[startRoad][endRoad][node2] = mr_partInRoads_all[startRoad][endRoad].size();

					//计算坐标
					double x2 = fileAccessor.Route[endRoad][0];
					double y2 = fileAccessor.Route[endRoad][1];
					double x1 = fileAccessor.Route[endRoad][2];
					double y1 = fileAccessor.Route[endRoad][3];
					double rate = dis_overflow / fileAccessor.Length[endRoad];

					double x = x1 + (x2 - x1)*rate;
					double y = y1 + (y2 - y1)*rate;

					mr_partInRoads_all[startRoad][endRoad].push_back(x1);
					mr_partInRoads_all[startRoad][endRoad].push_back(y1);
					mr_partInRoads_all[startRoad][endRoad].push_back(x);
					mr_partInRoads_all[startRoad][endRoad].push_back(y);
				}

				outRoad.insert(endRoad);
			}
		}

		//outRoad=inRoad+partin的路
		mr_reachAllInRoads[startRoad].insert(inRoad.begin(), inRoad.end());
		outRoad.insert(inRoad.begin(), inRoad.end());
	}
	else
	{
		outRoad.insert(startRoad);
		mr_partIn[startRoad].push_back(2 * MRLimit);

		//计算坐标
		double x1 = fileAccessor.Route[startRoad][0];
		double y1 = fileAccessor.Route[startRoad][1];
		double x2 = fileAccessor.Route[startRoad][2];
		double y2 = fileAccessor.Route[startRoad][3];
		double x_mid = (x1 + x2) / 2;
		double y_mid = (y1 + y2) / 2;

		int node1 = newRoadNode[startRoad][0];		//endrode的端点1
		int node2 = newRoadNode[startRoad][1];		//endrode的端点2

		double rate = 2 * MRLimit / fileAccessor.Length[startRoad];

		double xn1 = fileAccessor.Route[startRoad][0];
		double yn1 = fileAccessor.Route[startRoad][1];
		double xn2 = fileAccessor.Route[startRoad][2];
		double yn2 = fileAccessor.Route[startRoad][3];
		double x_1 = x_mid + (xn1 - x_mid)*rate;
		double y_1 = y_mid + (yn1 - y_mid)*rate;
		double x_2 = x_mid + (xn2 - x_mid)*rate;
		double y_2 = y_mid + (yn2 - y_mid)*rate;

		mr_partInRoads_all[startRoad][startRoad].push_back(x_1);
		mr_partInRoads_all[startRoad][startRoad].push_back(y_1);
		mr_partInRoads_all[startRoad][startRoad].push_back(x_2);
		mr_partInRoads_all[startRoad][startRoad].push_back(y_2);
	}
}

/* 优先完成：将mr、dr、jnr、mdr单独再拆一套出来重写，因为很多指标并不需要计算。初始参数->调用mr、dr、jnr、mdr->输出shp文件和统计表
项目2计算子集作为一个整体所触及的线长度（含子集内部和外部的线），计算的时候需要撇掉重叠！针对每个Reach选项生成一个独立的shp文件存储图形和数据表
Angle Threshold in degrees=?
Metric Reach @ Radius=?
Directional Reach @ Directional Changes=?
Metric-direction Reach @ Radius=?, Directional Changes=?
Junction Reach @ Junctions Limit=?
*/
void Calculation::get_subset_collective_reach(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs) {
	setFinished(false);
	
	//清空旧的绘图备用数据
	temp->subset_reachRoad_all.clear();
	temp->subset_partInRoads_all.clear();

	//无向图复制
	Graph edgGraph;
	Graph *g = fileAccessor.ProcessShapeFile();
	edgGraph.copy_impl(*g);
	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);

	//数据结构
	std::set<int> inNode;
	std::set<int> inRoad;
	std::set<int> outRoad;
	std::set<int> validRoad;

	//mr
	std::map<int, std::vector<double>> mr_partIn;
	std::map<int, std::set<int>> mr_reachAllInRoads;
	std::map<int, std::map<int, std::vector<double>>> mr_partInRoads_all;
	std::map<int, std::map<int, std::map<int, double>>> mr_partInReachRoadNodeLen;
	std::map<int, std::map<int, std::map<int, int>>> mr_partInReachRoadNodeCoor;	//标识node端部分覆盖对应的坐标索引

	//dr
	std::map<int, std::set<int>> dr_reachAllInRoads;

	//mdr
	std::map<int, std::vector<double>> mdr_partIn;
	std::map<int, std::set<int>> mdr_reachAllInRoads;
	std::map<int, std::map<int, std::vector<double>>> mdr_partInRoads_all;
	std::map<int, std::map<int, std::map<int, double>>> mdr_partInReachRoadNodeLen;
	std::map<int, std::map<int, std::map<int, int>>> mdr_partInReachRoadNodeCoor;
	std::map<int, std::set<int>> mdr_reachRoad_all;

	//jnr
	std::map<int, std::set<int>> jnr_reachAllInRoads;

	//增加
	EdgeProperty ep1, ep2, ep3;

	//创建有向图		
	Graph_d edgGraph_DR;
	GenerateDirectedGraph(fileAccessor, edgGraph_DR);
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;
	std::map<int, double> new_dict_real_min_dist;

	//创建交叉口的新图
	Graph edgGraph3;
	GenerateUndirectedGraph_Jnc(fileAccessor, old_numEdges, edgGraph3);
	int numVertices3 = int(boost::num_vertices(edgGraph3));
	int numEdges3 = int(boost::num_edges(edgGraph3));

	std::vector<vertex_descriptor> parents3(numVertices3);
	std::vector<double> distances3(numVertices3);

	extern bool NeedStop;
	for (auto it = subsetIDs.begin(); it != subsetIDs.end(); it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *it;

		/*************************************MR NetReach********************************************/
		if (para.isMR) {
			//修改图：删一条，加两条			
			double dis = fileAccessor.Length[startRoad] / 2;
			int startNode = numVertices;
			int node1 = fileAccessor.roadNode[startRoad][0];
			int node2 = fileAccessor.roadNode[startRoad][1];

			//删除边-起始边
			boost::remove_edge(node1, node2, edgGraph);

			//添加边-中点到起始边两端
			ep1.m_base = dis;
			ep1.m_value = startRoad;
			boost::add_edge(startNode, node1, ep1, edgGraph);

			ep2.m_base = dis;
			ep2.m_value = numEdges;
			boost::add_edge(startNode, node2, ep2, edgGraph);

			boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

			GetValidRoadsByMR(fileAccessor, fileAccessor.roadNode, fileAccessor.roadID, numVertices, distances, para.MR_Para.mr_limit, startRoad,
				outRoad, mr_partIn, mr_reachAllInRoads, mr_partInRoads_all, mr_partInReachRoadNodeLen, mr_partInReachRoadNodeCoor);

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);
		}

		/*************************************DR NetReach********************************************/
		int startRoad2 = startRoad + old_numEdges;

		if (para.isDR) {
			double dr_subDRLimit = para.DR_Para.dc_limit;

			//两次有向图搜索
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

			std::map<int, double>().swap(dict_min_dist);
			for (int i = 0; i < numVertices_DR; i++)
				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

			//收集DR有效的道路队列
			dict_real_min_dist.clear();
			for (int k = 0; k < old_numEdges; k++) {
				//找出有向边i对应的反向边id
				int reverse_k = k + old_numEdges;
				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				dict_real_min_dist.insert(std::make_pair(k, min_d));

				if (dr_subDRLimit == -1)
					dr_reachAllInRoads[startRoad].insert(k);
				else if (dict_real_min_dist[k] <= dr_subDRLimit)
					dr_reachAllInRoads[startRoad].insert(k);
			}
		}

		/*************************************MDR NetReach********************************************/
		if (para.isMDR) {
			//先进行mr搜索
			/*GetValidRoadsByMR(fileAccessor, fileAccessor.roadNode, fileAccessor.roadID, numVertices, distances, para.MDR_Para.mr_limit, startRoad, outRoad,
				mdr_partIn, mdr_reachAllInRoads, mdr_partInRoads_all, mdr_partInReachRoadNodeLen, mdr_partInReachRoadNodeCoor);*/


				//修改图：删一条，加两条			
			double dis = fileAccessor.Length[startRoad] / 2;
			int startNode = numVertices;
			int node1 = fileAccessor.roadNode[startRoad][0];
			int node2 = fileAccessor.roadNode[startRoad][1];

			//删除边-起始边
			boost::remove_edge(node1, node2, edgGraph);

			//添加边-中点到起始边两端
			ep1.m_base = dis;
			ep1.m_value = startRoad;
			boost::add_edge(startNode, node1, ep1, edgGraph);

			ep2.m_base = dis;
			ep2.m_value = numEdges;
			boost::add_edge(startNode, node2, ep2, edgGraph);

			boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

			GetValidRoadsByMR(fileAccessor, fileAccessor.roadNode, fileAccessor.roadID, numVertices, distances, para.MDR_Para.mr_limit, startRoad,
				outRoad, mdr_partIn, mdr_reachAllInRoads, mdr_partInRoads_all, mdr_partInReachRoadNodeLen, mdr_partInReachRoadNodeCoor);

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);

			if (outRoad.size() == 1 || para.MDR_Para.dc_limit == -1) {
				//do nothing
			}
			else {
				//开始dr搜索
				new_dict_real_min_dist.clear();
				//Generate_new_dict_real_min_dist(fileAccessor, new_dict_real_min_dist, dict_min_dist, outRoad, startRoad, old_numEdges, parents1, distances1, parents2, distances2);

				std::map<int, std::map<int, int>> partInNodeIDs;
				std::map<std::tuple<int, int, int>, int> nodeToEdge;
				std::map<int, std::tuple<int, int, int>> edgeToNode;
				std::map<int, std::map<int, int>> validPartInRoads;
				Generate_new_dict_real_min_dist_partIn(fileAccessor, new_dict_real_min_dist, dict_min_dist, outRoad, startRoad, mdr_reachAllInRoads[startRoad], mdr_partInReachRoadNodeLen[startRoad],
					old_numEdges, 2 * old_numEdges, parents1, distances1, parents2, distances2, partInNodeIDs, nodeToEdge, edgeToNode);

				//收集DR有效的道路队列
				double subDRLimit = para.MDR_Para.dc_limit;
				validRoad.clear();
				for (auto it2 = outRoad.begin(); it2 != outRoad.end(); it2++) {
					int road_id = *it2;
					if (mdr_partInReachRoadNodeLen[startRoad].count(road_id)) {
						if (partInNodeIDs.count(road_id) == 0)
							continue;
						for (auto node_it = partInNodeIDs[road_id].begin(); node_it != partInNodeIDs[road_id].end(); node_it++) {
							int node = node_it->first;
							//部分覆盖线条在方向上只有出去的份
							int edge_id = node_it->second;
							if (new_dict_real_min_dist[edge_id] <= subDRLimit) {
								validRoad.insert(road_id);
								validPartInRoads[road_id][node] = edge_id;
							}
						}
					}
					else {	//全覆盖
						if (new_dict_real_min_dist[road_id] <= subDRLimit)
							validRoad.insert(road_id);
					}
				}
				outRoad.clear();
				outRoad.insert(validRoad.begin(), validRoad.end());

				//更新全覆盖线条
				for (auto road_it = mdr_reachAllInRoads[startRoad].begin(); road_it != mdr_reachAllInRoads[startRoad].end(); ) {
					int road_id = *road_it;
					if (outRoad.count(road_id) == 0) {
						road_it = mdr_reachAllInRoads[startRoad].erase(road_it);
					}
					else {
						road_it++;
					}
				}

				//更新部分覆盖的线条
				std::map<int, std::vector<double>> tmp_partInRoadsCoor;
				std::map<int, std::map<int, double>> tmp_partInNodeLen;
				for (auto road_it = validPartInRoads.begin(); road_it != validPartInRoads.end(); road_it++) {
					int road_id = road_it->first;
					for (auto node_it = road_it->second.begin(); node_it != road_it->second.end(); node_it++) {
						int node = node_it->first;
						int pos = mdr_partInReachRoadNodeCoor[startRoad][road_id][node];
						tmp_partInNodeLen[road_id][node] = mdr_partInReachRoadNodeLen[startRoad][road_id][node];
						if (road_id == 129)
							bool check = true;
						tmp_partInRoadsCoor[road_id].insert(tmp_partInRoadsCoor[road_id].end(),
							mdr_partInRoads_all[startRoad][road_id].begin() + pos, mdr_partInRoads_all[startRoad][road_id].begin() + pos + 4);
					}
				}
				mdr_partInRoads_all[startRoad].clear();
				mdr_partInRoads_all[startRoad] = tmp_partInRoadsCoor;
				mdr_partInReachRoadNodeLen[startRoad].clear();
				mdr_partInReachRoadNodeLen[startRoad] = tmp_partInNodeLen;
			}

			//可达线路
			mdr_reachRoad_all.insert(std::make_pair(startRoad, outRoad));
		}

		/*************************************JncR NetReach********************************************/
		if (para.isJnR) {
			boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

			//收集JnR有效的道路队列
			for (int k = 0; k < old_numEdges; k++) {
				if (para.JnR_Para.jnc_limit == -1)
					jnr_reachAllInRoads[startRoad].insert(k);
				else if (distances3[k] <= para.JnR_Para.jnc_limit)
					jnr_reachAllInRoads[startRoad].insert(k);
			}
		}

		addFinishedCount();
	}

	//对Net Reach的线条进行合并，并去重
	//mr
	std::set<int> mr_reachAllInRoads_total;	//全覆盖的线条
	std::map<int, std::vector<double>> mr_partInRoads_total;	//与mr_partIn中dis_overflow对应的覆盖线段的坐标
	std::map<int, std::map<int, double>> mr_partInReachRoadNodeLen_total;	//终点线上各端点方向上对应的部分覆盖线段长度(dis_overflow)
	std::map<int, std::map<int, std::tuple<int, int, int>>> mr_partInReachRoadNodeCoor_total;	//tuple<起点线id，终点线id，vector中的pos>

	//dr
	std::set<int> dr_reachAllInRoads_total;

	//mdr
	std::set<int> mdr_reachAllInRoads_total;
	std::map<int, std::vector<double>> mdr_partInRoads_total;
	std::map<int, std::map<int, double>> mdr_partInReachRoadNodeLen_total;
	std::map<int, std::map<int, std::tuple<int, int, int>>> mdr_partInReachRoadNodeCoor_total;

	//jnr
	std::set<int> jnr_reachAllInRoads_total;

	/*************************************MR NetReach********************************************/
	if (para.isMR) {
		for (auto it = mr_reachAllInRoads.begin(); it != mr_reachAllInRoads.end(); it++) {	//全覆盖
			mr_reachAllInRoads_total.insert(it->second.begin(), it->second.end());
		}
		for (auto it = mr_partInReachRoadNodeLen.begin(); it != mr_partInReachRoadNodeLen.end(); it++) {	//部分覆盖
			int start_id = it->first;
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				int road_id = iter->first;
				if (mr_reachAllInRoads_total.count(road_id))	//如果被全覆盖了，do nothing
					continue;

				for (auto sub_iter = iter->second.begin(); sub_iter != iter->second.end(); sub_iter++) {
					int node = sub_iter->first;
					double len = sub_iter->second;
					if (len > mr_partInReachRoadNodeLen_total[road_id][node]) {
						mr_partInReachRoadNodeLen_total[road_id][node] = len;
						mr_partInReachRoadNodeCoor_total[road_id][node] = { start_id,road_id,mr_partInReachRoadNodeCoor[start_id][road_id][node] };
					}
				}
			}
		}
		//从双端部分覆盖中抽出全覆盖线条，并删除部分覆盖
		for (auto it = mr_partInReachRoadNodeLen_total.begin(); it != mr_partInReachRoadNodeLen_total.end(); ) {
			if (it->second.size() < 2) {	//单端覆盖，放过去
				it++;
				continue;
			}

			double len_all = 0;
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++)
				len_all += iter->second;

			if (len_all >= fileAccessor.Length[it->first]) {
				mr_reachAllInRoads_total.insert(it->first);
				it = mr_partInReachRoadNodeLen_total.erase(it);
			}
			else {
				it++;
			}
		}
		//收集部分覆盖的坐标
		for (auto it = mr_partInReachRoadNodeLen_total.begin(); it != mr_partInReachRoadNodeLen_total.end(); it++) {
			int road_id = it->first;
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				int node = iter->first;
				auto elem = mr_partInReachRoadNodeCoor_total[road_id][node];
				mr_partInRoads_total[road_id].insert(mr_partInRoads_total[road_id].begin(),
					mr_partInRoads_all[get<0>(elem)][get<1>(elem)].begin() + get<2>(elem), mr_partInRoads_all[get<0>(elem)][get<1>(elem)].begin() + get<2>(elem) + 4);
			}
		}
		//收集绘图备用数据
		temp->subset_reachRoad_all["MR"] = mr_reachAllInRoads_total;
		temp->subset_partInRoads_all["MR"] = mr_partInRoads_total;
	}

	/*************************************DR NetReach********************************************/
	if (para.isDR) {
		for (auto it = dr_reachAllInRoads.begin(); it != dr_reachAllInRoads.end(); it++) {
			dr_reachAllInRoads_total.insert(it->second.begin(), it->second.end());
		}
		//收集绘图备用数据
		temp->subset_reachRoad_all["DR"] = dr_reachAllInRoads_total;
	}

	/*************************************MDR NetReach********************************************/
	if (para.isMDR) {
		for (auto it = mdr_reachAllInRoads.begin(); it != mdr_reachAllInRoads.end(); it++) {	//全覆盖
			mdr_reachAllInRoads_total.insert(it->second.begin(), it->second.end());
		}
		//mdr的全覆盖、部分覆盖数据与mr不同，需要：要么在计算中处理好，做成跟mr一致，要么在这里单独筛选处理
		for (auto it = mdr_partInReachRoadNodeLen.begin(); it != mdr_partInReachRoadNodeLen.end(); it++) {	//部分覆盖
			int start_id = it->first;
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				int road_id = iter->first;
				if (mdr_reachAllInRoads_total.count(road_id))	//如果被全覆盖了，do nothing
					continue;

				for (auto sub_iter = iter->second.begin(); sub_iter != iter->second.end(); sub_iter++) {
					int node = sub_iter->first;
					double len = sub_iter->second;
					if (len > mdr_partInReachRoadNodeLen_total[road_id][node]) {
						mdr_partInReachRoadNodeLen_total[road_id][node] = len;
						mdr_partInReachRoadNodeCoor_total[road_id][node] = { start_id,road_id,mdr_partInReachRoadNodeCoor[start_id][road_id][node] };
					}
				}
			}
		}
		//从双端部分覆盖中抽出全覆盖线条，并删除部分覆盖
		for (auto it = mdr_partInReachRoadNodeLen_total.begin(); it != mdr_partInReachRoadNodeLen_total.end(); ) {
			if (it->second.size() < 2) {	//单端覆盖，放过去
				it++;
				continue;
			}

			double len_all = 0;
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++)
				len_all += iter->second;

			if (len_all >= fileAccessor.Length[it->first]) {
				mdr_reachAllInRoads_total.insert(it->first);
				it = mdr_partInReachRoadNodeLen_total.erase(it);
			}
			else {
				it++;
			}
		}
		//收集部分覆盖的坐标
		for (auto it = mdr_partInReachRoadNodeLen_total.begin(); it != mdr_partInReachRoadNodeLen_total.end(); it++) {
			int road_id = it->first;
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				int node = iter->first;
				std::tuple<int, int, int> elem = mdr_partInReachRoadNodeCoor_total[road_id][node];
				int startRoad = get<0>(elem), b = get<1>(elem), c = get<2>(elem);
				mdr_partInRoads_total[road_id].insert(mdr_partInRoads_total[road_id].begin(),
					mdr_partInRoads_all[startRoad][road_id].begin(), mdr_partInRoads_all[startRoad][road_id].end());
			}
		}
		//收集绘图备用数据
		temp->subset_reachRoad_all["MDR"] = mdr_reachAllInRoads_total;
		temp->subset_partInRoads_all["MDR"] = mdr_partInRoads_total;
	}
	
	/*************************************JnR NetReach********************************************/
	if (para.isJnR) {
		for (auto it = jnr_reachAllInRoads.begin(); it != jnr_reachAllInRoads.end(); it++) {
			jnr_reachAllInRoads_total.insert(it->second.begin(), it->second.end());
		}
		//收集绘图备用数据
		temp->subset_reachRoad_all["JnR"] = jnr_reachAllInRoads_total;
	}

	//计算叠加后的总里程
	double sum_mr_len = 0, sum_dr_len = 0, sum_jnr_len = 0, sum_mdr_len = 0;
	if (para.isMR) {
		for (auto it = mr_reachAllInRoads_total.begin(); it != mr_reachAllInRoads_total.end(); it++) {	//全覆盖
			sum_mr_len += fileAccessor.Length[*it];
		}
		for (auto it = mr_partInReachRoadNodeLen_total.begin(); it != mr_partInReachRoadNodeLen_total.end(); it++) {
			int road_id = it->first;
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				sum_mr_len += iter->second;
			}
		}
	}
	if (para.isDR) {
		for (auto it = dr_reachAllInRoads_total.begin(); it != dr_reachAllInRoads_total.end(); it++) {	//全覆盖
			sum_dr_len += fileAccessor.Length[*it];
		}
	}
	if (para.isJnR) {
		for (auto it = jnr_reachAllInRoads_total.begin(); it != jnr_reachAllInRoads_total.end(); it++) {	//全覆盖
			sum_jnr_len += fileAccessor.Length[*it];
		}
	}
	if (para.isMDR) {
		for (auto it = mdr_reachAllInRoads_total.begin(); it != mdr_reachAllInRoads_total.end(); it++) {	//全覆盖
			sum_mdr_len += fileAccessor.Length[*it];
		}
		for (auto it = mdr_partInReachRoadNodeLen_total.begin(); it != mdr_partInReachRoadNodeLen_total.end(); it++) {
			int road_id = it->first;
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				sum_mdr_len += iter->second;
			}
		}
	}

	//数据存入results
	temp->results.result_1.sumMR_2 = sum_mr_len;
	temp->results.result_1.sumDR_2 = sum_dr_len;
	temp->results.result_1.sumJncR_2 = sum_jnr_len;
	temp->results.result_1.sumMDR_2 = sum_mdr_len;

	setFinished(true);
	return;
}

inline double calculateMRAttributesMR(ShapeFileAccessor &fileAccessor, int startRoad, std::vector<double> &distances, std::set<int> &allInRoads, std::map<int, std::vector<double>> &partInRoads) {
	//计算MR、meanMD
	double mr = 0;
	for (auto it = allInRoads.begin(); it != allInRoads.end(); it++)
	{
		int endEdge = *it;
		mr += fileAccessor.Length[endEdge];
	}
	for (auto it2 = partInRoads.begin(); it2 != partInRoads.end(); it2++)
	{
		for (int q = 0; q < int(it2->second.size()); q++)
		{
			mr += it2->second[q];
		}
	}

	return mr;
}


inline std::vector<double> calculateMRAttributes(ShapeFileAccessor &fileAccessor, int startRoad, std::vector<double> &distances, std::set<int> &allInRoads, std::map<int, std::vector<double>> &partInRoads ) {
	//计算MR、meanMD
	double mr = 0, Ls0 = 0, sumLs = 0, sumdLs = 0;
	for (auto it = allInRoads.begin(); it != allInRoads.end(); it++)
	{
		int endEdge = *it;
		mr += fileAccessor.Length[endEdge];

		//全覆盖的路：将整条路加进去
		int end_node1 = fileAccessor.roadNode[endEdge][0];
		int end_node2 = fileAccessor.roadNode[endEdge][1];

		if (endEdge == startRoad){
			//计算Ls0,sumLs
			Ls0 += pow(fileAccessor.Length[startRoad], 2) / 4;
			sumLs += fileAccessor.Length[startRoad];
			continue;
		}

		if (startRoad == 70 && endEdge == 0)
			int chack = 1;

		//计算ds
		double ds = min(distances[end_node1], distances[end_node2]) + 0.5*fileAccessor.Length[endEdge];

		//计算sumLs,sumdLs
		sumLs += fileAccessor.Length[endEdge];
		sumdLs += ds * fileAccessor.Length[endEdge];

	}
	for (auto it2 = partInRoads.begin(); it2 != partInRoads.end(); it2++)
	{
		for (int q = 0; q < int(it2->second.size()); q++)
		{
			mr += it2->second[q];

			if (it2->second[q] > 0)
			{
				//部分覆盖的路：将实际超出距离加进去
				int endEdge = it2->first;
				int end_node1 = fileAccessor.roadNode[endEdge][0];
				int end_node2 = fileAccessor.roadNode[endEdge][1];

				if (endEdge == startRoad)
				{
					//计算Ls0,sumLs
					Ls0 += pow(it2->second[q], 2) / 4;
					sumLs += it2->second[q];
					continue;
				}

				//计算ds
				double ds = 0;
				if (q == 0) {
					ds = distances[end_node1] + 0.5*it2->second[q];
				}
				else {
					ds = distances[end_node2] + 0.5*it2->second[q];
				}

				//计算sumLs,sumdLs
				sumLs += it2->second[q];
				sumdLs += ds * it2->second[q];

			}
		}
	}
	double meanmd = 0;
	if (sumLs > 0)
		meanmd = (Ls0 + sumdLs) / sumLs;

	return { mr, meanmd };
}

bool Calculation::is_all_isolation(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs, std::string type) {
	//判断是否每一个线在subsetIDs中都没有邻接边
	for (auto it = subsetIDs.begin(); it != subsetIDs.end(); it++) {
		int road_id = *it;
		//查出它的邻接边集合
		std::set<int> copy_ids(subsetIDs);
		copy_ids.erase(road_id);
		std::set<int> temp;
		set_intersection(copy_ids.begin(), copy_ids.end(), fileAccessor.AdjRoadList[road_id].begin(), fileAccessor.AdjRoadList[road_id].end(), inserter(temp, temp.begin()));
		if (temp.size() > 0)
			return false;
	}

	return true;
}

/*
项目3可理解为采样计算，即只计算子集线之间的距离，计算结果用另外的shp文件（图形＋数据表）存储
MeanDD: Mean DD within the subset;
MeanDDL: Mean DDL within the subset;
MeanMD: Mean Metric Distance within the subset;
MeanJncD: Mean Junction Distance within the subset;
*/
void Calculation::get_members_within_subset(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::set<int> &subsetIDs) {
	//相当于reach分析，但是范围缩减到子集线范围，无向图、有向图、交叉口图需要重新构建

	setFinished(false);

	//准备好构造距离无向图的数据――――旧边到新边的映射+旧端点到新端点的映射
	std::map<int, int> newMapNodes;
	std::map<int, std::vector<int>> newRoadNode;
	std::map<std::string, int> newNodeToRoad;
	std::vector<int> tmp{ 0,0 };
	for (auto it1 = subsetIDs.begin(); it1 != subsetIDs.end(); it1++){
		int edge = *it1;

		newRoadNode.insert(std::make_pair(edge, tmp));

		for (int j = 0; j < 2; j++){
			int key = fileAccessor.roadNode[edge][j];
			auto iter = newMapNodes.find(key);

			if (iter != newMapNodes.end()) {
				newRoadNode[edge][j] = newMapNodes[key];
			}
			else {
				int nodeNum = int(newMapNodes.size());
				newMapNodes.insert(std::pair<int, int>(key, nodeNum));
				newRoadNode[edge][j] = nodeNum;
			}
		}

		int sub_node1 = newRoadNode[edge][0];
		int sub_node2 = newRoadNode[edge][1];
		newNodeToRoad.insert(std::make_pair(IndexToKey(sub_node1, sub_node2), edge));
		newNodeToRoad.insert(std::make_pair(IndexToKey(sub_node2, sub_node1), edge));
	}
	//多个里程限制共用一张距离无向图――――构造有效边集构成的无向图
	Graph edgGraph;
	GenerateUndirectedGraph(fileAccessor, subsetIDs, newRoadNode, edgGraph);

	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);

	//数据结构
	std::set<int> inNode;
	std::set<int> inRoad;
	std::set<int> outRoad;
	std::set<int> validRoad;

	//mr
	std::map<int, std::vector<double>> mr_partIn;
	std::map<int, std::set<int>> mr_reachAllInRoads;
	std::map<int, std::map<int, std::vector<double>>> mr_partInRoads_all;
	std::map<int, std::map<int, std::map<int, double>>> mr_partInReachRoadNodeLen;
	std::map<int, std::map<int, std::map<int, int>>> mr_partInReachRoadNodeCoor;

	//dr
	std::map<int, std::set<int>> dr_reachAllInRoads;

	//mdr
	std::map<int, std::vector<double>> mdr_partIn;
	std::map<int, std::set<int>> mdr_reachAllInRoads;
	std::map<int, std::map<int, std::vector<double>>> mdr_partInRoads_all;
	std::map<int, std::map<int, std::map<int, double>>> mdr_partInReachRoadNodeLen;
	std::map<int, std::map<int, std::map<int, int>>> mdr_partInReachRoadNodeCoor;
	std::map<int, std::set<int>> mdr_reachRoad_all;

	//jnr
	std::map<int, std::set<int>> jnr_reachAllInRoads;

	//增加
	EdgeProperty ep1, ep2, ep3;

	//创建有向图		
	Graph_d edgGraph_DR;
	int old_numEdges = fileAccessor.roadID.size();
	GenerateDirectedGraph2(fileAccessor, subsetIDs, old_numEdges, edgGraph_DR);
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;
	std::map<int, double> new_dict_real_min_dist;

	//创建交叉口的新图
	Graph edgGraph3;
	GenerateUndirectedGraph_Jnc_Part(fileAccessor, subsetIDs, old_numEdges, edgGraph3);
	int numVertices3 = int(boost::num_vertices(edgGraph3));
	int numEdges3 = int(boost::num_edges(edgGraph3));

	std::vector<vertex_descriptor> parents3(numVertices3);
	std::vector<double> distances3(numVertices3);

	//收割数据
	//mr
	std::map<int, double> mr_all;
	//dr
	std::map<int, double> dd_all;
	std::map<int, double> ddl_all;
	//jnr
	std::map<int, double> jnc_all;

	extern bool NeedStop;
	
		for (auto it = subsetIDs.begin(); it != subsetIDs.end(); it++)	//对每条起始边
		{

			if (NeedStop) {
				return;
			}

			int startRoad = *it;

			/*************************************MR********************************************/
			if (para.isMR) {
				//修改图：删一条，加两条			
				double dis = fileAccessor.Length[startRoad] / 2;
				int startNode = numVertices;
				int node1 = newRoadNode[startRoad][0];
				int node2 = newRoadNode[startRoad][1];

				//删除边-起始边
				boost::remove_edge(node1, node2, edgGraph);

				//添加边-中点到起始边两端
				ep1.m_base = dis;
				ep1.m_value = startRoad;
				boost::add_edge(startNode, node1, ep1, edgGraph);

				ep2.m_base = dis;
				ep2.m_value = numEdges;
				boost::add_edge(startNode, node2, ep2, edgGraph);

				boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

				GetValidRoadsByMR(fileAccessor, newRoadNode, subsetIDs, numVertices, distances, para.MR_Para.mr_limit, startRoad, outRoad,
					mr_partIn, mr_reachAllInRoads, mr_partInRoads_all, mr_partInReachRoadNodeLen, mr_partInReachRoadNodeCoor);

				//计算MR属性：mr
				mr_all[startRoad] = calculateMRAttributesMR(fileAccessor, startRoad, distances, mr_reachAllInRoads[startRoad], mr_partIn);

				//把图撤销修改：删两条，加一条
				boost::remove_edge(startNode, node1, edgGraph);
				boost::remove_edge(startNode, node2, edgGraph);

				ep3.m_base = fileAccessor.Length[startRoad];
				ep3.m_value = startRoad;
				boost::add_edge(node1, node2, ep3, edgGraph);
			}

			/*************************************DR********************************************/
			int startRoad2 = startRoad + old_numEdges;

			double dr_subDRLimit = para.DR_Para.dc_limit;

			if (para.isDR && temp->is_all_isolation(temp, fileAccessor, para, subsetIDs,"DR")==false) {
				//两次有向图搜索
				boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
				boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

				std::map<int, double>().swap(dict_min_dist);
				for (int i = 0; i < numVertices_DR; i++)
					dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

				//收集DR有效的道路队列
				dict_real_min_dist.clear();
				for (auto road_it = subsetIDs.begin(); road_it != subsetIDs.end(); road_it++) {
					int k = *road_it;
					//找出有向边i对应的反向边id
					int reverse_k = k + old_numEdges;
					double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
					dict_real_min_dist.insert(std::make_pair(k, min_d));
				}

				//计算DR属性：dd、ddl
				std::map<int, std::set<int>> ddMap;
				double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
				for (auto road_it = subsetIDs.begin(); road_it != subsetIDs.end(); road_it++) {
					int road_id = *road_it;

					ddMap[int(dict_real_min_dist[road_id])].insert(road_id);
					M_ddlSum += dict_real_min_dist[road_id] * fileAccessor.Length[road_id];
					M_lenSum += fileAccessor.Length[road_id];
				}
				double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
				for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++) {
					M_ddSum1 += ((it3->first)*(it3->second.size()));
					M_ddSum2 += it3->second.size();
				}
				M_dd = M_ddSum1 / M_ddSum2;
				M_ddl = M_ddlSum / M_lenSum;
				dd_all[startRoad] = M_dd;
				ddl_all[startRoad] = M_ddl;
			}

			/*************************************JncR********************************************/
			if (para.isJnR  && temp->is_all_isolation(temp, fileAccessor, para, subsetIDs, "JnR")==false) {
				boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

				//计算JnR属性：jnc
				double jnc = 0;
				for (auto road_it = subsetIDs.begin(); road_it != subsetIDs.end(); road_it++) {
					jnc += distances3[*road_it];
				}
				jnc_all[startRoad] = jnc / double(subsetIDs.size());
			}

			addFinishedCount();
		}


	//为csv准备数据
	temp->results.result_1.dd = dd_all;
	temp->results.result_1.ddl = ddl_all;
	temp->results.result_1.mean_mr = mr_all;
	temp->results.result_1.mean_jnr = jnc_all;

	//计算综合指标
	double mr = 0, dd = 0, ddl = 1, jnc = 0;
	for (auto road_it = subsetIDs.begin(); road_it != subsetIDs.end(); road_it++) {
		int road_id = *road_it;
		mr += mr_all[road_id];
		dd += dd_all[road_id];
		ddl += ddl_all[road_id];
		jnc += jnc_all[road_id];
	}
	temp->results.result_1.meanMD_3 = mr / double(subsetIDs.size());
	temp->results.result_1.meanDD_3 = dd / double(subsetIDs.size());
	temp->results.result_1.meanDDL_3 = ddl / double(subsetIDs.size());
	temp->results.result_1.meanJnCD_3 = jnc / double(subsetIDs.size());

	setFinished(true);
	return;
}

/************************************************多组子集线分析********************************************************/
void Calculation::calculate_subset_2(ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs) {
	//初始换状态集
	FinishedVec.clear();
	subset_finishedCount = 0;
	needProcessCount = 0;
	for (auto it = SubsetIDs.begin(); it != SubsetIDs.end(); it++) {
		needProcessCount += it->second.size();
	}
	//因为两组数据互相针对，实际计算数目是2倍
	needProcessCount *= 2;

	std::thread thrd_1(&Calculation::get_members_between_subsets, this, fileAccessor, para, SubsetIDs);
	thrd_1.detach();

	std::thread thrd_2(&Calculation::get_nearest_distance_to_other_subsets, this, fileAccessor, para, SubsetIDs);
	thrd_2.detach();
}

void Calculation::CalaculateAverDistace(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs, int pos_1, int pos_2,
	Graph &edgGraph, Graph_d &edgGraph_DR, Graph &edgGraph3, std::map<int, std::map<std::string, std::map<int, double>>> &attributes) {

	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);

	//数据结构
	std::map<int, std::vector<double>> mr_distanceAll;
	std::map<int, double> dd_distanceAll;
	std::map<int, double> ddl_distanceAll;
	std::map<int, std::vector<double>> jnr_distanceAll;

	//增加
	EdgeProperty ep1, ep2, ep3;

	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;

	int numVertices3 = int(boost::num_vertices(edgGraph3));
	int numEdges3 = int(boost::num_edges(edgGraph3));

	std::vector<vertex_descriptor> parents3(numVertices3);
	std::vector<double> distances3(numVertices3);

	extern bool NeedStop;
	for (auto startRoad_it = SubsetIDs[pos_1].begin(); startRoad_it != SubsetIDs[pos_1].end(); startRoad_it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *startRoad_it;

		/*************************************Step_MR********************************************/
		if (para.isMR) {
			//修改图：删一条，加两条			
			double dis = fileAccessor.Length[startRoad] / 2;
			int startNode = numVertices;
			int node1 = fileAccessor.roadNode[startRoad][0];
			int node2 = fileAccessor.roadNode[startRoad][1];

			//删除边-起始边
			boost::remove_edge(node1, node2, edgGraph);

			//添加边-中点到起始边两端
			ep1.m_base = dis;
			ep1.m_value = startRoad;
			boost::add_edge(startNode, node1, ep1, edgGraph);

			ep2.m_base = dis;
			ep2.m_value = numEdges;
			boost::add_edge(startNode, node2, ep2, edgGraph);

			boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

			for (auto endRoad_it = SubsetIDs[pos_2].begin(); endRoad_it != SubsetIDs[pos_2].end(); endRoad_it++) {
				int endRoad = *endRoad_it;

				int endnode = 0;
				int endnode1 = fileAccessor.roadNode[endRoad][0];
				int endnode2 = fileAccessor.roadNode[endRoad][1];

				//获取短的分支			
				if (distances[endnode1] < distances[endnode2])
					endnode = endnode1;
				else
					endnode = endnode2;

				double len = distances[endnode] + 0.5 * fileAccessor.Length[endRoad];
				if (endRoad == startRoad)
					len = 0;

				mr_distanceAll[endRoad].push_back(len);
			}

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);
		}

		/*************************************Step_DR********************************************/
		int startRoad2 = startRoad + old_numEdges;

		if (para.isDR) {
			//两次有向图搜索
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

			std::map<int, double>().swap(dict_min_dist);
			for (int i = 0; i < numVertices_DR; i++)
				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));

			//收集DR有效的道路队列
			dict_real_min_dist.clear();
			for (int k = 0; k < old_numEdges; k++) {
				//找出有向边i对应的反向边id
				int reverse_k = k + old_numEdges;
				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				dict_real_min_dist.insert(std::make_pair(k, min_d));
			}

			//计算dd、ddl：计算起点线到终点线所有通行路径的dd与ddl
			double aver_dd = 0, aver_ddl = 0;
			std::vector<int> tmp_routeRoads;
			std::set<int> lineRoads;
			for (auto road_it = SubsetIDs[pos_2].begin(); road_it != SubsetIDs[pos_2].end(); road_it++) {
				int road_id = *road_it;

				//搜索路径加入reached_roads
				getInRoads_DR2(tmp_routeRoads, lineRoads, fileAccessor, startRoad, road_id, parents1, distances1, parents2, distances2, old_numEdges);
				
				std::map<int, std::set<int>> ddMap;
				double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
				for (auto road_it = lineRoads.begin(); road_it != lineRoads.end(); road_it++) {
					int road_id = *road_it;

					ddMap[int(dict_real_min_dist[road_id])].insert(road_id);
					M_ddlSum += dict_real_min_dist[road_id] * fileAccessor.Length[road_id];
					M_lenSum += fileAccessor.Length[road_id];
				}
				double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
				for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++) {
					M_ddSum1 += ((it3->first)*(it3->second.size()));
					M_ddSum2 += it3->second.size();
				}
				M_dd = M_ddSum1 / M_ddSum2;
				M_ddl = M_ddlSum / M_lenSum;

				aver_dd += M_dd;
				aver_ddl += M_ddl;
			}
			dd_distanceAll[startRoad] = aver_dd / double(SubsetIDs[pos_2].size());
			ddl_distanceAll[startRoad] = aver_ddl / double(SubsetIDs[pos_2].size());
		}
		
		/*************************************Step_JnR********************************************/
		if (para.isJnR) {
			boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

			for (auto endRoad_it = SubsetIDs[pos_2].begin(); endRoad_it != SubsetIDs[pos_2].end(); endRoad_it++) {
				int endRoad = *endRoad_it;
				jnr_distanceAll[endRoad].push_back(distances3[endRoad]);
			}
		}

		addFinishedCount();
	}

	//收集数据，计算最短
	std::map<int, double> mr_averStartAll;
	std::map<int, double> jnr_averStartAll;
	int start_pos = 0;
	for (auto Road_it = SubsetIDs[pos_1].begin(); Road_it != SubsetIDs[pos_1].end(); Road_it++) {
		int road_id = *Road_it;

		double sum_mr = 0, sum_jnr = 0;
		for (auto iter = SubsetIDs[pos_2].begin(); iter != SubsetIDs[pos_2].end(); iter++) {
			int endRoad = *iter;
			if (para.isMR)
				sum_mr += mr_distanceAll[endRoad][start_pos];
			if (para.isJnR)
				sum_jnr += jnr_distanceAll[endRoad][start_pos];
		}
		mr_averStartAll[road_id] = sum_mr / double(SubsetIDs[pos_2].size());
		jnr_averStartAll[road_id] = sum_jnr / double(SubsetIDs[pos_2].size());

		++start_pos;
	}

	//计算综合指标
	double meanMD = 0, meanDD = 0, meanDDL = 0, meanJnC = 0;
	for (auto endRoad_it = SubsetIDs[pos_1].begin(); endRoad_it != SubsetIDs[pos_1].end(); endRoad_it++) {
		int endRoad = *endRoad_it;

		meanMD += mr_averStartAll[endRoad];
		meanJnC += jnr_averStartAll[endRoad];
	}
	for (auto endRoad_it = SubsetIDs[pos_1].begin(); endRoad_it != SubsetIDs[pos_1].end(); endRoad_it++) {
		int endRoad = *endRoad_it;

		meanDD += dd_distanceAll[endRoad];
		meanDDL += ddl_distanceAll[endRoad];
	}

	//为csv准备数据
	std::pair<int, int> tmp_1 = { pos_1,pos_2 };
	temp->results.result_2.aver_dd[tmp_1] = dd_distanceAll;
	temp->results.result_2.aver_ddl[tmp_1] = ddl_distanceAll;
	temp->results.result_2.aver_mean_mr[tmp_1] = mr_averStartAll;
	temp->results.result_2.aver_mean_jnr[tmp_1] = jnr_averStartAll;

	//存入results
	std::pair<int, int> pos_pair = { pos_1,pos_2 };
	temp->results.result_2.result_1[pos_pair].meanMD = meanMD / double(SubsetIDs[pos_1].size());
	temp->results.result_2.result_1[pos_pair].meanDD = meanDD / double(SubsetIDs[pos_1].size());
	temp->results.result_2.result_1[pos_pair].meanDDL = meanDDL / double(SubsetIDs[pos_1].size());
	temp->results.result_2.result_1[pos_pair].meanJnCD = meanJnC / double(SubsetIDs[pos_1].size());
}

//项目1：计算从一个子集中的每条线到另外一个子集中每条线的平均米制距离、平均转弯距离、平均交叉口距离
void Calculation::get_members_between_subsets(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs) {
	setFinished(false);
	
	//无向图复制
	Graph edgGraph;
	Graph *g = fileAccessor.ProcessShapeFile();
	edgGraph.copy_impl(*g);

	//创建有向图		
	Graph_d edgGraph_DR;
	GenerateDirectedGraph(fileAccessor, edgGraph_DR);

	//创建交叉口的新图
	Graph edgGraph3;
	int old_numEdges = int(boost::num_edges(*g));
	GenerateUndirectedGraph_Jnc(fileAccessor, old_numEdges, edgGraph3);

	std::map<int, std::map<std::string, std::map<int, double>>> attributes;

	//执行搜索
	std::set<std::pair<int, int>> pairs = GetAllPairs(SubsetIDs.size());
	for (auto it = pairs.begin(); it != pairs.end(); it++) {
		std::pair<int, int> pair = *it;
		CalaculateAverDistace(temp, fileAccessor, para, SubsetIDs, pair.first, pair.second, edgGraph, edgGraph_DR, edgGraph3, attributes);
	}

	////输出文件
	//std::string file_dir_path = para.FileDirPath + "/" + "Subset_2_1";
	////创建文件夹
	//bool flag = RemoveDirectory(file_dir_path.c_str());		//删除现有文件夹
	//flag = CreateDirectory(file_dir_path.c_str(), NULL);	//创建文件夹
	////写入shp文件
	//std::string shpfilename = file_dir_path + "/" + para.FileName.substr(0, para.FileName.size() - 4) + ".shp";
	//ouputSubsetPartReachAll(fileAccessor, SubsetIDs, attributes, shpfilename);

	setFinished(true);
	return;
}

void Calculation::CalculateMinDistace(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs, int pos_1, int pos_2,
	Graph &edgGraph, Graph_d &edgGraph_DR, Graph &edgGraph3, std::map<int, std::map<std::string, std::map<int, double>>> &attributes,
	std::map<std::pair<int, int>, std::map<std::string, std::set<int>>> &routeRoads) {

	int numEdges = int(boost::num_edges(edgGraph));
	int numVertices = int(boost::num_vertices(edgGraph));

	//准备接数据
	std::vector<vertex_descriptor> parents(numVertices + 1);
	std::vector<double> distances(numVertices + 1);
	//增加
	EdgeProperty ep1, ep2, ep3;
	Graph *g = fileAccessor.ProcessShapeFile();
	int old_numEdges = int(boost::num_edges(*g));
	int numVertices_DR = int(boost::num_vertices(edgGraph_DR));

	std::vector<vertex_descriptor> parents1(numVertices_DR);
	std::vector<double> distances1(numVertices_DR);
	std::vector<vertex_descriptor> parents2(numVertices_DR);
	std::vector<double> distances2(numVertices_DR);

	//记录转弯的祖先
	std::map<int, std::set<int>> parents_dr;
	std::map<int, std::set<int>> parents_DR;

	std::map<int, double> dict_min_dist;
	std::map<int, double> dict_real_min_dist;

	int numVertices3 = int(boost::num_vertices(edgGraph3));
	int numEdges3 = int(boost::num_edges(edgGraph3));

	std::vector<vertex_descriptor> parents3(numVertices3);
	std::vector<double> distances3(numVertices3);

	//数据结构
	std::map<int, std::set<double>> mr_distanceAll;
	std::map<int, std::map<double, std::set<int>>> mr_distanceAll_Roads;
	std::map<int, std::set<double>> dr_distanceAll;
	std::map<int, std::map<double, std::set<int>>> dr_distanceAll_Roads;
	std::map<int, std::map<double, double>> dr_distanceAll_lens;
	std::map<int, std::set<double>> jnr_distanceAll;
	std::map<int, std::map<double, std::set<int>>> jnr_distanceAll_Roads;
	std::map<int, std::map<double, double>> jnr_distanceAll_lens;

	std::map<int, double> dd_distanceAll;
	std::map<int, double> ddl_distanceAll;

	extern bool NeedStop;
	for (auto startRoad_it = SubsetIDs[pos_1].begin(); startRoad_it != SubsetIDs[pos_1].end(); startRoad_it++)	//对每条起始边
	{
		if (NeedStop) {
			return;
		}

		int startRoad = *startRoad_it;

		/*************************************Step_MR********************************************/
		if (para.isMR) {
			//修改图：删一条，加两条			
			double dis = fileAccessor.Length[startRoad] / 2;
			int startNode = numVertices;
			int node1 = fileAccessor.roadNode[startRoad][0];
			int node2 = fileAccessor.roadNode[startRoad][1];

			//删除边-起始边
			boost::remove_edge(node1, node2, edgGraph);

			//添加边-中点到起始边两端
			ep1.m_base = dis;
			ep1.m_value = startRoad;
			boost::add_edge(startNode, node1, ep1, edgGraph);

			ep2.m_base = dis;
			ep2.m_value = numEdges;
			boost::add_edge(startNode, node2, ep2, edgGraph);

			boost::dijkstra_shortest_paths(edgGraph, startNode, boost::predecessor_map(&parents[0]).distance_map(&distances[0]));

			for (auto endRoad_it = SubsetIDs[pos_2].begin(); endRoad_it != SubsetIDs[pos_2].end(); endRoad_it++) {
				int endRoad = *endRoad_it;

				int endnode = 0;
				int endnode1 = fileAccessor.roadNode[endRoad][0];
				int endnode2 = fileAccessor.roadNode[endRoad][1];

				//获取短的分支			
				if (distances[endnode1] < distances[endnode2])
					endnode = endnode1;
				else
					endnode = endnode2;

				double len = distances[endnode] + 0.5 * fileAccessor.Length[endRoad];
				if (endRoad == startRoad)
					len = 0;

				//mr_distanceAll[endRoad].insert(len);
				mr_distanceAll[startRoad].insert(len);

				//收集路径
				std::vector<int> routeRoads;
				std::set<int> lineRoads;
				getInRoads_MR2(routeRoads, lineRoads, fileAccessor, startRoad, endRoad, startNode, parents, distances);

				if (mr_distanceAll_Roads[startRoad].count(len) == 0) {	//保存第一次搜索到的最短路径
					mr_distanceAll_Roads[startRoad][len] = lineRoads;
				}
			}

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);

			//把图撤销修改：删两条，加一条
			boost::remove_edge(startNode, node1, edgGraph);
			boost::remove_edge(startNode, node2, edgGraph);

			ep3.m_base = fileAccessor.Length[startRoad];
			ep3.m_value = startRoad;
			boost::add_edge(node1, node2, ep3, edgGraph);
		}

		/*************************************Step_DR********************************************/
		int startRoad2 = startRoad + old_numEdges;

		if (para.isDR) {
			//两次有向图搜索
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad, boost::predecessor_map(&parents1[0]).distance_map(&distances1[0]));
			boost::dijkstra_shortest_paths(edgGraph_DR, startRoad2, boost::predecessor_map(&parents2[0]).distance_map(&distances2[0]));

			//清空旧数据
			parents_dr.clear();
			parents_DR.clear();

			std::map<int, double>().swap(dict_min_dist);
			for (int i = 0; i < numVertices_DR; i++) {
				dict_min_dist.insert(std::make_pair(i, min(distances1[i], distances2[i])));
				if (distances1[i] < distances2[i]) {
					parents_dr[i].insert(parents1[i] >= old_numEdges ? parents1[i] - old_numEdges : parents1[i]);
				}
				else if (distances1[i] > distances2[i]) {
					parents_dr[i].insert(parents2[i] >= old_numEdges ? parents2[i] - old_numEdges : parents2[i]);
				}
				else {
					parents_dr[i].insert(parents1[i] >= old_numEdges ? parents1[i] - old_numEdges : parents1[i]);
					parents_dr[i].insert(parents2[i] >= old_numEdges ? parents2[i] - old_numEdges : parents2[i]);
				}
			}

			//收集DR有效的道路队列
			dict_real_min_dist.clear();
			for (int k = 0; k < old_numEdges; k++) {
				//找出有向边i对应的反向边id
				int reverse_k = k + old_numEdges;
				double min_d = min(dict_min_dist[k], dict_min_dist[reverse_k]);
				dict_real_min_dist.insert(std::make_pair(k, min_d));

				//记录祖先
				if (dict_min_dist[k] < dict_min_dist[reverse_k]) {
					parents_DR[k] = parents_dr[k];
				}
				else if (dict_min_dist[k] > dict_min_dist[reverse_k]) {
					parents_DR[k] = parents_dr[reverse_k];
				}
				else {
					parents_DR[k].insert(parents_dr[k].begin(), parents_dr[k].end());
					parents_DR[k].insert(parents_dr[reverse_k].begin(), parents_dr[reverse_k].end());
				}
			}

			//计算dd、ddl：计算起点线到终点线所有通行路径的dd与ddl
			double min_dd = INT_MAX, min_ddl = INT_MAX;
			std::vector<int> tmp_routeRoads;
			std::set<int> lineRoads;
			for (auto road_it = SubsetIDs[pos_2].begin(); road_it != SubsetIDs[pos_2].end(); road_it++) {
				int road_id = *road_it;

				//搜索路径加入reached_roads
				getInRoads_DR2(tmp_routeRoads, lineRoads, fileAccessor, startRoad, road_id, parents1, distances1, parents2, distances2, old_numEdges);
				
				//计算dd、ddl
				std::map<int, std::set<int>> ddMap;
				double M_ddlSum = 0, M_lenSum = 0, M_ddl = 0;
				for (auto road_it = lineRoads.begin(); road_it != lineRoads.end(); road_it++) {
					int road_id = *road_it;

					ddMap[int(dict_real_min_dist[road_id])].insert(road_id);
					M_ddlSum += dict_real_min_dist[road_id] * fileAccessor.Length[road_id];
					M_lenSum += fileAccessor.Length[road_id];
				}
				double M_dd = 0, M_ddSum1 = 0, M_ddSum2 = 0;
				for (auto it3 = ddMap.begin(); it3 != ddMap.end(); it3++) {
					M_ddSum1 += ((it3->first)*(it3->second.size()));
					M_ddSum2 += it3->second.size();
				}
				M_dd = M_ddSum1 / M_ddSum2;
				M_ddl = M_ddlSum / M_lenSum;

				min_dd = min(min_dd, M_dd);
				min_ddl = min(min_ddl, M_ddl);
			}
			dd_distanceAll[startRoad] = min_dd;
			ddl_distanceAll[startRoad] = min_ddl;

			for (auto endRoad_it = SubsetIDs[pos_2].begin(); endRoad_it != SubsetIDs[pos_2].end(); endRoad_it++) {
				int endRoad = *endRoad_it;
				//dr_distanceAll[endRoad].insert(dict_real_min_dist[endRoad]);
				dr_distanceAll[startRoad].insert(dict_real_min_dist[endRoad]);

				if (endRoad == 0)
					int chaec = 1;

				//收集路径
				std::vector<int> path;
				std::set<std::vector<int>> pathAll;
				dr_last_size = 0;
				dr_failed_count = 0;
				SearchDRPath(fileAccessor, dict_real_min_dist, parents_DR, pathAll, path, endRoad, startRoad);

				if (pathAll.size() == 0)
					int chaec = 1;

				//判断有没有多条最短路径
				bool isMulti = false;
				std::vector<int> routeRoads;
				std::set<int> lineRoads;
				if (pathAll.size() > 1) {
					double min_len = 0x7fffffff;
					for (auto tt = pathAll.begin(); tt != pathAll.end(); tt++) {
						double len = 0;
						for (int k = 0; k < (*tt).size(); k++) len += fileAccessor.Length[(*tt)[k]];
						if (len < min_len) {
							min_len = len;
							routeRoads = *tt;
						}
					}
					isMulti = true;
				}
				else if (pathAll.size() == 1) {
					auto tt = pathAll.begin();
					routeRoads = *tt;
				}
				else {
					//把经历的道路建一个队列
					getInRoads_DR2(routeRoads, lineRoads, fileAccessor, startRoad, endRoad, parents1, distances1, parents2, distances2, old_numEdges);
				}

				lineRoads.insert(routeRoads.begin(), routeRoads.end());

				////收集代价小于终点线的道路
				//std::set<int> valid_roads;
				//double dc_limit = dict_real_min_dist[endRoad];
				//for (int k = 0; k < old_numEdges; k++) {
				//	if (dict_real_min_dist[k] <= dc_limit)
				//		valid_roads.insert(k);
				//}

				////重新做mr搜索出一条到终点线的最短路径
				//std::set<int> lineRoads;
				//SearchDRPathMR(fileAccessor, valid_roads, lineRoads, startRoad, endRoad);

				double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endRoad);

				int dc = dict_real_min_dist[endRoad];
				if (dr_distanceAll_Roads[startRoad].count(dc) == 0 || len < dr_distanceAll_lens[startRoad][dc]) {
					dr_distanceAll_Roads[startRoad][dc] = lineRoads;
					dr_distanceAll_lens[startRoad][dc] = len;
				}
			}
		}

		/*************************************Step_JnR********************************************/
		boost::dijkstra_shortest_paths(edgGraph3, startRoad, boost::predecessor_map(&parents3[0]).distance_map(&distances3[0]));

		for (auto endRoad_it = SubsetIDs[pos_2].begin(); endRoad_it != SubsetIDs[pos_2].end(); endRoad_it++) {
			int endRoad = *endRoad_it;
			//jnr_distanceAll[endRoad].insert(distances3[endRoad]);
			jnr_distanceAll[startRoad].insert(distances3[endRoad]);

			//收集路径
			std::vector<int> path;
			std::set<std::vector<int>> pathAll;
			dr_last_size = 0;
			dr_failed_count = 0;
			SearchJnRPath(fileAccessor, distances3, parents3, pathAll, path, endRoad, startRoad);

			//判断有没有多条最短路径
			bool isMulti = false;
			std::vector<int> routeRoads;
			std::set<int> lineRoads;
			if (pathAll.size() > 1) {
				double min_len = 0x7fffffff;
				for (auto tt = pathAll.begin(); tt != pathAll.end(); tt++) {
					double len = 0;
					for (int k = 0; k < (*tt).size(); k++) len += fileAccessor.Length[(*tt)[k]];
					if (len < min_len) {
						min_len = len;
						routeRoads = *tt;
					}
				}
				isMulti = true;
			}
			else if (pathAll.size() == 1) {
				auto tt = pathAll.begin();
				routeRoads = *tt;
			}
			else {
				//把经历的道路建一个队列
				getInRoads_JncR2(routeRoads, lineRoads, fileAccessor, startRoad, endRoad, parents3, distances3);
			}
			
			lineRoads.insert(routeRoads.begin(), routeRoads.end());

			////收集代价小于终点线的道路
			//std::set<int> valid_roads;
			//double dc_limit = distances3[endRoad];
			//for (int k = 0; k < old_numEdges; k++) {
			//	if (dict_real_min_dist[k] <= dc_limit)
			//		valid_roads.insert(k);
			//}

			////重新做mr搜索出一条到终点线的最短路径
			//std::set<int> lineRoads;
			//SearchDRPathMR(fileAccessor, valid_roads, lineRoads, startRoad, endRoad);

			double len = CalculateRelen(fileAccessor, lineRoads, startRoad, endRoad);

			int jnc = distances3[endRoad];
			if (jnr_distanceAll_Roads[startRoad].count(jnc) == 0 || len < jnr_distanceAll_lens[startRoad][jnc]) {
				jnr_distanceAll_Roads[startRoad][jnc] = lineRoads;
				jnr_distanceAll_lens[startRoad][jnc] = len;
			}
		}

		addFinishedCount();
	}

	//收集数据，计算最短
	std::map<int, double> mr_minStartAll;
	std::map<int, double> dr_minStartAll;
	std::map<int, double> jnr_minStartAll;
	std::pair<int, int> tmp = { pos_1,pos_2 };
	double min_mr = INT_MAX, min_dr = INT_MAX, min_jnr = INT_MAX;
	for (auto startRoad_it = SubsetIDs[pos_1].begin(); startRoad_it != SubsetIDs[pos_1].end(); startRoad_it++) {
		int startRoad = *startRoad_it;

		//这里计算的是：以第一组子集线作为起点，第二组子集线作为终点线的最近终点线的mr距离
		auto mr_iter = mr_distanceAll[startRoad].begin();
		mr_minStartAll.insert(std::make_pair(startRoad, *mr_iter));

		auto dr_iter = dr_distanceAll[startRoad].begin();
		dr_minStartAll.insert(std::make_pair(startRoad, *dr_iter));

		auto jnr_iter = jnr_distanceAll[startRoad].begin();
		jnr_minStartAll.insert(std::make_pair(startRoad, *jnr_iter));

		if (para.isMR && mr_distanceAll_Roads[startRoad].begin()->first < min_mr) {
			routeRoads[tmp]["MR"] = mr_distanceAll_Roads[startRoad].begin()->second;
			min_mr = mr_distanceAll_Roads[startRoad].begin()->first;
		}
		if (para.isDR && dr_distanceAll_Roads[startRoad].begin()->first < min_dr) {
			routeRoads[tmp]["DR"] = dr_distanceAll_Roads[startRoad].begin()->second;
			min_dr = dr_distanceAll_Roads[startRoad].begin()->first;
		}
		if (para.isJnR && jnr_distanceAll_Roads[startRoad].begin()->first < min_jnr) {
			routeRoads[tmp]["JnR"] = jnr_distanceAll_Roads[startRoad].begin()->second;
			min_jnr = jnr_distanceAll_Roads[startRoad].begin()->first;
		}
		
	}

	//收集数据
	attributes[pos_2]["mr"] = mr_minStartAll;
	attributes[pos_2]["dr"] = dr_minStartAll;
	attributes[pos_2]["jnr"] = jnr_minStartAll;

	//计算综合指标
	double meanMD = 0, meanDD = 0, meanDDL = 0, meanJnC = 0;
	for (auto endRoad_it = SubsetIDs[pos_1].begin(); endRoad_it != SubsetIDs[pos_1].end(); endRoad_it++) {
		int endRoad = *endRoad_it;

		meanMD += mr_minStartAll[endRoad];
		meanJnC += jnr_minStartAll[endRoad];
	}
	for (auto endRoad_it = SubsetIDs[pos_1].begin(); endRoad_it != SubsetIDs[pos_1].end(); endRoad_it++) {
		int endRoad = *endRoad_it;

		meanDD += dd_distanceAll[endRoad];
		meanDDL += ddl_distanceAll[endRoad];
	}

	//为csv准备数据
	std::pair<int, int> tmp_1 = { pos_1,pos_2 };
	temp->results.result_2.min_dd[tmp_1] = dd_distanceAll;
	temp->results.result_2.min_ddl[tmp_1] = ddl_distanceAll;
	temp->results.result_2.min_mr[tmp_1] = mr_minStartAll;
	temp->results.result_2.min_jnr[tmp_1] = jnr_minStartAll;

	//存入results
	std::pair<int, int> pos_pair = { pos_1,pos_2 };
	temp->results.result_2.result_2[pos_pair].meanMD = meanMD / double(SubsetIDs[pos_1].size());
	temp->results.result_2.result_2[pos_pair].meanDD = meanDD / double(SubsetIDs[pos_1].size());
	temp->results.result_2.result_2[pos_pair].meanDDL = meanDDL / double(SubsetIDs[pos_1].size());
	temp->results.result_2.result_2[pos_pair].meanJnCD = meanJnC / double(SubsetIDs[pos_1].size());
}

//项目2：计算从一个子集中的每条线到另外一个子集的最近线之间的米制距离、转弯距离、交叉口距离：与模块2（1）相似，如果将子集外部的线看作另外一个子集的话
void Calculation::get_nearest_distance_to_other_subsets(Calculation *temp, ShapeFileAccessor &fileAccessor, subsetPara &para, std::map<int, std::set<int>> &SubsetIDs) {
	setFinished(false);
	
	//清空旧的绘图备用数据
	temp->subset_reachRoad_all.clear();
	temp->subset_partInRoads_all.clear();

	//无向图复制
	Graph edgGraph;
	Graph *g = fileAccessor.ProcessShapeFile();
	edgGraph.copy_impl(*g);
	
	//数据结构
	std::map<int, std::set<double>> mr_distanceAll;
	std::map<int, std::set<double>> dr_distanceAll;
	std::map<int, std::set<double>> jnr_distanceAll;

	//创建有向图		
	Graph_d edgGraph_DR;
	GenerateDirectedGraph(fileAccessor, edgGraph_DR);	

	//创建交叉口的新图
	Graph edgGraph3;
	int old_numEdges = int(boost::num_edges(*g));
	GenerateUndirectedGraph_Jnc(fileAccessor, old_numEdges, edgGraph3);

	std::map<int, std::map<std::string, std::map<int, double>>> attributes;
	std::map<std::pair<int, int>, std::map<std::string, std::set<int>>> routeRoads;

	//执行搜索
	std::set<std::pair<int, int>> pairs = GetAllPairs(SubsetIDs.size());
	for (auto it = pairs.begin(); it != pairs.end(); it++) {
		std::pair<int, int> pair = *it;
		CalculateMinDistace(temp, fileAccessor, para, SubsetIDs, pair.first, pair.second, edgGraph, edgGraph_DR, edgGraph3, attributes, routeRoads);
	}

	//收集路径
	for (auto it = routeRoads.begin(); it != routeRoads.end(); it++) {
		temp->subset_reachRoad_all["MR"].insert(it->second["MR"].begin(), it->second["MR"].end());
		temp->subset_reachRoad_all["DR"].insert(it->second["DR"].begin(), it->second["DR"].end());
		temp->subset_reachRoad_all["JnR"].insert(it->second["JnR"].begin(), it->second["JnR"].end());
		break;
	}

	////输出文件
	//std::string file_dir_path = para.FileDirPath + "/" + "Subset_2_2";
	////创建文件夹
	//bool flag = RemoveDirectory(file_dir_path.c_str());		//删除现有文件夹
	//flag = CreateDirectory(file_dir_path.c_str(), NULL);	//创建文件夹
	////写入shp文件
	//std::string shpfilename = file_dir_path + "/" + para.FileName.substr(0, para.FileName.size() - 4) + ".shp";
	//ouputSubsetPartReachAll(fileAccessor, SubsetIDs, attributes, shpfilename);

	setFinished(true);
	return;
}
