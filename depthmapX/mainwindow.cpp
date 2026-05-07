// Copyright (C) 2011-2012, Tasos Varoudis

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "mainwindow.h"

#include "depthmapX/views/depthmapview/depthmapview.h"
#include "depthmapX/views/3dview/3dview.h"
#include "depthmapX/views/plotview/plotview.h"
#include "depthmapX/views/tableview/tableview.h"
#include "dialogs/AboutDlg.h"
#include "dialogs/settings/settingsdialog.h"
#include "depthmapX/CSVProcess.h"

#include <QtGui>
#include <QDesktopServices>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>

#include <algorithm>
#include <string>
#include <qstring>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <chrono>
#include <functional>
#include <QGuiApplication>
#include <QScreen>
#include <sys/stat.h>
#include <type_traits>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <unistd.h>
#endif
using DWORD = unsigned long;
using DWORDLONG = unsigned long long;
using HANDLE = void *;
struct SYSTEM_INFO {
	unsigned int dwNumberOfProcessors;
};
struct MEMORYSTATUSEX {
	unsigned long dwLength;
	DWORDLONG ullTotalPhys;
	DWORDLONG ullAvailPhys;
};
constexpr int SM_CXSCREEN = 0;
constexpr int SM_CYSCREEN = 1;
inline void Sleep(unsigned int milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}
inline HANDLE GetCurrentThread()
{
	return nullptr;
}
inline DWORD GetCurrentThreadId()
{
	return static_cast<DWORD>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}
inline void GetSystemInfo(SYSTEM_INFO *systemInfo)
{
	systemInfo->dwNumberOfProcessors = std::max(1u, std::thread::hardware_concurrency());
}
inline bool GlobalMemoryStatusEx(MEMORYSTATUSEX *statex)
{
	DWORDLONG total = 8ULL * 1024ULL * 1024ULL * 1024ULL;
#if defined(__APPLE__)
	uint64_t memsize = 0;
	size_t size = sizeof(memsize);
	if (sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0) == 0 && memsize > 0) {
		total = static_cast<DWORDLONG>(memsize);
	}
#elif defined(__linux__)
	long pages = sysconf(_SC_PHYS_PAGES);
	long pageSize = sysconf(_SC_PAGE_SIZE);
	if (pages > 0 && pageSize > 0) {
		total = static_cast<DWORDLONG>(pages) * static_cast<DWORDLONG>(pageSize);
	}
#endif
	statex->ullTotalPhys = total;
	statex->ullAvailPhys = total / 2;
	return true;
}
inline unsigned long long SetThreadAffinityMask(HANDLE, long long)
{
	return 0;
}
inline int GetSystemMetrics(int metric)
{
	const QScreen *screen = QGuiApplication::primaryScreen();
	const QRect geometry = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
	return metric == SM_CYSCREEN ? geometry.height() : geometry.width();
}
template <typename A, typename B>
inline typename std::common_type<A, B>::type min(A a, B b)
{
	typedef typename std::common_type<A, B>::type Result;
	return a < b ? static_cast<Result>(a) : static_cast<Result>(b);
}
template <typename A, typename B>
inline typename std::common_type<A, B>::type max(A a, B b)
{
	typedef typename std::common_type<A, B>::type Result;
	return a > b ? static_cast<Result>(a) : static_cast<Result>(b);
}
#endif
#include <iostream>
#include <fstream>

#include "GraphDefine.h"
#include <stdlib.h>
#include "shapefil.h"
#include <stdio.h>
#include <regex>
#include <string.h>
#include <vector>
#include <mutex>
#include<cmath>

/* 状态说明
* |  run start   | calculate over    |     状态     |
  | ------------ | ----------------- | -------------|
  |     0        |         0         |    未启动    |
  |     0        |         1         |    可写csv   |
  |     1        |         0         |    计算中    |
  |     1        |         1         |   计算完毕   |
  
*/
volatile bool MainWindow::run_start = false; //开始计算的标识
volatile bool MainWindow::needOver = false; //需要结束的标识
volatile bool MainWindow::calculate_over = false; //计算完毕的标识(计算完毕的路数量==总路数量)
int MainWindow::process_pos = 0;
QString MainWindow::process_str = "";

//多线程
int MainWindow::thread_num = 4;
clock_t MainWindow::T_timeBegin;
clock_t MainWindow::T_timeEnd;
std::vector<Calculation> MainWindow::CAVec;

//ofstream logOut("test/run_log.txt");

bool NeedStop;		//是否停止

//运行时间
QString time_qstr = "";

//weights
std::vector<std::string> weightAttributesSet;

//坐标数据
AttributesData Attributes;

//锁
std::mutex mutex_t;
std::mutex mutex_fc;

std::mutex mutex_run_start;
std::mutex mutex_needOver;
std::mutex mutex_NeedStop;
std::mutex mutex_calculate_over;
std::mutex mutex_process_count;

//转换后的data map名称
QString data_map_name = "";

//Add
QString mean_angle_qstr = "";

//子集线
subsetPara subset_para;

//线条数目
int lines_count = INT_MAX - 1;

//鼠标select
std::vector<int> Ref_number_list;
QString ref_numer = "";
QString ref_numer_last = "";

//线条颜色控制
std::map<std::string, std::map<int, PafColor>> FixedColoredLinesMapAll;	//global线条在不同属性下的颜色
std::map<int, PafColor> NetFixedColoredLinesMap;	//Net线条的颜色
std::map<int, PafColor> GeoFixedColoredLinesMap;	//Geo线条的颜色
std::map<int, PafColor> subset_NetFixedColoredLinesMap;	//Net线条的颜色
std::map<int, PafColor> subset_GeoFixedColoredLinesMap;	//Geo线条的颜色
std::map<int, PafColor> FixedColoredLinesMap;		//实际视图的颜色

inline PafColor getColor(int red, int green, int blue);
PafColor GrayColor = getColor(200, 200, 200);
PafColor BlueColor = getColor(0, 0, 255);
PafColor GreenColor = getColor(0, 255, 0);
PafColor WhiteColor = getColor(51, 51, 221);
PafColor global_color = WhiteColor;	//全局线条颜色
PafColor SubsetLinesColor = getColor(255, 20, 147);
//PafColor StartLinesColor = getColor(255, 0, 0);
//PafColor EndLinesColor = getColor(138, 43, 226);

//文件名
QString filename_qstr = "";

//线条粗细控制
std::map<std::string, std::map<float, set<int>>> FixedThickLinesMapAll;
std::map<float, set<int>> NetFixedThickLinesMap;
std::map<float, set<int>> GeoFixedThickLinesMap;
std::map<float, set<int>> subset_NetFixedThickLinesMap;
std::map<float, set<int>> subset_GeoFixedThickLinesMap;
std::map<float, set<int>> FixedThickLinesMap;	//key为粗细，value为线条序列
std::map<int, float> ID_to_width;
const float FixedThick = 2.0;

//属性
QVector<QString> AttributesVec;

std::vector<int> Selected_list;
float change_thickness = 1;
QColor change_color;
bool changeHappen = false;

int loadedCount = 0;

//view标题
QString mapViewName = "";

//记录上次的参数设置
int spinBox_value = 1;
int comboBox_idx = 0;
int comboBox_idx_select = 0;
QString comboBox_value = "ID";
int spinBox_2_value = 2;
QString lineEdit_value = "0.00";
int comboBox_2_idx = 0;
QString comboBox_2_value = "5";

//匹配串
std::string regex_str1("\\d+$");
std::regex oneIntNumber(regex_str1, std::regex::icase);
std::string regex_str2("[\\,?\\d+]+(\\,n)?$");
std::regex multiIntNumber(regex_str2, std::regex::icase);
std::string regex_str3("\\d+(\\.\\d+)?$");
std::regex oneFloatNumber(regex_str3, std::regex::icase);
std::string regex_str4("n?([\\,?\\d+(\\.\\d+)?]+(\\,n)?)?$");
std::regex multiFloatNumber(regex_str4, std::regex::icase);
std::string regex_str5("[\\,?\\d+]+$");
std::regex justFrom_1(regex_str5, std::regex::icase);
std::string regex_str6("value\\(.{1,10}\\)(\\>)?(\\<)?(\\=)?\\d+(\\.\\d+)?$");
std::regex justFrom_2(regex_str6, std::regex::icase);
std::string regex_str7("[\\,?\\d+]+\\;To ID=[\\,?\\d+]+$");
std::regex FromTo_2_1(regex_str7, std::regex::icase);
std::string regex_str8("From ID=[\\,?\\d+]+\\;To ID=value\\(.{1,10}\\)(\\>)?(\\<)?(\\=)?\\d+(\\.\\d+)?$");
std::regex FromTo_2_2(regex_str8, std::regex::icase);
std::string regex_str9("From ID=value\\(.{1,10}\\)(\\>)?(\\<)?(\\=)?\\d+(\\.\\d+)?\\;To ID=[\\,?\\d+]+\\$");
std::regex FromTo_2_3(regex_str9, std::regex::icase);
std::string regex_str10("From ID=value\\(.{1,10}\\)(\\>)?(\\<)?(\\=)?\\d+(\\.\\d+)?\\;To ID=value\\(.{1,10}\\)(\\>)?(\\<)?(\\=)?\\d+(\\.\\d+)?$");
std::regex FromTo_2_4(regex_str10, std::regex::icase);

std::string regex_str11("rgb\\(\\d+\\,\\d+\\,\\d+\\)$");
std::regex rgb_color(regex_str11, std::regex::icase);

std::string regex_str12("(white)?(black)?(red)?(green)?(blue)?(cyan-blue)?(purple)?$");
std::regex direct_color(regex_str12, std::regex::icase);

std::string regex_str13("group_\\d=\\(\\d+(\\.\\d+)?(\\<)?(\\=)?.{1,10}(\\<)?(\\=)?\\d+(\\.\\d+)?\\)$");
std::regex subset_str1(regex_str13, std::regex::icase);

std::string regex_str14("group_\\d=\\(.{1,10}(\\<)?(\\>)?(\\=)?\\d+(\\.\\d+)?\\)$");
std::regex subset_str2(regex_str14, std::regex::icase);

std::string regex_str15(".{1,10}$");
std::regex subset_str3(regex_str15, std::regex::icase);

std::string regex_str16("group_\\d=\\([\\,?\\d+]+\\)$");
std::regex subset_str4(regex_str16, std::regex::icase);

static int current_view_type = 0;

const QString editstatetext[] = { "Not Editable", "Editable Off", "Editable On" };

int global_number = 0;

//将路按照下标顺序平均分组
inline void splitArrayIntoGroups(const std::vector<int>& array, int maxNum, std::vector<std::set<int>>& groups) {
	int len = array.size();
	int numGroups = std::min(len, maxNum); // 确定实际需要的组数

	// 初始化每个组
	for (int i = 0; i < numGroups; ++i) {
		groups[i].clear();
	}

	// 根据索引分配元素到对应的组
	for (int i = 0; i < len; ++i) {
		int groupIndex = i % numGroups; // 使用模运算来确定组索引
		groups[groupIndex].insert(array[i]);
	}
}

inline void splitArrayIntoGroups(const std::set<int>& set, int maxNum, std::vector<std::set<int>>& groups) {
	std::vector<int> array(set.begin(), set.end());
	int len = array.size();
	int numGroups = std::min(len, maxNum); // 确定实际需要的组数

	// 初始化每个组
	for (int i = 0; i < numGroups; ++i) {
		groups[i].clear();
	}

	// 根据索引分配元素到对应的组
	for (int i = 0; i < len; ++i) {
		int groupIndex = i % numGroups; // 使用模运算来确定组索引
		groups[groupIndex].insert(array[i]);
	}
}

inline void splitArrayIntoGroupsRandomly(const std::vector<int>& array, int maxNum, std::vector<std::set<int>>& groups) {
	int len = array.size();
	int numGroups = std::min(len, maxNum); // 确定实际需要的组数

	srand((unsigned)time(NULL));
	int rest = len % numGroups;
	int aver_len = len / numGroups;
	std::vector<int> remain_roads;
	remain_roads.insert(remain_roads.begin(), array.begin(), array.end());

	int groupIndex = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		//[0,rest)组的长度为aver_len+1, [rest,numGroups)为aver_len
		if ((groupIndex < rest && groups[groupIndex].size() == aver_len + 1) ||
			(groupIndex >= rest && groupIndex < numGroups - 1 && groups[groupIndex].size() == aver_len)) {
			groupIndex++;
		}
		groups[groupIndex].insert(road);
	}
}


inline void splitArrayIntoGroupsRandomly(const std::set<int>& set, int maxNum, std::vector<std::set<int>>& groups) {
	std::vector<int> array(set.begin(), set.end());
	int len = array.size();
	int numGroups = std::min(len, maxNum); // 确定实际需要的组数

	srand((unsigned)time(NULL));
	int rest = len % numGroups;
	int aver_len = len / numGroups;
	std::vector<int> remain_roads;
	remain_roads.insert(remain_roads.begin(), array.begin(), array.end());

	int groupIndex = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		//[0,rest)组的长度为aver_len+1, [rest,numGroups)为aver_len
		if ((groupIndex < rest && groups[groupIndex].size() == aver_len+1 )||
			(groupIndex >= rest && groupIndex < numGroups-1 && groups[groupIndex].size() == aver_len )) {
			groupIndex++;
		}
		groups[groupIndex].insert(road);
	}
}

//根据功能可能使用的内存量决定线程数
inline int getThreadNumByMem(std::string func) {
	//B
	std::unordered_map<std::string, DWORDLONG> memFuncUsed{
		{"NET_DD", static_cast< DWORDLONG>(1) * 1024 * 1024 *1024},  //估算NET_DD使用1GB
	};
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);

	if (GlobalMemoryStatusEx(&statex)) {
		DWORDLONG totalPhysMem = statex.ullTotalPhys;
		DWORDLONG availPhysMem = statex.ullAvailPhys;
		DWORDLONG targetMemUsage = availPhysMem * 0.9; //用90%内存

		return static_cast<int>(targetMemUsage / memFuncUsed[func]);
	}
	return 1;
}

//根据系统决定线程数
inline int getThreadNumBySys() {
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int si_maxNum = int(si.dwNumberOfProcessors);

	//获取cpu最大提供线程数，超了会降低效率
	int thread_maxNum = std::thread::hardware_concurrency();
	return std::min(si_maxNum, thread_maxNum);
}

inline std::string newName() {
	++global_number;
	return std::to_string(global_number);
}

inline std::string GetNowTime() {
	time_t setTime;
	time(&setTime);
	tm* ptm = localtime(&setTime);
	std::string time = std::to_string(ptm->tm_year + 1900)
		+ "-"
		+ std::to_string(ptm->tm_mon + 1)
		+ "-"
		+ std::to_string(ptm->tm_mday)
		+ "-"
		+ std::to_string(ptm->tm_hour) + "-"
		+ std::to_string(ptm->tm_min) + "-"
		+ std::to_string(ptm->tm_sec);
	return time;
}

inline PafColor getColor(int red, int green, int blue) {
	int color_num = red * 16 * 16 * 16 * 16 + green * 16 * 16 + blue;
	PafColor fixed_color(color_num);
	return std::move(fixed_color);
}

inline void getParaSet(std::string cstrTest, std::vector<int> &result)
{	
	result.clear();

	if (cstrTest.length() == 0)
		return;

	std::string str;
	std::string number("0123456789");
	std::stringstream ss(cstrTest);

	while (std::getline(ss, str, ','))
	{
		result.push_back(stoi(str.substr(str.find_first_of(number))));
	}
}

inline std::string qstr2str(const QString qstr)
{
	QByteArray cdata = qstr.toLocal8Bit();
	return std::string(cdata);
}

inline std::string transform(std::string oldstr)
{
	std::string newstr = oldstr;
	for (int i = 0; i < 2; i++)
	{
		if (newstr.find(".csv") != std::string::npos)
		{
			newstr = newstr.substr(0, newstr.length() - 4);
			while (newstr.find("\\") != std::string::npos)
			{
				int pos = int(newstr.find("\\"));
				newstr = newstr.substr(pos + 1, newstr.length());
			}
		}
		if (newstr.find(">") != std::string::npos)
		{
			int pos = int(newstr.find(">"));
			newstr.replace(pos, 1, " more than ");
		}
		if (newstr.find("<") != std::string::npos)
		{
			int pos = int(newstr.find("<"));
			newstr.replace(pos, 1, " less than ");
		}
		//替换掉分号，为了GIS
		replace(newstr.begin(), newstr.end(), ';', '-');
	}

	return newstr;
}

QmyEvent::QmyEvent(Type type, void* wp, int lp)
	: QEvent(type)
{
	registerEventType(type);
	wparam = wp;
	lparam = lp;
}


bool MainWindow::get_run_start() {
	mutex_run_start.lock();
	bool flag = run_start;
	mutex_run_start.unlock();
	return flag;
}

bool MainWindow::get_needOver() {
	mutex_needOver.lock();
	bool flag = needOver;
	mutex_needOver.unlock();
	return flag;
}

bool MainWindow::get_NeedStop() {
	mutex_NeedStop.lock();
	bool flag = NeedStop;
	mutex_NeedStop.unlock();
	return flag;
}

bool MainWindow::get_calculate_over() {
	mutex_calculate_over.lock();
	bool flag = calculate_over;
	mutex_calculate_over.unlock();
	return flag;
}

void MainWindow::set_run_start(bool flag) {
	mutex_run_start.lock();
	run_start = flag;
	mutex_run_start.unlock();
}

void MainWindow::set_needOver(bool flag) {
	mutex_needOver.lock();
	needOver = flag;
	mutex_needOver.unlock();
}

void MainWindow::set_NeedStop(bool flag) {
	mutex_NeedStop.lock();
	NeedStop = flag;
	mutex_NeedStop.unlock();
}

void MainWindow::set_calculate_over(bool flag) {
	mutex_calculate_over.lock();
	calculate_over = flag;
	mutex_calculate_over.unlock();
}

//检查输入
bool MainWindow::CheckReachMRInput() {
	QString qstrRadiiThreshold = this->work->ui.lineEdit_11->text();
	std::string txRadiiThreshold = std::string((const char *)qstrRadiiThreshold.toLocal8Bit());

	if (txRadiiThreshold.length()==0 || txRadiiThreshold=="n" || std::regex_match(txRadiiThreshold, multiFloatNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Metric Radius");
		return false;
	}
}

bool MainWindow::CheckReachDRInput() {
	QString qstrAngleThreshold = this->work->ui.lineEdit_15->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_13->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());

	if (std::regex_match(txAngleThreshold, multiFloatNumber) &&
		(txDirectionalChanges.length() == 0 || std::regex_match(txDirectionalChanges, multiIntNumber))) {

		std::istringstream iss(txAngleThreshold);
		std::string number;
		// 解析所有浮点数
		while (std::getline(iss, number, ',')) {
			double angleThreshold = std::stod(number);
			// 检查每个角度阈值是否小于360
			if (angleThreshold >= 180) {
				return false;
			}
		}

		return true;
	}
	else {
		//this->FA.BaseInputError("Direction Reach");
		return false;
	}
}

bool MainWindow::CheckReachJnRInput() {
	QString qstrJunctionDegree = this->work->ui.lineEdit_41->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());
	QString qstrJunctionsLimit = this->work->ui.lineEdit_14->text();
	std::string txJunctionsLimit = std::string((const char *)qstrJunctionsLimit.toLocal8Bit());

	if (std::regex_match(txJunctionDegree, oneIntNumber) && std::regex_match(txJunctionsLimit, multiIntNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Junction Reach");
		return false;
	}
}

bool MainWindow::CheckReachMDRInput() {
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_16->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	QString qstrAngleThreshold = this->work->ui.lineEdit_44->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_43->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());

	if ((txRadiiThreshold2.length() == 0 || txRadiiThreshold2 == "n" || std::regex_match(txRadiiThreshold2, multiFloatNumber)) &&
		std::regex_match(txAngleThreshold, multiFloatNumber) &&
		(txDirectionalChanges.length() == 0 || std::regex_match(txDirectionalChanges, multiIntNumber))) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Combined Reach");
		return false;
	}
}

bool MainWindow::CheckReachMDInput() {
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_12->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());

	if ((txRadiiThreshold2.length() == 0 || txRadiiThreshold2 == "n" || std::regex_match(txRadiiThreshold2, multiFloatNumber))) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Combined Reach");
		return false;
	}
}

bool MainWindow::CheckReachDDLInput() {
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_17->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	QString qstrAngleThreshold = this->work->ui.lineEdit_18->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());

	if ((txRadiiThreshold2.length() == 0 || txRadiiThreshold2 == "n" || std::regex_match(txRadiiThreshold2, multiFloatNumber)) &&
		std::regex_match(txAngleThreshold, multiFloatNumber) ) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Combined Reach");
		return false;
	}
}

bool MainWindow::CheckReachJnDDLInput() {
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_19->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	QString qstrDegreeThreshold = this->work->ui.lineEdit_26->text();
	std::string txDegreeThreshold = std::string((const char *)qstrDegreeThreshold.toLocal8Bit());

	if ((txRadiiThreshold2.length() == 0 || txRadiiThreshold2 == "n" || std::regex_match(txRadiiThreshold2, multiFloatNumber)) &&
		std::regex_match(txDegreeThreshold, multiIntNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Combined Reach");
		return false;
	}
}

bool MainWindow::CheckNetReachMRInput() {
	QString qstrRadiiThreshold = this->work->ui.lineEdit_35->text();
	std::string txRadiiThreshold = std::string((const char *)qstrRadiiThreshold.toLocal8Bit());

	if (txRadiiThreshold.length() == 0 || txRadiiThreshold == "n" || std::regex_match(txRadiiThreshold, multiFloatNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Metric Radius");
		return false;
	}
}

bool MainWindow::CheckNetReachDRInput() {
	QString qstrAngleThreshold = this->work->ui.lineEdit_38->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_36->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());

	if (std::regex_match(txAngleThreshold, multiFloatNumber) &&
		(txDirectionalChanges.length() == 0 || std::regex_match(txDirectionalChanges, multiIntNumber))) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Direction Reach");
		return false;
	}
}

bool MainWindow::CheckNetReachJnRInput() {
	QString qstrJunctionDegree = this->work->ui.lineEdit_53->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());
	QString qstrJunctionsLimit = this->work->ui.lineEdit_39->text();
	std::string txJunctionsLimit = std::string((const char *)qstrJunctionsLimit.toLocal8Bit());

	if (std::regex_match(txJunctionDegree, oneIntNumber) && std::regex_match(txJunctionsLimit, multiIntNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Junction Reach");
		return false;
	}
}

bool MainWindow::CheckNetReachMDRInput() {
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_40->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	QString qstrAngleThreshold = this->work->ui.lineEdit_55->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_54->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());

	if ((txRadiiThreshold2.length() == 0 || txRadiiThreshold2 == "n" || std::regex_match(txRadiiThreshold2, multiFloatNumber)) &&
		std::regex_match(txAngleThreshold, multiFloatNumber) &&
		(txDirectionalChanges.length() == 0 || std::regex_match(txDirectionalChanges, multiIntNumber))) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Combined Reach");
		return false;
	}
}

bool MainWindow::CheckNetreachIDInput()
{
	QString qstr = this->work->ui.lineEdit_56->text();
	std::string tx = std::string((const char *)qstr.toLocal8Bit());

	if (tx.length()>0 && std::regex_match(tx, multiIntNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Netreach By Selection");
		return false;
	}
}

bool MainWindow::CheckDistanceModeInput() {
	QString qstr = this->work->ui.lineEdit_110->text();
	std::string tx = std::string((const char *)qstr.toLocal8Bit());

	//if (tx.length() > 0 && std::regex_match(tx, multiIntNumber)) {
	if (tx.length() > 0 || (Ref_number_list.size() > 0 && Ref_number_list[0] != -1)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Start Roads");
		return false;
	}
}

//bool MainWindow::CheckDistanceRadiusInput() {
//	QString qstr = this->work->ui.lineEdit_119->text();
//	std::string tx = std::string((const char *)qstr.toLocal8Bit());
//
//	if (tx.length() == 0 || std::regex_match(tx, oneFloatNumber)) {
//		return true;
//	}
//	else {
//		//this->FA.BaseInputError("Radius(Metric)");
//		return false;
//	}
//}

bool MainWindow::CheckDistanceDRInput() {
	QString qstrAngleThreshold = this->work->ui.lineEdit_121->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());

	if (txAngleThreshold.length()>0 && std::regex_match(txAngleThreshold, oneFloatNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Angle Threshold");
		return false;
	}
}

bool MainWindow::CheckDistanceJnRInput() {
	QString qstrJunctionDegree = this->work->ui.lineEdit_122->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());

	if (txJunctionDegree.length() > 0 && std::regex_match(txJunctionDegree, oneIntNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Junction Degree");
		return false;
	}
}


bool MainWindow::CheckGeodesicsDRInput() {
	QString qstrAngleThreshold = this->work->ui.lineEdit_29->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());

	if (txAngleThreshold.length() > 0 && std::regex_match(txAngleThreshold, oneFloatNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Angle Threshold");
		return false;
	}
}

bool MainWindow::CheckGeodesicsJnRInput() {
	QString qstrJunctionDegree = this->work->ui.lineEdit_31->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());

	if (txJunctionDegree.length()>0 && std::regex_match(txJunctionDegree, oneIntNumber)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("Junction Degree");
		return false;
	}
}

bool MainWindow::CheckGeodesicsFromIDInput() {
	QString qstrInput = this->work->ui.lineEdit_6->text();
	std::string txInput = std::string((const char *)qstrInput.toLocal8Bit());

	if (std::regex_match(txInput, justFrom_1) || std::regex_match(txInput, justFrom_2)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("From ID");
		return false;
	}
}

bool MainWindow::CheckGeodesicsToIDInput() {
	QString qstrInput = this->work->ui.lineEdit_7->text();
	std::string txInput = std::string((const char *)qstrInput.toLocal8Bit());

	if (std::regex_match(txInput, justFrom_1) || std::regex_match(txInput, justFrom_2)) {
		return true;
	}
	else {
		//this->FA.BaseInputError("To ID");
		return false;
	}
}

bool MainWindow::CheckOpenFile()
{
	std::string txInput= std::string((const char *)shpFileName.toLocal8Bit());

	struct stat buffer;
	if (stat(txInput.c_str(), &buffer) != 0) {
		//this->FA.FileNotExist();
		return false;
	}
	else {
		return true;
	}
}

bool MainWindow::CheckGeodesicsCSVInput() {
	QString qstrInput = this->work->ui.lineEdit_2->text();
	std::string txInput = std::string((const char *)qstrInput.toLocal8Bit());

	if (txInput.length() > 0) {
		struct stat buffer;
		if (stat(txInput.c_str(), &buffer) != 0) {
			//this->FA.FileNotExist();
			return false;
		}

		if (txInput.length() > 4 && (txInput.substr(txInput.length() - 4, 4) == ".csv" || txInput.substr(txInput.length() - 4, 4) == ".txt")) {
			return true;
		}
		else {
			//this->FA.BaseInputError("file");
			return false;
		}
	}
	else {
		return true;
	}
	
}


bool MainWindow::CheckThreadNumberInput() {
	QString qstrInput = this->work->ui.lineEdit->text();
	std::string txJunctionDegree = std::string((const char *)qstrInput.toLocal8Bit());

	if (this->work->ui.radioButton_3->isChecked()) {
		if (txJunctionDegree == "auto" || std::regex_match(txJunctionDegree, oneIntNumber)) {
			return true;
		}
		else {
			this->FA.BaseInputError("Thread Number Input Error. Please Check");
			return false;
		}
	}
	
	return true;
}


void MainWindow::calculateMR_Single_thread()
{
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold = this->work->ui.lineEdit_11->text();
	std::string txRadiiThreshold = std::string((const char *)qstrRadiiThreshold.toLocal8Bit());
	bool ckJnc = false;
	bool ckWgt = false;
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_32->isChecked()) {
		if (this->work->ui.lineEdit_83->text().size() > 0) {
			ckJnc = true;
			txNewJnc = std::string((const char *)(this->work->ui.lineEdit_83->text()).toLocal8Bit());
		}
		//if (this->work->ui.lineEdit_84->text().size() > 0) {
		//	ckWgt = true;
		//	txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_84->text()).toLocal8Bit());
		//}
		
	}

	CA.init_MR_para(FA, *g, this->infilepath, txRadiiThreshold, ckJnc, ckWgt, txNewJnc, weightAttributesSet);

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 1);

	set_run_start(true);

	//计算MR
	CA.calculateMR(FA);
	if (get_NeedStop()) {
		return;
	}

	//输出
	CA.OutputData(FA);
	//更新Attributes栏目
	this->updateAttributes(this->CA);
	this->set_run_start(false);
}

void MainWindow::calculateDDL_Single_thread() {
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_17->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	QString qstrAngleThreshold = this->work->ui.lineEdit_18->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	std::string txDirectionalChanges = "";
	bool ckJnc = false;
	bool ckWgt = false;
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";
	//if (this->work->ui.radioButton_40->isChecked()) {
	//	if (this->work->ui.lineEdit_86->text().size() > 0) {
	//		ckWgt = true;
	//		txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_86->text()).toLocal8Bit());
	//	}
	//}

	//根据角度划分计算次数
	std::string sourceAngleStr = txAngleThreshold;
	std::stringstream ss(sourceAngleStr);
	txAngleThreshold = "";

	while (getline(ss, txAngleThreshold, ',')) {
		//排除上一次主计算数据的干扰
		CA.clearOldData();
		CA.init_DDL_para(FA, *g, this->infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, ckJnc, ckWgt, txNewJnc, weightAttributesSet);

		//多线程设置
		CA.setMultiPara(FA, 1, INT_MAX - 1, 1);

		set_run_start(true);

		//计算MR
		CA.calculateDDL(FA);
		if (get_NeedStop()) {
			return;
		}

		//输出
		CA.OutputData(FA);
		//更新Attributes栏目
		this->updateAttributes(this->CA);
		this->set_run_start(false);
		++finished_angle_count;
	}
}

void MainWindow::calculateMDR_Single_thread() {
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_16->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	QString qstrAngleThreshold = this->work->ui.lineEdit_44->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_43->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());
	bool ckJnc = false;
	bool ckWgt = false;
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_32->isChecked()) {
		if (this->work->ui.lineEdit_83->text().size() > 0) {
			ckJnc = true;
			txNewJnc = std::string((const char *)(this->work->ui.lineEdit_83->text()).toLocal8Bit());
		}
		/*if (this->work->ui.lineEdit_84->text().size() > 0) {
			ckWgt = true;
			txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_84->text()).toLocal8Bit());
		}*/
	}

	//根据角度划分计算次数
	std::string sourceAngleStr = txAngleThreshold;
	std::stringstream ss(sourceAngleStr);
	txAngleThreshold = "";

	while (getline(ss, txAngleThreshold, ',')) {
		//排除上一次主计算数据的干扰
		CA.clearOldData();
		CA.init_MDR_para(FA, *g, this->infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, ckJnc, ckWgt, txNewJnc, weightAttributesSet);

		//多线程设置
		CA.setMultiPara(FA, 1, INT_MAX - 1, 1);

		set_run_start(true);

		//计算MR
		CA.calculateMDR(FA);
		if (get_NeedStop()) {
			return;
		}

		//输出
		CA.OutputData(FA);
		//更新Attributes栏目
		this->updateAttributes(this->CA);
		this->set_run_start(false);
		++finished_angle_count;
	}
}

void MainWindow::calculateDR_Single_thread()
{
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txRadiiThreshold2 = "";
	QString qstrAngleThreshold = this->work->ui.lineEdit_15->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_13->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());
	bool ckJnc = false;
	bool ckWgt = false;
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_32->isChecked()) {
		if (this->work->ui.lineEdit_83->text().size() > 0) {
			ckJnc = true;
			txNewJnc = std::string((const char *)(this->work->ui.lineEdit_83->text()).toLocal8Bit());
		}
	/*	if (this->work->ui.lineEdit_84->text().size() > 0) {
			ckWgt = true;
			txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_84->text()).toLocal8Bit());
		}*/
	}

	//根据角度划分计算次数
	std::string sourceAngleStr = txAngleThreshold;
	std::stringstream ss(sourceAngleStr);
	txAngleThreshold = "";

	while (getline(ss, txAngleThreshold, ',')) {
		//排除上一次主计算数据的干扰
		CA.clearOldData();
		CA.init_DR_para(FA, *g, this->infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, ckJnc, ckWgt, txNewJnc, weightAttributesSet);

		//多线程设置
		CA.setMultiPara(FA, 1, INT_MAX - 1, 1);

		set_run_start(true);

		//计算MR
		CA.calculateDR(FA);
		if (get_NeedStop()) {
			return;
		}

		//输出
		CA.OutputData(FA);
		//更新Attributes栏目
		this->updateAttributes(this->CA);
		this->set_run_start(false);
		++finished_angle_count;
	}
}

void MainWindow::calculateJnR_Single_thread()
{
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrJunctionDegree = this->work->ui.lineEdit_41->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());
	QString qstrJunctionsLimit = this->work->ui.lineEdit_14->text();
	std::string txJunctionsLimit = std::string((const char *)qstrJunctionsLimit.toLocal8Bit());
	bool ckJnDD = false;
	std::string txAngleThreshold2 = "";
	bool ckWgt = false;
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_32->isChecked()) {
		//if (this->work->ui.lineEdit_84->text().size() > 0) {
		//	ckWgt = true;
		//	txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_84->text()).toLocal8Bit());
		//}
	}

	CA.init_JnR_para(FA, *g, infilepath, txJunctionDegree, txJunctionsLimit, ckJnDD, txAngleThreshold2, ckWgt, weightAttributesSet);

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 1);

	set_run_start(true);

	//计算MR
	CA.calculateJncR(FA);
	if (get_NeedStop()) {
		return;
	}

	//输出
	CA.OutputData(FA);
	//更新Attributes栏目
	this->updateAttributes(this->CA);
	this->set_run_start(false);
}

void MainWindow::Net_calculateMR_Single_thread()
{
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold = this->work->ui.lineEdit_35->text();
	std::string txRadiiThreshold = std::string((const char *)qstrRadiiThreshold.toLocal8Bit());	

	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_48->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_77->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
		/*QString qstr2 = this->work->ui.lineEdit_78->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
	}

	CA.init_Net_MR_para(FA, *g, infilepath, txRadiiThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_56->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		weightAttributesSet.clear();
		return;
	}
	CA.searchLimitStr = transform(searchidStr);
	CA.getFromToID(FA, searchidStr);

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Net
	CA.Net_calculateMR(FA, CA.newMRLimit);	
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	////修改进度条提示
	//while (true) {
	//	if (this->get_calculate_over())
	//		break;
	//}
	//Sleep(5);
	//process_str = "Update layers, please wait ...";
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//可视化
	this->addNetMapsAll(CA, FA);
	//process_str = "Run Over, "+ time_qstr;
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	weightAttributesSet.clear();
}

void MainWindow::Net_calculateMD_Single_thread()
{
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txRadiiThreshold = "0";
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";

	CA.init_Net_MR_para(FA, *g, infilepath, txRadiiThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_110->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		return;
	}
	if (searchidStr.length()>0) {
		CA.searchLimitStr = transform(searchidStr);
		CA.getFromToID(FA, searchidStr);
	}
	else	//注意：必须输入起点id
	{
		weightAttributesSet.clear();
		return;
	}
	//设置
	CA.isStepDepth = true;
	CA.SD_MR = true;

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Net
	CA.Net_calculateMR(FA, CA.newMRLimit);
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	//输出
	CA.outputStepDepth(FA, "StepD_mr");
	//更新Attributes栏目
	this->updateAttributes(this->CA);
	this->set_run_start(false);
	weightAttributesSet.clear();
}

void MainWindow::Net_calculateDR_Single_thread()
{
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txRadiiThreshold2 = "";
	QString qstrAngleThreshold = this->work->ui.lineEdit_38->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_36->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());

	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_48->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_77->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
	/*	QString qstr2 = this->work->ui.lineEdit_78->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
	}

	CA.init_Net_DR_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_56->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		weightAttributesSet.clear();
		return;
	}
	CA.searchLimitStr = transform(searchidStr);
	CA.getFromToID(FA, searchidStr);

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Net
	CA.Net_calculateDR(FA, CA.newDRLimit, CA.newMRLimit);
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	////修改进度条提示
	//while (true) {
	//	if (this->get_calculate_over())
	//		break;
	//}
	//Sleep(5);
	//process_str = "Update layers, please wait ...";
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//可视化
	this->addNetMapsAll(CA, FA);

	//process_str = "Run Over, "+ time_qstr;
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	weightAttributesSet.clear();
}

void MainWindow::Net_calculateDD_Single_thread()
{
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txRadiiThreshold2 = "";
	QString qstrAngleThreshold = this->work->ui.lineEdit_121->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	std::string txDirectionalChanges = "0";

	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";

	CA.init_Net_DR_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_110->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		weightAttributesSet.clear();
		return;
	}
	if (searchidStr.length() > 0) {
		CA.searchLimitStr = transform(searchidStr);
		CA.getFromToID(FA, searchidStr);
	}
	else	//注意：必须输入起点id
	{
		weightAttributesSet.clear();
		return;
	}
	//设置
	CA.isStepDepth = true;
	CA.SD_DR = true;

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Net
	CA.Net_calculateDR(FA, CA.newDRLimit, CA.newMRLimit);
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	//输出
	CA.outputStepDepth(FA, "StepD_dr");
	//更新Attributes栏目
	this->updateAttributes(this->CA);
	this->set_run_start(false);
	weightAttributesSet.clear();
}


void MainWindow::Net_calculateMDR_Single_thread()
{
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_40->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	QString qstrAngleThreshold = this->work->ui.lineEdit_55->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_54->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());

	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_48->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_77->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
		/*QString qstr2 = this->work->ui.lineEdit_78->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
	}

	CA.init_Net_DR_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_56->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		weightAttributesSet.clear();
		return;
	}
	CA.searchLimitStr = transform(searchidStr);
	CA.getFromToID(FA, searchidStr);

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Net
	CA.Net_calculateDR(FA, CA.newDRLimit, CA.newMRLimit);
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	////修改进度条提示
	//while (true) {
	//	if (this->get_calculate_over())
	//		break;
	//}
	//Sleep(5);
	//process_str = "Update layers, please wait ...";
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//可视化
	this->addNetMapsAll(CA, FA);

	//process_str = "Run Over, "+ time_qstr;
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());
	weightAttributesSet.clear();
}

void MainWindow::Net_calculateJnR_Single_thread()
{
	//logOut << GetNowTime() << ", " << "Net_calculateJnR_Single_thread Start.\n";

	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrJunctionDegree = this->work->ui.lineEdit_53->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());
	QString qstrJunctionsLimit = this->work->ui.lineEdit_39->text();
	std::string txJunctionsLimit = std::string((const char *)qstrJunctionsLimit.toLocal8Bit());

	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_48->isChecked()) {
	/*	QString qstr2 = this->work->ui.lineEdit_78->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
	}

	CA.init_Net_JnR_para(FA, *g, infilepath, txJunctionDegree, txJunctionsLimit, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_56->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		weightAttributesSet.clear();
		return;
	}
	CA.searchLimitStr = transform(searchidStr);
	CA.getFromToID(FA, searchidStr);

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Net
	CA.Net_calculateJncR(FA, CA.newJnc_maxNum);
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	////修改进度条提示
	//while (true) {
	//	if (this->get_calculate_over())
	//		break;
	//}
	//Sleep(5);
	//process_str = "Update layers, please wait ...";
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//可视化
	this->addNetMapsAll(CA, FA);

	//process_str = "Run Over, "+ time_qstr;
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//logOut << GetNowTime() << ", " << "Net_calculateJnR_Single_thread Over.\n";
	weightAttributesSet.clear();
	return;
}

void MainWindow::Net_calculateJnD_Single_thread()
{
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrJunctionDegree = this->work->ui.lineEdit_122->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());
	std::string txJunctionsLimit = "0";

	std::string txWgtLimitStr = "";

	CA.init_Net_JnR_para(FA, *g, infilepath, txJunctionDegree, txJunctionsLimit, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_110->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		weightAttributesSet.clear();
		return;
	}
	if (searchidStr.length() > 0) {
		CA.searchLimitStr = transform(searchidStr);
		CA.getFromToID(FA, searchidStr);
	}
	else	//注意：必须输入起点id
	{
		weightAttributesSet.clear();
		return;
	}
	//设置
	CA.isStepDepth = true;
	CA.SD_JnR = true;

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Net
	CA.Net_calculateJncR(FA, CA.newJnc_maxNum);
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	//输出
	CA.outputStepDepth(FA, "StepD_jnr");
	//更新Attributes栏目
	this->updateAttributes(this->CA);
	this->set_run_start(false);
	weightAttributesSet.clear();
}


void MainWindow::Geo_calculateMR_Single_thread()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txAngleThreshold = std::to_string(INT_MAX - 1);
	std::string txNewJnc = std::to_string(INT_MAX - 1);
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_30->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_80->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
		/*QString qstr2 = this->work->ui.lineEdit_81->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
		QString qstr3 = this->work->ui.lineEdit_79->text();
		txAngleThreshold = std::string((const char *)qstr3.toLocal8Bit());
	}

	CA.init_Geo_MR_para(FA, *g, infilepath, txAngleThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrFromStr = this->work->ui.lineEdit_6->text();
	std::string FromStr = std::string((const char *)qstrFromStr.toLocal8Bit());
	QString qstrToStr = this->work->ui.lineEdit_7->text();
	std::string ToStr = std::string((const char *)qstrToStr.toLocal8Bit());
	std::string fromToStr = FromStr;
	if (ToStr.length() > 0)
		fromToStr = FromStr + ";" + ToStr;

	//获取csv文件路径
	QString qstrCsvfilePath = this->work->ui.lineEdit_2->text();
	std::string CsvfilePath = std::string((const char *)qstrCsvfilePath.toLocal8Bit());
	CA.outputPath = true;
	if (CsvfilePath.find(".csv") != std::string::npos)
	{
		fromToStr = CsvfilePath;
		CA.incsvfilename = CsvfilePath;
	}

	CA.searchLimitStr = transform(fromToStr);
	CA.getFromToID(FA, fromToStr);

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Geo
	CA.Geo_calculateMR(FA);
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	////修改进度条提示
	//while (true) {
	//	if (this->get_calculate_over())
	//		break;
	//}
	//Sleep(5);
	//process_str = "Update layers, please wait ...";
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//当计算csv最短路径需要输出csv文件，否则do nothing
	output_shp_data(this);

	//可视化
	if (this->CA.isOneToOne == false){
		this->addGeoMapsAll(CA, FA);
	}

	//process_str = "Run Over, "+ time_qstr;
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	weightAttributesSet.clear();
}

void MainWindow::Geo_calculateDR_Single_thread()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrAngleThreshold = this->work->ui.lineEdit_29->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());

	std::string txNewJnc = std::to_string(INT_MAX - 1);
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_30->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_80->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
	/*	QString qstr2 = this->work->ui.lineEdit_81->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
		//QString qstr3 = this->work->ui.lineEdit_79->text();
		//txAngleThreshold = std::string((const char *)qstr3.toLocal8Bit());
	}

	CA.init_Geo_DR_para(FA, *g, infilepath, txAngleThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrFromStr = this->work->ui.lineEdit_6->text();
	std::string FromStr = std::string((const char *)qstrFromStr.toLocal8Bit());
	QString qstrToStr = this->work->ui.lineEdit_7->text();
	std::string ToStr = std::string((const char *)qstrToStr.toLocal8Bit());
	std::string fromToStr = FromStr;
	if (ToStr.length() > 0)
		fromToStr = FromStr + ";" + ToStr;

	//获取csv文件路径
	QString qstrCsvfilePath = this->work->ui.lineEdit_2->text();
	std::string CsvfilePath = std::string((const char *)qstrCsvfilePath.toLocal8Bit());
	CA.outputPath = true;
	if (CsvfilePath.find(".csv") != std::string::npos)
	{
		fromToStr = CsvfilePath;
		CA.incsvfilename = CsvfilePath;
	}

	CA.searchLimitStr = transform(fromToStr);
	CA.getFromToID(FA, fromToStr);

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Geo
	CA.Geo_calculateDR(FA);
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	////修改进度条提示
	//while (true) {
	//	if (this->get_calculate_over())
	//		break;
	//}
	//Sleep(5);
	//process_str = "Update layers, please wait ...";
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//当计算csv最短路径需要输出csv文件，否则do nothing
	output_shp_data(this);

	//可视化
	if (this->CA.isOneToOne == false) {
		this->addGeoMapsAll(CA, FA);
	}

	//process_str = "Run Over, "+ time_qstr;
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	weightAttributesSet.clear();
}

void MainWindow::Geo_calculateJnR_Single_thread()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrNewJnc = this->work->ui.lineEdit_31->text();
	std::string txNewJnc = std::string((const char *)qstrNewJnc.toLocal8Bit());

	std::string txAngleThreshold = std::to_string(INT_MAX - 1);
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_30->isChecked()) {
		//QString qstr1 = this->work->ui.lineEdit_80->text();
		//txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
	/*	QString qstr2 = this->work->ui.lineEdit_81->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
		QString qstr3 = this->work->ui.lineEdit_79->text();
		txAngleThreshold = std::string((const char *)qstr3.toLocal8Bit());
	}

	CA.init_Geo_JnR_para(FA, *g, infilepath, txAngleThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrFromStr = this->work->ui.lineEdit_6->text();
	std::string FromStr = std::string((const char *)qstrFromStr.toLocal8Bit());
	QString qstrToStr = this->work->ui.lineEdit_7->text();
	std::string ToStr = std::string((const char *)qstrToStr.toLocal8Bit());
	std::string fromToStr = FromStr;
	if (ToStr.length() > 0)
		fromToStr = FromStr + ";" + ToStr;

	//获取csv文件路径
	QString qstrCsvfilePath = this->work->ui.lineEdit_2->text();
	std::string CsvfilePath = std::string((const char *)qstrCsvfilePath.toLocal8Bit());
	CA.outputPath = true;
	if (CsvfilePath.find(".csv") != std::string::npos)
	{
		fromToStr = CsvfilePath;
		CA.incsvfilename = CsvfilePath;
	}

	CA.searchLimitStr = transform(fromToStr);
	CA.getFromToID(FA, fromToStr);

	//多线程设置
	CA.setMultiPara(FA, 1, INT_MAX - 1, 2);

	set_run_start(true);

	//计算Geo
	CA.Geo_calculateJncR(FA);
	if (get_NeedStop()) {
		weightAttributesSet.clear();
		return;
	}

	////修改进度条提示
	//while (true) {
	//	if (this->get_calculate_over())
	//		break;
	//}
	//Sleep(5);
	//process_str = "Update layers, please wait ...";
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//当计算csv最短路径需要输出csv文件，否则do nothing
	output_shp_data(this);

	//可视化
	if (this->CA.isOneToOne == false) {
		this->addGeoMapsAll(CA, FA);
	}

	//process_str = "Run Over, "+ time_qstr;
	////更新进度条
	//emit(this->pDisplaydow->ui->Button_start_scan->clicked());
	weightAttributesSet.clear();
}

void MainWindow::Net_calculateMR_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Net_MR();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

void MainWindow::Net_calculateMD_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Net_MD();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

void MainWindow::Net_calculateDR_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Net_DR();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

void MainWindow::Net_calculateDD_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Net_DD();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

void MainWindow::Net_calculateMDR_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Net_MDR();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

void MainWindow::Net_calculateJnR_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Net_JnR();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

void MainWindow::Net_calculateJnD_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Net_JnD();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

void MainWindow::Geo_calculateMR_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Geo_MR();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

void MainWindow::Geo_calculateDR_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Geo_DR();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

void MainWindow::Geo_calculateJnR_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_Geo_JnR();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_NetGeo(this);
}

//生成Net/Geo对应的txt文件
void MainWindow::drawTxtFile(std::string shpfilename, std::string dbffilename, std::string txtfilename, bool needDelete)
{
	//创建txt文件
	std::ofstream out;
	out.open(txtfilename.c_str());

	//1.读取对应shp文件，把线段坐标取出来
	std::vector<std::vector<double>> coordinateData;
	FA.getCoordinateData(shpfilename, coordinateData);

	//2.读取dbf文件，把field字段值取出来
	DBFHandle	hDBF;
	hDBF = DBFOpen(dbffilename.c_str(), "r");
	if (hDBF == NULL)
		return;

	//3.添加字段名称
	int fieldCount = DBFGetFieldCount(hDBF);
	std::vector<std::string> fieldNames;
	char  fieldName[100];
	for (int fieldNum = 0; fieldNum < fieldCount; fieldNum++)
	{
		DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, fieldNum, fieldName, NULL, NULL);
		if (fieldtype == FTInteger || fieldtype == FTDouble)
		{
			std::string name = fieldName;
			fieldNames.push_back(name);
		}
	}

	out << "Ref\tx1\ty1\tx2\ty2\t";
	for (auto it = fieldNames.begin(); it != fieldNames.end(); it++)
	{
		out << *it << '\t';
	}
	out << std::endl;

	//4.开始逐行添加数据
	int recordCount = DBFGetRecordCount(hDBF);
	for (int record = 0; record < recordCount; record++)
	{
		//添加线段坐标
		out << record << '\t' << std::setprecision(16) << coordinateData[record][0] << '\t' << std::setprecision(16) << coordinateData[record][1] << '\t' << std::setprecision(16) << coordinateData[record][2] << '\t' << std::setprecision(16) << coordinateData[record][3] << '\t';

		//添加线段属性		
		for (int fieldOrder = 0; fieldOrder < fieldCount; fieldOrder++)
		{
			DBFFieldType fieldtype = DBFGetFieldInfo(hDBF, fieldOrder, NULL, NULL, NULL);
			if (fieldtype == FTInteger)
			{
				int value = DBFReadIntegerAttribute(hDBF, record, fieldOrder);
				out << value << '\t';
			}
			if (fieldtype == FTDouble)
			{
				double value = DBFReadDoubleAttribute(hDBF, record, fieldOrder);
				out << value << '\t';
			}
		}
		out << std::endl;
	}

	DBFClose(hDBF);
	out.close();

	//重新加载
	this->drawGraph();
}

void MainWindow::drawGraph()
{
	this->nowCount = 0;
	this->Ref_to_Id.clear();
	this->Id_to_Ref.clear();
	Ref_number_list.clear();

	//准备重写graph.txt
	//std::ofstream out;
	//out.open(this->graph_file_path.c_str());
	//out.close();

	//以追加方式书写
	//std::ofstream outfile;
	//outfile.open(this->graph_file_path.c_str(), std::ios::app);

	std::string str;

	//判断是否只有一个在显示
	bool isOnlyOne = false;
	if ((this->global_view && !this->netreach_view && !this->geodesics_view) ||
		(!this->global_view && this->netreach_view && !this->geodesics_view) ||
		(!this->global_view && !this->netreach_view && this->geodesics_view)) {
		isOnlyOne = true;
	}
	std::string filename;

	//判断是否需要加粗显示Netreach和Geodesics
	bool needNG = false;
	if (this->global_view && (this->netreach_view || this->geodesics_view)) {
		needNG = true;		
	}

	//global
	if (this->global_view) {
		if (isOnlyOne) {
			//filename = this->global_file_path.substr(0, this->global_file_path.length() - 4) + "All.txt";
			filename = this->global_file_path;
		}
		else {
			filename = this->global_file_path;
		}

		//this->work->ui.radioButton_4->setChecked(true);
		std::ifstream infile(filename, std::ios::in);
		//outfile << infile.rdbuf();
		infile.close();
		this->nowCount += this->global_roads_num;

		this->Ref_to_Id.insert(this->Global_Ref_to_Id.begin(), this->Global_Ref_to_Id.end());
		this->Id_to_Ref.insert(this->Global_Id_to_Ref.begin(), this->Global_Id_to_Ref.end());

	}
	else {
		//this->work->ui.radioButton_4->setChecked(false);
	}

	//netreach
	if (this->netreach_view) {
		if (isOnlyOne) {
			//filename = this->netreach_file_path.substr(0, this->netreach_file_path.length() - 4) + "All.txt";
			filename = this->netreach_file_path;
		}
		else {
			filename = this->netreach_file_path;
		}

		this->work->ui.radioButton_5->setChecked(true);
		std::ifstream infile(filename, std::ios::in);

		if (this->global_view == false) {
			//outfile << infile.rdbuf();

			//直接拷贝，Ref不需要修改
		}
		else {
			int count = 0;
			this->Netreach_Ref_to_Id.clear();
			this->Netreach_Id_to_Ref.clear();
			while (getline(infile, str))
			{				
				if (count == 0) {
					++count;
					continue;	//跳过“Ref	x1	y1	x2	y2	ID”
				}
				else {
					//str.replace(pos,old_value.length(),new_value);   
					std::string old_Ref = std::to_string(count - 1);
					int new_Ref = nowCount + count - 1;
					str.replace(0, old_Ref.length(), std::to_string(new_Ref));
					//outfile << str << std::endl;
					++count;

					//拿到id
					char fn[100];
					char buf[500];
					strcpy(buf, str.c_str());
					char *ptr = strrchr(buf, '\t');
					sprintf(fn, "%s", ptr + 1);
					std::string id_str = fn;
					int id = stoi(id_str);

					this->Netreach_Ref_to_Id[new_Ref] = id;
					this->Netreach_Id_to_Ref[id] = new_Ref;
				}				
			}
		}		

		infile.close();

		this->nowCount += this->netreach_roads_num;

		if (needNG) {
			for (auto iter = this->Netreach_Ref_to_Id.begin(); iter != this->Netreach_Ref_to_Id.end(); iter++) {
				Ref_number_list.push_back(iter->first);
			}
		}

		this->Ref_to_Id.insert(this->Netreach_Ref_to_Id.begin(), this->Netreach_Ref_to_Id.end());
		this->Id_to_Ref.insert(this->Netreach_Id_to_Ref.begin(), this->Netreach_Id_to_Ref.end());

		
	}
	else {
		this->work->ui.radioButton_5->setChecked(false);
	}

	//geodesics
	if (this->geodesics_view) {
		if (isOnlyOne) {
			//filename = this->geodesics_file_path.substr(0, this->geodesics_file_path.length() - 4) + "All.txt";
			filename = this->geodesics_file_path;
		}
		else {
			filename = this->geodesics_file_path;
		}

		this->work->ui.radioButton_6->setChecked(true);
		std::ifstream infile(filename, std::ios::in);
		if (this->global_view == false && this->netreach_view == false) {
			//outfile << infile.rdbuf();
		}
		else {
			int count = 0;
			this->Geodesics_Ref_to_Id.clear();
			this->Geodesics_Id_to_Ref.clear();
			while (getline(infile, str))
			{
				if (count == 0) {
					++count;
					continue;	//跳过“Ref	x1	y1	x2	y2	ID”
				}
				else { 
					std::string old_Ref = std::to_string(count - 1);
					int new_Ref = nowCount + count - 1;
					str.replace(0, old_Ref.length(), std::to_string(nowCount + count - 1));
					//outfile << str << std::endl;
					++count;

					//拿到id
					char fn[100];
					char buf[500];
					strcpy(buf, str.c_str());
					char *ptr = strrchr(buf, '\t');
					sprintf(fn, "%s", ptr + 1);
					std::string id_str = fn;
					int id = stoi(id_str);

					this->Geodesics_Ref_to_Id[new_Ref] = id;
					this->Geodesics_Id_to_Ref[id] = new_Ref;
				}				
			}
		}	

		infile.close();

		this->nowCount += this->geodesics_roads_num;

		if (needNG) {
			for (auto iter = this->Geodesics_Ref_to_Id.begin(); iter != this->Geodesics_Ref_to_Id.end(); iter++) {
				Ref_number_list.push_back(iter->first);
			}
		}

		this->Ref_to_Id.insert(this->Geodesics_Ref_to_Id.begin(), this->Geodesics_Ref_to_Id.end());
		this->Id_to_Ref.insert(this->Geodesics_Id_to_Ref.begin(), this->Geodesics_Id_to_Ref.end());

	}
	else {
		this->work->ui.radioButton_6->setChecked(false);
	}

	//outfile.close();

	//绘图
	QGraphDoc* new_m_p = activeMapDoc();
	if (new_m_p)
	{
		new_m_p->OnLayerDelete();
		new_m_p->OnTXTFileImport(filename);

		////测试切换属性为ID
		//MetaGraph *graph = m_treeDoc->m_meta_graph;
		//if (graph->viewingProcessed()) {
		//	graph->setDisplayedAttribute(0);
		//}
		//SetAttributeChecks();
	}

	//show selected lines
	emit(this->pDisplaydow->ui->Button_start_scan_8->clicked());

	//修改属性名称
	//if (isOnlyOne) {
	//	this->modifyColNames(filename);
	//}
}

void MainWindow::modifyColNames(std::vector<std::string> &SourceNames)
{
	this->ColNames.clear();
	for (std::string name : SourceNames) {
		if (name == "Ref" || name == "x1" || name == "y1" || name == "x2" || name == "y2")
			continue;

		this->ColNames.insert(name);
	}

	//改名
	QGraphDoc* new_m_p = activeMapDoc();
	if (!new_m_p)
	{
		return;
	}

	int col = 0;
	for (auto it = this->ColNames.begin(); it != this->ColNames.end();it++) {
		new_m_p->RenameColumnByCol(col, *it);
		++col;
	}
}

void MainWindow::modifyTXT(std::map<int, double> &Result, std::string fieldName, std::string fileName)
{
	//费时，看能不能找到csv库来用：读取旧文件，增写新文件，删除旧文件，新文件重命名为旧文件
	std::string newFileName = "new" + fileName;

	std::ifstream infile(fileName, std::ios::in);
	std::ofstream out(newFileName, std::ios::out);
	
	std::string str, tmp;
	std::stringstream input(str);
	std::string LineStr;

	//根据fieldName判断是需要覆盖，还是新增属性字段
	bool isRepeated = false;
	int pos = 0, oldPos;
	while (getline(input, tmp, '\t')) {
		if (tmp == fieldName) {
			isRepeated = true;
			oldPos = pos;
		}
			
		//复制第一行
		out << tmp << '\t';
		++pos;
	}

	//逐行修改数据
	if (isRepeated) {	//修改原有字段
		int id = 0;
		while (getline(infile, str)) {
			std::stringstream input(str);

			int pos = 0;
			while (getline(input, tmp, '\t')) {
				if (tmp == "ID") {
					id = atoi(tmp.c_str());
				}
				if (pos == oldPos) {	//修改字段数据
					tmp = std::to_string(Result[id]);
				}

				//copy
				out << tmp << '\t';

				++pos;
			}
			out << std::endl;
		}
	}
	else {	//新增字段
		out << fieldName << '\t' << std::endl;

		//直接在每行数据的末尾进行添加，这里不查id，认为graph文件是按ID升序排列的
		int lineCount = 0;
		auto it = Result.begin();
		while (getline(infile, str)) {
			out << str << '\t' << std::to_string(it->second) << std::endl;

			++lineCount;
			it++;
		}
	}

	//操作文件
	infile.close();
	out.close();
	remove(fileName.c_str());
	rename(newFileName.c_str(), fileName.c_str());
}

void MainWindow::showSelectedLines()
{
	//显示select lines
	ref_numer = "";
	if (Ref_number_list.size() > 0 && Ref_number_list[0] == -1)
		Ref_number_list.erase(Ref_number_list.begin());
	for (int i = 0; i < Ref_number_list.size(); i++)
	{
		if (Ref_number_list[i] == -1)
			continue;
		if (i > 0 && ref_numer.length() > 0) ref_numer += ",";
		ref_numer += QString::number(this->Ref_to_Id[Ref_number_list[i]]);
		if (i > 2000)
			break;
	}
	/*QString info_str = "selectd lines: " + ref_numer;
	this->work->ui.textBrowser->setText(info_str);*/
}

void MainWindow::showSummaryInfo() {
	this->work->ui.textBrowser->setText(this->summary_info);
}

void MainWindow::AddLines(std::string shpfilename, std::string dbffilename)
{
	//1.读取对应shp文件，把线段坐标取出来
	std::vector<std::vector<double>> coordinateData;
	FA.getCoordinateData(shpfilename, coordinateData);

	//2.开启视图可编辑
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	for (auto item : m_treegraphmap) {
		QTreeWidgetItem* key = item.first;
		ItemTreeEntry entry = item.second;
		if (entry.m_cat == -1)
			continue;
		graph->getDataMaps()[entry.m_cat].setEditable(true);
	/*	m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_TABLE);
		OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);*/
	}

	//3.批量绘制线段
	MapView* m_p = activeMapView();
	if (m_p)
	{
		m_p->getGraphDoc()->addLines(coordinateData);
	}

	//4.关闭视图可编辑
	for (auto item : m_treegraphmap) {
		QTreeWidgetItem* key = item.first;
		ItemTreeEntry entry = item.second;
		if (entry.m_cat == -1)
			continue;
		graph->getDataMaps()[entry.m_cat].setEditable(false);
	/*	m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_TABLE);
		OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);*/
	}
}

void MainWindow::RemoveLines(std::vector<int> &ids)
{
	//开启视图可编辑
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	for (auto item : m_treegraphmap) {
		QTreeWidgetItem* key = item.first;
		ItemTreeEntry entry = item.second;
		if (entry.m_cat == -1)
			continue;
		graph->getDataMaps()[entry.m_cat].setEditable(true);
	}

	MapView* m_p = activeMapView();
	if (m_p)
	{
		m_p->getGraphDoc()->removeLines(ids);
	}

	//关闭视图可编辑
	for (auto item : m_treegraphmap) {
		QTreeWidgetItem* key = item.first;
		ItemTreeEntry entry = item.second;
		if (entry.m_cat == -1)
			continue;
		graph->getDataMaps()[entry.m_cat].setEditable(false);
	}
}

bool MainWindow::init_for_MR()
{

	thread_num = getThreadNumBySys();

	//构建新的临时类，避免混淆
	CA.clearOldData();
	int order = 1, length = INT_MAX - 1;

	//获取shp文件
	std::string infilepath = std::string((const char *)(this->shpFileName).toLocal8Bit());
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold = this->work->ui.lineEdit_11->text();
	std::string txRadiiThreshold = std::string((const char *)qstrRadiiThreshold.toLocal8Bit());
	bool ckJnc = false;
	bool ckWgt = false;
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_32->isChecked()) {
		if (this->work->ui.lineEdit_83->text().size() > 0) {
			ckJnc = true;
			txNewJnc = std::string((const char *)(this->work->ui.lineEdit_83->text()).toLocal8Bit());
		}
	/*	if (this->work->ui.lineEdit_84->text().size() > 0) {
			ckWgt = true;
			txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_84->text()).toLocal8Bit());
		}*/
	}

	CA.init_MR_para(FA, *g, infilepath, txRadiiThreshold, ckJnc, ckWgt, txNewJnc, weightAttributesSet);

	//多线程设置				
	length = 1 + int(FA.roadID.size()) / thread_num;
	
	//CAVec.reserve(maxNum * sizeof(CA) + 1024);

	//将待计算线条随机分组
	//srand((unsigned)time(NULL));
	//int len = FA.roadID.size();
	//int aver_len = len / maxNum;
	//std::vector<int> remain_roads;
	//std::vector<std::set<int>> CA_Roads(maxNum);
	//remain_roads.insert(remain_roads.begin(), FA.roadID.begin(), FA.roadID.end());

	////remain_roads记录还没分组的路，CA_Roads则是分好组的路。
	//int num_idx = 0;
	//while (remain_roads.size() > 0) {
	//	int road_idx = rand() % len;
	//	int road = remain_roads[road_idx];
	//	remain_roads.erase(remain_roads.begin() + road_idx);
	//	len = remain_roads.size();
	//	if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
	//	CA_Roads[num_idx].insert(road);
	//}
	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroupsRandomly(FA.roadID, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++){
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 1);
		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_DR(std::string txAngleThreshold)
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	
	thread_num = getThreadNumBySys();

	//构建新的临时类，避免混淆
	CA.clearOldData();
	int order = 1, length = INT_MAX - 1;

	//获取shp文件
	std::string infilepath = std::string((const char *)(this->shpFileName).toLocal8Bit());
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txRadiiThreshold2 = "";
	//QString qstrAngleThreshold = this->work->ui.lineEdit_15->text();
	//std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_13->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());
	bool ckJnc = false;
	bool ckWgt = false;
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_32->isChecked()) {
		if (this->work->ui.lineEdit_83->text().size() > 0) {
			ckJnc = true;
			txNewJnc = std::string((const char *)(this->work->ui.lineEdit_83->text()).toLocal8Bit());
		}
	/*	if (this->work->ui.lineEdit_84->text().size() > 0) {
			ckWgt = true;
			txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_84->text()).toLocal8Bit());
		}*/
	}
	CA.init_DR_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, ckJnc, ckWgt, txNewJnc, weightAttributesSet);

	//Add：显示平均夹角
	this->summary_info = filename_qstr;
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	if (mean_angle > 0) {
		mean_angle_qstr = QString::number(mean_angle, 10, 1);
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	}
	this->displaySummary();


	//多线程设置				
	length = 1 + int(FA.roadID.size()) / thread_num;

	//CAVec.reserve(maxNum * sizeof(CA) + 1024);

	//将待计算线条随机分组
	/*srand((unsigned)time(NULL));
	int len = FA.roadID.size();
	int aver_len = len / maxNum;
	std::vector<int> remain_roads;
	std::vector<std::set<int>> CA_Roads(maxNum);
	remain_roads.insert(remain_roads.begin(), FA.roadID.begin(), FA.roadID.end());

	int num_idx = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
		CA_Roads[num_idx].insert(road);
	}*/
	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroups(FA.roadID, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++) {
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 1);

		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_MD() {
	thread_num = getThreadNumBySys();

	//构建新的临时类，避免混淆
	CA.clearOldData();
	int order = 1, length = INT_MAX - 1;

	//获取shp文件
	std::string infilepath = std::string((const char *)(this->shpFileName).toLocal8Bit());
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold = this->work->ui.lineEdit_12->text();
	std::string txRadiiThreshold = std::string((const char *)qstrRadiiThreshold.toLocal8Bit());
	bool ckJnc = false;
	bool ckWgt = false;
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_40->isChecked()) {
	/*	if (this->work->ui.lineEdit_86->text().size() > 0) {
			ckWgt = true;
			txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_86->text()).toLocal8Bit());
		}*/
	}

	CA.init_MD_para(FA, *g, infilepath, txRadiiThreshold, ckJnc, ckWgt, txNewJnc, weightAttributesSet);

	//多线程设置				
	length = 1 + int(FA.roadID.size()) / thread_num;

	//将待计算线条随机分组
	/*srand((unsigned)time(NULL));
	int len = FA.roadID.size();
	int aver_len = len / maxNum;
	std::vector<int> remain_roads;
	std::vector<std::set<int>> CA_Roads(maxNum);
	remain_roads.insert(remain_roads.begin(), FA.roadID.begin(), FA.roadID.end());

	int num_idx = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
		CA_Roads[num_idx].insert(road);
	}*/
	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroupsRandomly(FA.roadID, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++) {
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 1);

		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_JnDDL() {
	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();

	//构建新的临时类，避免混淆
	CA.clearOldData();
	int order = 1, length = INT_MAX - 1;

	//获取shp文件
	std::string infilepath = std::string((const char *)(this->shpFileName).toLocal8Bit());
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold = this->work->ui.lineEdit_19->text();
	std::string txRadiiThreshold = std::string((const char *)qstrRadiiThreshold.toLocal8Bit());
	QString qstrJunctionsLimit = this->work->ui.lineEdit_26->text();
	std::string txJunctionsLimit = std::string((const char *)qstrJunctionsLimit.toLocal8Bit());
	bool ckJnDD = false;
	std::string txAngleThreshold2 = "";
	bool ckWgt = false;
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_40->isChecked()) {
		/*if (this->work->ui.lineEdit_86->text().size() > 0) {
			ckWgt = true;
			txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_86->text()).toLocal8Bit());
		}*/
	}

	CA.init_JnD_para(FA, *g, infilepath, txRadiiThreshold, txJunctionsLimit, ckJnDD, txAngleThreshold2, ckWgt, weightAttributesSet);

	//多线程设置				
	length = 1 + int(FA.roadID.size()) / thread_num;

	//将待计算线条随机分组
	/*srand((unsigned)time(NULL));
	int len = FA.roadID.size();
	int aver_len = len / maxNum;
	std::vector<int> remain_roads;
	std::vector<std::set<int>> CA_Roads(maxNum);
	remain_roads.insert(remain_roads.begin(), FA.roadID.begin(), FA.roadID.end());

	int num_idx = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
		CA_Roads[num_idx].insert(road);
	}*/
	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroupsRandomly(FA.roadID, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++) {
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 1);

		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_DDL(std::string txAngleThreshold) {
	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();


	//构建新的临时类，避免混淆
	CA.clearOldData();
	int order = 1, length = INT_MAX - 1;

	//获取shp文件
	std::string infilepath = std::string((const char *)(this->shpFileName).toLocal8Bit());
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_17->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	//QString qstrAngleThreshold = this->work->ui.lineEdit_44->text();
	//std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	std::string txDirectionalChanges = "";
	bool ckJnc = false;
	bool ckWgt = false;
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_40->isChecked()) {
		/*if (this->work->ui.lineEdit_86->text().size() > 0) {
			ckWgt = true;
			txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_86->text()).toLocal8Bit());
		}*/
	}

	CA.init_DDL_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, ckJnc, ckWgt, txNewJnc, weightAttributesSet);

	//多线程设置				
	length = 1 + int(FA.roadID.size()) / thread_num;

	//将待计算线条随机分组
	/*srand((unsigned)time(NULL));
	int len = FA.roadID.size();
	int aver_len = len / maxNum;
	std::vector<int> remain_roads;
	std::vector<std::set<int>> CA_Roads(maxNum);
	remain_roads.insert(remain_roads.begin(), FA.roadID.begin(), FA.roadID.end());

	int num_idx = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
		CA_Roads[num_idx].insert(road);
	}*/
	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroupsRandomly(FA.roadID, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++) {
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 1);

		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_MDR(std::string txAngleThreshold)
{
	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();


	//构建新的临时类，避免混淆
	CA.clearOldData();
	int order = 1, length = INT_MAX - 1;

	//获取shp文件
	std::string infilepath = std::string((const char *)(this->shpFileName).toLocal8Bit());
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_16->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	//QString qstrAngleThreshold = this->work->ui.lineEdit_44->text();
	//std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_43->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());
	bool ckJnc = false;
	bool ckWgt = false;
	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_32->isChecked()) {
		if (this->work->ui.lineEdit_83->text().size() > 0) {
			ckJnc = true;
			txNewJnc = std::string((const char *)(this->work->ui.lineEdit_83->text()).toLocal8Bit());
		}
		/*if (this->work->ui.lineEdit_84->text().size() > 0) {
			ckWgt = true;
			txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_84->text()).toLocal8Bit());
		}*/
	}

	CA.init_MDR_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, ckJnc, ckWgt, txNewJnc, weightAttributesSet);

	//Add：显示平均夹角
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	mean_angle_qstr = QString::number(mean_angle, 10, 1);
	this->summary_info = filename_qstr;
	if (mean_angle_qstr.size() > 0)
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	//this->summary_info += "------------------------------------\n";
	this->displaySummary();

	//多线程设置				
	length = 1 + int(FA.roadID.size()) / thread_num;

	//将待计算线条随机分组
	/*srand((unsigned)time(NULL));
	int len = FA.roadID.size();
	int aver_len = len / maxNum;
	std::vector<int> remain_roads;
	std::vector<std::set<int>> CA_Roads(maxNum);
	remain_roads.insert(remain_roads.begin(), FA.roadID.begin(), FA.roadID.end());

	int num_idx = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
		CA_Roads[num_idx].insert(road);
	}*/
	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroupsRandomly(FA.roadID, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++) {
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 1);

		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_JnR()
{

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();


	//构建新的临时类，避免混淆
	CA.clearOldData();
	int order = 1, length = INT_MAX - 1;

	//获取shp文件
	std::string infilepath = std::string((const char *)(this->shpFileName).toLocal8Bit());
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrJunctionDegree = this->work->ui.lineEdit_41->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());
	QString qstrJunctionsLimit = this->work->ui.lineEdit_14->text();
	std::string txJunctionsLimit = std::string((const char *)qstrJunctionsLimit.toLocal8Bit());
	bool ckJnDD = false;
	std::string txAngleThreshold2 = "";
	bool ckWgt = false;
	std::string txWgtLimitStr = "";
	if (this->work->ui.radioButton_32->isChecked()) {
		/*if (this->work->ui.lineEdit_84->text().size() > 0) {
			ckWgt = true;
			txWgtLimitStr = std::string((const char *)(this->work->ui.lineEdit_84->text()).toLocal8Bit());
		}*/
	}

	CA.init_JnR_para(FA, *g, infilepath, txJunctionDegree, txJunctionsLimit, ckJnDD, txAngleThreshold2, ckWgt, weightAttributesSet);

	//Add：显示平均夹角
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	mean_angle_qstr = QString::number(mean_angle, 10, 1);
	this->summary_info = filename_qstr;
	if (mean_angle_qstr.size() > 0)
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	//this->summary_info += "------------------------------------\n";
	this->displaySummary();

	//多线程设置				
	length = 1 + int(FA.roadID.size()) / thread_num;

	//CAVec.reserve(maxNum * sizeof(CA) + 1024);

	//将待计算线条随机分组
	/*srand((unsigned)time(NULL));
	int len = FA.roadID.size();
	int aver_len = len / maxNum;
	std::vector<int> remain_roads;
	std::vector<std::set<int>> CA_Roads(maxNum);
	remain_roads.insert(remain_roads.begin(), FA.roadID.begin(), FA.roadID.end());

	int num_idx = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
		CA_Roads[num_idx].insert(road);
	}*/
	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroupsRandomly(FA.roadID, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++) {
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 1);

		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_Net_MR()
{
	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold = this->work->ui.lineEdit_35->text();
	std::string txRadiiThreshold = std::string((const char *)qstrRadiiThreshold.toLocal8Bit());

	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_48->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_77->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
		/*QString qstr2 = this->work->ui.lineEdit_78->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
	}

	CA.init_Net_MR_para(FA, *g, infilepath, txRadiiThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_56->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		return false;
	}
	CA.searchLimitStr = transform(searchidStr);
	CA.getFromToID(FA, searchidStr);

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();


	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.FromIDVec.size()) / thread_num;
	//CAVec.reserve(maxNum * sizeof(CA) + 1028);
	for (int i = 0; i < thread_num; i++){
		order = i + 1;
		CA.setMultiPara(FA, order, length, 2);

		CAVec.push_back(CA);
	}

	return true;
}

bool MainWindow::init_for_Net_MD()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txRadiiThreshold = "0";
	std::string txNewJnc = std::to_string(INT_MAX - 1);
	std::string txWgtLimitStr = "";

	CA.init_Net_MR_para(FA, *g, infilepath, txRadiiThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_110->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		return false;
	}
	if (searchidStr.length() > 0) {
		CA.searchLimitStr = transform(searchidStr);
		CA.getFromToID(FA, searchidStr);
	}
	else	//注意：必须输入起点id
	{
		return false;
	}
	//设置
	CA.isStepDepth = true;
	CA.SD_MR = true;

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();


	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.FromIDVec.size()) / thread_num;

	//将待计算线条随机分组
	/*srand((unsigned)time(NULL));
	int len = CA.FromIDVec.size();
	int aver_len = len / maxNum;
	std::vector<int> remain_roads;
	std::vector<std::set<int>> CA_Roads(maxNum);
	remain_roads.insert(remain_roads.begin(), CA.FromIDVec.begin(), CA.FromIDVec.end());

	int num_idx = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
		CA_Roads[num_idx].insert(road);
	}*/
	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroupsRandomly(CA.FromIDVec, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++) {
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 2);

		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_Net_DR()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txRadiiThreshold2 = "";
	QString qstrAngleThreshold = this->work->ui.lineEdit_38->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_36->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());

	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_48->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_77->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
		/*QString qstr2 = this->work->ui.lineEdit_78->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
	}

	CA.init_Net_DR_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_56->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		return false;
	}
	CA.searchLimitStr = transform(searchidStr);
	CA.getFromToID(FA, searchidStr);

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();


	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.FromIDVec.size()) / thread_num;
	//CAVec.reserve(maxNum * sizeof(CA) + 1028);
	for (int i = 0; i < thread_num; i++)
	{
		order = i + 1;
		CA.setMultiPara(FA, order, length, 2);

		CAVec.push_back(CA);
	}

	return true;
}

bool MainWindow::init_for_Net_DD()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txRadiiThreshold2 = "";
	QString qstrAngleThreshold = this->work->ui.lineEdit_121->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	std::string txDirectionalChanges = "0";

	std::string txNewJnc = std::to_string(INT_MAX - 1);
	std::string txWgtLimitStr = "";

	CA.init_Net_DR_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_110->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		// OutputDebugString(("ref number length:" + std::to_string(ref_numer.size())).c_str());
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		return false;
	}
	if (searchidStr.length() > 0) {
		CA.searchLimitStr = transform(searchidStr);
		CA.getFromToID(FA, searchidStr);
	}
	else	//注意：必须输入起点id
	{
		return false;
	}
	//设置
	CA.isStepDepth = true;
	CA.SD_DR = true;
	
	//DD太占用内存，根据内存可用量决定线程数
	thread_num = getThreadNumByMem("NET_DD");

	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.FromIDVec.size()) / thread_num;
	
	////将待计算线条随机分组
	//srand((unsigned)time(NULL));
	//int len = CA.FromIDVec.size();
	//int aver_len = len / maxNum;
	//std::vector<int> remain_roads;
	//std::vector<std::set<int>> CA_Roads(maxNum);
	//remain_roads.insert(remain_roads.begin(), CA.FromIDVec.begin(), CA.FromIDVec.end());

	//int num_idx = 0;
	//while (remain_roads.size() > 0) {
	//	int road_idx = rand() % len;
	//	int road = remain_roads[road_idx];
	//	remain_roads.erase(remain_roads.begin() + road_idx);
	//	len = remain_roads.size();
	//	if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
	//	CA_Roads[num_idx].insert(road);
	//}

	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroups(CA.FromIDVec, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++) {
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 2);

		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_Net_MDR()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrRadiiThreshold2 = this->work->ui.lineEdit_40->text();
	std::string txRadiiThreshold2 = std::string((const char *)qstrRadiiThreshold2.toLocal8Bit());
	QString qstrAngleThreshold = this->work->ui.lineEdit_55->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	QString qstrDirectionalChanges = this->work->ui.lineEdit_54->text();
	std::string txDirectionalChanges = std::string((const char *)qstrDirectionalChanges.toLocal8Bit());

	std::string txNewJnc = "";
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_48->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_77->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
		/*QString qstr2 = this->work->ui.lineEdit_78->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
	}

	CA.init_Net_DR_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_56->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		return false;
	}
	CA.searchLimitStr = transform(searchidStr);
	CA.getFromToID(FA, searchidStr);

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();


	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.FromIDVec.size()) / thread_num;
	//CAVec.reserve(maxNum * sizeof(CA) + 1028);
	for (int i = 0; i < thread_num; i++)
	{
		order = i + 1;
		CA.setMultiPara(FA, order, length, 2);

		CAVec.push_back(CA);
	}

	return true;
}

bool MainWindow::init_for_Net_JnR()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrJunctionDegree = this->work->ui.lineEdit_53->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());
	QString qstrJunctionsLimit = this->work->ui.lineEdit_39->text();
	std::string txJunctionsLimit = std::string((const char *)qstrJunctionsLimit.toLocal8Bit());

	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_48->isChecked()) {
		/*QString qstr2 = this->work->ui.lineEdit_78->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
	}

	CA.init_Net_JnR_para(FA, *g, infilepath, txJunctionDegree, txJunctionsLimit, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_56->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		return false;
	}
	CA.searchLimitStr = transform(searchidStr);
	CA.getFromToID(FA, searchidStr);

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();


	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.FromIDVec.size()) / thread_num;
	//CAVec.reserve(maxNum * sizeof(CA) + 1028);
	for (int i = 0; i < thread_num; i++)
	{
		order = i + 1;
		CA.setMultiPara(FA, order, length, 2);

		CAVec.push_back(CA);
	}

	return true;
}

bool MainWindow::init_for_Net_JnD()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrJunctionDegree = this->work->ui.lineEdit_122->text();
	std::string txJunctionDegree = std::string((const char *)qstrJunctionDegree.toLocal8Bit());
	std::string txJunctionsLimit = "0";

	std::string txWgtLimitStr = "";

	CA.init_Net_JnR_para(FA, *g, infilepath, txJunctionDegree, txJunctionsLimit, weightAttributesSet);

	//获取From ID
	QString qstrsearchidStr = this->work->ui.lineEdit_110->text();
	std::string searchidStr;
	if (qstrsearchidStr.length() > 0) {
		searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());
	}
	else if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1) {
		searchidStr = std::string((const char *)ref_numer.toLocal8Bit());
	}
	else {
		return false;
	}
	if (searchidStr.length() > 0) {
		CA.searchLimitStr = transform(searchidStr);
		CA.getFromToID(FA, searchidStr);
	}
	else	//注意：必须输入起点id
	{
		return false;
	}
	//设置
	CA.isStepDepth = true;
	CA.SD_JnR = true;

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();



	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.FromIDVec.size()) / thread_num;
	
	//将待计算线条随机分组
	/*srand((unsigned)time(NULL));
	int len = CA.FromIDVec.size();
	int aver_len = len / maxNum;
	std::vector<int> remain_roads;
	std::vector<std::set<int>> CA_Roads(maxNum);
	remain_roads.insert(remain_roads.begin(), CA.FromIDVec.begin(), CA.FromIDVec.end());

	int num_idx = 0;
	while (remain_roads.size() > 0) {
		int road_idx = rand() % len;
		int road = remain_roads[road_idx];
		remain_roads.erase(remain_roads.begin() + road_idx);
		len = remain_roads.size();
		if (num_idx != maxNum - 1 && CA_Roads[num_idx].size() == aver_len) ++num_idx;
		CA_Roads[num_idx].insert(road);
	}*/
	//索引顺序分组
	std::vector<std::set<int>> CA_Roads(thread_num);
	splitArrayIntoGroupsRandomly(CA.FromIDVec, thread_num, CA_Roads);

	//准备好FIA和CA类组
	for (int i = 0; i < thread_num; i++) {
		//多线程参数设置
		order = i + 1;
		//CA.setMultiPara(FA, order, length, 1);
		CA.setMultiParaByRand(CA_Roads[i], 2);

		CAVec.emplace_back(CA);
	}

	return true;
}

bool MainWindow::init_for_Geo_MR()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	std::string txAngleThreshold = std::to_string(INT_MAX - 1);
	std::string txNewJnc = std::to_string(INT_MAX - 1);
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_30->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_80->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
		/*QString qstr2 = this->work->ui.lineEdit_81->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
		QString qstr3 = this->work->ui.lineEdit_79->text();
		txAngleThreshold = std::string((const char *)qstr3.toLocal8Bit());
	}

	CA.init_Geo_MR_para(FA, *g, infilepath, txAngleThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrFromStr = this->work->ui.lineEdit_6->text();
	std::string FromStr = std::string((const char *)qstrFromStr.toLocal8Bit());
	QString qstrToStr = this->work->ui.lineEdit_7->text();
	std::string ToStr = std::string((const char *)qstrToStr.toLocal8Bit());
	std::string fromToStr = FromStr;
	if (ToStr.length() > 0)
		fromToStr = FromStr + ";" + ToStr;

	//获取csv文件路径
	QString qstrCsvfilePath = this->work->ui.lineEdit_2->text();
	std::string CsvfilePath = std::string((const char *)qstrCsvfilePath.toLocal8Bit());
	CA.outputPath = true;
	if (CsvfilePath.find(".csv") != std::string::npos)
	{
		fromToStr = CsvfilePath;
		CA.incsvfilename = CsvfilePath;
	}

	CA.searchLimitStr = transform(fromToStr);
	CA.getFromToID(FA, fromToStr);

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();


	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.fromIDSet.size()) / thread_num;
	for (int i = 0; i < thread_num; i++)
	{
		order = i + 1;
		CA.setMultiPara(FA, order, length, 2);

		CAVec.push_back(CA);
	}

	return true;
}

bool  MainWindow::init_for_Geo_DR()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrAngleThreshold = this->work->ui.lineEdit_29->text();
	std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());

	std::string txNewJnc = std::to_string(INT_MAX - 1);
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_30->isChecked()) {
		QString qstr1 = this->work->ui.lineEdit_80->text();
		txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
	/*	QString qstr2 = this->work->ui.lineEdit_81->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
		//QString qstr3 = this->work->ui.lineEdit_79->text();
		//txAngleThreshold = std::string((const char *)qstr3.toLocal8Bit());
	}

	CA.init_Geo_DR_para(FA, *g, infilepath, txAngleThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrFromStr = this->work->ui.lineEdit_6->text();
	std::string FromStr = std::string((const char *)qstrFromStr.toLocal8Bit());
	QString qstrToStr = this->work->ui.lineEdit_7->text();
	std::string ToStr = std::string((const char *)qstrToStr.toLocal8Bit());
	std::string fromToStr = FromStr;
	if (ToStr.length() > 0)
		fromToStr = FromStr + ";" + ToStr;

	//获取csv文件路径
	QString qstrCsvfilePath = this->work->ui.lineEdit_2->text();
	std::string CsvfilePath = std::string((const char *)qstrCsvfilePath.toLocal8Bit());
	CA.outputPath = true;
	if (CsvfilePath.find(".csv") != std::string::npos)
	{
		fromToStr = CsvfilePath;
		CA.incsvfilename = CsvfilePath;
	}

	CA.searchLimitStr = transform(fromToStr);
	CA.getFromToID(FA, fromToStr);

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();

	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.fromIDSet.size()) / thread_num;
	//CAVec.reserve(thread_num * sizeof(CA) + 1028);
	for (int i = 0; i < thread_num; i++)
	{
		order = i + 1;
		CA.setMultiPara(FA, order, length, 2);

		CAVec.push_back(CA);
	}

	return true;
}

bool MainWindow::init_for_Geo_JnR()
{

	//获取shp文件
	Graph *g = FA.ProcessShapeFile();

	//初始化参数
	QString qstrNewJnc = this->work->ui.lineEdit_31->text();
	std::string txNewJnc = std::string((const char *)qstrNewJnc.toLocal8Bit());

	std::string txAngleThreshold = std::to_string(INT_MAX - 1);
	std::string txWgtLimitStr = "";

	if (this->work->ui.radioButton_30->isChecked()) {
		//QString qstr1 = this->work->ui.lineEdit_80->text();
		//txNewJnc = std::string((const char *)qstr1.toLocal8Bit());
		/*QString qstr2 = this->work->ui.lineEdit_81->text();
		txWgtLimitStr = std::string((const char *)qstr2.toLocal8Bit());*/
		QString qstr3 = this->work->ui.lineEdit_79->text();
		txAngleThreshold = std::string((const char *)qstr3.toLocal8Bit());
	}

	CA.init_Geo_JnR_para(FA, *g, infilepath, txAngleThreshold, txNewJnc, weightAttributesSet);

	//获取From ID
	QString qstrFromStr = this->work->ui.lineEdit_6->text();
	std::string FromStr = std::string((const char *)qstrFromStr.toLocal8Bit());
	QString qstrToStr = this->work->ui.lineEdit_7->text();
	std::string ToStr = std::string((const char *)qstrToStr.toLocal8Bit());
	std::string fromToStr = FromStr;
	if (ToStr.length() > 0)
		fromToStr = FromStr + ";" + ToStr;

	//获取csv文件路径
	QString qstrCsvfilePath = this->work->ui.lineEdit_2->text();
	std::string CsvfilePath = std::string((const char *)qstrCsvfilePath.toLocal8Bit());
	CA.outputPath = true;
	if (CsvfilePath.find(".csv") != std::string::npos)
	{
		fromToStr = CsvfilePath;
		CA.incsvfilename = CsvfilePath;
	}

	CA.searchLimitStr = transform(fromToStr);
	CA.getFromToID(FA, fromToStr);

	//探测cpu逻辑核心数目
	thread_num = getThreadNumBySys();

	//准备好FIA和CA类组
	int order = 1, length = 1 + int(CA.fromIDSet.size()) / thread_num;
	//CAVec.reserve(maxNum * sizeof(CA) + 1028);
	for (int i = 0; i < thread_num; i++)
	{
		order = i + 1;
		CA.setMultiPara(FA, order, length, 2);

		CAVec.push_back(CA);
	}

	return true;
}

void MainWindow::output_calculate_Data_DD(MainWindow* temp) {
	while (true)
	{
		bool isAllFinished = true;
		for (int i = 0; i < thread_num; i++)
		{
			if (temp->get_NeedStop()) {
				return;
			}

			if (!CAVec[i].isFinished) {
				isAllFinished = false;
				Sleep(100);
			}
				
		}
		if (isAllFinished)
		{
			//修改进度条提示
			Sleep(100);
			process_str = "writing data, please wait ...";
			//更新进度条
			emit(temp->pDisplaydow->ui->Button_start_scan->clicked());

			//依次输出，避免数据访问冲突
			for (int i = 0; i < thread_num; i++)
			{
				CAVec[i].OutputData(temp->FA);
			}

			if (CAVec[0].isNet || CAVec[0].isGeo) {
				//重写txt
				std::string SHPfilename = std::string((const char *)(temp->shpFileName).toLocal8Bit());
				std::string shpfilename = SHPfilename;
				SHPfilename = SHPfilename.substr(0, SHPfilename.length() - 4);
				std::string dbffilename = SHPfilename + ".dbf";
				std::string txtfilename;
				if (CAVec[0].isNet)
					txtfilename = temp->netreach_file_path;
				else
					txtfilename = temp->geodesics_file_path;

				temp->drawTxtFile(shpfilename, dbffilename, txtfilename, false);
			}
			else {
				//合并新计算的属性到CAVec[0]
				for (int i = 1; i < thread_num; i++) {
					for (auto it = CAVec[0].TempModifyData.begin(); it != CAVec[0].TempModifyData.end(); it++) {
						string field_name = it->first;
						if (CAVec[i].TempModifyData.count(field_name))
							CAVec[0].TempModifyData[field_name].insert(CAVec[i].TempModifyData[field_name].begin(), CAVec[i].TempModifyData[field_name].end());
					}
				}
				temp->updateAttributes(CAVec[0]);
			}
			if (temp->get_calculate_over() && time_qstr.size()) {
				process_str = "Run Over, " + time_qstr;
				//更新进度条
				emit(temp->pDisplaydow->ui->Button_start_scan->clicked());
			}

			//告知结束
			temp->set_run_start(false);
			mutex_fc.lock();
			temp->finished_angle_count += 1;
			mutex_fc.unlock();

			return;
		}
	}
}

void MainWindow::output_calculate_Data(MainWindow* temp)
{
	while (true)
	{
		bool isAllFinished = true;
		for (int i = 0; i < thread_num; i++)
		{
			if (temp->get_NeedStop()) {
				weightAttributesSet.clear();
				return;
			}

			if (!CAVec[i].isFinished) {
				isAllFinished = false;
				Sleep(100);
			}
				
		}
		if (isAllFinished)
		{
			Sleep(100);
			process_str = "writing data, please wait ...";
			//更新进度条
			emit(temp->pDisplaydow->ui->Button_start_scan->clicked()); //pDisplaydow是计算模块的

			//依次输出，避免数据访问冲突
			for (int i = 0; i < thread_num; i++)
			{
				CAVec[i].OutputData(temp->FA);
			}

			if (CAVec[0].isNet || CAVec[0].isGeo) {
				//重写txt
				std::string SHPfilename = std::string((const char *)(temp->shpFileName).toLocal8Bit());
				std::string shpfilename = SHPfilename;
				SHPfilename = SHPfilename.substr(0, SHPfilename.length() - 4);
				std::string dbffilename = SHPfilename + ".dbf";
				std::string txtfilename;
				if (CAVec[0].isNet)
					txtfilename = temp->netreach_file_path;
				else
					txtfilename = temp->geodesics_file_path;

				temp->drawTxtFile(shpfilename, dbffilename, txtfilename, false);
			}
			else {
				//合并新计算的属性到CAVec[0]
				for (int i = 1; i < thread_num; i++) {
					for (auto it = CAVec[0].TempModifyData.begin(); it != CAVec[0].TempModifyData.end(); it++) {
						string field_name = it->first;
						if (CAVec[i].TempModifyData.count(field_name))
							CAVec[0].TempModifyData[field_name].insert(CAVec[i].TempModifyData[field_name].begin(), CAVec[i].TempModifyData[field_name].end());
					}
				}
				//开始更新属性栏吧
				temp->updateAttributes(CAVec[0]);
			}
			//告知结束
			temp->set_run_start(false);
			mutex_fc.lock();
			temp->finished_angle_count += 1;
			mutex_fc.unlock();

			process_str = "Run Over, "+ time_qstr;
			//更新进度条
			emit(temp->pDisplaydow->ui->Button_start_scan->clicked());

			weightAttributesSet.clear();
			return;
		}
	}
}

void MainWindow::reDraw() {
	std::string SHPfilename = std::string((const char *)(this->shpFileName).toLocal8Bit());
	std::string DBFfilename = SHPfilename.substr(0, SHPfilename.length() - 4) + ".dbf";
	std::string txtfilename = this->global_file_path;

	this->drawTxtFile(SHPfilename, DBFfilename, txtfilename, false);
}

void MainWindow::reDrawNet(bool checked) {
	if (!isOpen) return;
	if (checked) {
		this->reDrawNetReach();
		this->work->ui.radioButton_6->setChecked(false);
	}		
	else {
		this->work->ui.radioButton_5->setChecked(true);
		this->cancelNetReach();

		////修改ID颜色
		//MetaGraph *graph = m_treeDoc->m_meta_graph;
		//if (!graph) return;
		//AttributeTable& tab = graph->getAttributeTable();
		//if (graph->viewingProcessed()) {
		//	graph->setDisplayedAttribute(tab.getColumnIndex("ID"));
		//	UpdateColorMap();
		//}
		//SetAttributeChecks();
	}
}

void MainWindow::reDrawGeo(bool checked) {
	if (!isOpen) return;
	if (checked) {
		this->reDrawGeoReach();
		this->geodesics_view = true;

		this->netreach_view = false;
		this->work->ui.radioButton_5->setChecked(false);
	}		
	else {
		this->work->ui.radioButton_6->setChecked(true);
		this->cancelGeoReach();

		////修改ID颜色
		//MetaGraph *graph = m_treeDoc->m_meta_graph;
		//if (!graph) return;
		//AttributeTable& tab = graph->getAttributeTable();
		//if (graph->viewingProcessed()) {
		//	graph->setDisplayedAttribute(tab.getColumnIndex("ID"));
		//	UpdateColorMap();
		//}
		//SetAttributeChecks();
	}
}

void MainWindow::setBackground(QColor newColor) {
	m_background = getColor(newColor.red(), newColor.green(), newColor.blue());

	auto settings = mSettings.getTransaction();
	settings->writeSetting(SettingTag::backgroundColour, newColor);

	if (!isOpen) return;
	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	for (int i = 0; i < windows.size(); ++i) {
		GLView *child = qobject_cast<GLView*>(windows.at(i)->widget());
		if (!child) continue;
		child->UpdateBackgroundColor(getColor(newColor.red(), newColor.green(), newColor.blue()));
		child->update();
	}
}

void MainWindow::PushFromID(bool checked) {
	if (checked)
		this->work->ui.lineEdit_6->setText(ref_numer);
}

void MainWindow::PushToID(bool checked) {
	if (checked)
		this->work->ui.lineEdit_7->setText(ref_numer);
}

void MainWindow::setStop() {
	set_NeedStop(true);
}

void MainWindow::UpdateColorMap() {
	FixedColoredLinesMap.clear();

	//获取当前GL视图展示的属性名称
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);

	//添加global线条颜色
	if (FixedColoredLinesMapAll[field_name].size() > 0)
		global_color = FixedColoredLinesMapAll[field_name][0];
	else
		global_color = WhiteColor;
	FixedColoredLinesMap = FixedColoredLinesMapAll[field_name];
	FixedThickLinesMap = FixedThickLinesMapAll[field_name];

	//添加Net/Geo线条颜色
	if (this->work->ui.radioButton_5->isChecked()) {
		for (auto it = NetFixedColoredLinesMap.begin(); it != NetFixedColoredLinesMap.end(); it++) {
			FixedColoredLinesMap[it->first] = it->second;
		}
		//数据临时存储
		ID_to_width.clear();
		for (auto it = FixedThickLinesMap.begin(); it != FixedThickLinesMap.end(); it++) {
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				ID_to_width[*iter] = it->first;
			}
		}
		for (auto it = NetFixedThickLinesMap.begin(); it != NetFixedThickLinesMap.end(); it++) {
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				ID_to_width[*iter] = it->first;
			}
		}
		//数据转换
		FixedThickLinesMap.clear();
		for (auto it = ID_to_width.begin(); it != ID_to_width.end(); it++) {
			FixedThickLinesMap[it->second].insert(it->first);
		}
	}
	else if (this->work->ui.radioButton_6->isChecked()) {
		for (auto it = GeoFixedColoredLinesMap.begin(); it != GeoFixedColoredLinesMap.end(); it++) {
			FixedColoredLinesMap[it->first] = it->second;
		}
		//数据临时存储
		ID_to_width.clear();
		for (auto it = FixedThickLinesMap.begin(); it != FixedThickLinesMap.end(); it++) {
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				ID_to_width[*iter] = it->first;
			}
		}
		for (auto it = GeoFixedThickLinesMap.begin(); it != GeoFixedThickLinesMap.end(); it++) {
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				ID_to_width[*iter] = it->first;
			}
		}
		//数据转换
		FixedThickLinesMap.clear();
		for (auto it = ID_to_width.begin(); it != ID_to_width.end(); it++) {
			FixedThickLinesMap[it->second].insert(it->first);
		}
	}

	//触发重绘显示
	this->updateGLWindows(true, false);
}
//reset current attribute lines color to blue
void MainWindow::ResetVisual() {
	if (!isOpen) return;

	// 判断选中的是 net(1)、geo(2)，还是 global(0)
	int flag = 0;

	if (this->work->ui.radioButton_5->isChecked()) {
		flag = 1; // net
	}
	else if (this->work->ui.radioButton_6->isChecked()) {
		flag = 2; // geo
	}

	// 记录上次的参数设置
	spinBox_value = 1;
	comboBox_idx = 0;
	comboBox_value = "ID";
	spinBox_2_value = 2;
	lineEdit_value = "0.00";
	comboBox_2_idx = 0;
	comboBox_2_value = "5";

	// 获取当前 GL 视图展示的属性名称
	MetaGraph* graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);

	// 根据 flag 更新视觉效果
	switch (flag) {
	case 0: // global
		FixedColoredLinesMap.clear();
		FixedThickLinesMap.clear();
		global_color = WhiteColor; // 恢复全局颜色

		// 从持久化映射中恢复颜色
		if (FixedColoredLinesMapAll.find(field_name) != FixedColoredLinesMapAll.end()) {
			FixedColoredLinesMap = FixedColoredLinesMapAll[field_name];
		}
		// 从持久化映射中恢复粗细
		if (FixedThickLinesMapAll.find(field_name) != FixedThickLinesMapAll.end()) {
			FixedThickLinesMap = FixedThickLinesMapAll[field_name];
		}
		break;
	case 1: // net
		if (this->net_map_all.size() == 0) break;

		// 从持久化映射中恢复网格线条颜色
		if (FixedColoredLinesMapAll.find(field_name) != FixedColoredLinesMapAll.end()) {
			FixedColoredLinesMap = FixedColoredLinesMapAll[field_name];
			for (auto iter = NetRoadsAll.begin(); iter != NetRoadsAll.end(); iter++) {
				FixedColoredLinesMap[*iter] = GreenColor; // 修改为绿色
			}
		}
		// 从持久化映射中恢复粗细
		if (FixedThickLinesMapAll.find(field_name) != FixedThickLinesMapAll.end()) {
			FixedThickLinesMap = FixedThickLinesMapAll[field_name];
		}
		break;
	case 2: // geo
		if (this->geo_map_all.size() == 0) break;

		// 从持久化映射中恢复地理线条颜色
		if (FixedColoredLinesMapAll.find(field_name) != FixedColoredLinesMapAll.end()) {
			FixedColoredLinesMap = FixedColoredLinesMapAll[field_name];
			for (auto iter = GeoRoadsAll.begin(); iter != GeoRoadsAll.end(); iter++) {
				FixedColoredLinesMap[*iter] = GreenColor; // 修改为绿色
			}
		}
		// 从持久化映射中恢复粗细
		if (FixedThickLinesMapAll.find(field_name) != FixedThickLinesMapAll.end()) {
			FixedThickLinesMap = FixedThickLinesMapAll[field_name];
		}
		break;
	default:
		break;
	}

	// 恢复颜色栏（移除直接设置蓝色的代码）
	// 这部分可以根据需要调整，或完全移除
	// QColor color(0, 0, 255);
	// this->work->ui.colorPalette_2->setColor(color);

	// 触发重绘显示
	this->updateGLWindows(true, false);
}
//根据Layers判断修改哪一类线条，根据鼠标是否选中线段判断修改具体哪些线条
void MainWindow::ChangeColor(QColor newColor) {
	if (!isOpen) return;

	// 获取颜色
	PafColor color = getColor(newColor.red(), newColor.green(), newColor.blue());

	// 判断选中的是 global(0)、net(1)、geo(2)
	int flag = 0;

	if (this->work->ui.radioButton_5->isChecked()) {
		flag = 1; // net
	}
	else if (this->work->ui.radioButton_6->isChecked()) {
		flag = 2; // geo
	}

	// 判断是否修改所有线条
	bool isAll = true;

	// 获取当前 GL 视图展示的属性名称
	MetaGraph* graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);

	if (isAll) {
		switch (flag) {
		case 0: // global
			global_color = color;
			for (int i = 0; i < this->global_map.RecordCount; i++) {
				FixedColoredLinesMap[i] = color; // 仅修改当前上下文的颜色
			}
			// 不再保存对 FixedColoredLinesMapAll 的修改
			break;
		case 1: // net
			if (this->net_map_all.size() == 0) break;
			for (auto iter = NetRoadsAll.begin(); iter != NetRoadsAll.end(); iter++) {
				FixedColoredLinesMap[*iter] = color;
				NetFixedColoredLinesMap[*iter] = color;
			}
			break;
		case 2: // geo
			if (this->geo_map_all.size() == 0) break;
			for (auto iter = GeoRoadsAll.begin(); iter != GeoRoadsAll.end(); iter++) {
				FixedColoredLinesMap[*iter] = color;
				GeoFixedColoredLinesMap[*iter] = color;
			}
			break;
		default:
			break;
		}
	}
	else {
		// 目前只做全体线条修改
	}

	// 触发重绘显示
	this->updateGLWindows(true, false);
}

void MainWindow::ChangeThickness() {
	if (!isOpen) return;

	// 获取输入
	bool change_global = this->highlight_window->ui.checkBox->isChecked();
	bool change_attribute = this->highlight_window->ui.checkBox_2->isChecked();
	bool open_greater = this->highlight_window->ui.radioButton->isChecked();
	bool open_top = this->highlight_window->ui.radioButton_2->isChecked();

	std::string global_width = (this->highlight_window->ui.spinBox->text().toLocal8Bit()).data();
	std::string attribute_name = (this->highlight_window->ui.comboBox->currentText().toLocal8Bit()).data();
	std::string attribute_width = (this->highlight_window->ui.spinBox_2->text().toLocal8Bit()).data();
	std::string greater_value = (this->highlight_window->ui.lineEdit->text().toLocal8Bit()).data();
	std::string top_value = (this->highlight_window->ui.comboBox_2->currentText().toLocal8Bit()).data();

	// 检查宽度
	double value_1 = std::stof(global_width), value_2 = std::stof(attribute_width);
	if (change_global && change_attribute && value_2 <= value_1) {
		QMessageBox::information(NULL, "Tips", "Second Line Width needs to be greater than First Line Width, please check");
		return;
	}

	// 判断选中的是 global(0)、net(1)、geo(2)
	int flag = 0;
	if (this->work->ui.radioButton_5->isChecked()) {
		flag = 1; // net
	}
	else if (this->work->ui.radioButton_6->isChecked()) {
		flag = 2; // geo
	}

	// 获取当前 GL 视图展示的属性名称
	MetaGraph* graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);

	// 数据临时存储
	ID_to_width.clear();
	FixedThickLinesMap = FixedThickLinesMapAll[field_name]; // 先恢复当前的粗细设置

	// 更新线条粗细
	switch (flag) {
	case 0: // global
		if (change_global) {
			double width = std::stof(global_width);
			for (int i = 0; i < this->global_map.RecordCount; i++) {
				ID_to_width[i] = width;
			}
		}

		if (change_attribute) {
			double width = std::stof(attribute_width);
			if (this->global_map.Attributes.AttributesDouble.count(attribute_name) == 0) break;

			if (open_greater) {
				double limit_value = std::stof(greater_value);
				for (auto it = this->global_map.Attributes.AttributesDouble[attribute_name].begin(); it != this->global_map.Attributes.AttributesDouble[attribute_name].end(); it++) {
					if (it->second > limit_value)
						ID_to_width[it->first] = width;
				}
			}
			else if (open_top) {
				double top = std::stof(top_value) / 100.0;
				int topCount = this->global_map.RecordCount * top;
				// 创建 map，key 是属性值，value 是 Ref
				std::map<double, std::set<int>> value_to_Ref;
				for (auto it = this->global_map.Attributes.AttributesDouble[attribute_name].begin(); it != this->global_map.Attributes.AttributesDouble[attribute_name].end(); it++) {
					int id = it->first;
					double value = it->second;
					value_to_Ref[value].insert(id);
				}
				int count = 0;
				for (auto it = value_to_Ref.rbegin(); it != value_to_Ref.rend(); it++) {
					for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
						if (count == topCount) break;
						ID_to_width[*iter] = width;
						++count;
					}
					if (count == topCount) break;
				}
			}
		}

		// 数据转换
		FixedThickLinesMap.clear();
		for (auto it = ID_to_width.begin(); it != ID_to_width.end(); it++) {
			FixedThickLinesMap[it->second].insert(it->first);
		}
		break;

	case 1: // net
		if (change_global) {
			for (auto iter = NetRoadsAll.begin(); iter != NetRoadsAll.end(); iter++) {
				ID_to_width[*iter] = std::stof(global_width);
			}
		}

		if (change_attribute) {
			double width = std::stof(attribute_width);
			if (this->global_map.Attributes.AttributesDouble.count(attribute_name) == 0) break;

			if (open_greater) {
				double limit_value = std::stof(greater_value);
				for (auto it = this->net_map_all.begin(); it != this->net_map_all.end(); it++) {
					for (auto iter = it->second.Id_to_Ref.begin(); iter != it->second.Id_to_Ref.end(); iter++) {
						int id = iter->first, ref = iter->second;
						if (this->global_map.Attributes.AttributesDouble[attribute_name][id] > limit_value)
							ID_to_width[ref] = width;
					}
				}
			}
			else if (open_top) {
				double top = std::stof(top_value) / 100.0;
				int topCount = this->global_map.RecordCount * top;
				// 创建 map，key 是属性值，value 是 Ref
				std::map<double, std::set<int>> value_to_Ref;
				for (auto it = this->net_map_all.begin(); it != this->net_map_all.end(); it++) {
					for (auto iter = it->second.Id_to_Ref.begin(); iter != it->second.Id_to_Ref.end(); iter++) {
						int id = iter->first, ref = iter->second;
						double value = this->global_map.Attributes.AttributesDouble[attribute_name][id];
						value_to_Ref[value].insert(ref);
					}
				}
				int count = 0;
				for (auto it = value_to_Ref.rbegin(); it != value_to_Ref.rend(); it++) {
					for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
						if (count == topCount) break;
						ID_to_width[*iter] = width;
						++count;
					}
					if (count == topCount) break;
				}
			}
		}

		// 数据转换
		NetFixedThickLinesMap.clear();
		for (auto it = ID_to_width.begin(); it != ID_to_width.end(); it++) {
			NetFixedThickLinesMap[it->second].insert(it->first);
			FixedThickLinesMap[it->second].insert(it->first);
		}
		break;

	case 2: // geo
		if (change_global) {
			for (auto iter = GeoRoadsAll.begin(); iter != GeoRoadsAll.end(); iter++) {
				ID_to_width[*iter] = std::stof(global_width);
			}
		}

		if (change_attribute) {
			double width = std::stof(attribute_width);
			if (this->global_map.Attributes.AttributesDouble.count(attribute_name) == 0) break;

			if (open_greater) {
				double limit_value = std::stof(greater_value);
				for (auto it = GeoRoadsAll.begin(); it != GeoRoadsAll.end(); it++) {
					if (this->global_map.Attributes.AttributesDouble[attribute_name][*it] > limit_value)
						ID_to_width[*it] = width;
				}
			}
			else if (open_top) {
				double top = std::stof(top_value) / 100.0;
				int topCount = this->global_map.RecordCount * top;
				// 创建 map，key 是属性值，value 是 Ref
				std::map<double, std::set<int>> value_to_Ref;
				for (auto it = GeoRoadsAll.begin(); it != GeoRoadsAll.end(); it++) {
					int id = *it;
					double value = this->global_map.Attributes.AttributesDouble[attribute_name][id];
					value_to_Ref[value].insert(id);
				}
				int count = 0;
				for (auto it = value_to_Ref.rbegin(); it != value_to_Ref.rend(); it++) {
					for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
						if (count == topCount) break;
						ID_to_width[*iter] = width;
						++count;
					}
					if (count == topCount) break;
				}
			}
		}

		// 数据转换
		GeoFixedThickLinesMap.clear();
		for (auto it = ID_to_width.begin(); it != ID_to_width.end(); it++) {
			GeoFixedThickLinesMap[it->second].insert(it->first);
			FixedThickLinesMap[it->second].insert(it->first);
		}
		break;

	default:
		break;
	}

	this->highlight_window->close();

	// 触发重绘显示
	this->updateGLWindows(true, false);
}

void MainWindow::start_Multi_thread_for_calculate_DD(MainWindow* temp) {
	//将线程跟CPU逻辑核心进行绑定，不允许出现线程切换
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpu_num = std::min((int)std::thread::hardware_concurrency(), (int)(si.dwNumberOfProcessors));
	long long cpu_pos = static_cast<long long>(pow(2, cpu_num - 1));

	//反馈线程数目
	//temp->summary_info += "calculation using "+ QString::number(cpu_num)+" threads\n";
	////double tmp_mem = double(CAVec.capacity()) / (1024.0*1024.0*1024.0);
	////temp->summary_info += "Current memory usage = " + QString::number(tmp_mem) + " GB\n";
	//temp->displaySummary();

	//按边顺序依次启动
	for (int i = 0; i < thread_num; i++)
	{
		//
		std::thread thrd(&Calculation::MultiCalculate, &CAVec[i], ref(temp->FA), cpu_pos);
		thrd.detach();

		//设置用哪个CPU核心处理该线程			
		//SetThreadAffinityMask(GetCurrentThread(), pos);
		cpu_pos = cpu_pos >> 1;
		if (cpu_pos == 0)
		{
			cpu_pos = static_cast<long long>(pow(2, cpu_num - 1));
		}
	}

	//输出数据
	std::thread thrd(output_calculate_Data_DD, temp);
	thrd.detach();
	

	////设置用哪个CPU核心处理该线程		
	//SetThreadAffinityMask(GetCurrentThread(), 2);
}
std::string GetCurrentThreadIdAsString()
{
	DWORD threadId = GetCurrentThreadId();
	return std::to_string(threadId)+"\n";
}
void MainWindow::start_Multi_thread_for_calculate(MainWindow* temp)
{
	//将线程跟CPU逻辑核心进行绑定，不允许出现线程切换
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpu_num = std::min((int)std::thread::hardware_concurrency(), (int)(si.dwNumberOfProcessors));
	long long cpu_pos = static_cast<long long>(pow(2, cpu_num - 1));

	//按边顺序依次启动
	for (int i = 0; i < thread_num; i++)
	{
		std::thread thrd(&Calculation::MultiCalculate, &CAVec[i], ref(temp->FA), cpu_pos);
		thrd.detach();
		cpu_pos = cpu_pos >> 1;
		if (cpu_pos == 0)
		{
			cpu_pos = static_cast<long long>(pow(2, cpu_num - 1));
		}
	}

	//输出数据
	std::thread thrd(output_calculate_Data, temp); 
	thrd.detach();	
}

void MainWindow::output_NetGeo_Data(MainWindow* temp)
{
	//收割数据，进行存储
	while (true)
	{
		bool isAllFinished = true;
		for (int i = 0; i < thread_num; i++)
		{
			if (temp->get_NeedStop()) {
				weightAttributesSet.clear();
				return;
			}

			if (!CAVec[i].isFinished)
				isAllFinished = false;
		}
		if (isAllFinished)
		{
			//logOut << GetNowTime() << ", " << "All Calculation Finished.\n";

			//temp->CA.clearNGdata();

			//修改进度条提示
			while (true) {
				if (temp->get_calculate_over())
					break;
			}
			Sleep(5);
			process_str = "writing data, please wait ...";
			//更新进度条
			emit(temp->pDisplaydow->ui->Button_start_scan->clicked());

			//输出stepDepth
			if (temp->CA.isStepDepth)
			{
				//复制数据
				temp->CA.distanceAll.clear();
				std::map<int, double> minStartAll;
				double value = -1;
				for (auto it = temp->FA.roadID.begin(); it != temp->FA.roadID.end(); it++)
				{
					int endRoad = *it;
					for (int CApos = 0; CApos < thread_num; CApos++)
					{
						if (CAVec[CApos].distanceAll.count(endRoad) > 0) {
							temp->CA.distanceAll[endRoad].insert(CAVec[CApos].distanceAll[endRoad].begin(), CAVec[CApos].distanceAll[endRoad].end());
						}						
					}

					if (temp->CA.distanceAll.count(endRoad) > 0) {
						auto iter = temp->CA.distanceAll[endRoad].begin();
						value = *iter;
					}
					else {
						value = -1;
					}

					minStartAll.insert(std::make_pair(endRoad, value));
				}


				//增加表列Step Depth，修改回原shp文件
				temp->CA.modifyDBF(minStartAll, CAVec[0].dbfFilePath, CAVec[0].stepDepthName);
				//temp->CA.modifyCSV(minStartAll, CAVec[0].csvFilePath, CAVec[0].stepDepthName);
				//更新Attributes栏目
				temp->updateAttributes(temp->CA);


				process_str = "Run Over, "+ time_qstr;
				//更新进度条
				emit(temp->pDisplaydow->ui->Button_start_scan->clicked());
				temp->set_run_start(false);
				weightAttributesSet.clear();
				return;
			}

			//将所有的Net数据、Geo数据叠加整合到第一个CA类，进行一次性输出
			for (int CApos = 0; CApos < thread_num; CApos++)
			{
				//拉取数据进行混合到CAVec[0]
				if (CAVec[0].isNet)
				{
					temp->CA.reachRoad_all.insert(CAVec[CApos].reachRoad_all.begin(), CAVec[CApos].reachRoad_all.end());
					temp->CA.partInRoads_all.insert(CAVec[CApos].partInRoads_all.begin(), CAVec[CApos].partInRoads_all.end());
					temp->CA.partInReachRoadLen.insert(CAVec[CApos].partInReachRoadLen.begin(), CAVec[CApos].partInReachRoadLen.end());
					temp->CA.partInReachRoadNodeLen.insert(CAVec[CApos].partInReachRoadNodeLen.begin(), CAVec[CApos].partInReachRoadNodeLen.end());

					for (int i = 0; i < int(CAVec[CApos].NetreachData[CAVec[0].NGlimitStr].To_OID.size()); i++)
					{
						temp->CA.NetreachData[CAVec[0].NGlimitStr].From_OID.push_back(CAVec[CApos].NetreachData[CAVec[0].NGlimitStr].From_OID[i]);
						temp->CA.NetreachData[CAVec[0].NGlimitStr].To_OID.push_back(CAVec[CApos].NetreachData[CAVec[0].NGlimitStr].To_OID[i]);
						temp->CA.NetreachData[CAVec[0].NGlimitStr].Radius.push_back(CAVec[CApos].NetreachData[CAVec[0].NGlimitStr].Radius[i]);
						temp->CA.NetreachData[CAVec[0].NGlimitStr].DirChg.push_back(CAVec[CApos].NetreachData[CAVec[0].NGlimitStr].DirChg[i]);
						temp->CA.NetreachData[CAVec[0].NGlimitStr].JncLmt.push_back(CAVec[CApos].NetreachData[CAVec[0].NGlimitStr].JncLmt[i]);
						temp->CA.NetreachData[CAVec[0].NGlimitStr].Relen.push_back(CAVec[CApos].NetreachData[CAVec[0].NGlimitStr].Relen[i]);
						temp->CA.NetreachData[CAVec[0].NGlimitStr].Jnc.push_back(CAVec[CApos].NetreachData[CAVec[0].NGlimitStr].Jnc[i]);
						temp->CA.NetreachData[CAVec[0].NGlimitStr].Wgt.push_back(CAVec[CApos].NetreachData[CAVec[0].NGlimitStr].Wgt[i]);
					}
				}

				if (CAVec[0].isGeo)
				{
					//增加
					if (CAVec[0].isOneToOne)
					{
						temp->CA.routeLen_all.insert(CAVec[CApos].routeLen_all.begin(), CAVec[CApos].routeLen_all.end());
						temp->CA.routeRoad_allMap.insert(CAVec[CApos].routeRoad_allMap.begin(), CAVec[CApos].routeRoad_allMap.end());
					}

					if (CAVec[0].isOneToOne == true && CAVec[0].outputPath == false)
					{
						//空
					}
					else
					{
						temp->CA.routeRoad_all.insert(CAVec[CApos].routeRoad_all.begin(), CAVec[CApos].routeRoad_all.end());
						temp->CA.GeoDataCount.insert(CAVec[CApos].GeoDataCount.begin(), CAVec[CApos].GeoDataCount.end());

						if (CAVec[0].isOneToOne == false)
						{
							for (int i = 0; i < int(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].To_OID.size()); i++)
							{
								temp->CA.GeodesicsData[CAVec[0].NGlimitStr].From_OID.push_back(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].From_OID[i]);
								temp->CA.GeodesicsData[CAVec[0].NGlimitStr].To_OID.push_back(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].To_OID[i]);
								temp->CA.GeodesicsData[CAVec[0].NGlimitStr].PathLen.push_back(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].PathLen[i]);
								temp->CA.GeodesicsData[CAVec[0].NGlimitStr].DC.push_back(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].DC[i]);
								temp->CA.GeodesicsData[CAVec[0].NGlimitStr].DD.push_back(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].DD[i]);
								temp->CA.GeodesicsData[CAVec[0].NGlimitStr].DDL.push_back(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].DDL[i]);
								temp->CA.GeodesicsData[CAVec[0].NGlimitStr].WDD.push_back(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].WDD[i]);
								temp->CA.GeodesicsData[CAVec[0].NGlimitStr].Jnc.push_back(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].Jnc[i]);
								temp->CA.GeodesicsData[CAVec[0].NGlimitStr].Wgt.push_back(CAVec[CApos].GeodesicsData[CAVec[0].NGlimitStr].Wgt[i]);
							}
						}
					}
				}
			}

			//当计算csv最短路径需要输出csv文件，否则do nothing
			output_shp_data(temp);

			//绘图
			if (CAVec[0].isNet && CAVec[0].isOneToOne == false) {
				temp->CA.weight.insert(CAVec[0].weight.begin(), CAVec[0].weight.end());
				//可视化
				temp->addNetMapsAll(temp->CA, temp->FA);
			}
			else if (CAVec[0].isGeo && CAVec[0].isOneToOne == false) {
				//可视化
				temp->addGeoMapsAll(temp->CA, temp->FA);
			}
			//告知结束
			temp->set_calculate_over(true);
			temp->set_run_start(false);
			//触发重绘显示
			temp->updateGLWindows(true, false);

			process_str = "Run Over, "+ time_qstr;
			//更新进度条
			emit(temp->pDisplaydow->ui->Button_start_scan->clicked());

			//logOut << GetNowTime() << ", " << "output_NetGeo_Data Finished.\n";

			weightAttributesSet.clear();
			return;
		}
	}
}

void MainWindow::output_shp_data(MainWindow* temp)
{
	////输出shp文件
	//if (temp->CA.isOneToOne == false)
	//{
	//	//输出存储数据到shp文件
	//	temp->CA.OutputVisualData(FIAVec[0]);
	//}

	//保存csv文件
	if (temp->CA.isOneToOne)
	{
		std::ofstream out;
		std::string incsvfilename = temp->CA.incsvfilename;
		std::string outcsvfilename = incsvfilename.substr(0, incsvfilename.length() - 4);
		if (temp->CA.isMR)
		{
			outcsvfilename += "_MR.csv";
		}
		if (temp->CA.isDR)
		{
			outcsvfilename += "_DR.csv";
		}
		if (temp->CA.isJncR)
		{
			outcsvfilename += "_JnR.csv";
		}

		out.open(outcsvfilename);

		//第一行
		out << "origin,destination,distance,path\n";

		for (int i = 0; i< int(temp->CA.FromIDVec.size()); i++){
			int startedge = temp->CA.FromIDVec[i];
			int endedge = temp->CA.ToIDVec[i];

			if (temp->CA.outputPath){
				out << startedge << "," << endedge << "," << temp->CA.routeLen_all[std::pair<int, int>(startedge, endedge)] << ",\"";
				int allCount = int(temp->CA.routeRoad_allMap[std::pair<int, int>(startedge, endedge)].size());
				int newCount = 0;
				for (auto iter = temp->CA.routeRoad_allMap[std::pair<int, int>(startedge, endedge)].begin();
					iter != temp->CA.routeRoad_allMap[std::pair<int, int>(startedge, endedge)].end(); iter++){
					++newCount;
					if (*iter == startedge || *iter == endedge)
						continue;
					else{
						if (newCount < allCount - 1)
							out << *iter << ",";
						else
							out << *iter;
					}
				}
				out << "\"\n";
			}
			else{
				out << startedge << "," << endedge << "," << temp->CA.routeLen_all[std::pair<int, int>(startedge, endedge)] << std::endl;
			}
		}

		out.close();

		//计算pass字段，并刷新视图和属性栏
		std::string new_column_name = "PathCount";
		map<int, double> PassCountMp;
		for (int i = 0; i < temp->global_map.RecordCount; i++)
			PassCountMp[i] = 0;
		for (int i = 0; i< int(temp->CA.FromIDVec.size()); i++) {
			int startedge = temp->CA.FromIDVec[i];
			int endedge = temp->CA.ToIDVec[i];

			++PassCountMp[startedge];
			++PassCountMp[endedge];
			for (auto iter = temp->CA.routeRoad_allMap[std::pair<int, int>(startedge, endedge)].begin();
				iter != temp->CA.routeRoad_allMap[std::pair<int, int>(startedge, endedge)].end(); iter++) {
				//经历过的道路计数加1
				++PassCountMp[*iter];
			}
		}
		if (PassCountMp.size() == 0)
			return;

		//数据同步到shp文件
		temp->CA.outputPathCount(PassCountMp, new_column_name);

		//更新属性栏
		temp->CA.TempModifyData.clear();
		temp->CA.TempModifyData[new_column_name] = PassCountMp;
		temp->updateAttributes(temp->CA);
	}
}

void MainWindow::setProcessPos() {
	this->m_pConnectProBar->setValue(process_pos);
	this->locationLabel->setText(process_str);
	//this->work->ui.lineEdit->setText(QString::number(thread_num));
}

void MainWindow::outputCsvFiles(Calculation &CA, ShapeFileAccessor &FA) {
	std::ofstream out;
	if (this->work->ui.radioButton_33->isChecked()) {	//单组子集线
		std::string filename = "single.csv";
		std::string filepath = this->fileDir + filename;

		//写文件	
		out.open(filepath);
		//out << "ID,meanMD,meanDD,meanDDL,meanJncD\n";
		out << "ID";
		if (subset_para.isMR) out << ",meanMD";
		if (subset_para.isDR) out << ",meanDD,meanDDL";
		if (subset_para.isJnR) out << ",meanJncD";
		out << "\n";

		for (auto it = CA.results.result_1.mean_mr.begin(); it != CA.results.result_1.mean_mr.end(); it++) {
			int road_id = it->first;
			//out << road_id << "," << CA.results.result_1.mean_mr[road_id] << "," << CA.results.result_1.dd[road_id] << ","
				//<< CA.results.result_1.ddl[road_id] << "," << CA.results.result_1.mean_jnr[road_id] << "\n";
			out << road_id;
			if (subset_para.isMR) out << "," << CA.results.result_1.mean_mr[road_id];
			if (subset_para.isDR) out << "," << CA.results.result_1.dd[road_id] << "," << CA.results.result_1.ddl[road_id];
			if (subset_para.isJnR) out << "," << CA.results.result_1.mean_jnr[road_id];
			out << "\n";
		}

		out.close();
	}
	else if (this->work->ui.radioButton_34->isChecked()) {	//双组子集线
		std::string filename = "multiple.csv";
		std::string filepath = this->fileDir + filename;

		//写文件
		out.open(filepath);
		//out << "group,ID,aver_MD,aver_DD,aver_DDL,aver_JncD,min_MD,min_DD,min_DDL,min_JncD\n";
		out << "group,ID";
		if (subset_para.isMR) out << ",aver_MD,min_MD";
		if (subset_para.isDR) out << ",aver_DD,min_DD,aver_DDL,min_DDL";
		if (subset_para.isJnR) out << ",aver_JncD,minJncD";
		out << "\n";
		for (auto it = CA.results.result_2.aver_mean_mr.begin(); it != CA.results.result_2.aver_mean_mr.end(); it++) {
			std::pair<int,int> group = it->first;
			for (auto iter = CA.results.result_2.aver_mean_mr[group].begin(); iter != CA.results.result_2.aver_mean_mr[group].end(); iter++) {
				int road_id = iter->first;

				/*out << "(" << group.first << "->" << group.second << ")," << road_id << "," << CA.results.result_2.aver_mean_mr[group][road_id] << "," << CA.results.result_2.aver_dd[group][road_id] << ","
					<< CA.results.result_2.aver_ddl[group][road_id] << "," << CA.results.result_2.aver_mean_jnr[group][road_id] << ","
					<< CA.results.result_2.min_mr[group][road_id] << "," << CA.results.result_2.min_dd[group][road_id] << ","
					<< CA.results.result_2.min_ddl[group][road_id] << "," << CA.results.result_2.min_jnr[group][road_id] << "\n";*/
				out << "(" << group.first << "->" << group.second << ")," << road_id;
				if (subset_para.isMR) 
					out << "," << CA.results.result_2.aver_mean_mr[group][road_id] << "," << CA.results.result_2.min_mr[group][road_id];
				if (subset_para.isDR)
					out << "," << CA.results.result_2.aver_dd[group][road_id] << "," << CA.results.result_2.min_dd[group][road_id] << ","
					<< CA.results.result_2.aver_ddl[group][road_id] << "," << CA.results.result_2.min_ddl[group][road_id];
				if (subset_para.isJnR)
					out << CA.results.result_2.aver_mean_jnr[group][road_id] << "," << CA.results.result_2.min_jnr[group][road_id];
				out << "\n";
			}
		}

		out.close();
	}
	else if (this->work->ui.radioButton_35->isChecked()) {	//整体子集线
		std::string filename = "whole.csv";
		std::string filepath = this->fileDir + filename;

		//写文件
		out.open(filepath);
		//out << "meanMD,meanDD,meanDDL,meanJncD\n";
		if (subset_para.isMR) out << ",meanMD";
		if (subset_para.isDR) out << ",meanDD,meanDDL";
		if (subset_para.isJnR) out << ",meanJncD";
		out << "\n";
		
		//out << CA.results.result_3.meanMD << "," << CA.results.result_3.meanDD << "," << CA.results.result_3.meanDDL << "," << CA.results.result_3.meanJnCD << "\n";
		if (subset_para.isMR) out << "," << CA.results.result_3.meanMD;
		if (subset_para.isDR) out << "," << CA.results.result_3.meanDD << "," << CA.results.result_3.meanDDL;
		if (subset_para.isJnR) out << "," << CA.results.result_3.meanJnCD;
		out << "\n";

		/*for (auto it = FA.roadID.begin(); it != FA.roadID.end(); it++) {
			int road_id = *it;
			out << road_id << "," << CA.results.result_3.mean_mr[road_id] << "," << CA.results.result_3.dd[road_id] << ","
				<< CA.results.result_3.ddl[road_id] << "," << CA.results.result_3.mean_jnr[road_id] << "\n";
		}*/

		out.close();
	}
}

void MainWindow::subsetProgressCount() {
	//计时开始
	T_timeBegin = clock();

	process_pos = 0;
	process_str = "calculating";

	//更新进度条
	emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	double pos = 0;
	double sum = 0;
	while (this->m_pConnectProBar->value() < this->nMax) {
		Sleep(50);	//没必要频繁查询占用CPU算力

		if (get_NeedStop()) {
			set_run_start(false);
			set_needOver(false);
			set_calculate_over(true);

			process_str = "stopped";

			//更新进度条
			emit(this->pDisplaydow->ui->Button_start_scan->clicked());

			return;
		}

		if (get_needOver()) {
			set_run_start(false);
			set_needOver(false);
			set_calculate_over(true);

			process_str = "param error";

			//更新进度条
			emit(this->pDisplaydow->ui->Button_start_scan->clicked());

			return;
		}

		//if (this->work->ui.radioButton_33->isChecked() || this->work->ui.radioButton_34->isChecked()) {	//暂时开放单组/双组子集线动态更新进度条
		//	double rate = this->CA.getFinishedRate();
		//	process_pos = int(rate * this->nMax);
		//}
		double rate = this->CA.getFinishedRate();
		process_pos = int(rate * this->nMax);

		if (this->CA.IsSubsetsAllFinished()) {
			process_pos = this->nMax;
		}

		//更新进度条
		emit(this->pDisplaydow->ui->Button_start_scan->clicked());
	}

	//计时结束
	T_timeEnd = clock();
	int endtime = (double)(T_timeEnd - T_timeBegin) / double(CLOCKS_PER_SEC);
	std::string str = std::to_string(endtime) + "s";
	time_qstr = QString(QString::fromLocal8Bit(str.c_str()));

	//set_run_start(false);
	set_needOver(false);
	set_calculate_over(true);

	//QMessageBox::information(NULL, "Run Time", str.c_str());

	process_pos = this->nMax;
	process_str = "calculate over: " + QString(QString::fromLocal8Bit(str.c_str()));

	//更新进度条
	emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//输出summary显示
	this->summarySubset(CA, FA);

	//输出csv文件
	this->outputCsvFiles(CA, FA);

	//全部关闭
	this->work->ui.radioButton_11->setChecked(false);
	this->work->ui.radioButton_13->setChecked(false);
	this->work->ui.radioButton_14->setChecked(false);
	this->work->ui.radioButton_15->setChecked(false);

	//开启绘图
	if (this->work->ui.radioButton_33->isChecked()) {	//单组子集线
		std::vector<std::string> items;
		this->work->ui.radioButton_10->setChecked(true);
		this->work->ui.widget_152->setVisible(true);

		//打开图层
		if (subset_para.isMR) {
			this->work->ui.radioButton_11->setChecked(true);
			items = { "MR" };
		}else if (subset_para.isDR) {
			this->work->ui.radioButton_13->setChecked(true);
			items = { "DR" };
		}
		else if (subset_para.isJnR) {
			this->work->ui.radioButton_15->setChecked(true);
			items = { "JnR" };
		}
		else if (subset_para.isMDR) {
			this->work->ui.radioButton_14->setChecked(true);
			items = { "MDR" };
		}

		reDrawSubsetNetReach(items);
	}
	else if (this->work->ui.radioButton_34->isChecked()) {	//双组子集线
		std::vector<std::string> items;
		this->work->ui.radioButton_10->setChecked(true);
		this->work->ui.widget_152->setVisible(true);

		//打开图层
		if (subset_para.isMR) {
			this->work->ui.radioButton_11->setChecked(true);
			items = { "MR" };
		}
		else if (subset_para.isDR) {
			this->work->ui.radioButton_13->setChecked(true);
			items = { "DR" };
		}
		else if (subset_para.isJnR) {
			this->work->ui.radioButton_15->setChecked(true);
			items = { "JnR" };
		}

		reDrawSubsetGeoReach(items);
	}
	
}

void MainWindow::progressCount()
{
	if (mutex_process_count.try_lock() == false)
		return;

	//logOut << GetNowTime() << ", " << "progressCount Satrt.\n";

	//计时开始
	T_timeBegin = clock();

	process_pos = 0;
	process_str = "calculating";

	//更新进度条
	emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//输入角度数目
	int angle_count = 1;
	QString qstrAngleThreshold;
	//查看要计算的功能模块
	std::string which = whichOpen();
	int index = this->String_to_Index[which];
	if (index == 3) qstrAngleThreshold = this->work->ui.lineEdit_15->text();
	else if (index == 7) qstrAngleThreshold = this->work->ui.lineEdit_44->text();
	else if(index == 21) qstrAngleThreshold = this->work->ui.lineEdit_18->text();
	
	if (qstrAngleThreshold.size() > 0) {
		std::string txAngleThreshold = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
		std::stringstream ss(txAngleThreshold);
		std::string str;
		angle_count = 0;
		while (getline(ss, txAngleThreshold, ',')) {
			++angle_count;
		}
	}

	double pos = 0;
	double sum = 0;
	std::map<int, int> isLastSearched;
	bool judgeNeedStop = false;
	bool judgeNeedOver = false;
	while (this->m_pConnectProBar->value() < this->nMax)
	{
		Sleep(50);	//没必要频繁查询占用CPU算力

		if (get_NeedStop()) {
			set_run_start(false);
			set_needOver(false);
			set_calculate_over(true);

			process_str = "stopped";

			//更新进度条
			emit(this->pDisplaydow->ui->Button_start_scan->clicked());

			mutex_process_count.unlock();
			return;
		}

		if (get_needOver()) {
			set_run_start(false);
			set_needOver(false);
			set_calculate_over(false);

			process_str = "param error";

			//更新进度条
			emit(this->pDisplaydow->ui->Button_start_scan->clicked());

			mutex_process_count.unlock();
			return;
		}

		/*if (calculate_over) {
			pos = this->nMax;
		}*/

		if (angle_count <= 1)
			sum = 0;
		else {
			if (sum >= double(this->FA.roadID.size()))
				sum = double(this->FA.roadID.size())*(int(sum) / this->FA.roadID.size());
			else
				sum = 0;
		}
		
		if (this->work->ui.radioButton_3->isChecked()) {
			if (int(CAVec.size() > 0)) {
				mutex_t.lock();
				for (int i = 0; i < int(CAVec.size()); i++){
					sum += CAVec[i].finishedCount;
				}
				if (angle_count > 1) {
					//对finished_angle_count加锁
					mutex_fc.lock();
					if (sum > finished_angle_count *this->FA.roadID.size() && sum < (finished_angle_count + 1)*this->FA.roadID.size())
						isLastSearched[finished_angle_count + 1] = 1;
					//判断是否是当前轮次刚刚结束但下一轮次尚未开始(目前处于数据输出阶段)
					if (finished_angle_count > 0 && isLastSearched[finished_angle_count+1] && sum >= (finished_angle_count+1) * this->FA.roadID.size())
						sum = max(finished_angle_count + 1, 1) * this->FA.roadID.size();
					//判断是否是当前轮次刚刚结束但下一轮次还处于构造初始数据阶段
					else if (finished_angle_count > 0 && isLastSearched[finished_angle_count + 1]==0 && sum >= (finished_angle_count + 1) * this->FA.roadID.size())
						sum = finished_angle_count * this->FA.roadID.size();
					//判断是不是刚结束一轮而新的一轮还没有开始
					else if (int(sum)%this->FA.roadID.size()==0 && int(sum) / this->FA.roadID.size()> finished_angle_count) {
						sum = max(finished_angle_count + 1, 1) * this->FA.roadID.size();
					}
					mutex_fc.unlock();
				}
			
				if (!CAVec[0].isNet && !CAVec[0].isGeo && CAVec[0].isDR) {
					pos = int(sum / double(angle_count*this->FA.roadID.size())*double((this->nMax - this->nMin))) + this->nMin;
				}
				else if (!CAVec[0].isNet && !CAVec[0].isGeo && (CAVec[0].isMR || CAVec[0].isJncR || CAVec[0].isJncDDL || CAVec[0].isJncD)) {
					pos = int(sum / double(this->FA.roadID.size())*double((this->nMax - this->nMin))) + this->nMin;
				}
				else if (CAVec[0].isNet || CAVec[0].isGeo) {
					if (this->CA.FromIDVec.size() == 0)
						pos = this->nMax;
					else if (CAVec[0].isGeo) {
						pos = int(sum / double(this->CAVec[0].fromIDSet.size())*double((this->nMax - this->nMin))) + this->nMin;
					}else
						pos = int(sum / double(this->CAVec[0].FromIDVec.size())*double((this->nMax - this->nMin))) + this->nMin;
				}
				mutex_t.unlock();
			}
		}
		else {
			sum += CA.finishedCount;
			if (angle_count > 1) {
				if (sum > (angle_count - 1)*this->FA.roadID.size() && sum < angle_count*this->FA.roadID.size())
					isLastSearched[finished_angle_count + 1] = 1;
				//判断是否是最后一轮结束了
				if (isLastSearched[finished_angle_count + 1] && sum == angle_count * this->FA.roadID.size());
				//判断是不是刚结束一轮而新的一轮还没有开始
				else if (sum == (finished_angle_count + 1)*this->FA.roadID.size()) {
					sum = max(finished_angle_count, 1) * this->FA.roadID.size();
				}
			}

			if (!CA.isNet && !CA.isGeo && CA.isDR) {
				pos = int(sum / double(angle_count*this->FA.roadID.size())*double((this->nMax - this->nMin))) + this->nMin;
			}
			else if (!CA.isNet && !CA.isGeo && (CA.isMR || CA.isJncR || CA.isJncDDL || CA.isJncD)) {
				pos = int(sum / double(this->FA.roadID.size())*double((this->nMax - this->nMin))) + this->nMin;
			}
			else if (CA.isNet || CA.isGeo) {
				if (this->CA.FromIDVec.size() == 0)
					pos = this->nMax;
				else
					pos = int(sum / double(this->CA.FromIDVec.size())*double((this->nMax - this->nMin))) + this->nMin;
			}
		}
		process_pos = pos;

		//更新进度条
		emit(this->pDisplaydow->ui->Button_start_scan->clicked());

		process_str = "Finished Count:" + QString::number(int(sum));

		//更新label
		emit(this->pDisplaydow->ui->Button_start_scan->clicked());
	}

	//计时结束
	T_timeEnd = clock();
	int endtime = (double)(T_timeEnd - T_timeBegin) / double(CLOCKS_PER_SEC);
	std::string str = std::to_string(endtime) + "s";
	time_qstr = QString(QString::fromLocal8Bit(str.c_str()));
	//QMessageBox::information(NULL, "Run Time", str.c_str());

	process_pos = this->nMax;
	process_str = "calculate over: " + QString(QString::fromLocal8Bit(str.c_str()));

	//更新进度条
	emit(this->pDisplaydow->ui->Button_start_scan->clicked());

	//logOut << GetNowTime() << ", " << "progressCount Over.\n";

	//set_run_start(false);
	set_needOver(false);
	set_calculate_over(true);

	mutex_process_count.unlock();
	return;
}


void MainWindow::progressOpenFile()
{
	this->m_pConnectProBar->setValue(0);

	while (true) {
		if (this->loadFilePathOver) {
			this->loadFilePathOver = false;
			break;
		}
	}

	std::string shpfilename= std::string((const char *)(this->shpFileName).toLocal8Bit()); 
	std::string dbFilePath = shpfilename.substr(0, shpfilename.length() - 4) + ".dbf";
	DBFHandle	hDBF;
	hDBF = DBFOpen(dbFilePath.c_str(), "rb+");
	int roadsAllCount = DBFGetRecordCount(hDBF);

	while (this->m_pConnectProBar->value() < this->nMax) {
		int pos = int(loadedCount / double(roadsAllCount)*double((this->nMax - this->nMin))) + this->nMin;
		this->m_pConnectProBar->setValue(pos);
	}
}

void MainWindow::timeCount()
{
	while (true)
	{
		bool isAllFinished = true;
		for (int i = 0; i < thread_num; i++)
		{
			if (!CAVec[i].isFinished)
				isAllFinished = false;
		}
		if (isAllFinished)
		{
			//计时结束
			T_timeEnd = clock();
			double endtime = (double)(T_timeEnd - T_timeBegin) / double(CLOCKS_PER_SEC);
			std::string str = "Run time is " + std::to_string(endtime) + "s.";
			QMessageBox::information(NULL, "Run Time", str.c_str());

			return;
		}
	}

}

void MainWindow::start_Multi_thread_for_NetGeo(MainWindow* temp)
{
	//将线程跟CPU逻辑核心进行绑定，不允许出现线程切换
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpu_num = std::min((int)std::thread::hardware_concurrency(), (int)(si.dwNumberOfProcessors));
	long long cpu_pos = static_cast<long long>(pow(2, cpu_num - 1));

	//按顺序分组，因此起点路id为最小的
	Calculation::stepDepthStartRoad = std::to_string(CAVec[0].subFromIDVec[0]);
	//按边顺序依次启动
	for (int i = 0; i < thread_num; i++)
	{
		std::thread thrd(&Calculation::MultiVisualize, &CAVec[i], std::ref(temp->FA), cpu_pos);
		thrd.detach();
		cpu_pos = cpu_pos >> 1;
		if (cpu_pos == 0)
		{
			cpu_pos = static_cast<long long>(pow(2, cpu_num - 1));
		}
	}

	//存储数据+画图
	std::thread thrd1(output_NetGeo_Data, temp);
	thrd1.detach();
	SetThreadAffinityMask(GetCurrentThread(), 1);

	
}

void MainWindow::calculateMD_Multi_thread() {
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_MD();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_calculate(this);
}

void MainWindow::calculateMR_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false); 
	mutex_t.unlock();
	bool isok = init_for_MR(); //创建MR的参数，FA、CA组
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true); // 【计算中】
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_calculate(this);
}

void MainWindow::calculateDR_Multi_thread()
{
	//先看角度数目，判断跑几次多线程
	QString qstrAngleThreshold = this->work->ui.lineEdit_15->text();
	std::string txAngleThresholdAll = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	std::stringstream ss(txAngleThresholdAll);
	std::string txAngleThreshold;
	last_finished_angle_count = 0;
	bool isFirstStart = true;
	while (getline(ss, txAngleThreshold, ',')) {
		if (isFirstStart)
			isFirstStart = false;
		else {
			while (true) {  //只有上一轮多线程跑完了才拉起下一轮的多线程
				Sleep(1000);
				if (finished_angle_count == last_finished_angle_count + 1)
					break;
			}
		}
		mutex_t.lock();
		//排除上一次主计算数据的干扰
		CA.clearOldData();
		//清空老数据
		std::vector<Calculation>().swap(CAVec);
		set_calculate_over(false);
		mutex_t.unlock();

		//初始化
		bool isok = init_for_DR(txAngleThreshold);
		if (!isok) {
			set_needOver(true);
			return;
		}

		set_run_start(true);
		if (get_NeedStop()) {
			return;
		}

		//启动多线程函数
		last_finished_angle_count = finished_angle_count;
		start_Multi_thread_for_calculate_DD(this);
	}
}

void MainWindow::calculateJnDDL_Multi_thread() {
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_JnDDL();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_calculate(this);
}

void MainWindow::calculateDDL_Multi_thread() {
	//先看角度数目，判断跑几次多线程
	QString qstrAngleThreshold = this->work->ui.lineEdit_18->text();
	std::string txAngleThresholdAll = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	std::stringstream ss(txAngleThresholdAll);
	std::string txAngleThreshold;
	last_finished_angle_count = 0;
	bool isFirstStart = true;
	while (getline(ss, txAngleThreshold, ',')) {
		if (isFirstStart)
			isFirstStart = false;
		else {
			while (true) {  //只有上一轮多线程跑完了才拉起下一轮的多线程
				Sleep(1000);
				if (finished_angle_count == last_finished_angle_count + 1)
					break;
			}
		}
		mutex_t.lock();
		//排除上一次主计算数据的干扰
		CA.clearOldData();
		//清空老数据
		std::vector<Calculation>().swap(CAVec);
		set_calculate_over(false);
		mutex_t.unlock();

		//初始化
		bool isok = init_for_DDL(txAngleThreshold);
		if (!isok) {
			set_needOver(true);
			return;
		}

		set_run_start(true);
		if (get_NeedStop()) {
			return;
		}

		//启动多线程函数
		last_finished_angle_count = finished_angle_count;
		start_Multi_thread_for_calculate_DD(this);

		////启动进度条
		//std::thread thrd2(&MainWindow::progressCount, this);
		//thrd2.detach();
	}
}

void MainWindow::calculateMDR_Multi_thread()
{
	//先看角度数目，判断跑几次多线程
	QString qstrAngleThreshold = this->work->ui.lineEdit_44->text();
	std::string txAngleThresholdAll = std::string((const char *)qstrAngleThreshold.toLocal8Bit());
	std::stringstream ss(txAngleThresholdAll);
	std::string txAngleThreshold;
	last_finished_angle_count = 0;
	bool isFirstStart = true;
	while (getline(ss, txAngleThreshold, ',')) {
		if (isFirstStart)
			isFirstStart = false;
		else {
			while (true) {  //只有上一轮多线程跑完了才拉起下一轮的多线程
				Sleep(1000);
				std::string str_process_str = std::string((const char *)process_str.toLocal8Bit());
				if (finished_angle_count == last_finished_angle_count + 1)
					break;
			}
		}
		mutex_t.lock();
		//排除上一次主计算数据的干扰
		CA.clearOldData();
		//清空老数据
		std::vector<Calculation>().swap(CAVec);
		set_calculate_over(false);
		mutex_t.unlock();

		//初始化
		bool isok = init_for_MDR(txAngleThreshold);
		if (!isok) {
			set_needOver(true);
			return;
		}

		set_run_start(true);
		if (get_NeedStop()) {
			return;
		}

		//启动多线程函数
		last_finished_angle_count = finished_angle_count;
		start_Multi_thread_for_calculate_DD(this);

	}
	
}


void MainWindow::calculateJnR_Multi_thread()
{
	mutex_t.lock();
	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);
	set_calculate_over(false);
	mutex_t.unlock();
	//初始化
	bool isok = init_for_JnR();
	if (!isok) {
		set_needOver(true);
		return;
	}

	set_run_start(true);
	if (get_NeedStop()) {
		return;
	}

	//启动多线程函数
	start_Multi_thread_for_calculate_DD(this);
}

void MainWindow::actionEvent(QActionEvent * event)
{
	int id;
	if (id = event->action()->data().toInt())
	{
		int k = id;

	}
}

bool MainWindow::eventFilter(QObject *object, QEvent *e)
{
	if (object == this && e->type() == (QEvent::Type)FOCUSGRAPH)
	{
		OnFocusGraph((QGraphDoc*)((QmyEvent*)e)->wparam, ((QmyEvent*)e)->lparam);
		return true;
	}
	return QObject::eventFilter(object, e);
}

void MainWindow::adaptiveWidth() {
	//获取属性列名的最长字符数目
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();

	//把所有属性字段全部扒下来，注意：字段长度超过10个字符的放弃掉
	int max_size = 0;
	for (int i = 0; i < tab.getNumColumns(); i++) {
		max_size = max(max_size, int(tab.getColumnName(i).size()));
	}

	//判断是否需要扩展属性栏宽度
	if (max_size <= 10)
		return;

	//根据字符数自适应扩展属性栏宽度
	int const_width = 130;
	int min_width = max(const_width, int(0.8*double(max_size) / 10.0*130.0));
	int max_width = max(const_width, int(1.1*double(max_size) / 10.0*130.0));
	//AttributesListDock->setMinimumWidth(min_width);
	//AttributesListDock->setMaximumWidth(max_width);
	
}

MainWindow::MainWindow(const QString &fileToLoad, Settings &settings) : mSettings(settings)
{
	m_loc = std::locale::global(std::locale(""));
	m_treeDoc = NULL;
	generateIndex();

	//创建可视化容器图框，居中放置
	mdiArea = new QMdiArea;
	setCentralWidget(mdiArea);
	connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(updateActiveWindows()));

	//创建信号翻译与转换器
	windowMapper = new QSignalMapper(this);
	connect(windowMapper, SIGNAL(mapped(QWidget *)), this, SLOT(setActiveSubWindow(QWidget *)));

	//创建Index目录树，靠左放置
	m_indexWidget = new IndexWidget(this);
	QDockWidget *indexDock = new QDockWidget(tr("Index"), this);
	indexDock->setObjectName(QLatin1String("IndexWindow"));
	indexDock->setWidget(m_indexWidget);
	//addDockWidget(Qt::LeftDockWidgetArea, indexDock);

	//创建Attributes目录树，靠左放置
	AttributesListDock = new QDockWidget(tr("AttributesList"), this);
	AttributesListDock->setObjectName(QLatin1String("AttributesListWindow"));
	AttributesListDock->setWidget(setupAttributesListWidget());
	
	QWidget* lTitleBar_2 = AttributesListDock->titleBarWidget();
	QWidget* lEmptyWidget_2 = new QWidget();
	AttributesListDock->setTitleBarWidget(lEmptyWidget_2);	//去除标题栏
	//AttributesListDock->setFloating(true);	//设置悬浮
	//获取屏幕分辨率
	int nWidth = GetSystemMetrics(SM_CXSCREEN);
	int nHeight = GetSystemMetrics(SM_CYSCREEN);
	int const_width = 130;
	int init_height = nHeight/3;
	//AttributesListDock->setMinimumWidth(const_width);
	//AttributesListDock->setMaximumWidth(const_width);
	//AttributesListDock->setMinimumHeight(nHeight / 3);
	//AttributesListDock->setMaximumHeight(nHeight - 10);
	

	//放置位置
	addDockWidget(Qt::RightDockWidgetArea, AttributesListDock);
	//AttributesListDock->move(nWidth- 1.5*const_width, 50);
	this->AttributesListDock->setVisible(false);	//隐藏

	//创建计算模块
	pDisplaydow = new CalculateUI(this);
	QDockWidget *CalculationDock = new QDockWidget(tr("Calculation"), this);
	CalculationDock->setObjectName(QLatin1String("CalculationWindow"));
	CalculationDock->setWidget(pDisplaydow);
	addDockWidget(Qt::LeftDockWidgetArea, CalculationDock);

	indexDock->setVisible(false);
	//AttributesListDock->setVisible(false);
	CalculationDock->setVisible(false);

	//创建侧边栏
	work = new UrbanConnectAnalyzer(this);
	QDockWidget *WorkDock = new QDockWidget(tr(""), this);
	//WorkDock->setObjectName(QLatin1String("Index"));
	WorkDock->setWidget(work);
	QWidget* lTitleBar = WorkDock->titleBarWidget();
	QWidget* lEmptyWidget = new QWidget();
	WorkDock->setTitleBarWidget(lEmptyWidget);	//去除标题栏
	WorkDock->setMinimumWidth(300);
	////滚动区域
	//QScrollArea* m_ScrollArea = new QScrollArea(WorkDock);
	////垂直滚动条不可见，只能通过鼠标滑动
	//m_ScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	////设置滚动区域的窗体
	//m_ScrollArea->setWidget(WorkDock);
	addDockWidget(Qt::LeftDockWidgetArea, WorkDock);

	//读取设置信息
	readSettings(); // read setting or generate default
	setWindowTitle(TITLE_BASE);

	////准备好动作响应函数，创建菜单栏、工具栏、状态栏，并更新位置信息
	createActions();
	//createMenus();
	createToolBars();
	fileToolBar->setVisible(false);
	createStatusBar();
	//updateToolbar();
	updateActiveWindows();
	updateGLWindows(true, true);

	installEventFilter(this);	//安装事件过滤器obj到这个对象

	//判断是否需要加载文件
	if (fileToLoad.length() > 0)
	{
		loadFile(fileToLoad);
	}

	//visual edit
	QColor color(0, 255, 0);
	this->work->ui.colorPalette_2->setColor(color);

	//添加FILE
	connect(work->ui.pushButton_8, SIGNAL(clicked()), this, SLOT(OnFileOpen()));
	//connect(work->ui.pushButton_11, SIGNAL(clicked()), this, SLOT(OnFileExport()));
	connect(work->ui.pushButton_14, SIGNAL(clicked()), this, SLOT(OnExportNetGeo())); //export analysis
	connect(work->ui.pushButton, SIGNAL(clicked()), this, SLOT(OnEditSave())); //export screen
	connect(work->ui.pushButton_18, SIGNAL(clicked()), this, SLOT(newScaleWin()));
	connect(work->ui.pushButton_30, SIGNAL(clicked()), this, SLOT(newSelectWin()));

	//subset
	connect(work->ui.radioButton_33, SIGNAL(clicked(bool)), this, SLOT(NewSubsetWin(bool)));
	connect(work->ui.radioButton_34, SIGNAL(clicked(bool)), this, SLOT(NewSubsetWin(bool)));
	connect(work->ui.radioButton_35, SIGNAL(clicked(bool)), this, SLOT(NewSubsetWin(bool)));
	connect(work->ui.pushButton_25, SIGNAL(clicked()), this, SLOT(NewSetFormWin()));

	//weight参数选择
	connect(work->ui.pushButton_26, SIGNAL(clicked()), this, SLOT(selectWeights()));
	connect(work->ui.pushButton_28, SIGNAL(clicked()), this, SLOT(selectWeights()));
	connect(work->ui.pushButton_27, SIGNAL(clicked()), this, SLOT(selectWeights()));
	connect(work->ui.pushButton_29, SIGNAL(clicked()), this, SLOT(selectWeights()));

	//隐藏
	work->ui.widget_75->setVisible(false);

	//添加Run
	connect(work->ui.pushButton_9, SIGNAL(clicked()), this, SLOT(Run()));

	//进度条
	connect(pDisplaydow->ui->Button_start_scan, SIGNAL(clicked()), this, SLOT(setProcessPos()));

	//检查输入
	connect(pDisplaydow->ui->Button_start_scan_3, SIGNAL(clicked()), this, SLOT(InputError()));

	//导入csv文件
	connect(work->ui.pushButton_6, SIGNAL(clicked()), this, SLOT(OnCSVFileOpen()));

	//图层
	connect(pDisplaydow->ui->Button_start_scan_4, SIGNAL(clicked()), this, SLOT(NetReachOpen()));
	connect(pDisplaydow->ui->Button_start_scan_2, SIGNAL(clicked()), this, SLOT(GeodesicsOPen()));
	connect(pDisplaydow->ui->Button_start_scan_11, SIGNAL(clicked()), this, SLOT(NetReachClose()));
	connect(pDisplaydow->ui->Button_start_scan_10, SIGNAL(clicked()), this, SLOT(GeodesicsClose()));

	//layers
	connect(work->ui.radioButton_5, SIGNAL(clicked(bool)), this, SLOT(reDrawNet(bool)));
	connect(work->ui.radioButton_6, SIGNAL(clicked(bool)), this, SLOT(reDrawGeo(bool)));

	//Visual Edit
	connect(work->ui.colorBackground, SIGNAL(colorChanged(QColor)), this, SLOT(setBackground(QColor)));
	connect(work->ui.colorPalette_2, SIGNAL(colorChanged(QColor)), this, SLOT(ChangeColor(QColor)));
	connect(work->ui.pushButton_22, SIGNAL(clicked()), this, SLOT(newHighlightWin()));
	connect(work->ui.pushButton_20, SIGNAL(clicked()), this, SLOT(ResetVisual()));

	//创建工具栏
	QDockWidget *TooolDock = new QDockWidget(tr("Tool"), this);
	TooolDock->setObjectName(QLatin1String("Tool"));
	TooolDock->setWidget(editToolBar);
	//QWidget* TitleBar2 = TooolDock->titleBarWidget();
	//QWidget* EmptyWidget2 = new QWidget();
	//TooolDock->setTitleBarWidget(EmptyWidget2);	//去除标题栏
	TooolDock->setFloating(true);
	//TooolDock->move(nWidth - 400, 50);
	TooolDock->setVisible(false);

	//工具栏按钮
	connect(work->ui.pushButton_10, SIGNAL(clicked()), this, SLOT(select_button()));
	connect(work->ui.pushButton_15, SIGNAL(clicked()), this, SLOT(drag_button()));
	connect(work->ui.pushButton_16, SIGNAL(clicked()), this, SLOT(zoomIn_button()));
	connect(work->ui.pushButton_17, SIGNAL(clicked()), this, SLOT(zoonOut_button()));
	connect(work->ui.pushButton_4, SIGNAL(clicked()), this, SLOT(maxView_button()));
	connect(work->ui.pushButton_3, SIGNAL(clicked()), this, SLOT(OninvertColor()));
	connect(work->ui.pushButton_21, SIGNAL(clicked()), this, SLOT(colorRange_button()));

	//属性栏
	connect(work->ui.radioButton_31, SIGNAL(clicked()), this, SLOT(showAttributes()));
	connect(work->ui.pushButton_2, SIGNAL(clicked()), this, SLOT(saveAttributes()));
	connect(pDisplaydow->ui->Button_start_scan_5, SIGNAL(clicked()), this, SLOT(selectAttributes()));

	//信息栏
	connect(pDisplaydow->ui->Button_start_scan_8, SIGNAL(clicked()), this, SLOT(showSummaryInfo()));

	//from id、to id
	connect(work->ui.radioButton_4, SIGNAL(clicked(bool)), this, SLOT(PushFromID(bool)));
	connect(work->ui.radioButton_7, SIGNAL(clicked(bool)), this, SLOT(PushToID(bool)));

	//stop
	connect(work->ui.pushButton_24, SIGNAL(clicked()), this, SLOT(setStop()));

	//进度条
	work->ui.radioButton_29->setChecked(true);

	//subset
	connect(work->ui.radioButton_11, SIGNAL(clicked()), this, SLOT(DrawSubsetReach()));
	connect(work->ui.radioButton_13, SIGNAL(clicked()), this, SLOT(DrawSubsetReach()));
	connect(work->ui.radioButton_14, SIGNAL(clicked()), this, SLOT(DrawSubsetReach()));
	connect(work->ui.radioButton_15, SIGNAL(clicked()), this, SLOT(DrawSubsetReach()));

	//恢复颜色栏
	QColor newcolor(0, 0, 255);
	this->work->ui.colorPalette_2->setColor(newcolor);

	//关闭MR Distance模块显示
	this->work->ui.widget_79->setVisible(false);
	this->work->ui.widget_80->setVisible(false);

	//屏蔽指定模块
	//this->work->ui.widget_35->setVisible(false);
	//this->work->ui.widget_35->setEnabled(false);
	//this->work->ui.widget_39->setVisible(false);
	//this->work->ui.widget_39->setEnabled(false);
}

MainWindow::~MainWindow() {
	std::locale::global(m_loc);//恢复全局locale
}

void MainWindow::showAttributes() {
	this->AttributesListDock->setVisible(this->work->ui.radioButton_31->isChecked());
	//mdiArea->setWindowState(Qt::WindowMaximized);
}

void MainWindow::newSelectWin() {
	if (!isOpen) return;

	if (this->selectWin)
		this->selectWin->close();

	this->selectWin = new Select;

	std::vector<std::string> AttributesNames;
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	for (int i = 0; i < tab.getNumColumns(); i++) {
		std::string fieldname = tab.getColumnName(i);
		AttributesNames.push_back(fieldname);
	}
	this->selectWin->update_win(AttributesNames);

	//设置初始参数
	if (comboBox_idx_select < this->selectWin->ui.comboBox->maxVisibleItems()) {
		//this->highlight_window->ui.comboBox->setCurrentText(comboBox_value);
		this->selectWin->ui.comboBox->setCurrentIndex(comboBox_idx_select);
	}

	connect(this->selectWin->ui.okButton, SIGNAL(clicked()), this, SLOT(selectLinesByAttribute()));
	connect(this->selectWin->ui.cancelButton, SIGNAL(clicked()), this, SLOT(CloseSelectWin()));
	this->selectWin->show();
}


void MainWindow::selectLinesByAttribute() {
	QString qstr = this->selectWin->ui.comboBox->currentText();//获取文本文档
	std::string searchidStr = std::string((const char *)qstr.toLocal8Bit());

	vector<int> select_lines;
	std::string err_info = this->CA.getSearchIDByAttribute(this->FA, searchidStr, select_lines);
	if (err_info.length() == 0) {
		Ref_number_list.clear();
		Ref_number_list.insert(Ref_number_list.begin(), select_lines.begin(), select_lines.end());
		ResetVisual();
		//修改颜色
		for (int road_id : Ref_number_list) {
			FixedColoredLinesMap[road_id] = getColor(255, 255, 0);
			FixedThickLinesMap[2.0].insert(road_id);
		}
		//触发重绘显示
		this->updateGLWindows(true, false);
	}
	else {
		QString info = QString(QString::fromLocal8Bit(err_info.c_str()));
		QMessageBox::information(NULL, "Tips", info);
	}

	this->selectWin->close();
}

void MainWindow::CloseSelectWin() {
	this->selectWin->close();
}

void MainWindow::newScaleWin() {
	if (!isOpen) return;

	if (this->scaleWin)
		this->scaleWin->close();

	this->scaleWin = new ScaleWin;
	this->scaleWin->setWindowFlags(this->scaleWin->windowFlags() | Qt::WindowStaysOnTopHint);
	connect(this->scaleWin->ui.okButton, SIGNAL(clicked()), this, SLOT(ChangeScale()));
	connect(this->scaleWin->ui.cancelButton, SIGNAL(clicked()), this, SLOT(CloseScaleWin()));
	this->scaleWin->show();
}

void MainWindow::ChangeScale() {
	extern double init_scale;
	extern double init_screenRatio;

	std::string top_value = std::string((const char *)(this->scaleWin->ui.comboBox_2->currentText()).toLocal8Bit());
	double scale_value = stof(top_value);

	double ratio = init_screenRatio * (scale_value / init_scale);
	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	for (int i = 0; i < windows.size(); ++i) {
		GLView *child = qobject_cast<GLView*>(windows.at(i)->widget());
		if (!child) continue;
		//先恢复视图
		maxView_button();
		//再缩放视图
		child->setScale(ratio);
	}

	this->scaleWin->close();
}

void MainWindow::CloseScaleWin() {
	this->scaleWin->close();
}

//subset
void MainWindow::NewSubsetWin(bool checked) {
	if (!isOpen) return;
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p) return;

	//关闭旧窗口
	if (this->subsetWin)
		this->subsetWin->close();
	if (this->setFormWin)
		this->setFormWin->close();

	this->summary_info = filename_qstr;
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	if (mean_angle > 0) {
		mean_angle_qstr = QString::number(mean_angle, 10, 1);
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	}
	this->displaySummary();
	this->SubsetIDs.clear();		//清空老数据
	cancelSubsetNetReach();
	cancelSubsetGeoReach();
	ResetVisual();					//取消线条可视化
	if (work->ui.radioButton_35->isChecked()) {	
		//修改subset图层名称
		this->work->ui.radioButton_11->setVisible(false);
		this->work->ui.radioButton_13->setVisible(false);
		this->work->ui.radioButton_15->setVisible(false);
		this->work->ui.radioButton_14->setVisible(false);

		//打开参数设置窗口
		emit(this->work->ui.pushButton_25->clicked());

		return;
	}

	if (checked == false) return;

	this->subsetWin = new Subset;
	this->subsetWin->setWindowFlags(this->subsetWin->windowFlags() | Qt::WindowStaysOnTopHint);

	//获取屏幕分辨率，SubsetWin的固定尺寸=431x374
	int nWidth = GetSystemMetrics(SM_CXSCREEN);
	int nHeight = GetSystemMetrics(SM_CYSCREEN);
	int titleHeight = frameGeometry().height() - geometry().height();
	int pos_x = nWidth - 431;
	int pos_y = titleHeight;
	this->subsetWin->move(pos_x, pos_y);

	//默认开启group_1按钮
	this->subsetWin->ui.radioButton->setChecked(true);
	this->subsetWin->setWindowTitle("Multiple Subsets");
	this->subsetWin->ui.okButton->setText("Next");

	//对于单子集分析，关闭group_2按钮
	if (this->work->ui.radioButton_33->isChecked()) {	//单组子集线
		this->subsetWin->ui.radioButton_2->setVisible(false);
		this->subsetWin->setWindowTitle("Single Subset");

		//修改subset图层名称
		this->work->ui.radioButton_11->setVisible(true);
		this->work->ui.radioButton_13->setVisible(true);
		this->work->ui.radioButton_15->setVisible(true);
		this->work->ui.radioButton_14->setVisible(true);

		this->work->ui.radioButton_11->setText("MR");
		this->work->ui.radioButton_13->setText("DR");
		this->work->ui.radioButton_15->setText("JnR");
		this->work->ui.radioButton_14->setText("MDR");
		
	}
	else if (this->work->ui.radioButton_34->isChecked()) {	//多组子集线
		//修改subset图层名称
		this->work->ui.radioButton_11->setVisible(true);
		this->work->ui.radioButton_13->setVisible(true);
		this->work->ui.radioButton_15->setVisible(true);
		this->work->ui.radioButton_14->setVisible(false);

		this->work->ui.radioButton_11->setText("Shortest Metric Path");
		this->work->ui.radioButton_13->setText("Shortest Directional Path");
		this->work->ui.radioButton_15->setText("Shortest Junctional Path");
	}
	

	//绑定响应函数
	connect(this->subsetWin->ui.pushButton, SIGNAL(clicked()), this, SLOT(SelectSubsetByMouse()));		//over按钮触发
	connect(this->subsetWin->ui.okButton, SIGNAL(clicked()), this, SLOT(SelectSubsetByInfo()));			//默认info筛选规则优于鼠标勾选的规则

	this->subsetWin->setWindowFlags(Qt::WindowStaysOnTopHint);		//保持窗口始终位于界面最前面
	this->subsetWin->show();
}

void MainWindow::NewSetFormWin() {
	if (!isOpen) return;
	if (this->work->ui.radioButton_33->isChecked()==false && this->work->ui.radioButton_34->isChecked() == false &&
		this->work->ui.radioButton_35->isChecked() == false) {
		//QMessageBox::information(NULL, "Tips", "Please select SUBSET MOUDLE first");
		return;
	}
	if ((this->work->ui.radioButton_33->isChecked() == true || this->work->ui.radioButton_34->isChecked() == true) && this->SubsetIDs.size()==0) {
		QMessageBox::information(NULL, "Tips", "Subset lines have not been selected, please check");
		return;
	}

	this->setFormWin = new setForm;
	this->setFormWin->setWindowFlags(this->setFormWin->windowFlags() | Qt::WindowStaysOnTopHint);

	//绑定响应函数
	connect(this->setFormWin->ui.radioButton_1, SIGNAL(clicked(bool)), this, SLOT(setMRPara(bool)));
	connect(this->setFormWin->ui.okButton, SIGNAL(clicked()), this, SLOT(CalculateSubset()));	//计算分析

	if (this->work->ui.radioButton_33->isChecked() == false) {	//整体子集线和多组子集线
		this->setFormWin->ui.widget_49->setVisible(false);
		this->setFormWin->ui.widget_52->setVisible(false);
		this->setFormWin->ui.widget_56->setVisible(false);
		this->setFormWin->ui.widget_58->setVisible(false);
		this->setFormWin->ui.widget_59->setVisible(false);
	}
	else {	//单组子集线
		if (this->setFormWin->ui.radioButton_1->isChecked())
			this->setFormWin->ui.widget_49->setVisible(true);
		this->setFormWin->ui.widget_52->setVisible(true);
		this->setFormWin->ui.widget_56->setVisible(true);
		this->setFormWin->ui.widget_58->setVisible(true);
		if (this->setFormWin->ui.radioButton_4->isChecked())
			this->setFormWin->ui.widget_59->setVisible(true);
	}

	this->setFormWin->show();
}

//只有在单组子集线才响应
void MainWindow::setMRPara(bool checked) {
	if (this->work->ui.radioButton_33->isChecked()) {
		this->setFormWin->ui.widget_49->setVisible(checked);
	}
}

void MainWindow::CalculateSubset() {
	//关闭参数窗口
	this->setFormWin->close();

	if (!CheckOpenFile()) {
		return;
	}

	set_NeedStop(false);
	set_needOver(false);
	set_calculate_over(false);

	if (get_run_start()) {
		this->FA.BaseInputError("Calculating, please try again later");
		return;
	}
	
	process_pos = 0;
	process_str = "calculating";
	this->m_pConnectProBar->setValue(process_pos);
	this->locationLabel->setText(process_str);

	//检查参数
	bool isok = this->CheckSetForm();
	if (!isok) {
		set_needOver(true);
		//QMessageBox::warning(this, tr("Warning"), tr("Parameter input error, please check."),
		//	QMessageBox::Ok, QMessageBox::Ok);

		set_NeedStop(true);
		set_needOver(true);
		set_calculate_over(true);

		return;
	}

	//检查FA是否初始化
	this->FA.ProcessShapeFile();

	//排除上一次主计算数据的干扰
	CA.clearOldData();

	//初始化参数
	std::string txRadiiThreshold2 = "";
	std::string txAngleThreshold = std::to_string(subset_para.DR_Para.angle_limit);		//角度阈值
	std::string txDirectionalChanges = "0";
	std::string txNewJnc = std::to_string(subset_para.JnR_Para.jnc_degree);				//交叉口阈值
	std::string txWgtLimitStr = "";

	//CA初始化
	Graph *g = FA.ProcessShapeFile();
	CA.init_Net_DR_para(FA, *g, infilepath, txRadiiThreshold2, txAngleThreshold, txDirectionalChanges, txNewJnc, weightAttributesSet);

	//防止进度条跑穿
	CA.resetFinishedState();

	//计算分析：暂不考虑可视化
	if (this->work->ui.radioButton_33->isChecked()) {	//单子集线分析
		this->CA.calculate_subset_1(this->FA, subset_para, this->SubsetIDs[1]);
	}
	else if (this->work->ui.radioButton_34->isChecked()) {	//多子集线分析
		this->CA.calculate_subset_2(this->FA, subset_para, this->SubsetIDs);
	}
	else if (this->work->ui.radioButton_35->isChecked()) {	//整体子集线分析
		this->CA.calculate_subset_3(this->FA, subset_para);
	}
	else {
		QMessageBox::warning(this, tr("Warning"), tr("Please select the subset lines."),
			QMessageBox::Ok, QMessageBox::Ok);

		process_pos = 0;
		process_str = "no subset lines selected";
		this->m_pConnectProBar->setValue(process_pos);
		this->locationLabel->setText(process_str);

		set_calculate_over(true);
		return;
	}

	//增开线程等待计算结束，并同步计算进度
	std::thread thrd(&MainWindow::subsetProgressCount, this);
	thrd.detach();
}



bool MainWindow::CheckSetForm() {
	//清空subset_para
	subsetPara tmp_para;
	subset_para = tmp_para;

	this->isInputValid = false;

	//可选计算开关
	subset_para.isMR = this->setFormWin->ui.radioButton_1->isChecked();
	subset_para.isDR = this->setFormWin->ui.radioButton_2->isChecked();
	subset_para.isJnR = this->setFormWin->ui.radioButton_3->isChecked();
	subset_para.isMDR = this->setFormWin->ui.radioButton_4->isChecked();

	if (!subset_para.isMR && !subset_para.isDR && !subset_para.isJnR && !subset_para.isMDR) {
		QMessageBox::warning(NULL, "Input Error", "Please select at least one item for calculation!!");
		return false;
	}

	//MR
	QString qstr_mr_1 = this->setFormWin->ui.lineEdit->text();
	std::string str_mr_1 = std::string((const char *)qstr_mr_1.toLocal8Bit());
	if (this->work->ui.radioButton_33->isChecked() && subset_para.isMR && this->work->ui.radioButton_35->isChecked()==false && std::regex_match(str_mr_1, oneFloatNumber) == false) {
		QMessageBox::warning(NULL, "Input Error", "Metric Radius of Metric Reach input error!!");
		return false;
	}

	//DR
	QString qstr_dr_1 = this->setFormWin->ui.lineEdit_2->text();
	std::string str_dr_1 = std::string((const char *)qstr_dr_1.toLocal8Bit());
	if (this->work->ui.radioButton_33->isChecked() && subset_para.isDR && this->work->ui.radioButton_35->isChecked() == false && std::regex_match(str_dr_1, oneIntNumber) == false) {
		QMessageBox::warning(NULL, "Input Error", "Directional Changes of Directional Reach input error!!");
		return false;
	}
	QString qstr_dr_2 = this->setFormWin->ui.lineEdit_3->text();
	std::string str_dr_2 = std::string((const char *)qstr_dr_2.toLocal8Bit());
	if (subset_para.isDR && std::regex_match(str_dr_2, oneFloatNumber) == false) {
		QMessageBox::warning(NULL, "Input Error", "Angle Threshold of Directional Reach input error!!");
		return false;
	}

	//JnR
	QString qstr_jnr_1 = this->setFormWin->ui.lineEdit_4->text();
	std::string str_jnr_1 = std::string((const char *)qstr_jnr_1.toLocal8Bit());
	if (this->work->ui.radioButton_33->isChecked() && subset_para.isJnR && this->work->ui.radioButton_35->isChecked() == false && std::regex_match(str_jnr_1, oneIntNumber) == false) {
		QMessageBox::warning(NULL, "Input Error", "Junctions of Junction Reach input error!!");
		return false;
	}
	QString qstr_jnr_2 = this->setFormWin->ui.lineEdit_5->text();
	std::string str_jnr_2 = std::string((const char *)qstr_jnr_2.toLocal8Bit());
	if (subset_para.isJnR && std::regex_match(str_jnr_2, oneIntNumber) == false) {
		QMessageBox::warning(NULL, "Input Error", "Degree Threshold of Junction Reach input error!!");
		return false;
	}

	//MDR
	QString qstr_mdr_1 = this->setFormWin->ui.lineEdit_8->text();
	std::string str_mdr_1 = std::string((const char *)qstr_mdr_1.toLocal8Bit());
	if (this->work->ui.radioButton_33->isChecked() && subset_para.isMDR && this->work->ui.radioButton_35->isChecked() == false && std::regex_match(str_mdr_1, oneFloatNumber) == false) {
		QMessageBox::warning(NULL, "Input Error", "Metric Radius of Combined Reach input error!!");
		return false;
	}
	QString qstr_mdr_2 = this->setFormWin->ui.lineEdit_6->text();
	std::string str_mdr_2 = std::string((const char *)qstr_mdr_2.toLocal8Bit());
	if (this->work->ui.radioButton_33->isChecked() && subset_para.isMDR && this->work->ui.radioButton_35->isChecked() == false && std::regex_match(str_mdr_2, oneIntNumber) == false) {
		QMessageBox::warning(NULL, "Input Error", "Directional Changes of Combined Reach input error!!");
		return false;
	}
	QString qstr_mdr_3 = this->setFormWin->ui.lineEdit_7->text();
	std::string str_mdr_3 = std::string((const char *)qstr_mdr_3.toLocal8Bit());
	if (this->work->ui.radioButton_33->isChecked() && subset_para.isMDR && this->work->ui.radioButton_35->isChecked() == false && std::regex_match(str_mdr_3, oneFloatNumber) == false) {
		QMessageBox::warning(NULL, "Input Error", "Angle Threshold of Combined Reach input error!!");
		return false;
	}

	//获取参数
	if (this->work->ui.radioButton_33->isChecked()) {	//单组子集线
		if (subset_para.isMR) {
			mr_MRLimit = std::stof(str_mr_1);

			subset_para.MR_Para.mr_limit = mr_MRLimit;
		}
		if (subset_para.isDR) {
			dr_Angle = std::stof(str_dr_2);
			dr_DRLimit = std::stof(str_dr_1);

			subset_para.DR_Para.dc_limit = dr_DRLimit, subset_para.DR_Para.angle_limit = dr_Angle;
		}
		if (subset_para.isJnR) {
			jnr_JnRLimit = std::stof(str_jnr_1);
			jnr_Degree = std::stof(str_jnr_2);

			subset_para.JnR_Para.jnc_limit = jnr_JnRLimit, subset_para.JnR_Para.jnc_degree = jnr_Degree;
		}
		if (subset_para.isMDR) {
			mdr_MRLimit = std::stof(str_mdr_1);
			mdr_DRLimit = std::stof(str_mdr_2);
			mdr_Angle = std::stof(str_mdr_3);

			subset_para.MDR_Para.mr_limit = mdr_MRLimit, subset_para.MDR_Para.dc_limit = mdr_DRLimit, subset_para.MDR_Para.angle_limit = mdr_Angle;
		}
	}
	else if (this->work->ui.radioButton_34->isChecked() || this->work->ui.radioButton_35->isChecked()) {		//双组子集线/整体子集线
		if (subset_para.isDR) {
			dr_Angle = std::stof(str_dr_2);
			subset_para.DR_Para.angle_limit = dr_Angle;
		}
		if (subset_para.isJnR) {
			jnr_Degree = std::stof(str_jnr_2);
			subset_para.JnR_Para.jnc_degree = jnr_Degree;
		}
	}
	
	//检查角度是否一致
	if (subset_para.isDR && subset_para.isMDR && dr_Angle != mdr_Angle) {
		QMessageBox::warning(NULL, "Input Error", "Angle Threshold of Combined Reach must be the same as Angle Threshold of Directional Reach!!");
		return false;
	}

	//设置状态栏
	std::string new_name = this->infilepath;
	subset_para.FileDirPath = "";
	subset_para.FileName = new_name;
	while (new_name.find("/") != std::string::npos) {
		int pos = int(new_name.find("/"));
		new_name = new_name.substr(pos + 1, new_name.length());
	}
	subset_para.FileName = new_name;
	subset_para.FileDirPath = this->infilepath.substr(0, this->infilepath.length() - new_name.length() - 1);

	isInputValid = true;
	return true;
}

//找出第idx个">="、"<="、"="等的左边起始位置的前一个位置
int findLimitPos(std::string input_str, int idx, std::string direction) {
	//可能的字符串：<=，>=，>，<，=
	std::map<int, std::string> pos_to_limit;
	if (input_str.find("<=") != std::string::npos && pos_to_limit.size() == 0)	//若存在
		pos_to_limit[input_str.find("<=")] = "<=";
	if (input_str.find(">=") != std::string::npos && pos_to_limit.size() == 0)
		pos_to_limit[input_str.find(">=")] = ">=";
	if (input_str.find("<") != std::string::npos && pos_to_limit.size() == 0)
		pos_to_limit[input_str.find("<")] = "<";
	if (input_str.find(">") != std::string::npos && pos_to_limit.size() == 0)
		pos_to_limit[input_str.find(">")] = ">";
	if (input_str.find("=") != std::string::npos && pos_to_limit.size() == 0)
		pos_to_limit[input_str.find("=")] = "=";

	//计算截断位置
	auto it = pos_to_limit.begin();
	int cut_pos = it->first + it->second.size();
	input_str = input_str.substr(cut_pos, input_str.length());
	if (idx == 1 || input_str.size() == 0) {
		if(direction == "left")
			return it->first - 1;
		else 
			return it->first + it->second.size();
	}
		
	if (input_str.find("<=") != std::string::npos && pos_to_limit.size() < 2)	//若存在
		pos_to_limit[cut_pos + input_str.find("<=")] = "<=";
	if (input_str.find(">=") != std::string::npos && pos_to_limit.size() < 2)
		pos_to_limit[cut_pos + input_str.find(">=")] = ">=";
	if (input_str.find("<") != std::string::npos && pos_to_limit.size() < 2)
		pos_to_limit[cut_pos + input_str.find("<")] = "<";
	if (input_str.find(">") != std::string::npos && pos_to_limit.size() < 2)
		pos_to_limit[cut_pos + input_str.find(">")] = ">";
	if (input_str.find("=") != std::string::npos && pos_to_limit.size() < 2)
		pos_to_limit[cut_pos + input_str.find("=")] = "=";

	auto iter = pos_to_limit.rbegin();
	if (idx == 2) {
		if (direction == "left")
			return iter->first - 1;
		else
			return iter->first + it->second.size();
	}
		
}

bool MainWindow::getWeightDataByName(std::string WgtLimitStr, std::map<int, double> &data) {
	//先判断属性数据中是否具有WgtLimitStr
	if (this->global_map.Attributes.AttributesDouble.count(WgtLimitStr) == 0)
		return false;

	//取出对应属性数据
	data.clear();
	data.insert(this->global_map.Attributes.AttributesDouble[WgtLimitStr].begin(), 
		this->global_map.Attributes.AttributesDouble[WgtLimitStr].end());

	return true;
}

inline std::set<int> getParaSet(std::string cstrTest)
{
	std::set<int> result;

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

bool MainWindow::SelectSubsetByInfoSingle(std::string info_str) {
	if (std::regex_match(info_str, subset_str4)) {
	//取出子集序号
	int pos_start = info_str.find("group_") + 6;
	int pos_end = info_str.find("=(");
	int pos_end_2 = info_str.find(")");
	int subset_id = std::stoi(info_str.substr(pos_start, pos_end - pos_start));
	std::string id_strs = info_str.substr(pos_end + 2, pos_end_2 - pos_end - 2);

	//构建subset
	this->SubsetIDs[subset_id] = getParaSet(id_strs);

	return true;
	}
	else if (std::regex_match(info_str, subset_str1)) {			//输入：group_1=(10.4<=seglength<=200)
		//取出子集序号
		int pos_start = info_str.find("group_") + 6;
		int pos_end = info_str.find("=(");
		int subset_id = std::stoi(info_str.substr(pos_start, pos_end - pos_start));
		//取出字段名称
		pos_start = findLimitPos(info_str, 1, "right");
		pos_end = findLimitPos(info_str, 2, "left") + 1;
		std::string field_name = info_str.substr(pos_start, pos_end - pos_start );
		//取出限制条件
		pos_start = findLimitPos(info_str, 1, "left") + 1;
		pos_end = findLimitPos(info_str, 1, "right");
		std::string limit_str_1 = info_str.substr(pos_start, pos_end - pos_start);
		pos_start = findLimitPos(info_str, 2, "left") + 1;
		pos_end = findLimitPos(info_str, 2, "right");
		std::string limit_str_2 = info_str.substr(pos_start, pos_end - pos_start);
		//取出限制参数
		pos_start = info_str.find("=(") + 2;
		pos_end = findLimitPos(info_str, 1, "left") + 1;
		double limit_1 = stof(info_str.substr(pos_start, pos_end - pos_start));
		pos_start = findLimitPos(info_str, 2, "right");
		pos_end = info_str.find(")") + 1;
		double limit_2 = stof(info_str.substr(pos_start, pos_end - pos_start));

		//获取字段数据
		std::map<int, double> weight;
		bool succeed = this->getWeightDataByName(field_name, weight);
		if (!succeed) {
			return false;
		}

		//筛选符合要求的id加入对应的subset
		std::vector<int> validRoads;
		if (limit_str_1.find("<") != std::string::npos) replace(limit_str_1.begin(), limit_str_1.end(), '<', '>');
		else if (limit_str_1.find(">") != std::string::npos) replace(limit_str_1.begin(), limit_str_1.end(), '>', '<');
		this->CA.getLimitID(this->FA, weight, limit_str_1 + std::to_string(limit_1), validRoads);			//根据条件1筛选
		std::map<int, double> new_weight;
		for (int id : validRoads) new_weight[id] = weight[id];
		this->CA.getLimitID(this->FA, new_weight, limit_str_2 + std::to_string(limit_2), validRoads);		//根据条件2筛选

		//构建subset
		this->SubsetIDs[subset_id].insert(validRoads.begin(), validRoads.end());

		return true;
	}
	else if (std::regex_match(info_str, subset_str2)) {		//输入：group_1=(ID>=20)
		//取出子集序号
		int pos_start = info_str.find("group_") + 6;
		int pos_end = info_str.find("=(");
		int subset_id = std::stoi(info_str.substr(pos_start, pos_end - pos_start));
		//取出字段名称
		pos_start = info_str.find("=(") + 2;
		pos_end = findLimitPos(info_str, 2, "left") + 1;
		std::string field_name = info_str.substr(pos_start, pos_end - pos_start);
		//取出限制条件
		pos_start = findLimitPos(info_str, 2, "left") + 1;
		pos_end = findLimitPos(info_str, 2, "right");
		std::string limit_str = info_str.substr(pos_start, pos_end - pos_start);
		//取出限制参数
		pos_start = findLimitPos(info_str, 2, "right");
		pos_end = info_str.find(")");
		double limit = stof(info_str.substr(pos_start, pos_end - pos_start));

		//获取字段数据：应该从属性栏获取数据，不应该从dbf文件获取数据，原因是字段名可能不一致
		std::map<int, double> weight;
		bool succeed = this->getWeightDataByName(field_name, weight);
		if (!succeed) {
			return false;
		}

		//筛选符合要求的id加入对应的subset
		std::vector<int> validRoads;
		this->CA.getLimitID(this->FA, weight, limit_str + std::to_string(limit), validRoads);

		//构建subset
		this->SubsetIDs[subset_id].insert(validRoads.begin(), validRoads.end());

		return true;

	}
	else {
		return false;
	}
}

std::vector<std::string> splitWithStl(const std::string &str, const std::string &pattern)
{
	std::vector<std::string> resVec;

	if ("" == str)
	{
		return resVec;
	}
	//方便截取最后一段数据
	std::string strs = str + pattern;

	size_t pos = strs.find(pattern);
	size_t size = strs.size();

	while (pos != std::string::npos)
	{
		std::string x = strs.substr(0, pos);
		resVec.push_back(x);
		strs = strs.substr(pos + 1, size);
		pos = strs.find(pattern);
	}

	return resVec;
}

/*	格式一：标识每个group如何筛选，支持以下三种方式
		group_1=(seglength>200);
		group_2=(20<=ID<=100);
		group_3=(2,3,4,5,20);
	格式二：直接给出一个在属性列表中标识group_id的字段名
		group
*/
void MainWindow::SelectSubsetByInfo() {
	QString qstr = this->subsetWin->ui.textEdit->toPlainText();
	std::string info_str = std::string((const char *)qstr.toLocal8Bit());

	if (info_str.size() == 0) {
		this->isFirstSelect = true;
		this->subsetWin->close();		//关闭窗口

		//检查子集线是否已经选出
		if (this->SubsetIDs.size() == 0) {
			QMessageBox::information(NULL, "Tips", "Please select lines. You can select by mouse, then click SET, or select by attributes' info.");
			return;
		}
			
		//打开参数设置窗口
		emit(this->work->ui.pushButton_25->clicked());

		return;
	}

	this->SubsetIDs.clear();		//清空老数据

	if (std::regex_match(info_str, subset_str3)) {		//输入：group
		std::map<int, double> weight;
		bool succeed = this->CA.getWeightDataByName(this->infilepath, info_str, weight);
		if (succeed) {
			for (auto it = weight.begin(); it != weight.end(); it++) {
				this->SubsetIDs[int(it->second)].insert(it->first);
			}
		}
		else {
			QMessageBox::information(NULL, "Tips", "The entered field name is invalid!!");
		}
	}
	else if(info_str.size() > 0){		//根据'\n'切分每组筛选条件
		std::vector<std::string> resVec;
		resVec = splitWithStl(info_str, "\n");
		for (std::string sub_str : resVec) {
			bool succeed = this->SelectSubsetByInfoSingle(sub_str);
			if (!succeed) {
				QMessageBox::information(NULL, "Tips", "Input format error!!");
				return;
			}
		}
	}
	this->subsetWin->close();	//关闭窗口

	//打开参数设置窗口
	emit(this->work->ui.pushButton_25->clicked());
}

//通过鼠标选择subsets，但仅仅支持不多于两个子集的选择
void MainWindow::SelectSubsetByMouse() {
	//获取当前GL视图展示的属性名称
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);

	if(this->isFirstSelect)	//第一次选择，清空老数据
		this->SubsetIDs.clear();
	if (this->subsetWin->ui.radioButton->isChecked() && !this->subsetWin->ui.radioButton_2->isChecked()) {	//group_1
		this->isFirstSelect = false;
		//清空旧的选择
		for (auto it = this->SubsetIDs[1].begin(); it != this->SubsetIDs[1].end(); it++) {
			FixedColoredLinesMap[*it] = FixedColoredLinesMapAll[field_name][*it];
			FixedThickLinesMap[4].erase(*it);
		}
		this->SubsetIDs[1].clear();
		//获取鼠标点选或是框选的所有线条
		this->SubsetIDs[1].insert(Ref_number_list.begin(), Ref_number_list.end());
		//为这批线条着色加粗
		for (auto it = this->SubsetIDs[1].begin(); it != this->SubsetIDs[1].end(); it++) {
			FixedColoredLinesMap[*it] = SubsetLinesColor;
			FixedThickLinesMap[4].insert(*it);
		}
		//触发重绘显示
		this->updateGLWindows(true, false);
	}
	else if (!this->subsetWin->ui.radioButton->isChecked() && this->subsetWin->ui.radioButton_2->isChecked()) {	//group_2
		this->isFirstSelect = false;
		//清空旧的选择
		for (auto it = this->SubsetIDs[2].begin(); it != this->SubsetIDs[2].end(); it++) {
			FixedColoredLinesMap[*it] = FixedColoredLinesMapAll[field_name][*it];
			FixedThickLinesMap[4].erase(*it);
		}
		this->SubsetIDs[2].clear();
		//获取鼠标点选或是框选的所有线条
		this->SubsetIDs[2].insert(Ref_number_list.begin(), Ref_number_list.end());
		//为这批线条着色加粗
		for (auto it = this->SubsetIDs[2].begin(); it != this->SubsetIDs[2].end(); it++) {
			FixedColoredLinesMap[*it] = SubsetLinesColor;
			FixedThickLinesMap[4].insert(*it);
		}
		//触发重绘显示
		this->updateGLWindows(true, false);
	}
}

void MainWindow::newHighlightWin() {
	if (!isOpen) return;

	if (this->highlight_window)
		this->highlight_window->close();

	this->highlight_window = new Highlight;
	this->highlight_window->setWindowFlags(this->highlight_window->windowFlags() | Qt::WindowStaysOnTopHint);

	std::vector<std::string> AttributesNames;
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	for (int i = 0; i < tab.getNumColumns(); i++) {
		std::string fieldname = tab.getColumnName(i);
		AttributesNames.push_back(fieldname);
	}
	this->highlight_window->update_win(AttributesNames);

	//设置初始参数
	this->highlight_window->ui.spinBox->setValue(spinBox_value);
	if (comboBox_idx < this->highlight_window->ui.comboBox->maxVisibleItems()) {
		//this->highlight_window->ui.comboBox->setCurrentText(comboBox_value);
		this->highlight_window->ui.comboBox->setCurrentIndex(comboBox_idx);
	}
	this->highlight_window->ui.spinBox_2->setValue(spinBox_2_value);
	this->highlight_window->ui.lineEdit->setText(lineEdit_value);
	if (comboBox_2_idx < this->highlight_window->ui.comboBox_2->maxVisibleItems()) {
		//this->highlight_window->ui.comboBox_2->setCurrentText(comboBox_2_value);
		this->highlight_window->ui.comboBox_2->setCurrentIndex(comboBox_2_idx);
	}

	connect(this->highlight_window->ui.okButton, SIGNAL(clicked()), this, SLOT(ChangeThickness()));
	connect(this->highlight_window->ui.cancelButton, SIGNAL(clicked()), this, SLOT(CloseHighlightWin()));
	this->highlight_window->show();
}

void  MainWindow::CloseHighlightWin() {
	this->highlight_window->close();
}

void MainWindow::selectWeights() {
	if (!isOpen) return;
	QMap<QString, bool> result;

	this->attributesWin = new AttributesWin;
	if (this->global_map.Ref_to_Id.size() == 0)
		return;
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	AttributesVec.clear();
	for (int i = 0; i < tab.getNumColumns(); i++) {
		std::string fieldname = tab.getColumnName(i);
		if (fieldname == "Netreach" || fieldname == "Geodesics" || fieldname == "ID")
			continue;

		AttributesVec.push_back(QString(QString::fromLocal8Bit(fieldname.c_str())));
	}
	this->attributesWin->update_win(AttributesVec);

	connect(this->attributesWin->ui.pushButton, SIGNAL(clicked()), this, SLOT(collectWeights()));

	this->attributesWin->show();
}

//打开窗体
void MainWindow::saveAttributes() {
	if (!isOpen) return;
	QMap<QString, bool> result;

	this->attributesWin = new AttributesWin;
	if (this->global_map.Ref_to_Id.size() == 0)
		return;
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	AttributesVec.clear();
	for (int i = 0; i < tab.getNumColumns(); i++) {
		std::string fieldname = tab.getColumnName(i);
		if (fieldname == "Netreach" || fieldname == "Geodesics" || fieldname == "ID")
			continue;

		AttributesVec.push_back(QString(QString::fromLocal8Bit(fieldname.c_str())));
	}
	this->attributesWin->update_win(AttributesVec);

	connect(this->attributesWin->ui.pushButton, SIGNAL(clicked()), this, SLOT(saveToCSV()));

	this->attributesWin->show();
}

//获得结果
void MainWindow::infoRecv(QString sInfo)
{
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();

	QString saveas;
	QFilePath path(windowFilePath());
	//   saveas = path.m_path + (path.m_name.isEmpty() ? windowTitle() : path.m_name);
	saveas = path.m_path + tr("Attributes.csv");

	QString template_string = tr("CSV file (*.csv)\nTXT file (*.txt)");

	QFileDialog::Options options = 0;
	QString selectedFilter;
	QString outfile = QFileDialog::getSaveFileName(
		0, tr("Save Attributes File to"),
		saveas,
		template_string,
		&selectedFilter,
		options);

	if (outfile.isEmpty())
		return;

	//1234 表示第1 2 3 4个属性被选中了，其他没选中
	QString aaa = sInfo;

	std::vector<int> SelectedAttributesIndex;

	for (int i = 0; i < sInfo.size(); i++) {
		QString tmp = (QString)(sInfo[i]);
		std::string str = std::string((const char *)tmp.toLocal8Bit());
		SelectedAttributesIndex.push_back(int(str[0]));
	}

	//判断是否输出文件
	if (!AttributesVec.empty() && !SelectedAttributesIndex.empty()) {
		//对需要输出的属性列名进行收集
		std::vector<std::string> AttributesSet;

		for (int i = 0; i < SelectedAttributesIndex.size(); i++) {
			std::string str = std::string((const char *)(AttributesVec[SelectedAttributesIndex[i]]).toLocal8Bit());
			AttributesSet.push_back(str);
		}

		//输出文件名
		//std::string outFileName = "Attributes.csv";
		std::string outFileName = std::string((const char *)outfile.toLocal8Bit());
		std::ofstream out;
		out.open(outFileName);

		int pos = 0;
		
		//第一行
		out << "ID";
		for (int i = 0; i<int(AttributesSet.size());i++) {
			out << "," << AttributesSet[i];
		}
		out << std::endl;

		for (auto it= this->global_map.Attributes.AttributesDouble["ID"].begin();it != this->global_map.Attributes.AttributesDouble["ID"].end(); it++) {
			int id = it->second;
			out << id;

			std::string fieldname;
			for (int i = 0; i<int(AttributesSet.size()); i++) {
				fieldname = AttributesSet[i];
				//out << "," << this->global_map.Attributes.AttributesDouble[fieldname][id];
				out << "," << tab.getRow(AttributeKey(id)).getValue(fieldname);
			}
			out << std::endl;
		}

		out.close();
	}
}

void MainWindow::collectWeights() {
	//对需要输出的属性列名进行收集
	weightAttributesSet.clear();
	for (int i = 0; i < this->attributesWin->ui.listWidget->count(); i++) {
		this->attributesWin->ui.listWidget->setCurrentRow(i);
		if (this->attributesWin->ui.listWidget->currentItem()->checkState() == Qt::Checked) {
			QString name = this->attributesWin->ui.listWidget->currentItem()->text();
			weightAttributesSet.push_back(std::string((const char *)name.toLocal8Bit()));
		}
	}
	this->attributesWin->close();
}

/*
根据csv文件的header和dbf中的字段生成完整的attributes
*/
void mergeCsvAttributesName(const std::vector<std::string>& header, AttributesData& attributes){
	std::unordered_map<std::string, std::string> fullNames;
	for (const auto& completeStr : header) {
		//将csv格式的字段转换成attributes格式（去掉下划线）
		std::string temp = completeStr;
		temp.erase(std::remove(temp.begin(), temp.end(), '_'), temp.end());
		std::string truncatedStr = temp.substr(0, 10);
		fullNames[truncatedStr] = temp;
	}

	for (auto query = attributes.AttributesNames.begin(); query != attributes.AttributesNames.end(); query++) {
		auto key = fullNames.find(*query);
		if (key != fullNames.end()) {
			*query = key->second;
		}
	}
}

std::string replace_k_with_thousands(const std::string& input) {
	std::string result;
	size_t pos = 0;

	while (pos < input.size()) {
		size_t k_pos = input.find('k', pos);
		if (k_pos != std::string::npos) {
			// 发现 k，提取前面的数字
			size_t start = k_pos;
			while (start > 0 && std::isdigit(input[start - 1])) {
				--start;
			}
			// 获取数字
			std::string num_str = input.substr(start, k_pos - start);
			int num = std::stoi(num_str);
			result += std::to_string(num * 1000) + " ";
			pos = k_pos + 1; // 更新位置
		}
		else {
			result += input.substr(pos);
			break;
		}
	}

	return result;
}

std::string attributesToCsvNames(std::string input) {
		//替换k
		std::regex pattern(R"((\d+)k)");
		std::string result;
		std::sregex_iterator it(input.begin(), input.end(), pattern);
		std::sregex_iterator end;

		size_t lastPos = 0;
		while (it != end) {
			result += input.substr(lastPos, it->position() - lastPos);
			result += it->str(1) + "000";
			lastPos = it->position() + it->length();
			++it;
		}
		result += input.substr(lastPos);

		// 优先处理不同模式，确保只匹配一种
		// 处理 R_m: m=metric radius
		if (std::regex_match(result, std::regex(R"(R(\d+|n))"))) {
			result = std::regex_replace(result, std::regex(R"(R(\d+|n))"), "R_$1");
		}
		// 处理 mMD_m: m=metric radius
		else if (std::regex_match(result, std::regex(R"(mMD(\d+|n))"))) {
			result = std::regex_replace(result, std::regex(R"(mMD(\d+|n))"), "mMD_$1");
		}
		// 处理 R_da: d=direction changes, a=Angle threshold
		else if (std::regex_match(result, std::regex(R"(R(\d+)d(\d+)a)"))) {
			result = std::regex_replace(result, std::regex(R"(R(\d+)d(\d+)a)"), "R_$1d$2a");
		}
		// 处理 R_da_m
		else if (std::regex_match(result, std::regex(R"(R(\d+)d(\d+)a(\d+|n))"))) {
			result = std::regex_replace(result, std::regex(R"(R(\d+)d(\d+)a(\d+|n))"), "R_$1d$2a_$3");
		}
		// 处理 R_jx: j=junctions, x=degree threshold
		else if (std::regex_match(result, std::regex(R"(R(\d+)j(\d)x)"))) {
			result = std::regex_replace(result, std::regex(R"(R(\d+)j(\d)x)"), "R_$1j$2x");
		}
		// 处理 D_a_m 和 D_x_m
		else if (std::regex_match(result, std::regex(R"(D(L?)(\d+)a(\d+|n))"))) {
			result = std::regex_replace(result, std::regex(R"(D(L?)(\d+)a(\d+|n))"), "D$1_$2a_$3");
		}
		else if (std::regex_match(result, std::regex(R"(D(\d+)x(\d+|n))"))) {
			result = std::regex_replace(result, std::regex(R"(D(\d+)x(\d+|n))"), "D_$1x_$2");
		}
		// 处理 PD_id, PD_a_id, PD_x_id
		else if (std::regex_match(result, std::regex(R"(PD(\d+))"))) {
			result = std::regex_replace(result, std::regex(R"(PD(\d+))"), "PD_$1");
		}
		else if (std::regex_match(result, std::regex(R"(PD(\d+)a(\d+))"))) {
			result = std::regex_replace(result, std::regex(R"(PD(\d+)a(\d+))"), "PD_$1a_$2");
		}
		else if (std::regex_match(result, std::regex(R"(PD(\d+)x(\d+))"))) {
			result = std::regex_replace(result, std::regex(R"(PD(\d+)x(\d+))"), "PD_$1x_$2");
		}

		// 处理后缀 “C_x”
		if (std::regex_match(result, std::regex(R"(.*C(\d+)x)"))) {
			result = std::regex_replace(result, std::regex(R"(C(\d+)x)"), "_C_$1x");
		}
		// 处理后缀 “_W_FieldName”
		if (std::regex_match(result, std::regex(R"(.*W(\w+))"))) {
			result = std::regex_replace(result, std::regex(R"(W(\w+))"), "_W_$1");
		}
		// 替换连续的下划线
		result = std::regex_replace(result, std::regex(R"(_+)"), "_");

		return result;

}
void MainWindow::saveAllAttributesToCSV() {
	while (!get_calculate_over() || get_run_start()) {
		// 当计算完成且未开始运行时，退出循环
	}

	// 覆盖方式重写文件
	QString outFileName=QString::fromLocal8Bit(csvFilePath.c_str());
	QFile file(outFileName);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		std::cerr << "Error opening file for writing: " << outFileName.toStdString() << std::endl;
		return;
	}

	QTextStream out(&file);
	out.setRealNumberPrecision(16);
	out.setAutoDetectUnicode(true); // 根据文件内容自动检测编码

	AttributesData& attributesData = this->global_map.Attributes;
	// 输出文件头，包含所有字段名
	for (size_t i = 0; i < attributesData.AttributesNames.size(); i++) {
		if (i > 0) {
			out << ","; // 逗号分隔
		}
		out << QString::fromLocal8Bit(attributesToCsvNames(attributesData.AttributesNames[i]).c_str()); // 写入字段名
	}
	out << "\n"; // 换行

	// 写入每一行的属性值
	int maxId = 0;
	for (const auto& attr : attributesData.AttributesDouble) {
		maxId = std::max(maxId, static_cast<int>(attr.second.size()));
	}

	for (int id = 0; id < maxId; id++) {
		for (size_t i = 0; i < attributesData.AttributesNames.size(); i++) {
			if (i > 0) {
				out << ","; // 逗号分隔
			}
			// 获取每个属性的值
			const std::string& attrName = attributesData.AttributesNames[i];
			auto it = attributesData.AttributesDouble.find(attrName);
			if (it != attributesData.AttributesDouble.end()) {
				auto valueIt = it->second.find(id);
				if (valueIt != it->second.end()) {
					out << valueIt->second; // 写入数值
				}
				else {
					out << "NaN"; // 如果没有值，写入 NaN
				}
			}
			else {
				out << "NaN"; // 如果属性名不存在，写入 NaN
			}
		}
		out << "\n"; // 换行
	}
	file.close();
}

void MainWindow::saveToCSV() {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();

	QString saveas;
	QFilePath path(windowFilePath());
	//   saveas = path.m_path + (path.m_name.isEmpty() ? windowTitle() : path.m_name);
	saveas = path.m_path + tr("Attributes.csv");

	QString template_string = tr("CSV file (*.csv)\nTXT file (*.txt)");

	QFileDialog::Options options = 0;
	QString selectedFilter;
	QString outfile = QFileDialog::getSaveFileName(
		0, tr("Save Attributes File to"),
		saveas,
		template_string,
		&selectedFilter,
		options);

	if (outfile.isEmpty())
		return;

	process_str = "Exporting, please wait...";
	this->locationLabel->setText(process_str);

	//对需要输出的属性列名进行收集
	std::vector<std::string> AttributesSet;
	for (int i = 0; i < this->attributesWin->ui.listWidget->count(); i++) {
		this->attributesWin->ui.listWidget->setCurrentRow(i);
		if (this->attributesWin->ui.listWidget->currentItem()->checkState()==Qt::Checked) {
			QString name = this->attributesWin->ui.listWidget->currentItem()->text();
			AttributesSet.push_back(std::string((const char *)name.toLocal8Bit()));
		}
	}
	
	if (AttributesSet.size() > 0) {
		//输出文件名
		//std::string outFileName = "Attributes.csv";
		std::string outFileName = std::string((const char *)outfile.toLocal8Bit());
		std::ofstream out;
		out.open(outFileName);

		int pos = 0;

		//第一行
		out << "ID";
		for (int i = 0; i<int(AttributesSet.size()); i++) {
			out << "," << AttributesSet[i];
		}
		out << std::endl;

		for (auto it = this->global_map.Attributes.AttributesDouble["ID"].begin(); it != this->global_map.Attributes.AttributesDouble["ID"].end(); it++) {
			int id = it->second;
			out << id;

			std::string fieldname;
			for (int i = 0; i<int(AttributesSet.size()); i++) {
				fieldname = AttributesSet[i];
				//out << "," << this->global_map.Attributes.AttributesDouble[fieldname][id];
				out << "," << std::fixed<<std::setprecision(4) << tab.getRow(AttributeKey(id)).getValue(fieldname);
			}
			out << std::endl;
		}

		out.close();
	}

	this->attributesWin->close();

	process_str = "Export Over!";
	this->locationLabel->setText(process_str);
}

void MainWindow::displaySummary() {
	emit(this->pDisplaydow->ui->Button_start_scan_8->clicked());
}

void MainWindow::select_button() {
	if (!isOpen) return;
	emit(this->SelectButton->clicked());
}

void MainWindow::drag_button() {
	if (!isOpen) return;
	emit(this->DragButton->clicked());
}

void MainWindow::zoomIn_button() {
	if (!isOpen) return;
	m_selected_mapbar_item = ID_MAPBAR_ITEM_ZOOM_IN;
	zoomInAct->setChecked(true);
	emit(this->zoomToolButton->clicked());
}

void MainWindow::zoonOut_button() {
	if (!isOpen) return;
	m_selected_mapbar_item = ID_MAPBAR_ITEM_ZOOM_OUT;
	zoomOutAct->setChecked(true);
	emit(this->zoomToolButton->clicked());
}

void MainWindow::maxView_button() {
	if (!isOpen) return;
	emit(this->RecentAct->triggered());
}

void MainWindow::colorRange_button() {
	emit(this->zoomToAct->triggered());
}



/********************************************************/
void MainWindow::fun2()
{
	//弹出错误提示
	emit(this->pDisplaydow->ui->Button_start_scan_3->clicked());
}

void MainWindow::InputError()
{
	QMessageBox::warning(NULL, "Input Error", "The input format is incorrect, please check");
}

void MainWindow::GlobalOpen()
{
	global_view = true, netreach_view = false, geodesics_view = false;

	//this->work->ui.radioButton_4->setChecked(true);
	this->work->ui.radioButton_5->setChecked(false);
	this->work->ui.radioButton_6->setChecked(false);
}

void MainWindow::NetReachOpen()
{
	this->work->ui.radioButton_5->setChecked(true);
	this->work->ui.radioButton_6->setChecked(false);
}

void MainWindow::GeodesicsOPen()
{
	this->work->ui.radioButton_6->setChecked(true);
	this->work->ui.radioButton_5->setChecked(false);
}

void MainWindow::NetReachClose()
{
	this->work->ui.radioButton_5->setChecked(false);
	this->summary_info = filename_qstr;
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	if (mean_angle > 0) {
		mean_angle_qstr = QString::number(mean_angle, 10, 1);
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	}
	this->displaySummary();
}

void MainWindow::GeodesicsClose()
{
	this->work->ui.radioButton_6->setChecked(false);
	this->summary_info = filename_qstr;
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	if (mean_angle > 0) {
		mean_angle_qstr = QString::number(mean_angle, 10, 1);
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	}
	this->displaySummary();
}

void MainWindow::ChangeView()
{
	//if (this->work->ui.radioButton_4->isChecked())
	//	this->global_view = true;
	//else
	//	this->global_view = false;

	if (this->work->ui.radioButton_5->isChecked())
		this->netreach_view = true;
	else
		this->netreach_view = false;

	if (this->work->ui.radioButton_6->isChecked())
		this->geodesics_view = true;
	else
		this->geodesics_view = false;

	//不允许出现全部不勾选，否则强制勾选global
	if (this->global_view == false && this->netreach_view == false && this->geodesics_view == false) {
		this->global_view = true;
		//this->work->ui.radioButton_4->setChecked(true);
	}

	//绘图
	drawGraph();
}

void MainWindow::searchId()
{
	//if (!CheckSearchInput()) {
	//	return;
	//}

	//QString qstr_ids = this->work->ui.lineEdit_8->text();
	//std::string str_ids = std::string((const char *)qstr_ids.toLocal8Bit());

	//std::vector<int> ids;
	//getParaSet(str_ids, ids);
	//Ref_number_list.clear();
	//for (auto it = ids.begin(); it != ids.end(); it++) {
	//	Ref_number_list.push_back(this->Id_to_Ref[*it]);
	//}

	////QScreen *screen = QGuiApplication::primaryScreen();
	////QPoint p2 = QCursor::pos(screen);
	////SetCursorPos(700, 700);//设置鼠标位置
	////mouse_event(MOUSEEVENTF_MOVE, 0, 0, 0, GetMessageExtraInfo());
	////SetCursorPos(p2.rx(), p2.ry());

	////reset action
	//OnViewCentreView();

	////show selected lines
	//emit(this->pDisplaydow->ui->Button_start_scan_8->clicked());
}

//修改线条颜色
void MainWindow::ModifyLinesColor() {
	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	for (int i = 0; i < windows.size(); ++i) {
		GLView *child = qobject_cast<GLView*>(windows.at(i)->widget());
		//触发颜色重载
		if (!child) continue;
		child->notifyDatasetChanged();
		child->matchViewToCurrentMetaGraph();
	}
}

//取消所有颜色修改
void MainWindow::CancelModifyLinesColor() {
	//清空Map即可
	FixedColoredLinesMap.clear();

	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	for (int i = 0; i < windows.size(); ++i) {
		GLView *child = qobject_cast<GLView*>(windows.at(i)->widget());
		//触发颜色重载
		if (!child) continue;
		child->notifyDatasetChanged();
		child->matchViewToCurrentMetaGraph();
	}
}

void MainWindow::Run()
{
	std::string str_process_str= std::string((const char *)process_str.toLocal8Bit());
	if (str_process_str.find("please wait") != string::npos) {
		return;
	}

	process_pos = 0;
	process_str = "calculating";
	this->m_pConnectProBar->setValue(process_pos);
	this->locationLabel->setText(process_str);

	set_NeedStop(false); 
	set_needOver(false);
	set_calculate_over(false); //恢复标志位后，此时状态为【未启动】或者【计算中】
	finished_angle_count = 0;

	if (!CheckOpenFile()) {
		return;
	}

	if (get_run_start()) {
		//【计算中】
		this->FA.BaseInputError("Calculating, please try again later");
		return;
	}
	//【未启动】

	//排除上一次主计算数据的干扰
	CA.clearOldData();
	//清空老数据
	std::vector<Calculation>().swap(CAVec);

	time_qstr = "";

	//logOut << std::endl;
	//logOut << GetNowTime() << ", " << "------------------ new run start ------------------" << endl;

	if (this->work->ui.radioButton_5->isChecked())
		this->cancelNetReach();
	else if (this->work->ui.radioButton_6->isChecked())
		this->cancelGeoReach();
	//触发重绘显示
	this->updateGLWindows(true, false);

	//logOut << "############# Create Thread ID = " << thrd1.get_id() << " #######################\n";

	//对与传入生成csv文件单独处理
	if (work->ui.widget_37->isVisible() && work->ui.widget_12->isVisible() && this->work->ui.lineEdit_2->text().size() > 0) {
		this->work->ui.radioButton_3->setChecked(true);

		process_pos = 0;
		process_str = "calculating";
		this->m_pConnectProBar->setValue(process_pos);
		this->locationLabel->setText(process_str);

		//检查FA是否初始化
		this->FA.ProcessShapeFile();
		std::thread thrd1(&MainWindow::run_calculate, this);
		thrd1.detach();

		std::thread thrd2(&MainWindow::progressCount, this);
		thrd2.detach();
		//logOut << "############# Create Thread ID = " << thrd2.get_id() << " #######################\n";

		SetThreadAffinityMask(GetCurrentThread(), 0);

		return;
	}

	if (this->work->ui.radioButton_12->isChecked() || 
		(this->work->ui.widget_37->isVisible() && this->work->ui.lineEdit_2->text().size()==0)) { //单线程:Net|Geo限制不允许勾选多线程
		this->work->ui.radioButton_3->setChecked(false);

		//检查是否有选择线条
		bool isEmpty = false;
		if (this->work->ui.radioButton_12->isChecked()) {	//reach by selection
			if (this->work->ui.lineEdit_56->text().size() == 0 && Ref_number_list.size() == 0)
				isEmpty = true;
		}
		if (isEmpty) {
			process_pos = 0;
			process_str = "no line selected";
			this->m_pConnectProBar->setValue(process_pos);
			this->locationLabel->setText(process_str);

			set_calculate_over(true);
			return;
		}

		process_pos = 0;
		process_str = "calculating";
		this->m_pConnectProBar->setValue(process_pos);
		this->locationLabel->setText(process_str);

		//检查FA是否初始化
		this->FA.ProcessShapeFile();
		run_calculate();

		process_pos = this->nMax;
		process_str = "calculate over";
		if (get_NeedStop()) {
			process_pos = 0;
			process_str = "stopped";
		}	
		this->m_pConnectProBar->setValue(process_pos);
		this->locationLabel->setText(process_str);
		set_run_start(false);
		set_calculate_over(true);
	}
	else {	
		//检查是否有选择线条
		bool isEmpty = false;
		if (this->work->ui.radioButton->isChecked()) {	//distance anlysis
			// OutputDebugString(("Ref_number_list: "+std::to_string(Ref_number_list.size())).c_str());
			if (this->work->ui.lineEdit_110->text().size() == 0 && Ref_number_list.size() == 0)
				isEmpty = true;
		}
		if (isEmpty) {
			process_pos = 0;
			process_str = "no line selected";
			this->m_pConnectProBar->setValue(process_pos);
			this->locationLabel->setText(process_str);

			set_calculate_over(true);
			return;
		}

		//检查是否是计算cav的path分析，是则打开多线程
		if (this->work->ui.widget_37->isVisible() && this->work->ui.lineEdit_2->text().size() > 0)
			this->work->ui.radioButton_3->setChecked(true);

		process_pos = 0;
		process_str = "calculating";
		this->m_pConnectProBar->setValue(process_pos);
		this->locationLabel->setText(process_str);

		//检查FA是否初始化
		this->FA.ProcessShapeFile();
		std::thread thrd1(&MainWindow::run_calculate, this);
		thrd1.detach();

		std::thread thrd2(&MainWindow::progressCount, this);
		thrd2.detach();
		//logOut << "############# Create Thread ID = " << thrd2.get_id() << " #######################\n";

		SetThreadAffinityMask(GetCurrentThread(), 0);
	}

	//logOut << GetNowTime() << ", " << "------------------ new run over ------------------" << endl;
	//新增：计算完毕重新写入csv
	//保存一份csv文件
	std::thread thrd3(&MainWindow::saveAllAttributesToCSV,this);
	//std::thread thrd3(&MainWindow::saveAllAttributesToCSV, this, this->csvFilePath, this->FA.myAttributes);
	thrd3.detach();
	return;
}

void MainWindow::recover() {
	//恢复到全局图
	if (this->work->ui.radioButton_5->isChecked()) {
		this->work->ui.radioButton_5->setChecked(false);
		emit(this->work->ui.radioButton_5->clicked());
	}
	else if (this->work->ui.radioButton_6->isChecked()) {
		this->work->ui.radioButton_6->setChecked(false);
		emit(this->work->ui.radioButton_6->clicked());
	}
}

//增加
void MainWindow::run_calculate()
{
	//查看要计算的功能模块
	std::string which = whichOpen();

	//查看多线程是否开启
	bool openMultiThread = work->ui.radioButton_3->isChecked();
	//bool openMultiThread = false;
	if (!CheckThreadNumberInput()) {
		//强制进度条退出
		set_needOver(true);
		return;
	}

	//分流
	int index = this->String_to_Index[which];
	switch (index) {
	case 1:		//MR_all
		if (!CheckReachMRInput()) {
			set_needOver(true);
			break;
		}
		if(!openMultiThread)
			calculateMR_Single_thread();
		else
			calculateMR_Multi_thread();
		break; 
	case 2:		//MR,相当于Net Reach中的MR
		if (!CheckNetReachMRInput()) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread)
			Net_calculateMR_Single_thread();
		else
			Net_calculateMR_Multi_thread();
		break; 
	case 3:		//DR_all
		if (!CheckReachDRInput()) {
			set_needOver(true);
			break;
		}
		//calculateDR_Single_thread();
		if (!openMultiThread)
			calculateDR_Single_thread();
		else
			calculateDR_Multi_thread();
		break;
	case 4:		//DR
		if (!CheckNetReachDRInput()) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread) 
			Net_calculateDR_Single_thread();
		else
			Net_calculateDR_Multi_thread();
		break;
	case 5:		//JnR_all
		if (!CheckReachJnRInput()) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread)
			calculateJnR_Single_thread();
		else
			calculateJnR_Multi_thread();
		break;
	case 6:		//JnR
		if (!CheckNetReachJnRInput()) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread)
			Net_calculateJnR_Single_thread();
		else
			Net_calculateJnR_Multi_thread();
		break;
	case 7:		//MDR_all
		if (!CheckReachMDRInput()) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread)
			calculateMDR_Single_thread();
		else
			calculateMDR_Multi_thread();
		break;
	case 8:		//MDR
		if (!CheckNetReachMDRInput()) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread)
			Net_calculateMDR_Single_thread();
		else
			Net_calculateMDR_Multi_thread();
		break;
	case 9:		//MD_all
		break;
	case 10:	//MD，即Step Depth
		if (!CheckDistanceModeInput()) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread)
			Net_calculateMD_Single_thread();
		else
			Net_calculateMD_Multi_thread();
		break;
	case 11:	//DD_all
		break;
	case 12:	//PD
		if (!CheckDistanceModeInput() || !CheckDistanceDRInput()) {
			set_needOver(true);
			break;
		}
		if (!CheckDistanceModeInput() || !openMultiThread)
			Net_calculateDD_Single_thread();
		else
			Net_calculateDD_Multi_thread(); 
		break;
	case 13:	//JnD_all
		break;
	case 14:	//JnD
		if (!CheckDistanceModeInput() || !CheckDistanceJnRInput()) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread)
			Net_calculateJnD_Single_thread();
		else
			Net_calculateJnD_Multi_thread();
		break;
	case 15:	//MRPA
		if (!CheckGeodesicsFromIDInput() && !CheckGeodesicsCSVInput() && !CheckGeodesicsToIDInput() ) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread && this->work->ui.lineEdit_2->text().size() == 0)	//只要是计算csv，就使用多线程
			Geo_calculateMR_Single_thread();
		else
			Geo_calculateMR_Multi_thread();
		break;
	case 16:	//DRPA
		if (!CheckGeodesicsDRInput() || !CheckGeodesicsCSVInput() && !CheckGeodesicsFromIDInput() && !CheckGeodesicsToIDInput() ) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread  && this->work->ui.lineEdit_2->text().size() == 0)
			Geo_calculateDR_Single_thread();
		else
			Geo_calculateDR_Multi_thread();
		break;
	case 17:	//JnRPA
		if (!CheckGeodesicsJnRInput() || !CheckGeodesicsCSVInput() &&!CheckGeodesicsFromIDInput() && !CheckGeodesicsToIDInput() ) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread  && this->work->ui.lineEdit_2->text().size() == 0)
			Geo_calculateJnR_Single_thread();
		else
			Geo_calculateJnR_Multi_thread();
		break;
	case 21:
		if (!CheckReachDDLInput()) {
			set_needOver(true);
			break;
		}
		if (!openMultiThread)
			calculateDDL_Single_thread();
		else
			calculateDDL_Multi_thread();
		break;
	case 22:
		if (!CheckReachMDInput()) {
			set_needOver(true);
			break;
		}
		calculateMD_Multi_thread();
		break;
	case 23:
		if (!CheckReachJnDDLInput()) {
			set_needOver(true);
			break;
		}
		calculateJnDDL_Multi_thread();
		break;
	default: 
		set_needOver(true);
		break;
	}

	//logOut << GetNowTime() << ", " << "run_calculate Over.\n";
	return;
}

void MainWindow::generateIndex()
{
	this->String_to_Index.clear();

	//Reach
	String_to_Index["MR_all"] = 1;
	String_to_Index["MR"] = 2;
	String_to_Index["DR_all"] = 3;
	String_to_Index["DR"] = 4;
	String_to_Index["JnR_all"] = 5;
	String_to_Index["JnR"] = 6;
	String_to_Index["MDR_all"] = 7;
	String_to_Index["MDR"] = 8;

	//Distance
	String_to_Index["MD_all"] = 9;
	String_to_Index["MD"] = 10;
	String_to_Index["DD_all"] = 11;
	String_to_Index["DD"] = 12;
	String_to_Index["JnD_all"] = 13;
	String_to_Index["JnD"] = 14;

	//Path Analysis
	String_to_Index["MRPA"] = 15;
	String_to_Index["DRPA"] = 16;
	String_to_Index["JnRPA"] = 17;

	//subsets analysis
	String_to_Index["Subset_1"] = 18;
	String_to_Index["Subset_2"] = 19;
	String_to_Index["Subset_3"] = 20;

	//增加
	String_to_Index["DDL_all"] = 21;
	String_to_Index["MD_all"] = 22;
	String_to_Index["JnDDL_all"] = 23;
}

std::string MainWindow::whichOpen()
{
	//Reach
	if (work->ui.widget_5->isVisible() && work->ui.widget_46->isVisible() && work->ui.radioButton_2->isChecked()) {

		if (work->ui.radioButton_25->isChecked()) {
			return "MR_all";
		}
		if (work->ui.radioButton_26->isChecked()) {
			return "DR_all";
		}
		if (work->ui.radioButton_27->isChecked()) {
			return "JnR_all";
		}
		if (work->ui.radioButton_28->isChecked()) {
			return "MDR_all";
		}
	}

	//Net Reach
	else if (work->ui.widget_5->isVisible() && work->ui.widget_47->isVisible() && work->ui.radioButton_12->isChecked()) {
		if (work->ui.radioButton_41->isChecked()) {
			return "MR";
		}
		if (work->ui.radioButton_42->isChecked()) {
			return "DR";
		}
		if (work->ui.radioButton_43->isChecked()) {
			return "JnR";
		}
		if (work->ui.radioButton_44->isChecked()) {
			return "MDR";
		}
	}

	else if (work->ui.widget_5->isVisible() && work->ui.widget_78->isVisible() && work->ui.radioButton_8->isChecked()) {

		if (work->ui.radioButton_36->isChecked()) {
			return "MD_all";
		}
		if (work->ui.radioButton_37->isChecked()) {
			return "DDL_all";
		}
		if (work->ui.radioButton_50->isChecked()) {
			return "JnDDL_all";
		}

	}

	//Distance
	else if (work->ui.widget_5->isVisible() && work->ui.widget_264->isVisible() && work->ui.radioButton->isChecked()) {

		QString qstrsearchidStr = this->work->ui.lineEdit_56->text();
		std::string searchidStr = std::string((const char *)qstrsearchidStr.toLocal8Bit());

		if (work->ui.radioButton_74->isChecked()) {
			return "MD";
		}
		if (work->ui.radioButton_75->isChecked()) {
			return "DD";
		}
		if (work->ui.radioButton_76->isChecked()) {
			return "JnD";
		}

	/*	if (searchidStr.length() == 0 && (Ref_number_list.size()==0 || (Ref_number_list.size()>0 && Ref_number_list[0]==-1))) {
			FA.BaseInputError("Interactive Mode");
		}
		else {
			if (work->ui.radioButton_74->isChecked()) {
				return "MD";
			}
			if (work->ui.radioButton_75->isChecked()) {
				return "DD";
			}
			if (work->ui.radioButton_76->isChecked()) {
				return "JnD";
			}
		} */
		
	}
	//Path Analysis
	else if (work->ui.widget_37->isVisible() && work->ui.widget_12->isVisible()) {
		if (work->ui.radioButton_21->isChecked()) {
			return "MRPA";
		}
		else if (work->ui.radioButton_22->isChecked()) {
			return "DRPA";
		}
		else if (work->ui.radioButton_23->isChecked()) {
			return "JnRPA";
		}
	}
	//Subsets Analysis
	else if (work->ui.widget_72->isVisible()) {
		if (work->ui.radioButton_33->isChecked()) {
			return "Subset_1";
		}
		else if (work->ui.radioButton_34->isChecked()) {
			return "Subset_2";
		}
		else if (work->ui.radioButton_35->isChecked()) {
			return "Subset_3";
		}
	}

	return "";
}

/********************************************************/
QWidget * MainWindow::setupAttributesListWidget()
{
	QWidget *widget = new QWidget(this);

	QLayout *vlayout = new QVBoxLayout(widget);
	vlayout->setMargin(1);

	QLayout *hlayout = new QHBoxLayout();
	vlayout->addWidget(m_attrWindow = new AttribWindow(this, false));
	vlayout->addItem(hlayout);

	hlayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

	attr_add_button = new QToolButton(widget);
	attr_add_button->setVisible(true);
	attr_add_button->setText(tr("Add"));
	attr_add_button->setIcon(QIcon(tr(":/images/win/b-5-19.png")));
	attr_add_button->setAutoRaise(true);
	attr_add_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	hlayout->addWidget(attr_add_button);
	connect(attr_add_button, SIGNAL(clicked()), this, SLOT(OnAddColumn()));

	attr_del_button = new QToolButton(widget);
	attr_del_button->setVisible(true);
	attr_del_button->setText(tr("Del"));
	attr_del_button->setIcon(QIcon(tr(":/images/win/b-5-21.png")));
	attr_del_button->setAutoRaise(true);
	attr_del_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	hlayout->addWidget(attr_del_button);
	connect(attr_del_button, SIGNAL(clicked()), this, SLOT(OnRemoveColumn()));

	return widget;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
	//调整进度条长度
	int nWidth = this->geometry().width();
	int width = min(max(nWidth - 320 - 200, 100)*0.9, nWidth*0.7);
	setProcessWidth(width);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	mdiArea->closeAllSubWindows();
	if (activeMapView()) {
		event->ignore();
	}
	else {
		QApplication::postEvent((QObject*)&m_wndColourScale, new QEvent(QEvent::Close));
		//writeSettings();  //取消调用这个，避免生成的窗口记住位置，多显示器切换到单显示器下可能会不显示窗口。
		event->accept();
	}
}

void MainWindow::OnFileNew()
{
	MapView *child = createMapView();
	child->getGraphDoc()->OnNewDocument();
	child->setCurrentFile("");
	child->postLoadFile();
	child->show();
	OnFocusGraph(child->getGraphDoc(), QGraphDoc::CONTROLS_LOADALL);
}

void MainWindow::loadFile(QString fileName)
{
	//查找是否已经存在对应文件名的Map可视化子窗口
	QMdiSubWindow *existing = findMapView(fileName);

	//若存在，则返回窗口
	if (existing) {
		mdiArea->setActiveSubWindow(existing);
		return;
	}

	//若不存在，则创建新窗口
	MapView *child = createMapView();

	QByteArray ba = fileName.toUtf8(); // quick fix for weird chars (russian filename bug report) 快速修复怪异字符（俄罗斯文件名错误报告）
	char *file = ba.data();

	//若加载文件合法
	if (child->getGraphDoc()->OnOpenDocument(file))
	{
		child->setCurrentFile(fileName);	//设置Map窗口当前文件
		child->postLoadFile();				//对Map窗口加载当前文件
		statusBar()->showMessage(tr("File loaded"), 2000);
		child->show();						//对Map窗口的加载数据进行输出显示
		OnFocusGraph(child->getGraphDoc(), QGraphDoc::CONTROLS_LOADALL);
		setCurrentFile(fileName);
	}
	else
	{
		child->close();
		QMessageBox::warning(this, "Failed to load", QString("Failed to load file ") + fileName, QMessageBox::Ok, QMessageBox::Ok);
	}
}

std::string getShpFileName(std::string filepath) {
	int pos = -1;
	for (int i = filepath.size() - 1; i >= 0; i--) {
		if (filepath[i] == '/') {
			pos = i;
			break;
		}
	}
	std::string filename = filepath;
	std::string shpfilename = filename + "/" + filename + ".shp";
	if (pos) {
		filename = filepath.substr(pos + 1);
		shpfilename = filepath + "/" + filename + ".shp";
	}
	
	return shpfilename;
}

void MainWindow::OnFileOpen()
{
	process_str = "Reading, please wait...";
	this->locationLabel->setText(process_str);

	// 创建 QSettings 对象以便于保存和读取设置
	QSettings settings("SZU", "URCONNECT");
	lastDirectory = settings.value("lastDirectory", QDir::homePath()).toString(); // 获取上次目录，默认为主目录

	QString template_string;
	template_string += "All formats(*.shp *.txt *.csv)\n";
	template_string += "Shape Files (*.shp)\n Text files (*.txt *.csv)";

	QFileDialog::Options options = 0;
	QString selectedFilter;
	QString fileName = QFileDialog::getOpenFileName(
		0, tr("Open"),
		lastDirectory,
		template_string,
		&selectedFilter,
		options);
	this->shpFileName = fileName;

	if (!fileName.isEmpty())
	{
		lastDirectory = QFileInfo(fileName).absolutePath();
		settings.setValue("lastDirectory", lastDirectory); // 保存最新的目录
		//清空老数据
		clearOldData();

		std::string fileNameStr = std::string((const char *)fileName.toLocal8Bit());

		//设置状态栏
		std::string new_name = fileNameStr;
		int pos = 0;
		while (new_name.find("/") != std::string::npos){
			pos = int(new_name.find("/"));
			new_name = new_name.substr(pos + 1, new_name.length());
		}
		this->fileDir = fileNameStr.substr(0, fileNameStr.size() - new_name.size());
		QString qstr = QString(QString::fromLocal8Bit(new_name.c_str()));
		mapViewName = qstr;

		bool readOver = false;

		if (fileNameStr.find(".shp") != std::string::npos)
		{
			data_map_name = QString(QString::fromLocal8Bit(new_name.substr(0, new_name.size() - 4).c_str()));
			infilepath = fileNameStr;
			CA.init_dbf_path(this->infilepath);
			this->dbfFilePath = fileNameStr.substr(0, fileNameStr.size() - 4) + ".dbf";
			this->shpFileName = QString(QString::fromLocal8Bit(this->infilepath.c_str()));
			this->csvFilePath = fileNameStr.substr(0, fileNameStr.size() - 4) + ".csv";
			this->loadFilePathOver = true;

			//准备地图数据
			this->global_map.readFile(fileNameStr, "FID");
			this->global_file_path = fileNameStr;

			//处理csv文件
			CSVProcess& csvProcessor = CSVProcess::getInstance();
			csvProcessor.open(csvFilePath);
			mergeCsvAttributesName(csvProcessor.getHeader(),this->global_map.Attributes);
			csvProcessor.close();
			

			FA.init(fileNameStr, this->global_map.Attributes);

			//创建新workspace
			OnFileNew();
			QGraphDoc* m_p = activeMapDoc();	//为导入txt文件备用
			//画图
			if (m_p) {
				m_p->OnStreamImport(this->global_file_path);			
			}
			readOver = true;
		}
		else if (
			fileNameStr.find(".txt") != std::string::npos || 
			fileNameStr.find(".csv") != std::string::npos)
		{
			data_map_name = QString(QString::fromLocal8Bit(new_name.substr(0, new_name.size() - 4).c_str()));
			this->infilepath = fileNameStr.substr(0, fileNameStr.size() - 4) + "/" + new_name.substr(0, new_name.size() - 4) + ".shp";
			CA.init_dbf_path(this->infilepath);
			this->dbfFilePath = fileNameStr.substr(0, fileNameStr.size() - 4) + "/" + new_name.substr(0, new_name.size() - 4) + ".dbf";
			this->csvFilePath = fileNameStr.substr(0, fileNameStr.size() - 4) + "/" + new_name.substr(0, new_name.size() - 4) + ".csv";
			this->shpFileName = QString(QString::fromLocal8Bit(this->infilepath.c_str()));
			this->loadFilePathOver = true;

			//创建新workspace
			OnFileNew();
			QGraphDoc* m_p = activeMapDoc();	//为导入txt文件备用

			//从graph->Attributes中读取数据，构造FA和gloabla_map
			MetaGraph *graph = m_treeDoc->m_meta_graph;
			if (!graph) return;

			//画图
			if (m_p) {
				m_p->OnFileImport(fileName);
				while (true) {		//OnFileImport增开线程导入文件，需要等待完成
					if (m_p->m_thread.IsRunOver())
						break;
				}
			}

			//转换成Data Map
			if (graph->getDataMaps().size() == 0) {
				this->OnLayerConvert();
			}

			//判断graph文件中是否有ID字段，没有的话进行添加
			AttributeTable& tab = graph->getAttributeTable();
			bool needID = false;
			if (tab.hasColumn("ID") == false) {
				std::map<int, double> id_data;
				this->global_map.Id_to_Ref.clear();
				this->global_map.Ref_to_Id.clear();
				for (int i = 0; i < tab.getNumRows(); i++) {
					id_data[i] = i;
					this->global_map.Id_to_Ref[i] = i;
					this->global_map.Ref_to_Id[i] = i;
				}
				this->addAttribute("ID", id_data);
			}

			bool succeed = this->global_map.readGraph(graph);
			if (!succeed) {
				process_str = "load error!!";
				this->locationLabel->setText(process_str);
				return;
			}

			//转shp文件名
			std::string shpfilename = getShpFileName(fileNameStr.substr(0, fileNameStr.size() - 4));
			this->global_file_path = shpfilename;
			FA.init(this->global_file_path, this->global_map.Attributes);

			//根据Attributes创建同名的文件夹，里面是同名的.shp文件，增开一个线程写文件，主线程不等待
			FA.Multi_thread_for_WriteFile(fileNameStr.substr(0, fileNameStr.size() - 4), FA.myAttributes);

			readOver = true;
		}
		else {	//警告：不受支持的文件类型，请重新选择
			QMessageBox::warning(this, tr("Warning"), tr("Unrecognised file format. Sorry, unable to import this file."),
				QMessageBox::Ok, QMessageBox::Ok);

			process_str = "Unrecognised file!!";
			this->locationLabel->setText(process_str);
		}

		if (readOver) {
			//收集文件名
			filename_qstr = mapViewName + "\n";
			filename_qstr += "------------------------------------\n";

			this->summary_info = filename_qstr;
			double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
			if (mean_angle > 0) {
				mean_angle_qstr = QString::number(mean_angle, 10, 1);
				this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
			}
			this->displaySummary();

			//处理global_map
			this->Ref_to_Id.insert(this->global_map.Ref_to_Id.begin(), this->global_map.Ref_to_Id.end());
			this->Id_to_Ref.insert(this->global_map.Id_to_Ref.begin(), this->global_map.Id_to_Ref.end());
			this->global_view = true;
			this->nowCount = this->global_map.RecordCount;

			lines_count = this->global_map.RecordCount;

			MetaGraph *graph = m_treeDoc->m_meta_graph;
			if (!graph) return;
			AttributeTable& tab = graph->getAttributeTable();

			//更新属性名称
			if (fileNameStr.find(".shp") != std::string::npos)
				this->modifyColNames(this->global_map.Attributes.AttributesNames);

			//隐藏掉Ref
			tab.modifyKeyColumn();	

			//切换属性为ID，修改ID颜色
			if (graph->viewingProcessed()) {
				int idx = tab.getColumnIndex("ID");
				graph->setDisplayedAttribute(idx);

				//固定ID属性为蓝色
				std::map<int, PafColor> IDColoredLinesMap;
				for (int idx = 0; idx < this->global_map.RecordCount; idx++) {
					IDColoredLinesMap[idx] = BlueColor;
				}
				FixedColoredLinesMapAll["ID"] = IDColoredLinesMap;

				UpdateColorMap();
			}
			SetAttributeChecks();

			//调整属性栏宽度
			this->adaptiveWidth();

			//删除标题栏
			auto mowWindow = mdiArea->currentSubWindow();
			mowWindow->setWindowFlags(Qt::FramelessWindowHint);
			mowWindow->showMaximized();

			isOpen = true;

			emit(this->work->ui.pushButton_20->clicked()); //RESET button

			process_str = "Ready";
			this->locationLabel->setText(process_str);
		}
	}
	else {
		process_str = "Ready";
		this->locationLabel->setText(process_str);
	}
	
}

void MainWindow::OnCSVFileOpen()
{
	QString template_string;
	//CString filter = L"Shape Files(*.shp)|*.shp|All Files(*.*)|*.*";
	template_string += "CSV Files (*.csv)\nTXT Files (*.txt)";

	QFileDialog::Options options = 0;
	QString selectedFilter;
	QString fileName = QFileDialog::getOpenFileName(
		0, tr("Open"),
		"",
		template_string,
		&selectedFilter,
		options);
	if (!fileName.isEmpty())
	{
		//设置输入框
		this->work->ui.lineEdit_2->setText(fileName);
	}
}

void MainWindow::showContextMenu(QPoint &point)
{
	QMenu menu;
	menu.addAction(renameColumnAct);
	menu.addAction(updateColumAct);
	menu.addAction(removeColumAct);
	menu.addSeparator();
	menu.addAction(columnPropertiesAct);

	menu.exec(point);
}

void MainWindow::OnFilePrint()
{

}

void MainWindow::OnFilePrintPreview()
{

}

void MainWindow::OnFilePrintSetup()
{

}

void MainWindow::OnEditUndo()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnEditUndo();
	}
}

void MainWindow::OnEditCopyData()
{
}

void MainWindow::OnEditCopy()
{
	MapView* m_p = activeMapView();
	if (m_p) m_p->OnEditCopy();
}

void MainWindow::OnEditSave()
{
	if (!isOpen) return;
	MapView* m_p = activeMapView();
	if (m_p)
	{
		process_str = "Exporting, please wait...";
		this->locationLabel->setText(process_str);
		m_p->OnEditSave();
		process_str = "Export Over!";
		this->locationLabel->setText(process_str);
	}
}

void MainWindow::OnEditClear()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnEditClear();
	}
}

void MainWindow::OnEditQuery()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnEditQuery();
	}
}

void MainWindow::OnViewZoomsel()
{
	MapView* m_p = activeMapView();
	if (m_p) m_p->OnViewZoomsel();
}

void MainWindow::OnEditSelectToLayer()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnEditSelectToLayer();
	}
}

void MainWindow::OnFileImport()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnFileImport();
	}
}

void MainWindow::OnLayerNew()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnLayerNew();
	}
}

void MainWindow::OnLayerDelete()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnLayerDelete();
	}
}

void MainWindow::OnLayerConvert()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnLayerConvert();
	}
}

void MainWindow::OnLayerConvertDrawing()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnLayerConvertDrawing();
	}
}

void MainWindow::OnConvertMapShapes()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnConvertMapShapes();
	}
}

void MainWindow::transferToSegment() {
	QGraphDoc* m_p = activeMapDoc();
	if (m_p) {
		m_p->OnTranferToSegment();
	}
}

void MainWindow::OnFileExport()
{
	if (!isOpen) return;
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		process_str = "Generating, please wait...";
		this->locationLabel->setText(process_str);
		//this->transferToSegment();	//转换Segment Map
		m_p->OnFileExport();		//输出文件
		//this->OnLayerDelete();		//删除Segment Map
		process_str = "Generate Over!";
		this->locationLabel->setText(process_str);
	}
}

void MainWindow::OnExportNetGeo() {
	if (!isOpen) return;
	QString saveas;
	QFilePath path(windowFilePath());
	std::string filename, net_geo, ng_str;
	if (this->work->ui.radioButton_5->isChecked()) {
		saveas = path.m_path + tr(CA.net_file_name.c_str());
		filename = CA.net_file_name;
		ng_str = CA.net_str;
		net_geo = "Net";
	}
	else if (this->work->ui.radioButton_6->isChecked()) {
		saveas = path.m_path + tr(CA.geo_file_name.c_str());
		filename = CA.geo_file_name;
		ng_str = CA.geo_str;
		net_geo = "Geo";
	}
	else {
		QMessageBox::information(NULL, "Tips", "Please choose Reach or Path");
		return;
	}

	QString template_string = tr("");

	QFileDialog::Options options = 0;
	QString selectedFilter;
	QString outfile = QFileDialog::getSaveFileName(
		0, tr("Save Analysis Files to"),
		saveas,
		template_string,
		&selectedFilter,
		options);

	if (outfile.isEmpty()) 
		return;

	process_str = "Exporting, please wait...";
	this->locationLabel->setText(process_str);

	std::string outFilePath = std::string((const char *)outfile.toLocal8Bit()) + "/";

	if (FA.Route.size() == 0)
		FA.ProcessShapeFile();
	//CA.OutputVisualData(FA, outFilePath);
	CA.OutputVisualData(FA, outFilePath,filename,net_geo,ng_str);

	process_str = "Export Over!";
	this->locationLabel->setText(process_str);
}

void MainWindow::OnFileExportMapGeometry()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnFileExportMapGeometry();
	}
}

void MainWindow::OnFileExportLinks()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnFileExportLinks();
	}
}

void MainWindow::OnAxialConnectionsExportAsDot()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnAxialConnectionsExportAsDot();
	}
}

void MainWindow::OnAxialConnectionsExportAsPairCSV()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnAxialConnectionsExportAsPairCSV();
	}
}

void MainWindow::OnSegmentConnectionsExportAsPairCSV()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnSegmentConnectionsExportAsPairCSV();
	}
}

void MainWindow::OnPointmapExportConnectionsAsCSV()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnPointmapExportConnectionsAsCSV();
	}
}

void MainWindow::OnAddColumn()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		MetaGraph *graph = m_treeDoc->m_meta_graph;
		if (!graph) return;
		AttributeTable *tab = &(graph->getAttributeTable());
		int col = graph->getDisplayedAttribute();
		std::string old_name = tab->getColumnName(col);

		m_p->OnAddColumn();

		col = graph->getDisplayedAttribute();
		std::string new_name = tab->getColumnName(col);
		//同步到dbf文件
		if (old_name != new_name && new_name.size() <= 10) {
			CA.AddField(this->dbfFilePath, new_name);
			//CA.CSVAddField(this->csvFilePath, new_name);
		}

		//调整属性栏宽度
		this->adaptiveWidth();
	}
}

void MainWindow::OnRenameColumn()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		MetaGraph *graph = m_treeDoc->m_meta_graph;
		if (!graph) return;
		AttributeTable *tab = &(graph->getAttributeTable());
		int col = graph->getDisplayedAttribute();
		std::string old_name = tab->getColumnName(col);

		m_p->OnRenameColumn();

		col = graph->getDisplayedAttribute();
		std::string new_name = tab->getColumnName(col);
		//同步到dbf文件
		if (old_name != new_name && new_name.size()<=10) {
			CA.RenameField(this->dbfFilePath, old_name, new_name);
			//CA.CSVRenameField(this->csvFilePath, old_name, new_name);
		}

		//调整属性栏宽度
		this->adaptiveWidth();
	}
}

void MainWindow::OnUpdateColumn()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		MetaGraph *graph = m_treeDoc->m_meta_graph;
		if (!graph) return;
		AttributeTable *tab = &(graph->getAttributeTable());
		int col = graph->getDisplayedAttribute();
		std::string old_name = tab->getColumnName(col);

		m_p->OnUpdateColumn();

		//更新dbf中的属性数据
		std::map<int, double> data;
		for (int i = 0; i < this->global_map.RecordCount; i++) {
			data[i] = tab->getRow(AttributeKey(i)).getValue(col);
		}
		if (data.size() > 0) {
			CA.UpdateField(this->dbfFilePath, old_name, data);
			//CA.CSVUpdateField(this->csvFilePath, old_name, data);
		}
	}
}

void MainWindow::OnRemoveColumn()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		MetaGraph *graph = m_treeDoc->m_meta_graph;
		if (!graph) return;
		AttributeTable *tab = &(graph->getAttributeTable());
		int col = graph->getDisplayedAttribute();
		std::string old_name = tab->getColumnName(col);
		m_p->OnRemoveColumn();
		
		//同步到GlobalMap
		this->global_map.removeAttribute(old_name);

		//同步到dbf文件
		CA.DeleteField(this->dbfFilePath, old_name);
		//CA.CSVDeleteField(this->csvFilePath, old_name);

		//调整属性栏宽度
		this->adaptiveWidth();
	}
}

void MainWindow::OnColumnProperties()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnColumnProperties();
	}
}

void MainWindow::OnPushToLayer()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnPushToLayer();
	}
}

void MainWindow::OnEditGrid()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnEditGrid();
	}
}

void MainWindow::OnToolsMakeGraph()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsMakeGraph();
	}
}

void MainWindow::OnToolsUnmakeGraph()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsUnmakeGraph();
	}
}

void MainWindow::OnToolsImportVGALinks()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnVGALinksFileImport();
	}
}

void MainWindow::OnToolsRun()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsRun();
	}
}

void MainWindow::OnToolsAgentRun()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsAgentRun();
	}
}

void MainWindow::OnToolsIsovistpath()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsIsovistpath();
	}
}

void MainWindow::OnToolsAgentLoadProgram()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		if (m_p->m_view[QGraphDoc::VIEW_3D])
			((Q3DView*)m_p->m_view[QGraphDoc::VIEW_3D])->OnToolsAgentLoadProgram();
	}
}

void MainWindow::OnToolsRunAxa()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsRunAxa();
	}
}

void MainWindow::OnToolsPD()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsPD();
	}
}

void MainWindow::OnToolsMakeFewestLineMap()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsMakeFewestLineMap();
	}
}

void MainWindow::OnToolsAxialConvShapeMap()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsAxialConvShapeMap();
	}
}

void MainWindow::OnToolsLineLoadUnlinks()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsLineLoadUnlinks();
	}
}

void MainWindow::OnToolsRunSeg()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsRunSeg();
	}
}

void MainWindow::OnToolsTopomet()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsTopomet();
	}
}

void MainWindow::OnToolsTPD()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsTPD();
	}
}

void MainWindow::OnToolsMPD()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsMPD();
	}
}

void MainWindow::OnToolsPointConvShapeMap()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsPointConvShapeMap();
	}
}

void MainWindow::OnToolsAPD()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnToolsAPD();
	}
}

void MainWindow::OnToolsOptions()
{
	SettingsDialog dialog(mSettings);
	if (QDialog::Accepted == dialog.exec()) {
		readSettings();
	}
}

void MainWindow::OnViewCentreView()
{
	activeMapDoc()->SetRedrawFlag(QGraphDoc::VIEW_MAP, QGraphDoc::REDRAW_TOTAL, QGraphDoc::NEW_DEPTHMAPVIEW_SETUP, this);
}

void MainWindow::OnViewShowGrid()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnViewShowGrid();
	}
}

void MainWindow::OnViewSummary()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		m_p->OnViewSummary();
	}
}

void MainWindow::OnViewColourRange()
{
	if (m_wndColourScale.isVisible()) {
		m_wndColourScale.hide();
	}
	else {
		QRect recta, rectb;
		recta = geometry();
		rectb = m_wndColourScale.geometry();
		m_wndColourScale.setGeometry(recta.right() - 7 - rectb.width(), recta.top() + 68, rectb.width(), rectb.height());
		//m_wndColourScale.setMinimumWidth(600);
		m_wndColourScale.m_docked = true;
		m_wndColourScale.show();
	}
}

void MainWindow::OnHelpBugs()
{
	bool foo = QDesktopServices::openUrl(QUrl("https://github.com/SpaceGroupUCL/depthmapX/issues"));
}

void MainWindow::OnHelpManual()
{
	bool foo = QDesktopServices::openUrl(QUrl("http://www.vr.ucl.ac.uk/depthmap/depthmap4r1.pdf"));
}

void MainWindow::OnHelpTutorials()
{
	bool foo = QDesktopServices::openUrl(QUrl("http://www.vr.ucl.ac.uk/depthmap/tutorials/"));
}

void MainWindow::OnHelpSalaManual()
{
	bool foo = QDesktopServices::openUrl(QUrl("http://www.vr.ucl.ac.uk/depthmap/scripting/"));
}

void MainWindow::OnFileClose()
{
	MapView* m_p = activeMapView();
	if (m_p) QApplication::postEvent((QObject*)m_p, new QEvent(QEvent::Close));
}

void MainWindow::OnFileSave()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		bool saved = m_p->OnFileSave();
		if (saved) {
			statusBar()->showMessage(tr("File saved"), 2000);
			setCurrentFile(m_p->m_opened_name);
			updateSubWindowTitles(m_p->m_base_title);
		}
		else {
			statusBar()->showMessage(tr("File not saved"), 2000);
		}
	}
}

void MainWindow::OnFileSaveAs()
{
	QGraphDoc* m_p = activeMapDoc();
	if (m_p)
	{
		bool saved = m_p->OnFileSaveAs();
		if (saved) {
			statusBar()->showMessage(tr("File saved"), 2000);
			setCurrentFile(m_p->m_opened_name);
			updateSubWindowTitles(m_p->m_base_title);
		}
		else {
			statusBar()->showMessage(tr("File not saved"), 2000);
		}
	}
}
void MainWindow::updateSubWindowTitles(QString newTitle) {
	QList<QMdiSubWindow *> windowList = mdiArea->subWindowList();
	QList<QMdiSubWindow *>::iterator iter = windowList.begin(), end =
		windowList.end();
	for (; iter != end; ++iter)
	{
		QWidget *p = 0;
		if (QMdiSubWindow *subWindow = *iter)
		{
			p = qobject_cast<MapView *>(subWindow->widget());
			//if (p) subWindow->setWindowTitle(newTitle + ":Map View");
			if (p) subWindow->setWindowTitle(newTitle);
			p = qobject_cast<QPlotView *>(subWindow->widget());
			if (p) subWindow->setWindowTitle(newTitle + ":Scatter Plot");
			p = qobject_cast<tableView *>(subWindow->widget());
			if (p) subWindow->setWindowTitle(newTitle + ":Table View");
			p = qobject_cast<Q3DView *>(subWindow->widget());
			if (p) subWindow->setWindowTitle(newTitle + ":3D View");
		}
	}

}

void MainWindow::OnAppAbout()
{
	//CAboutDlg aboutDlg;
	//aboutDlg.exec();
}

MapView *MainWindow::createMapView()
{
	QGraphDoc* doc = new QGraphDoc("", "");
	doc->m_mainFrame = this;

	if (m_defaultMapWindowIsLegacy)
	{
		QDepthmapView *child = new QDepthmapView(*doc, mSettings);
		mdiArea->addSubWindow(child);
		return child;
	}
	else
	{
		GLView *child = new GLView(*doc, mSettings);
		mdiArea->addSubWindow(child);
		return child;
	}

}

MapView *MainWindow::activeMapView()
{
	QWidget *p = 0;
	if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
	{
		p = qobject_cast<MapView *>(activeSubWindow->widget());
		if (p) return (MapView *)p;
		if (!p)
		{
			p = qobject_cast<QPlotView *>(activeSubWindow->widget());
			if (p) return (MapView *)(((QPlotView*)p)->pDoc->m_view[1]);
		}
		if (!p)
		{
			p = qobject_cast<tableView *>(activeSubWindow->widget());
			if (p) return (MapView *)(((tableView*)p)->pDoc->m_view[1]);
		}
		if (!p)
		{
			p = qobject_cast<Q3DView *>(activeSubWindow->widget());
			if (p) return (MapView *)(((Q3DView*)p)->pDoc->m_view[1]);
		}
	}
	current_view_type = 0;
	return 0;
}

QGraphDoc *MainWindow::activeMapDoc()
{
	QWidget *p = 0;
	if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
	{
		p = qobject_cast<MapView *>(activeSubWindow->widget());
		if (p) return ((MapView *)p)->getGraphDoc();
		p = qobject_cast<QPlotView *>(activeSubWindow->widget());
		if (p) return ((QPlotView *)p)->pDoc;
		p = qobject_cast<tableView *>(activeSubWindow->widget());
		if (p) return ((tableView *)p)->pDoc;
		p = qobject_cast<Q3DView *>(activeSubWindow->widget());
		if (p) return ((Q3DView *)p)->pDoc;
	}
	return 0;
}

QMdiSubWindow *MainWindow::findMapView(const QString &fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

	foreach(QMdiSubWindow *window, mdiArea->subWindowList()) {
		MapView *mdiChild = qobject_cast<MapView *>(window->widget());
		if (mdiChild && mdiChild->getCurrentFile() == canonicalFilePath) return window;
	}
	return 0;
}

void MainWindow::OnWindowMap()
{
	MapView* m_p = activeMapView();
	if (m_p)
	{
		if (m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_MAP])
			return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_MAP]);
		QDepthmapView *child = new QDepthmapView(*m_p->getGraphDoc(), mSettings);
		mdiArea->addSubWindow(child);
		child->show();
	}
}

void MainWindow::OnViewTable()
{
	MapView* m_p = activeMapView();
	if (m_p)
	{
		if (m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_TABLE])
			return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_TABLE]);
		tableView *child = new tableView(this, m_p->getGraphDoc());
		child->pDoc = m_p->getGraphDoc();
		mdiArea->addSubWindow(child);
		child->show();
	}
}

void MainWindow::OnWindow3dView()
{
	MapView* m_p = activeMapView();
	if (m_p)
	{
		if (m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_3D])
			return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_3D]);
		Q3DView *child = new Q3DView(this, m_p->getGraphDoc());
		child->pDoc = m_p->getGraphDoc();
		mdiArea->addSubWindow(child);
		child->show();
	}
}

void MainWindow::OnWindowGLView()
{
	MapView* m_p = activeMapView();
	if (m_p)
	{
		if (m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_MAP_GL])
			return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_MAP_GL]);
		GLView *child = new GLView(*m_p->getGraphDoc(), mSettings);
		mdiArea->addSubWindow(child);
		child->show();
	}
}

void MainWindow::OnViewScatterplot()
{
	MapView* m_p = activeMapView();
	if (m_p)
	{
		if (m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_SCATTER])
			return setActiveSubWindow(m_p->getGraphDoc()->m_view[QGraphDoc::VIEW_SCATTER]);
		QPlotView *child = new QPlotView;
		child->pDoc = m_p->getGraphDoc();
		child->m_parent = this;
		mdiArea->addSubWindow(child);
		child->show();
	}
}

void MainWindow::update3DToolbar()
{
	updateActiveWindows();
}

void MainWindow::updateActiveWindows()
{
	current_view_type = 0;
	QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow();
	if (!activeSubWindow)
	{
		editToolBar->hide();
		thirdViewToolBar->hide();
		plotToolBar->hide();
		return;
	}

	QWidget* p = qobject_cast<QPlotView *>(activeSubWindow->widget());
	if (p)
	{
		editToolBar->hide();
		thirdViewToolBar->hide();
		plotToolBar->show();
		current_view_type = QGraphDoc::VIEW_SCATTER;
		OnFocusGraph(((QPlotView*)p)->pDoc, QGraphDoc::CONTROLS_LOADALL);
		RedoPlotViewMenu(((QPlotView*)p)->pDoc);

		if (((QPlotView*)p)->m_view_monochrome) toggleColor->setChecked(true);
		else toggleColor->setChecked(false);
		if (((QPlotView*)p)->m_view_origin) toggleOrg->setChecked(true);
		else toggleOrg->setChecked(false);
		if (((QPlotView*)p)->m_view_trend_line) viewTrend->setChecked(true);
		else viewTrend->setChecked(false);
		if (((QPlotView*)p)->m_view_equation) yx->setChecked(true);
		else yx->setChecked(false);
		if (((QPlotView*)p)->m_view_rsquared) Rtwo->setChecked(true);
		else Rtwo->setChecked(false);
	}
	else if (qobject_cast<tableView *>(activeSubWindow->widget()))
	{
		editToolBar->hide();
		thirdViewToolBar->hide();
		plotToolBar->hide();
		current_view_type = QGraphDoc::VIEW_TABLE;
		return;
	}
	else if (p = qobject_cast<Q3DView *>(activeSubWindow->widget()))
	{
		editToolBar->hide();
		plotToolBar->hide();
		thirdViewToolBar->show();
		QGraphDoc* pDoc = activeMapDoc();
		Q3DView *ptr = (Q3DView *)p;

		if (ptr->m_animating) toolsAgentsPlayAct->setChecked(true);
		else toolsAgentsPlayAct->setChecked(0);
		if (!ptr->m_animating) toolsAgentsPauseAct->setChecked(true);
		else toolsAgentsPauseAct->setChecked(0);

		if (ptr->m_mouse_mode == ID_3D_PAN) thirdPanAct->setChecked(true);
		else thirdPanAct->setChecked(0);
		if (ptr->m_mouse_mode == ID_3D_ROT) thirdRotAct->setChecked(true);
		else thirdRotAct->setChecked(0);
		if (ptr->m_mouse_mode == ID_3D_ZOOM) thirdZoomAct->setChecked(true);
		else thirdZoomAct->setChecked(0);
		if (ptr->m_mouse_mode == ID_3D_PLAY_LOOP) playLoopAct->setChecked(true);
		else playLoopAct->setChecked(0);
		if (ptr->m_fill) thirdFilledAct->setChecked(true);
		else thirdFilledAct->setChecked(0);

		if (pDoc->m_meta_graph && pDoc->m_meta_graph->viewingProcessedPoints())
			toolsImportTracesAct->setEnabled(true);
		else
			toolsImportTracesAct->setEnabled(false);

		if (!pDoc->m_meta_graph || !pDoc->m_meta_graph->viewingProcessedPoints())
		{
			if (ptr->m_mouse_mode == ID_ADD_AGENT) ptr->m_mouse_mode = ID_3D_ROT;
			addAgentAct->setEnabled(false);
		}
		else
		{
			addAgentAct->setEnabled(true);
			if (ptr->m_mouse_mode == ID_ADD_AGENT) addAgentAct->setChecked(1);
			else addAgentAct->setChecked(0);
		}

		if (ptr->m_mannequins.size())
		{
			toolsAgentsPlayAct->setEnabled(true);
			toolsAgentsPauseAct->setEnabled(true);
			toolsAgentsStopAct->setEnabled(true);

			if (((Q3DView *)p)->m_animating) toolsAgentsPlayAct->setChecked(true);
			else toolsAgentsPlayAct->setChecked(false);
		}
		else
		{
			toolsAgentsPlayAct->setChecked(false);
			toolsAgentsPlayAct->setEnabled(false);
			toolsAgentsPauseAct->setEnabled(false);
			toolsAgentsStopAct->setEnabled(false);
		}

		if (ptr->m_mannequins.size()) {
			toolsAgentsPauseAct->setEnabled(true);
			if (!ptr->m_animating) {
				toolsAgentsPauseAct->setChecked(true);
			}
			else {
				toolsAgentsPauseAct->setChecked(false);
			}
		}
		else {
			toolsAgentsPauseAct->setChecked(false);
			toolsAgentsPauseAct->setEnabled(false);
		}

		if (ptr->m_mannequins.size()) {
			toolsAgentsStopAct->setEnabled(true);
		}
		else {
			toolsAgentsStopAct->setEnabled(false);
		}

		if (ptr->m_drawtrails) {
			agentTrailsAct->setChecked(1);
		}
		else {
			agentTrailsAct->setChecked(0);
		}

		if (ptr->m_fill) {
			thirdFilledAct->setChecked(1);
		}
		else {
			thirdFilledAct->setChecked(0);
		}
		current_view_type = QGraphDoc::VIEW_3D;
		return;
	}
	else if ((p = qobject_cast<MapView *>(activeSubWindow->widget())))
	{
		editToolBar->show();
		thirdViewToolBar->hide();
		plotToolBar->hide();
		current_view_type = QGraphDoc::VIEW_MAP;
		QWidget* v = qobject_cast<MapView *>(activeSubWindow->widget());
		if (v) current_view_type = QGraphDoc::VIEW_MAP_GL;
		switch (m_selected_mapbar_item)
		{
		case ID_MAPBAR_ITEM_SELECT:
			SelectButton->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_MOVE:
			DragButton->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_ZOOM_IN:
			zoomToolButton->setIcon(QIcon(":/images/win/b-5-3.png"));
			zoomToolButton->setChecked(true);
			zoomInAct->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_ZOOM_OUT:
			zoomToolButton->setIcon(QIcon(":/images/win/b-5-4.png"));
			zoomToolButton->setChecked(true);
			zoomOutAct->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_FILL:
			fillColorToolButton->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_SEMIFILL:
			fillColorToolButton->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_AUGMENT_FILL: // AV TV
			fillColorToolButton->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_PENCIL:
			SelectPenButton->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_LINETOOL:
			lineToolButton->setIcon(QIcon(":/images/win/b-5-10.png"));
			lineToolButton->setChecked(true);
			SelectLineAct->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_POLYGON:
			lineToolButton->setIcon(QIcon(":/images/win/b-5-11.png"));
			lineToolButton->setChecked(true);
			SelectPolyLineAct->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_ISOVIST:
			newisoToolButton->setIcon(QIcon(":/images/win/b-5-12.png"));
			newisoToolButton->setChecked(true);
			MakeIosAct->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_HALFISOVIST:
			newisoToolButton->setIcon(QIcon(":/images/win/b-5-13.png"));
			newisoToolButton->setChecked(true);
			PartialMakeIosAct->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_AL2:
			AxialMapButton->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_JOIN:
			JoinToolButton->setIcon(QIcon(":/images/win/b-5-16.png"));
			JoinToolButton->setChecked(true);
			JoinAct->setChecked(true);
			break;
		case ID_MAPBAR_ITEM_UNJOIN:
			JoinToolButton->setIcon(QIcon(":/images/win/b-5-17.png"));
			JoinToolButton->setChecked(true);
			JoinUnlinkAct->setChecked(true);
			break;
		default:
			SelectButton->setChecked(true);
			SelectButton->setChecked(false);
			break;
		}
		QGraphDoc* m_p = activeMapDoc();
		OnFocusGraph(m_p, QGraphDoc::CONTROLS_LOADALL);
		m_p->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_FOCUS);
	}
}

void MainWindow::updateGLWindows(bool datasetChanged, bool recentreView) {
	//this->locationLabel->setText(process_str+", painting");
	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	for (int i = 0; i < windows.size(); ++i) {
		GLView *child = qobject_cast<GLView*>(windows.at(i)->widget());
		if (!child) continue;
		if (datasetChanged) child->notifyDatasetChanged();
		if (recentreView) child->matchViewToCurrentMetaGraph();
	}
	//this->locationLabel->setText(process_str + ", paint over");

	
}

void MainWindow::setProcessWidth(int width) {
	m_pConnectProBar->setMinimumWidth(width);
	m_pConnectProBar->setMaximumWidth(width);
}

void MainWindow::setActiveSubWindow(QWidget *win)
{
	if (!win) return;
	foreach(QMdiSubWindow *window, mdiArea->subWindowList())
	{
		if (window->widget() == win)
		{
			mdiArea->setActiveSubWindow(window);
			return;
		}
	}
	QString t = QString(TITLE_BASE);
	setWindowTitle(t + " " + windowTitle());
}

///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static bool in_FocusGraph;
int MainWindow::OnFocusGraph(QGraphDoc* pDoc, int lParam)
{
	in_FocusGraph = true;
	updateToolbar();
	x_coord->clear();
	y_coord->clear();

	// Replacement for m_linelayer_chooser is my tree ctrl:
	if (lParam == QGraphDoc::CONTROLS_DESTROYALL && pDoc == m_treeDoc) {        // Lost graph
		delete pDoc;
		m_treeDoc = NULL;
		m_topgraph = NULL;
		m_backgraph = NULL;
		m_attrWindow->clear();
		m_indexWidget->clear();
	}
	else if (lParam == QGraphDoc::CONTROLS_LOADALL && pDoc != m_treeDoc) {     // [Possible] change of window (sent on focus)
		m_treeDoc = pDoc;
		m_topgraph = NULL;
		m_backgraph = NULL;
		MakeTree();
	}
	else if (lParam == QGraphDoc::CONTROLS_LOADGRAPH && pDoc == m_treeDoc) {     // Force update if match current window
		m_topgraph = NULL;
		m_backgraph = NULL;
		m_attrWindow->clear();
		m_indexWidget->clear();
		ClearGraphTree();
		MakeGraphTree();
		// also make drawing tree as this overrides layer visible status sometimes:
		MakeDrawingTree();
	}
	else if (lParam == QGraphDoc::CONTROLS_RELOADGRAPH && pDoc == m_treeDoc) {     // Force reload of graph tree if match current window
		m_topgraph = NULL;
		m_backgraph = NULL;
		m_attrWindow->clear();
		m_indexWidget->clear();
		ClearGraphTree();
		MakeTree();
	}
	else if (lParam == QGraphDoc::CONTROLS_LOADDRAWING && pDoc == m_treeDoc) {     // Force update if match current window
		m_backgraph = NULL;
		m_attrWindow->clear();
		m_indexWidget->clear();
		ClearGraphTree();
		MakeGraphTree();
		MakeDrawingTree();
	}
	else if (lParam == QGraphDoc::CONTROLS_LOADATTRIBUTES && pDoc == m_treeDoc) {     // Force update if match current window
		MakeAttributeList();
	}
	else if (lParam == QGraphDoc::CONTROLS_CHANGEATTRIBUTE && pDoc == m_treeDoc) {     // Force update if match current window
		SetAttributeChecks();
	}
	else if (lParam == QGraphDoc::CONTROLS_LOADCONVERT && pDoc == m_treeDoc) {
		m_topgraph = NULL;
		m_backgraph = NULL;
		m_attrWindow->clear();
		m_indexWidget->clear();
		ClearGraphTree();
		MakeGraphTree();
		// conversions typically turn off drawing layers:
		SetDrawingTreeChecks();
	}
	if (m_treeDoc == NULL) {
		//		tree.EnableWindow(FALSE);
				// Stop some strange auto scroll property:
		//		SetTreeStyle(TVS_NOSCROLL, TRUE);
	}
	else {
		//		tree.EnableWindow(TRUE);
				// Stop some strange auto scroll property:
		//		SetTreeStyle(TVS_NOSCROLL, FALSE);
	}

	m_wndColourScale.OnFocusGraph(pDoc, lParam);

	in_FocusGraph = false;
	return 0;
}

void MainWindow::MakeTree()
{
	m_indexWidget->clear();
	m_treegraphmap.clear();
	m_treedrawingmap.clear();

	for (int i = 0; i < 5; i++) m_treeroots[i] = NULL;

	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;

	int state = graph->getState();
	int viewclass = graph->getViewClass();

	MakeGraphTree();
	MakeDrawingTree();
}

void MainWindow::TestAddColumn() {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;

	std::string new_column_name = "test_name_0";

	bool success = false;
	AttributeTable& tab = graph->getAttributeTable();
	bool found = false;
	for (int i = 0; i < tab.getNumColumns(); i++) {
		if (tab.getColumnName(i) == new_column_name) {
			QMessageBox::warning(this, tr("Notice"), tr("Sorry, another column already has this name, please choose a unique column name"), QMessageBox::Ok, QMessageBox::Ok);
			found = true;
			break;
		}
	}
	if (!found) {
		success = true;
	}

	//添加属性列
	if (success) {
		int col = graph->addAttribute(new_column_name);
		graph->setDisplayedAttribute(col);
		UpdateColorMap();
		m_treeDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
		// Tell the views to update their menus
		m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_COLUMN);
	}

	//添加属性值
	int new_val = -1;
	for (int k = 0; k < 207; k++) {
		if (k > 100) new_val = k;
		tab.getRow(AttributeKey(k)).setValue(new_column_name, new_val);
	}
}

void MainWindow::updateAttributes(Calculation &CA) {
	if (CA.TempModifyData.size() == 0)
		return;

	//修改对应属性列
	std::string new_column_name = "";
	for (auto it = CA.TempModifyData.begin(); it != CA.TempModifyData.end(); it++) {
		this->addAttribute(it->first, it->second);
		if (it->first != "seglength" && new_column_name=="")
			new_column_name = it->first;

		if (new_column_name.size() > 10) {
			//调整属性栏宽度
			this->adaptiveWidth();
		}
	}

	//修改结束，清空临时数据
	CA.TempModifyData.clear();

	//切换视图到最新计算的属性
	if (new_column_name == "") return;
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	graph->setDisplayedAttribute(tab.getColumnIndex(new_column_name));
	UpdateColorMap();
	emit(this->pDisplaydow->ui->Button_start_scan_5->clicked());
	//触发重绘显示
	this->updateGLWindows(true, false);
}

void MainWindow::reDrawSubsetNetReach(std::vector<std::string> &items) {
	this->cancelSubsetNetReach();
	this->cancelSubsetGeoReach();
	subset_NetRoadsAll.clear();
	subset_NetFixedColoredLinesMap.clear();
	subset_NetFixedThickLinesMap.clear();

	//加入NetReach线条，同一个起点是同一个配色，这里会新增一个属性“NetReach”，不同颜色通过数值控制，这里是收集线条数据
	NetMap subNetMap;
	subNetMap.subset_init(CA, FA, items);
	this->net_map_all[-1] = subNetMap;
	//逐个NetReach通过addRow添加到Global地图中
	this->addSubsetNetMap(this->net_map_all[-1]);

	//绘制subsetIDs
	for (auto it = this->SubsetIDs.begin(); it != this->SubsetIDs.end(); it++) {
		for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
			FixedColoredLinesMap[*iter] = SubsetLinesColor;
			FixedThickLinesMap[4].insert(*iter);
		}
	}

	//触发重绘显示
	this->updateGLWindows(true, false);

	//统计信息summary info

	//告知结束
	set_calculate_over(true);
}

void MainWindow::reDrawSubsetGeoReach(std::vector<std::string> &items) {
	this->cancelSubsetNetReach();
	this->cancelSubsetGeoReach();
	subset_GeoRoadsAll.clear();
	subset_GeoFixedColoredLinesMap.clear();
	subset_GeoFixedThickLinesMap.clear();

	//加入NetReach线条，同一个起点是同一个配色，这里会新增一个属性“NetReach”，不同颜色通过数值控制，这里是收集线条数据
	GeoMap subGeoMap;
	subGeoMap.subset_init(CA, FA, items);
	this->geo_map_all[-1] = subGeoMap;
	//逐个NetReach通过addRow添加到Global地图中
	this->addSubsetGeoMap(this->geo_map_all[-1]);

	//绘制subsetIDs
	for (auto it = this->SubsetIDs.begin(); it != this->SubsetIDs.end(); it++) {
		for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
			FixedColoredLinesMap[*iter] = SubsetLinesColor;
			FixedThickLinesMap[4].insert(*iter);
		}
	}

	//触发重绘显示
	this->updateGLWindows(true, false);

	//统计信息summary info

	//告知结束
	set_calculate_over(true);

}

void MainWindow::addNetMapsAll(Calculation &CA, ShapeFileAccessor &FA) {
	this->cancelNetReach();
	this->cancelGeoReach();
	NetRoadsAll.clear();
	NetFixedColoredLinesMap.clear();
	NetFixedThickLinesMap.clear();

	//加入NetReach线条，同一个起点是同一个配色，这里会新增一个属性“NetReach”，不同颜色通过数值控制，这里是收集线条数据
	this->net_map_all.clear();
	std::set<int> HaveValues;
	for (auto it = CA.reachRoad_all.begin(); it != CA.reachRoad_all.end(); it++) {
		int startRoad = it->first;
		NetMap subNetMap;
		subNetMap.init(CA, FA, startRoad);

		//设置统一颜色
		/*subNetMap.red = rand() % 246 + 10;
		subNetMap.green = rand() % 246 + 10;
		subNetMap.blue = 0;*/
		subNetMap.red = 0;
		subNetMap.green = 255;
		subNetMap.blue = 0;

		this->net_map_all[startRoad] = subNetMap;
	}
	//逐个NetReach通过addRow添加到Global地图中
	for (auto it = this->net_map_all.begin(); it != this->net_map_all.end(); it++) {
		this->addNetMap(it->second);
	}
	////对起点线条单独设置
	//NetFromIDs.clear();
	//for (auto it = CA.FromIDVec.begin(); it != CA.FromIDVec.end(); it++) {
	//	int startRoad = *it;
	//	NetFromIDs.insert(startRoad);
	//	FixedColoredLinesMap[this->net_map_all[startRoad].Id_to_Ref[startRoad]] = StartLinesColor;
	//	NetFixedColoredLinesMap[this->net_map_all[startRoad].Id_to_Ref[startRoad]] = StartLinesColor;
	//}

	//触发重绘显示
	this->updateGLWindows(true, false);

	//统计信息
	this->summaryNet(CA, FA);

	//告知结束
	set_calculate_over(true);

	//打开开关
	emit(this->pDisplaydow->ui->Button_start_scan_4->clicked());
}

void MainWindow::addGeoMapsAll(Calculation &CA, ShapeFileAccessor &FA) {
	this->cancelNetReach();
	this->cancelGeoReach();
	GeoRoadsAll.clear();
	GeoFixedColoredLinesMap.clear();
	GeoFixedThickLinesMap.clear();

	srand(time(NULL)); /*根据当前时间设置“随机数种子”*/

	//加入NetReach线条，同一个起点是同一个配色，这里会新增一个属性“NetReach”，不同颜色通过数值控制，这里是收集线条数据
	this->geo_map_all.clear();
	std::set<int> HaveValues;
	for (auto it = CA.routeRoad_all.begin(); it != CA.routeRoad_all.end(); it++) {
		int startRoad = it->first;
		GeoMap subGeoMap;
		subGeoMap.init(CA, FA, startRoad);

		//设置统一颜色
		/*subGeoMap.red = rand() % 246 + 10;
		subGeoMap.green = rand() % 246 + 10;
		subGeoMap.blue = 0;*/
		subGeoMap.red = 0;
		subGeoMap.green = 255;
		subGeoMap.blue = 0;

		this->geo_map_all[startRoad] = subGeoMap;
	}
	//逐个Geodesics通过addRow添加到Global地图中
	for (auto it = this->geo_map_all.begin(); it != this->geo_map_all.end(); it++) {
		this->addGeoMap(it->second);
	}
	////对起点线条单独设置
	//GeoFromIDs.clear();
	//GeoToIDs.clear();
	//for (auto it = CA.FromIDVec.begin(); it != CA.FromIDVec.end(); it++) {
	//	int startRoad = *it;
	//	GeoFromIDs.insert(startRoad);
	//	FixedColoredLinesMap[this->geo_map_all[startRoad].Id_to_Ref[startRoad]] = StartLinesColor;
	//	GeoFixedColoredLinesMap[this->geo_map_all[startRoad].Id_to_Ref[startRoad]] = StartLinesColor;
	//	//对终点线条单独设置
	//	for (auto iter = CA.ToIDVec.begin(); iter != CA.ToIDVec.end(); iter++) {
	//		int endRoad = *iter;
	//		GeoToIDs.insert(endRoad);
	//		FixedColoredLinesMap[this->geo_map_all[startRoad].Id_to_Ref[endRoad]] = EndLinesColor;
	//		GeoFixedColoredLinesMap[this->geo_map_all[startRoad].Id_to_Ref[endRoad]] = EndLinesColor;
	//	}
	//}

	//触发重绘显示
	this->updateGLWindows(true, false);

	//统计信息
	this->summaryGeo(CA, FA);

	//告知结束
	set_calculate_over(true);

	//打开开关
	emit(this->pDisplaydow->ui->Button_start_scan_2->clicked());
}

void MainWindow::addAttribute(std::string new_column_name, std::map<int, double> &data) {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;

	bool success = false;
	AttributeTable& tab = graph->getAttributeTable();
	bool found = false;
	for (int i = 0; i < tab.getNumColumns(); i++) {
		if (tab.getColumnName(i) == new_column_name) {
			//QMessageBox::warning(this, tr("Notice"), tr("Sorry, another column already has this name, please choose a unique column name"), QMessageBox::Ok, QMessageBox::Ok);
			found = true;
			break;
		}
	}
	if (!found) {
		success = true;
	}

	//往global_map同步新数据
	this->global_map.ModifyDoubleData(new_column_name, data);

	//添加属性列
	if (success) {
		int col = graph->addAttribute(new_column_name);
		graph->setDisplayedAttribute(col);
		UpdateColorMap();
		m_treeDoc->SetUpdateFlag(QGraphDoc::NEW_DATA);
		// Tell the views to update their menus
		m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_COLUMN);

		//更新属性名称
		m_treeDoc->RenameColumnByCol(col, new_column_name);
	}

	//添加属性列的数据
	for (auto it = data.begin(); it != data.end(); it++) {
		tab.getRow(AttributeKey(this->global_map.Id_to_Ref[it->first])).setValue(new_column_name, it->second);
	}
}

void MainWindow::cancelSubsetNetReach() {
	//获取当前GL视图展示的属性名称
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);
	FixedThickLinesMap = FixedThickLinesMapAll[field_name];
	FixedColoredLinesMap = FixedColoredLinesMapAll[field_name];

	if (tab.getNumRows() <= this->global_map.RecordCount) {
		//do nothing
	}
	else {
		//Ref从高到低，逐条删除
		std::vector<int> ref_list;
		for (auto it = this->net_map_all.rbegin(); it != this->net_map_all.rend(); it++) {
			for (auto iter = it->second.NewLineRefs.rbegin(); iter != it->second.NewLineRefs.rend(); iter++) {
				ref_list.emplace_back(*iter);
			}
		}
		graph->getDisplayedDataMap().removeIds(ref_list);

		//删除Ref、ID映射关系
		for (auto it = this->net_map_all.rbegin(); it != this->net_map_all.rend(); it++) {
			it->second.Ref_to_Id.clear();
			it->second.Id_to_Ref.clear();
		}
	}

	//关闭开关

	//触发重绘显示
	this->updateGLWindows(true, false);
}

//取消NetReach绘制
void MainWindow::cancelNetReach() {

	//获取当前GL视图展示的属性名称
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);
	FixedThickLinesMap = FixedThickLinesMapAll[field_name];
	FixedColoredLinesMap = FixedColoredLinesMapAll[field_name];

	if (tab.getNumRows() <= this->global_map.RecordCount) {
		//do nothing
	}
	else {
		//Ref从高到低，逐条删除
		std::vector<int> ref_list;
		for (auto it = this->net_map_all.rbegin(); it != this->net_map_all.rend(); it++) {
			for (auto iter = it->second.NewLineRefs.rbegin(); iter != it->second.NewLineRefs.rend(); iter++) {
				ref_list.emplace_back(*iter);
			}
		}
		graph->getDisplayedDataMap().removeIds(ref_list);

		//删除Ref、ID映射关系
		for (auto it = this->net_map_all.rbegin(); it != this->net_map_all.rend(); it++) {
			it->second.Ref_to_Id.clear();
			it->second.Id_to_Ref.clear();
		}
	}	

	emit(this->pDisplaydow->ui->Button_start_scan_5->clicked());
	//关闭开关
	emit(this->pDisplaydow->ui->Button_start_scan_11->clicked());

	//触发重绘显示
	this->updateGLWindows(true, false);

}

void MainWindow::deleteGlobal() {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;

	AttributeTable& tab = graph->getAttributeTable();

	//Ref从高到低，逐条删除
	std::vector<int> ref_list;
	for (auto iter = this->global_map.Ref_to_Id.rbegin(); iter != this->global_map.Ref_to_Id.rend(); iter++) {
		ref_list.emplace_back(iter->first);
	}
	graph->getDisplayedDataMap().removeIds(ref_list);

	////删除Ref、ID映射关系：global的信息不应该删除，因为始终是1=1
	//this->global_map.Ref_to_Id.clear();
	//this->global_map.Id_to_Ref.clear();

	emit(this->pDisplaydow->ui->Button_start_scan_5->clicked());

	////取消属性当前勾选
	//if (tab.hasColumn("ID")) {
	//	graph->setDisplayedAttribute(tab.getColumnIndex("ID"));
	//	UpdateColorMap();
	//}
	//else {
	//	graph->setDisplayedAttribute(-1);
	//	UpdateColorMap();
	//}

	//触发重绘显示
	this->updateGLWindows(true, false);
}

void MainWindow::cancelSubsetGeoReach() {
	//获取当前GL视图展示的属性名称
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);
	FixedThickLinesMap = FixedThickLinesMapAll[field_name];
	FixedColoredLinesMap = FixedColoredLinesMapAll[field_name];

	//删除Ref、ID映射关系
	for (auto it = this->geo_map_all.rbegin(); it != this->geo_map_all.rend(); it++) {
		it->second.Ref_to_Id.clear();
		it->second.Id_to_Ref.clear();
	}

	//关闭开关

	//触发重绘显示
	this->updateGLWindows(true, false);
}

void MainWindow::cancelGeoReach() {

	//获取当前GL视图展示的属性名称
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);
	FixedThickLinesMap = FixedThickLinesMapAll[field_name];
	FixedColoredLinesMap = FixedColoredLinesMapAll[field_name];

	//删除Ref、ID映射关系
	for (auto it = this->geo_map_all.rbegin(); it != this->geo_map_all.rend(); it++) {
		it->second.Ref_to_Id.clear();
		it->second.Id_to_Ref.clear();
	}

	emit(this->pDisplaydow->ui->Button_start_scan_5->clicked());
	//关闭开关
	emit(this->pDisplaydow->ui->Button_start_scan_10->clicked());
	
	//触发重绘显示
	this->updateGLWindows(true, false);

}

//重绘NetReach
void MainWindow::reDrawNetReach() {
	//备份颜色和粗细
	std::map<int, PafColor> tmp_NetFixedColoredLinesMap(NetFixedColoredLinesMap);
	std::map<float, set<int>> tmp_NetFixedThickLinesMap(NetFixedThickLinesMap);

	this->cancelNetReach();
	this->cancelGeoReach();

	//打开开关
	emit(this->pDisplaydow->ui->Button_start_scan_4->clicked());

	//逐个NetReach通过addRow添加到Global地图中
	for (auto it = this->net_map_all.begin(); it != this->net_map_all.end(); it++) {
		this->addNetMap(it->second);
	}
	//for (auto it = NetFromIDs.begin(); it != NetFromIDs.end(); it++) {
	//	int startRoad = *it;
	//	FixedColoredLinesMap[this->net_map_all[startRoad].Id_to_Ref[startRoad]] = StartLinesColor;
	//	NetFixedColoredLinesMap[this->net_map_all[startRoad].Id_to_Ref[startRoad]] = StartLinesColor;
	//}

	//恢复颜色和粗细
	NetFixedColoredLinesMap = tmp_NetFixedColoredLinesMap;
	NetFixedThickLinesMap = tmp_NetFixedThickLinesMap;

	//获取当前GL视图展示的属性名称
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& tab = graph->getAttributeTable();
	int col_index = graph->getDisplayedAttribute();
	std::string field_name = tab.getColumnName(col_index);

	//恢复上次的颜色修改
	for (auto it = NetFixedColoredLinesMap.begin(); it != NetFixedColoredLinesMap.end(); it++) {
		FixedColoredLinesMap[it->first] = it->second;
	}
	//恢复线的粗细
	if (NetFixedThickLinesMap.size() > 0) {
		ID_to_width.clear();
		FixedThickLinesMap = FixedThickLinesMapAll[field_name];
		for (auto it = FixedThickLinesMap.begin(); it != FixedThickLinesMap.end(); it++) {
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				ID_to_width[*iter] = it->first;
			}
		}
		for (auto it = NetFixedThickLinesMap.begin(); it != NetFixedThickLinesMap.end(); it++) {
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				ID_to_width[*iter] = it->first;
			}
		}
		//数据转换
		FixedThickLinesMap.clear();
		for (auto it = ID_to_width.begin(); it != ID_to_width.end(); it++) {
			FixedThickLinesMap[it->second].insert(it->first);
		}
	}

	//触发重绘显示
	this->updateGLWindows(true, false);

	//统计信息
	this->summary_info = NetInfo;
	this->displaySummary();
}

void MainWindow::reDrawGeoReach() {
	//备份颜色和粗细
	std::map<int, PafColor> tmp_GeoFixedColoredLinesMap(GeoFixedColoredLinesMap);
	std::map<float, set<int>> tmp_GeoFixedThickLinesMap(GeoFixedThickLinesMap);

	this->cancelNetReach();
	this->cancelGeoReach();

	//打开开关
	emit(this->pDisplaydow->ui->Button_start_scan_2->clicked());

	//逐个NetReach通过addRow添加到Global地图中
	for (auto it = this->geo_map_all.begin(); it != this->geo_map_all.end(); it++) {
		this->addGeoMap(it->second);
	}
	//for (auto it = GeoFromIDs.begin(); it != GeoFromIDs.end(); it++) {
	//	int startRoad = *it;
	//	FixedColoredLinesMap[this->geo_map_all[startRoad].Id_to_Ref[startRoad]] = StartLinesColor;
	//	GeoFixedColoredLinesMap[this->geo_map_all[startRoad].Id_to_Ref[startRoad]] = StartLinesColor;
	//	//对终点线条单独设置
	//	for (auto iter = GeoToIDs.begin(); iter != GeoToIDs.end(); iter++) {
	//		int endRoad = *iter;
	//		FixedColoredLinesMap[this->geo_map_all[startRoad].Id_to_Ref[endRoad]] = EndLinesColor;
	//		GeoFixedColoredLinesMap[this->geo_map_all[startRoad].Id_to_Ref[endRoad]] = EndLinesColor;
	//	}
	//}

	//恢复颜色和粗细
	GeoFixedColoredLinesMap = tmp_GeoFixedColoredLinesMap;
	GeoFixedThickLinesMap = tmp_GeoFixedThickLinesMap;

	//恢复上次的颜色修改
	for (auto it = GeoFixedColoredLinesMap.begin(); it != GeoFixedColoredLinesMap.end(); it++) {
		FixedColoredLinesMap[it->first] = it->second;
	}
	//恢复线的粗细
	if (GeoFixedThickLinesMap.size() > 0) {
		ID_to_width.clear();
		for (auto it = FixedThickLinesMap.begin(); it != FixedThickLinesMap.end(); it++) {
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				ID_to_width[*iter] = it->first;
			}
		}
		for (auto it = GeoFixedThickLinesMap.begin(); it != GeoFixedThickLinesMap.end(); it++) {
			for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
				ID_to_width[*iter] = it->first;
			}
		}
		//数据转换
		FixedThickLinesMap.clear();
		for (auto it = ID_to_width.begin(); it != ID_to_width.end(); it++) {
			FixedThickLinesMap[it->second].insert(it->first);
		}
	}

	//触发重绘显示
	this->updateGLWindows(true, false);

	//统计信息
	this->summary_info = GeoInfo;
	this->displaySummary();

	
}

////重绘全局地图
//void MainWindow::reDrawGlobalMap() {
//	//清空
//	this->deleteAllMap();
//
//	QGraphDoc* m_p = activeMapDoc();
//
//	//画图
//	if (m_p)
//	{
//		m_p->OnStreamImport(newName());
//	}
//
//	//更新属性名称
//	this->modifyColNames(this->global_map.Attributes.AttributesNames);
//
//	//修改Ref Number颜色
//	MetaGraph *graph = m_treeDoc->m_meta_graph;
//	if (!graph) return;
//	AttributeTable& tab = graph->getAttributeTable();
//	DisplayParams param;
//	param.red = 128, param.blue = 128;
//	tab.getColumn(tab.getColumnIndex("ID")).setDisplayParams(param);
//	tab.modifyKeyColumn();
//
//	//测试切换属性为ID
//	if (graph->viewingProcessed()) {
//		graph->setDisplayedAttribute(tab.getColumnIndex("ID"));
//	}
//	SetAttributeChecks();
//}

////删除所有线段，无论是全局、Net、Geo
//void MainWindow::deleteAllMap() {
//	MetaGraph *graph = m_treeDoc->m_meta_graph;
//	if (!graph) return;
//
//	AttributeTable& tab = graph->getAttributeTable();
//
//	tab.clear();
//
//	//取消属性当前勾选
//	if (tab.hasColumn("ID"))
//		graph->setDisplayedAttribute(tab.getColumnIndex("ID"));
//	else
//		graph->setDisplayedAttribute(-1);
//	emit(this->pDisplaydow->ui->Button_start_scan_5->clicked());
//
//	//触发重绘显示
//	this->updateGLWindows(true, false);
//}


void MainWindow::DrawSubsetReach() {
	std::vector<std::string> items;
	if (this->work->ui.radioButton_11->isChecked())
		items.push_back("MR");
	if (this->work->ui.radioButton_13->isChecked())
		items.push_back("DR");
	if (this->work->ui.radioButton_14->isChecked())
		items.push_back("MDR");
	if (this->work->ui.radioButton_15->isChecked())
		items.push_back("JnR");

	bool needCancel = false;
	if (items.size() == 0) {	//均未勾选
		needCancel = true;;
	}

	if (this->work->ui.radioButton_33->isChecked()) {	//单组子集线
		if (needCancel) {
			cancelSubsetNetReach();
		}
		else {
			reDrawSubsetNetReach(items);
		}
	}
	else if (this->work->ui.radioButton_34->isChecked()) {	//双组子集线
		if (needCancel) {
			cancelSubsetGeoReach();
		}
		else {
			reDrawSubsetGeoReach(items);
		}
	}
}

void MainWindow::addSubsetNetMap(NetMap &subNetMap) {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;

	bool success = false;
	AttributeTable& tab = graph->getAttributeTable();

	//添加部分通过的新线条
	int RefCount = tab.getNumRows();
	graph->getDataMaps()[0].setEditable(true);
	PafColor fixed_color = getColor(subNetMap.red, subNetMap.green, subNetMap.blue);
	for (auto it = subNetMap.PartInRoadsCoordinate.begin(); it != subNetMap.PartInRoadsCoordinate.end(); it++) {
		int id = it->first;

		//添加新线段
		Point2f FirstPoint(it->second[0], it->second[1]);
		Point2f SecondPoint(it->second[2], it->second[3]);
		if (graph->makeShape(Line(FirstPoint, SecondPoint)));
		else return;

		subNetMap.Ref_to_Id[RefCount] = id;
		subNetMap.Id_to_Ref[id] = RefCount;
		subNetMap.NewLineRefs.insert(RefCount);
		NetRoadsAll.insert(RefCount);

		//设置颜色
		FixedColoredLinesMap[RefCount] = fixed_color;
		subset_NetFixedColoredLinesMap[RefCount] = fixed_color;
		//加粗线条
		FixedThickLinesMap[FixedThick].insert(RefCount);
		subset_NetFixedThickLinesMap[FixedThick].insert(RefCount);

		//无关属性设置Global的数值
		for (int i = 0; i < tab.getNumColumns(); i++) {
			string field_name = tab.getColumnName(i);
			tab.getRow(AttributeKey(RefCount)).setValue(field_name, this->global_map.Attributes.AttributesDouble[field_name][id]);
		}

		++RefCount;

		if (it->second.size() > 4) {
			//添加新线段
			Point2f FirstPoint_2(it->second[4], it->second[5]);
			Point2f SecondPoint_2(it->second[6], it->second[7]);
			if (graph->makeShape(Line(FirstPoint_2, SecondPoint_2)));
			else return;

			subNetMap.Ref_to_Id[RefCount] = id;
			subNetMap.Id_to_Ref[id] = RefCount;
			subNetMap.NewLineRefs.insert(RefCount);
			subset_NetRoadsAll.insert(RefCount);

			//设置颜色
			FixedColoredLinesMap[RefCount] = fixed_color;
			subset_NetFixedColoredLinesMap[RefCount] = fixed_color;
			//加粗线条
			FixedThickLinesMap[FixedThick].insert(RefCount);
			subset_NetFixedThickLinesMap[FixedThick].insert(RefCount);

			//无关属性设置Global对应数值
			for (int i = 0; i < tab.getNumColumns(); i++) {
				string field_name = tab.getColumnName(i);
				tab.getRow(AttributeKey(RefCount)).setValue(field_name, this->global_map.Attributes.AttributesDouble[field_name][id]);
			}

			++RefCount;
		}
	}
	//设置完全通过的线条
	for (auto it = subNetMap.AllInRoads.begin(); it != subNetMap.AllInRoads.end(); it++) {
		int id = *it;

		subNetMap.Ref_to_Id[id] = id;
		subNetMap.Id_to_Ref[id] = id;

		//设置颜色
		FixedColoredLinesMap[id] = fixed_color;
		subset_NetFixedColoredLinesMap[id] = fixed_color;
		//加粗线条
		FixedThickLinesMap[FixedThick].insert(id);
		subset_NetFixedThickLinesMap[FixedThick].insert(id);
	}
	subset_NetRoadsAll.insert(subNetMap.AllInRoads.begin(), subNetMap.AllInRoads.end());

	graph->getDataMaps()[0].setEditable(false);
}

void MainWindow::addNetMap(NetMap &subNetMap) {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;

	bool success = false;
	AttributeTable& tab = graph->getAttributeTable();

	////处理全局地图
	//if (FixedColoredLinesMap.size() < this->global_map.RecordCount) {
	//	for (int i = 0; i < this->global_map.RecordCount; i++) {
	//		//设置灰色
	//		FixedColoredLinesMap[i] = GrayColor;
	//	}
	//}

	//添加部分通过的新线条
	int RefCount = tab.getNumRows();
	graph->getDataMaps()[0].setEditable(true);
	PafColor fixed_color = getColor(subNetMap.red, subNetMap.green, subNetMap.blue);
	for (auto it = subNetMap.PartInRoadsCoordinate.begin(); it != subNetMap.PartInRoadsCoordinate.end(); it++) {
		int id = it->first;

		//添加新线段
		Point2f FirstPoint(it->second[0], it->second[1]);
		Point2f SecondPoint(it->second[2], it->second[3]);
		if (graph->makeShape(Line(FirstPoint, SecondPoint)));
		else return;

		subNetMap.Ref_to_Id[RefCount] = id;
		subNetMap.Id_to_Ref[id] = RefCount;
		subNetMap.NewLineRefs.insert(RefCount);
		NetRoadsAll.insert(RefCount);

		//设置颜色
		FixedColoredLinesMap[RefCount] = fixed_color;
		NetFixedColoredLinesMap[RefCount] = fixed_color;
		//加粗线条
		FixedThickLinesMap[FixedThick].insert(RefCount);
		NetFixedThickLinesMap[FixedThick].insert(RefCount);

		//无关属性设置Global的数值
		for (int i = 0; i < tab.getNumColumns(); i++) {
			string field_name = tab.getColumnName(i);
			tab.getRow(AttributeKey(RefCount)).setValue(field_name, this->global_map.Attributes.AttributesDouble[field_name][id]);
		}

		++RefCount;

		if (it->second.size() > 4) {
			//添加新线段
			Point2f FirstPoint_2(it->second[4], it->second[5]);
			Point2f SecondPoint_2(it->second[6], it->second[7]);
			if (graph->makeShape(Line(FirstPoint_2, SecondPoint_2)));
			else return;

			subNetMap.Ref_to_Id[RefCount] = id;
			subNetMap.Id_to_Ref[id] = RefCount;
			subNetMap.NewLineRefs.insert(RefCount);
			NetRoadsAll.insert(RefCount);

			//设置颜色
			FixedColoredLinesMap[RefCount] = fixed_color;
			NetFixedColoredLinesMap[RefCount] = fixed_color;
			//加粗线条
			FixedThickLinesMap[FixedThick].insert(RefCount);
			NetFixedThickLinesMap[FixedThick].insert(RefCount);

			//无关属性设置Global对应数值
			for (int i = 0; i < tab.getNumColumns(); i++) {
				string field_name = tab.getColumnName(i);
				tab.getRow(AttributeKey(RefCount)).setValue(field_name, this->global_map.Attributes.AttributesDouble[field_name][id]);
			}

			++RefCount;
		}
	}
	//设置完全通过的线条
	for (auto it = subNetMap.AllInRoads.begin(); it != subNetMap.AllInRoads.end(); it++) {
		int id = *it;

		subNetMap.Ref_to_Id[id] = id;
		subNetMap.Id_to_Ref[id] = id;

		//设置颜色
		FixedColoredLinesMap[id] = fixed_color;
		NetFixedColoredLinesMap[id] = fixed_color;
		//加粗线条
		FixedThickLinesMap[FixedThick].insert(id);
		NetFixedThickLinesMap[FixedThick].insert(id);
	}
	NetRoadsAll.insert(subNetMap.AllInRoads.begin(), subNetMap.AllInRoads.end());

	graph->getDataMaps()[0].setEditable(false);
}

void MainWindow::addSubsetGeoMap(GeoMap &subGeoMap) {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;

	AttributeTable& tab = graph->getAttributeTable();

	//添加新线条
	int RefCount = tab.getNumRows();
	graph->getDataMaps()[0].setEditable(true);
	PafColor fixed_color = getColor(subGeoMap.red, subGeoMap.green, subGeoMap.blue);
	//设置完全通过的线条
	for (auto it = subGeoMap.AllInRoads.begin(); it != subGeoMap.AllInRoads.end(); it++) {
		int id = *it;

		subGeoMap.Ref_to_Id[id] = id;
		subGeoMap.Id_to_Ref[id] = id;

		//设置颜色
		FixedColoredLinesMap[id] = fixed_color;
		subset_GeoFixedColoredLinesMap[id] = fixed_color;
		//加粗线条
		FixedThickLinesMap[FixedThick].insert(id);
		subset_GeoFixedThickLinesMap[FixedThick].insert(id);
	}
	subset_GeoRoadsAll.insert(subGeoMap.AllInRoads.begin(), subGeoMap.AllInRoads.end());

	graph->getDataMaps()[0].setEditable(false);
}

void MainWindow::addGeoMap(GeoMap &subGeoMap) {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;

	AttributeTable& tab = graph->getAttributeTable();

	////处理全局地图
	//if (FixedColoredLinesMap.size() < this->global_map.RecordCount) {
	//	for (int i = 0; i < this->global_map.RecordCount; i++) {
	//		//设置灰色
	//		FixedColoredLinesMap[i] = GrayColor;
	//	}
	//}

	//添加新线条
	int RefCount = tab.getNumRows();
	graph->getDataMaps()[0].setEditable(true);
	PafColor fixed_color = getColor(subGeoMap.red, subGeoMap.green, subGeoMap.blue);
	//设置完全通过的线条
	for (auto it = subGeoMap.AllInRoads.begin(); it != subGeoMap.AllInRoads.end(); it++) {
		int id = *it;

		subGeoMap.Ref_to_Id[id] = id;
		subGeoMap.Id_to_Ref[id] = id;

		//设置颜色
		FixedColoredLinesMap[id] = fixed_color;
		GeoFixedColoredLinesMap[id] = fixed_color;
		//加粗线条
		FixedThickLinesMap[FixedThick].insert(id);
		GeoFixedThickLinesMap[FixedThick].insert(id);
	}
	GeoRoadsAll.insert(subGeoMap.AllInRoads.begin(), subGeoMap.AllInRoads.end());

	graph->getDataMaps()[0].setEditable(false);
}

void MainWindow::summaryFirstLoad() {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& table = graph->getAttributeTable();

	this->summary_info = filename_qstr;
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	if (mean_angle > 0) {
		mean_angle_qstr = QString::number(mean_angle, 10, 1);
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	}


	//global:需要统计哪些信息,lines count,min length, max length
	this->summary_info += "Lines Count: " + QString::number(this->global_map.Attributes.AttributesDouble["ID"].size()) + "\n";
	this->summary_info += "-------------------------------------------\n";

	for (int i = 0; i < table.getNumColumns(); i++) {
		const AttributeColumn& column = table.getColumn(i);
		std::string fieldname = column.getName();
		if (fieldname == "Ref Number" || fieldname == "ID" || fieldname == "Netreach" || fieldname == "Geodesics") {
			continue;
		}

		this->summary_info += QString(QString::fromLocal8Bit(fieldname.c_str())) + "\n";
		this->summary_info += "Min: " + QString::number(column.getStats().min) + "  ";
		this->summary_info += "Max:  " + QString::number(column.getStats().max) + "  ";
		this->summary_info += "Aver: " + QString::number(column.getStats().total / table.getNumRows()) + " ";
		this->summary_info += "Total : " + QString::number(column.getStats().total) + "\n";

		this->summary_info += "-------------------------------------------\n";
	}

	this->displaySummary();
}

void MainWindow::summaryGlobal(Calculation &CA) {
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (!graph) return;
	AttributeTable& table = graph->getAttributeTable();

	this->summary_info = filename_qstr;
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	if (mean_angle > 0) {
		mean_angle_qstr = QString::number(mean_angle, 10, 1);
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	}


	//reach分析：新增或者修改属性列
	this->summary_info += "Lines Count: " + QString::number(this->global_map.Attributes.AttributesDouble["ID"].size()) + "\n";
	this->summary_info += "-------------------------------------------\n";

	for (int i = 0; i < table.getNumColumns(); i++) {
		const AttributeColumn& column = table.getColumn(i);
		std::string fieldname = column.getName();
		if (fieldname == "Ref Number" || fieldname == "ID" || fieldname == "Netreach" || fieldname == "Geodesics") {
			continue;
		}

		this->summary_info += QString(QString::fromLocal8Bit(fieldname.c_str())) + "\n";

		//对于distance分析，增加起点+Radius参数说明
		if (fieldname == "StepD_mr" || fieldname == "StepD_dr" || fieldname == "StepD_jnr") {
			this->summary_info += "Start Roads:" + this->work->ui.lineEdit_110->text() + "\n";
			//this->summary_info += "Radius(Metric):" + this->work->ui.lineEdit_119->text() + "\n";
		}

		this->summary_info += "Min: " + QString::number(column.getStats().min) + "  ";
		this->summary_info += "Max:  " + QString::number(column.getStats().max) + "  ";
		this->summary_info += "Aver: " + QString::number(column.getStats().total / table.getNumRows()) + " ";
		this->summary_info += "Total : " + QString::number(column.getStats().total) + "\n";

		this->summary_info += "-------------------------------------------\n";
	}

	this->displaySummary();
}

set<set<int>> max_connect_start;

void dfs(map<int, set<int>> &start_to_start, set<int> &path, int from) {
	if (start_to_start.count(from) == 0 || path.size() == start_to_start.size()) {
		max_connect_start.insert(path);
		return;
	}

	for (auto it = start_to_start[from].begin(); it != start_to_start[from].end(); it++) {
		int to = *it;
		if (path.count(to)) {
			continue;
		}
		path.insert(to);
		dfs(start_to_start, path, to);
		path.erase(to);
	}
}

using PIV = pair<int, vector<int>>;
vector<PIV> tree;

int find(int val) {
	int root = val;
	while (root != tree[root].first) {
		root = tree[root].first;
	}
	while (root != val) {
		val = tree[val].first;
		tree[val].first = root;
	} //路径压缩
	return val;
}

void merge(int a, int b) {
	int roota = find(a);
	int rootb = find(b);
	tree[roota].first = rootb;
}

void findMaxGraph(map<int, set<int>> &reachRoads, int max_t, map<int, set<int>> &ans) {
	tree.clear();
	ans.clear();

	tree.resize(max_t + 1);
	for (int i = 0; i < tree.size(); ++i) {
		tree[i] = { i, {} };
	}

	map<int, vector<int>> mp;
	for (auto it = reachRoads.begin(); it != reachRoads.end(); it++) {
		mp[it->first].assign(it->second.begin(), it->second.end());
	}

	for (auto& kv : mp) {
		vector<int>& temp = kv.second;
		if (temp.size() == 0) continue;
		tree[temp[0]].second.emplace_back(kv.first);
		for (int i = 1; i < temp.size(); ++i) {
			tree[temp[i]].second.emplace_back(kv.first);
			merge(temp[i - 1], temp[i]);
		}
	}

	for (auto& ele : tree) {
		if (ele.second.size())
			ans[find(ele.first)].insert(ele.second.begin(), ele.second.end());
	}
}

void dfs(map<int, set<int>> &connect, set<int> &path, int from, set<set<int>> &ans, set<int> &selected) {
	if (connect.count(from) == 0) {
		//与ans现有的path逐条检查是否有交集，如果有，则新path合并到原有path中
		bool flag = true;
		set<set<int>> new_path_all;
		for (auto it = ans.begin(); it != ans.end(); ) {
			set<int> tmp_path;
			set_intersection((*it).begin(), (*it).end(), path.begin(), path.end(), inserter(tmp_path, tmp_path.begin()));
			if (int(tmp_path.size()) > 0) {
				set<int> new_path(*it);
				new_path.insert(path.begin(), path.end());
				new_path_all.insert(new_path);
				it = ans.erase(it);	//删除非最大连通子图
				flag = false;
			}
			else
				it++;
		}
		if (flag)
			ans.insert(path);
		else {
			ans.insert(new_path_all.begin(), new_path_all.end());
		}
		selected.insert(path.begin(), path.end());
		return;
	}

	path.insert(connect[from].begin(), connect[from].end());
	for (auto it = connect[from].begin(); it != connect[from].end(); it++) {
		dfs(connect, path, *it, ans, selected);
	}
}

void mergeConnect(map<int, set<int>> &connect, set<set<int>> &ans) {
	ans.clear();
	set<int> selected;

	for (auto it = connect.begin(); it != connect.end(); it++) {
		if (selected.count(it->first))
			continue;

		set<int> path;
		path.insert(it->first);
		dfs(connect, path, it->first, ans, selected);
	}
}

bool MainWindow::judgeConnect(Calculation &CA, ShapeFileAccessor &FA, int start_road_1, int start_road_2) {
	//1.先把reach道路的交集找出来，没有就false
	set<int> inter_roads;
	set_intersection(CA.reachRoad_all[start_road_1].begin(), CA.reachRoad_all[start_road_1].end(), 
		CA.reachRoad_all[start_road_2].begin(), CA.reachRoad_all[start_road_2].end(), inserter(inter_roads, inter_roads.begin()));    /*取交集运算*/
	if (int(inter_roads.size()) == 0)
		return false;

	//2.判断是否存在完全通过的道路重合，有就返回true
	set<int> allInRoads;
	allInRoads.insert(this->net_map_all[start_road_1].AllInRoads.begin(), this->net_map_all[start_road_1].AllInRoads.end());
	allInRoads.insert(this->net_map_all[start_road_2].AllInRoads.begin(), this->net_map_all[start_road_2].AllInRoads.end());
	set<int> tmp_roads;
	set_intersection(allInRoads.begin(), allInRoads.end(), inter_roads.begin(), inter_roads.end(), inserter(tmp_roads, tmp_roads.begin()));
	if (int(tmp_roads.size()) > 0)
		return true;

	//3.判断是否存在部分通过的道路重合，没有就false
	set<int> partInRoads;
	partInRoads.insert(this->net_map_all[start_road_1].PartInRoads.begin(), this->net_map_all[start_road_1].PartInRoads.end());
	partInRoads.insert(this->net_map_all[start_road_2].PartInRoads.begin(), this->net_map_all[start_road_2].PartInRoads.end());
	tmp_roads.clear();
	set_intersection(partInRoads.begin(), partInRoads.end(), inter_roads.begin(), inter_roads.end(), inserter(tmp_roads, tmp_roads.begin()));
	if (int(tmp_roads.size()) == 0)
		return false;

	//4.判断是否能够接触，接触能true
	map<int, map<int, double>> partRoadLens; //第一个key是road，第二个key是道路端点
	for (auto it = tmp_roads.begin(); it != tmp_roads.end();it++) {
		for (auto node_iter = CA.partInReachRoadNodeLen[start_road_1][*it].begin(); 
			node_iter != CA.partInReachRoadNodeLen[start_road_1][*it].end(); node_iter++) {
			int node = node_iter->first;
			double len = node_iter->second;
			if (partRoadLens[*it][node] < len)
				partRoadLens[*it][node] = len;
		}
		for (auto node_iter = CA.partInReachRoadNodeLen[start_road_2][*it].begin();
			node_iter != CA.partInReachRoadNodeLen[start_road_2][*it].end(); node_iter++) {
			int node = node_iter->first;
			double len = node_iter->second;
			if (partRoadLens[*it][node] < len)
				partRoadLens[*it][node] = len;
		}
	}
	//计算部分通过的路
	for (auto iter = partRoadLens.begin(); iter != partRoadLens.end(); iter++) {
		double sub_sum = 0;
		for (auto tmp_iter = iter->second.begin(); tmp_iter != iter->second.end(); tmp_iter++) {
			sub_sum += tmp_iter->second;
		}
		if (sub_sum >= FA.Length[iter->first])
			return true;
	}

	return false;
}

void MainWindow::findConnect(Calculation &CA, ShapeFileAccessor &FA, map<int, set<int>> &connect) {
	connect.clear();

	vector<int> startRoads;
	for (auto it = CA.reachRoad_all.begin(); it != CA.reachRoad_all.end(); it++) {
		startRoads.push_back(it->first);
	}

	//严格按从小到大顺序计算和存储
	for (int i = 0; i < int(startRoads.size()); i++) {
		int start_road_1 = startRoads[i];
		for (int j = i + 1; j<int(startRoads.size()); j++) {
			int start_road_2 = startRoads[j];

			if (judgeConnect(CA, FA, start_road_1, start_road_2))
				connect[start_road_1].insert(start_road_2);
		}
	}
}

void MainWindow::clearOldData() {
	lines_count = INT_MAX - 1;
	extern std::map<std::string, Point2f> RealMap;
	RealMap.clear();

	//鼠标select
	Ref_number_list.clear();
	ref_numer = "";
	ref_numer_last = "";

	//线条粗细控制
	std::map<std::string, std::map<float, set<int>>> FixedThickLinesMapAll;
	std::map<float, set<int>> FixedThickLinesMap;	//key为粗细，value为线条序列
	const float FixedThick = 1.0;

	//线条颜色控制
	FixedColoredLinesMapAll.clear();
	NetFixedColoredLinesMap.clear();
	GeoFixedColoredLinesMap.clear();
	FixedColoredLinesMap.clear();

	//线条粗细控制
	FixedThickLinesMapAll.clear();
	NetFixedThickLinesMap.clear();
	GeoFixedThickLinesMap.clear();
	FixedThickLinesMap.clear();
	ID_to_width.clear();

	//属性
	AttributesVec.clear();
	Selected_list.clear();
	changeHappen = false;
	loadedCount = 0;
	mapViewName = "";

	//类里面的东西
	locationLabel->setText("Ready");
	m_pConnectProBar->setValue(0);
	this->work->ui.lineEdit->setText("auto");

	CA.clearOldData();
	CA.clearNGdata();
	FA.clearOldData();

	T_timeBegin = 0, T_timeEnd = 0;
	global_view = false, netreach_view = false, geodesics_view = false;
	global_file_path = "global.txt", netreach_file_path = "netreach.txt", geodesics_file_path = "geodesics.txt";
	Global_Ref_to_Id.clear();	//value=<Ref, id>
	Global_Id_to_Ref.clear();
	Netreach_Ref_to_Id.clear();
	Netreach_Id_to_Ref.clear();
	Geodesics_Ref_to_Id.clear();
	Geodesics_Id_to_Ref.clear();
	Ref_to_Id.clear();
	Id_to_Ref.clear();

	loadFilePathOver = false;

	ColNames.clear();

	//data
	display.clearOldData();
	global_map.clearOldData();
	NetFromIDs.clear();
	GeoFromIDs.clear();
	GeoToIDs.clear();
	NetRoadsAll.clear();
	GeoRoadsAll.clear();
	NetInfo = "";
	GeoInfo = "";
	net_map_all.clear();	//key是起点坐标
	geo_map_all.clear();	//key是起点坐标

	//信息栏
	summary_info = "";
	this->work->ui.textBrowser->setText("");

	//GL窗口
	auto mowWindow = mdiArea->currentSubWindow();
	if(mowWindow)
		mdiArea->removeSubWindow(mowWindow);
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

void MainWindow::summarySubset(Calculation &CA, ShapeFileAccessor &FA) {
	this->summary_info = filename_qstr;
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	if (mean_angle > 0) {
		mean_angle_qstr = QString::number(mean_angle, 10, 1);
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	}
	//this->summary_info += "SUMMARY\n";
	//this->summary_info += "------------------------------------\n";

	int max_count = 40;
	if (this->work->ui.radioButton_33->isChecked()) {	//单组子集线
		//子集线数目与ID序列
		this->summary_info += "Measures for Single Subsets\n\n";
		this->summary_info += "Subset Lines: " + QString::number(this->SubsetIDs[1].size()) + " lines\n";
		int line_count = 0;
		for (auto road_it = this->SubsetIDs[1].begin(); road_it != this->SubsetIDs[1].end(); road_it++) {
			if (road_it == this->SubsetIDs[1].begin())
				this->summary_info += QString::number(*road_it);
			else
				this->summary_info += "," + QString::number(*road_it);
			if (line_count >= max_count) {
				this->summary_info += ",...";
				break;
			}
			++line_count;
		}
		this->summary_info += "\n";

		//项目一结果
		this->summary_info += "------------------------------------\n";
		this->summary_info += "Nearest Distance to the Subset:\n";
		if (subset_para.isDR) this->summary_info += "MeanDD = " + QString::number(CA.results.result_1.meanDD_1) + "\n";
		if (subset_para.isDR) this->summary_info += "MeanDDL = " + QString::number(CA.results.result_1.meanDDL_1) + "\n";
		if (subset_para.isMR) this->summary_info += "MeanMD = " + QString::number(CA.results.result_1.meanMD_1) + "\n";
		if (subset_para.isJnR) this->summary_info += "MeanJncD = " + QString::number(CA.results.result_1.meanJnCD_1) + "\n";

		//项目二结果
		this->summary_info += "------------------------------------\n";
		this->summary_info += "Subset Collective Reach:\n";
		if (subset_para.isMR) this->summary_info += "MR total lines length = " + QString::number(CA.results.result_1.sumMR_2) + "\n";
		if (subset_para.isDR) this->summary_info += "DR total lines length = " + QString::number(CA.results.result_1.sumDR_2) + "\n";
		if (subset_para.isJnR) this->summary_info += "JnR total lines length = " + QString::number(CA.results.result_1.sumJncR_2) + "\n";
		if (subset_para.isMDR) this->summary_info += "MDR total lines length = " + QString::number(CA.results.result_1.sumMDR_2) + "\n";

		//项目三结果
		this->summary_info += "------------------------------------\n";
		this->summary_info += "Members within the Subset:\n";
		if (subset_para.isDR) this->summary_info += "MeanDD = " + QString::number(CA.results.result_1.meanDD_3) + "\n";
		if (subset_para.isDR) this->summary_info += "MeanDDL = " + QString::number(CA.results.result_1.meanDDL_3) + "\n";
		if (subset_para.isMR) this->summary_info += "MeanMD = " + QString::number(CA.results.result_1.meanMD_3) + "\n";
		if (subset_para.isJnR) this->summary_info += "MeanJncD = " + QString::number(CA.results.result_1.meanJnCD_3) + "\n";
	}
	else if (this->work->ui.radioButton_34->isChecked()) {	//双组子集线
		//子集线数目与ID序列
		this->summary_info += "Measures for Multiple Subsets\n\n";
		//this->summary_info += "Subsets Number: " + QString::number(this->SubsetIDs.size()) + "\n";
		for (auto it = this->SubsetIDs.begin(); it != this->SubsetIDs.end(); it++) {
			int pos = it->first;
			this->summary_info += "Subset_" + QString::number(pos) + " Lines: " + QString::number(this->SubsetIDs[pos].size()) + " lines\n";
			int line_count = 0;
			for (auto road_it = this->SubsetIDs[pos].begin(); road_it != this->SubsetIDs[pos].end(); road_it++) {
				if (road_it == this->SubsetIDs[pos].begin())
					this->summary_info += QString::number(*road_it);
				else
					this->summary_info += "," + QString::number(*road_it);
				if (line_count >= max_count) {
					this->summary_info += ",...";
					break;
				}
				++line_count;
			}
			this->summary_info += "\n";
		}

		//项目一结果
		this->summary_info += "------------------------------------\n";
		this->summary_info += "Members between Subsets:\n";
		//输出所有两两组合下的计算指标
		std::set<std::pair<int, int>> pairs = GetAllPairs(this->SubsetIDs.size());
		for (auto it = pairs.begin(); it != pairs.end(); it++) {
			std::pair<int, int> pair = *it;
			if (subset_para.isDR) this->summary_info += "(" + QString::number(pair.first) + "->" + QString::number(pair.second) + ") MeanDD = " + QString::number(CA.results.result_2.result_1[pair].meanDD) + "\n";
			if (subset_para.isDR) this->summary_info += "(" + QString::number(pair.first) + "->" + QString::number(pair.second) + ") MeanDDL = " + QString::number(CA.results.result_2.result_1[pair].meanDDL) + "\n";
			if (subset_para.isMR) this->summary_info += "(" + QString::number(pair.first) + "->" + QString::number(pair.second) + ") MeanMD = " + QString::number(CA.results.result_2.result_1[pair].meanMD) + "\n";
			if (subset_para.isJnR) this->summary_info += "(" + QString::number(pair.first) + "->" + QString::number(pair.second) + ") MeanJncD = " + QString::number(CA.results.result_2.result_1[pair].meanJnCD) + "\n";
			this->summary_info += "\n";
		}

		//项目二结果
		this->summary_info += "------------------------------------\n";
		this->summary_info += "Nearest Distance to Other Subsets:\n";
		//输出所有两两组合下的计算指标
		for (auto it = pairs.begin(); it != pairs.end(); it++) {
			std::pair<int, int> pair = *it;
			if (subset_para.isDR) this->summary_info += "(" + QString::number(pair.first) + "->" + QString::number(pair.second) + ") MeanDD = " + QString::number(CA.results.result_2.result_2[pair].meanDD) + "\n";
			if (subset_para.isDR) this->summary_info += "(" + QString::number(pair.first) + "->" + QString::number(pair.second) + ") MeanDDL = " + QString::number(CA.results.result_2.result_2[pair].meanDDL) + "\n";
			if (subset_para.isMR) this->summary_info += "(" + QString::number(pair.first) + "->" + QString::number(pair.second) + ") MeanMD = " + QString::number(CA.results.result_2.result_2[pair].meanMD) + "\n";
			if (subset_para.isJnR) this->summary_info += "(" + QString::number(pair.first) + "->" + QString::number(pair.second) + ") MeanJncD = " + QString::number(CA.results.result_2.result_2[pair].meanJnCD) + "\n";
			this->summary_info += "\n";
		}
	}
	else if (this->work->ui.radioButton_35->isChecked()) {	//整体子集线
		this->summary_info += "Measures for a system as a whole\n\n";

		if (subset_para.isDR) this->summary_info += "MeanDD = " + QString::number(CA.results.result_3.meanDD) + "\n";
		if (subset_para.isDR) this->summary_info += "MeanDDL = " + QString::number(CA.results.result_3.meanDDL) + "\n";
		if (subset_para.isMR) this->summary_info += "MeanMD = " + QString::number(CA.results.result_3.meanMD) + "\n";
		if (subset_para.isJnR) this->summary_info += "MeanJncD = " + QString::number(CA.results.result_3.meanJnCD) + "\n";
	}

	this->displaySummary();
}

void MainWindow::summaryNet(Calculation &CA, ShapeFileAccessor &FA) {
	this->summary_info = filename_qstr;
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	if (mean_angle > 0) {
		mean_angle_qstr = QString::number(mean_angle, 10, 1);
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	}
	//this->summary_info += "SUMMARY\n";
	//this->summary_info += "------------------------------------\n";

	//netreach:按startRoad划分区域
	for (auto it = CA.reachRoad_all.begin(); it != CA.reachRoad_all.end(); it++) {
		this->summary_info += "From ID: " + QString::number(it->first) + "\n";
		double rate = double(it->second.size()) / max(double(FA.roadID.size()),1.0) * 100;
		this->summary_info += "Reached lines count: " + QString::number(it->second.size()) + "(" + QString::number(rate) + "%)\n";

		//计算Netreach总距离
		double sum = 0;
		for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
			int endRoad = *iter;

			if (CA.partInReachRoadNodeLen[it->first].count(endRoad)) {
				for (auto tt = CA.partInReachRoadNodeLen[it->first][endRoad].begin(); tt != CA.partInReachRoadNodeLen[it->first][endRoad].end(); tt++)
					sum += tt->second;
			}
			else {
				sum += FA.Length[endRoad];
			}
		}
		rate = sum / max(FA.length_all,1.0) * 100;
		this->summary_info += "Reached line length: " + QString::number(sum) + "(" + QString::number(rate) + "%)\n";

		//判断可选项jnc、wgt是否开启
		if (this->work->ui.radioButton_48->isChecked()) {
			if (this->work->ui.radioButton_43->isChecked() || this->work->ui.lineEdit_77->text().size() > 0) {
				//int jnc = CA.CalculateJnc(FA, CA.reachRoad_all[it->first]);
				int jnc = CA.CalculateJncByPart(FA, CA.reachRoad_all[it->first], CA.partInReachRoadNodeLen[it->first]);
				rate = double(jnc) / max(double(FA.jnc_all), 1.0) * 100;
				this->summary_info += "Reached junction count: " + QString::number(jnc) + "(" + QString::number(rate) + "%)\n";
			}
			if (weightAttributesSet.size() > 0) {
				this->summary_info += "Reached line Weight: \n";
				std::map<std::string, double> wgtvalue = CA.CalculateWgtNet(FA, CA.weight, CA.reachRoad_all[it->first], CA.partInReachRoadLen[it->first]);
				for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
					double weight_all = 0;
					for (auto road_it = FA.roadID.begin(); road_it != FA.roadID.end(); road_it++) {
						weight_all += CA.weight[wgt_it->first][*road_it];
					}
					rate = wgtvalue[wgt_it->first] / max(weight_all, 1.0) * 100;
					this->summary_info += "        " + QString(QString::fromLocal8Bit(wgt_it->first.c_str())) + ": " +
						QString::number(wgtvalue[wgt_it->first]) + "(" + QString::number(rate) + "%)\n";
				}
				
			}
		}

		this->summary_info += "------------------------------------\n";
	}

	//对MR/DR/JnR/MDR三种方式应该分成两类讨论：有部分通过的MR/MDR，只有完全通过的DR/JnR
	if (this->work->ui.radioButton_41->isChecked() || this->work->ui.radioButton_44->isChecked()) {	//MR/MDR
		//TODO: 找出跟每个集合直接相连的集合序列
		map<int, set<int>> connect;
		findConnect(CA, FA, connect);
		//TODO: 划分各个区域
		set<set<int>> ans;
		mergeConnect(connect, ans);

		//把连通子图中只有1个起点的剔除
		for (auto it = ans.begin(); it != ans.end(); ) {
			if ((*it).size() == 1) {
				it = ans.erase(it);
			}
			else
				it++;
		}

		//对多个起点reach范围有交集的进行处理：计算lenth/Dc/Jnc
		for (auto it = ans.begin(); it != ans.end(); it++) {
			set<int> startRoads(*it);

			//需要计算的数据：len
			int roadNumber = 0;
			double len = 0;
			int jnc = 0;
			double wgt = 0;

			//计算非重复的线条id集合
			set<int> roads;
			for (auto iter = startRoads.begin(); iter != startRoads.end(); iter++) {
				roads.insert(CA.reachRoad_all[*iter].begin(), CA.reachRoad_all[*iter].end());
			}
			roadNumber = roads.size();

			//划分出非重复的完全通过、部分通过的道路，对于部分通过的道路，需要记录道路起点序号
			set<int> allInRoads;
			for (auto iter = this->net_map_all.begin(); iter != this->net_map_all.end(); iter++) {
				if (startRoads.count(iter->first) == 0)
					continue;
				allInRoads.insert(iter->second.AllInRoads.begin(), iter->second.AllInRoads.end());
			}
			map<int, map<int, double>> partRoadLens; //第一个key是road，第二个key是道路端点
			std::map<std::string, double> wgtvalue;
			for (auto iter = this->net_map_all.begin(); iter != this->net_map_all.end(); iter++) {
				int startRoad = iter->first;
				if (startRoads.count(startRoad) == 0)
					continue;
				for (auto tmp_iter = iter->second.PartInRoadsCoordinate.begin(); tmp_iter != iter->second.PartInRoadsCoordinate.end(); tmp_iter++) {
					int endRoad = tmp_iter->first;
					if (allInRoads.count(endRoad))
						continue;

					for (auto node_iter = CA.partInReachRoadNodeLen[startRoad][endRoad].begin();
						node_iter != CA.partInReachRoadNodeLen[startRoad][endRoad].end(); node_iter++) {
						int node = node_iter->first;
						double len = node_iter->second;
						if (partRoadLens[endRoad][node] < len)
							partRoadLens[endRoad][node] = len;
					}
				}
			}
			//计算部分通过的路
			for (auto iter = partRoadLens.begin(); iter != partRoadLens.end(); iter++) {
				double sub_sum = 0;
				for (auto tmp_iter = iter->second.begin(); tmp_iter != iter->second.end(); tmp_iter++) {
					sub_sum += tmp_iter->second;
				}
				if (sub_sum >= FA.Length[iter->first]) {
					len += FA.Length[iter->first];
					for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
						wgtvalue[wgt_it->first] += CA.weight[wgt_it->first][iter->first];
					}
				}
				else {
					len += sub_sum;
					double rate = sub_sum / FA.Length[iter->first];
					for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
						wgtvalue[wgt_it->first] += CA.weight[wgt_it->first][iter->first];
					}
				}
			}
			//计算全部通过的路
			for (auto iter = allInRoads.begin(); iter != allInRoads.end(); iter++) {
				len += FA.Length[*iter];
				for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
					wgtvalue[wgt_it->first] += CA.weight[wgt_it->first][*iter];
				}
			}

			//判断是否需要计算Jnc、wgt
			if (this->work->ui.radioButton_48->isChecked()) {
				jnc = CA.CalculateJncByPart(FA, roads, partRoadLens);
			}

			//打印输出
			this->summary_info += "From ID: ";
			for (auto iter = startRoads.begin(); iter != startRoads.end(); iter++) {
				if (iter == startRoads.begin())
					this->summary_info += QString::number(*iter);
				else
					this->summary_info += "," + QString::number(*iter);
			}
			this->summary_info += "\n";
			double rate = double(roadNumber) / max(double(FA.roadID.size()), 1.0) * 100;
			this->summary_info += "Reached lines count: " + QString::number(roadNumber) + "(" + QString::number(rate) + "%)\n";
			rate = len / max(FA.length_all, 1.0) * 100;
			this->summary_info += "Reached line length: " + QString::number(len) + "(" + QString::number(rate) + "%)\n";
			if (this->work->ui.radioButton_48->isChecked()) {
				if (this->work->ui.radioButton_43->isChecked() || this->work->ui.lineEdit_77->text().size() > 0) {
					rate = double(jnc) / max(double(FA.jnc_all), 1.0) * 100;
					this->summary_info += "Reached junction count: " + QString::number(jnc) + "(" + QString::number(rate) + "%)\n";
				}
				if (weightAttributesSet.size() > 0) {
					this->summary_info += "Reached line Weight: \n";
					//std::map<std::string, double> wgtvalue = CA.CalculateWgtNet(FA, CA.weight, CA.reachRoad_all[it->first], CA.partInReachRoadLen[it->first]);
					for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
						double weight_all = 0;
						for (auto road_it = FA.roadID.begin(); road_it != FA.roadID.end(); road_it++) {
							weight_all += CA.weight[wgt_it->first][*road_it];
						}
						rate = wgtvalue[wgt_it->first] / max(weight_all, 1.0) * 100;
						this->summary_info += "        " + QString(QString::fromLocal8Bit(wgt_it->first.c_str())) + ": " +
							QString::number(wgtvalue[wgt_it->first]) + "(" + QString::number(rate) + "%)\n";
					}
				}
			}	
			this->summary_info += "------------------------------------\n";
		}
	}
	else if (this->work->ui.radioButton_42->isChecked() || this->work->ui.radioButton_43->isChecked()) { //DR/JnR
		//找出连通的集合序列
		map<int, set<int>> ans;
		findMaxGraph(CA.reachRoad_all, this->global_map.RecordCount, ans);

		//把连通子图中只有1个起点的剔除
		for (auto it = ans.begin(); it != ans.end(); ) {
			if (it->second.size() == 1) {
				it = ans.erase(it);
			}
			else
				it++;
		}

		//对多个起点reach范围有交集的进行处理：计算lenth/Dc/Jnc
		for (auto it = ans.begin(); it != ans.end(); it++) {
			set<int> startRoads(it->second);

			//需要计算的数据：len
			int roadNumber = 0;
			double len = 0;
			int jnc = 0;

			//计算非重复的线条id集合
			set<int> roads;
			for (auto iter = startRoads.begin(); iter != startRoads.end(); iter++) {
				roads.insert(CA.reachRoad_all[*iter].begin(), CA.reachRoad_all[*iter].end());
			}
			roadNumber = roads.size();

			for (auto iter = roads.begin(); iter != roads.end(); iter++) {
				len += FA.Length[*iter];
			}

			//判断是否需要计算Jnc、wgt
			std::map<std::string, double> wgtvalue;
			if (this->work->ui.radioButton_48->isChecked()) {
				jnc = CA.CalculateJnc(FA, roads);
				for (auto iter = roads.begin(); iter != roads.end(); iter++) {
					for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
						wgtvalue[wgt_it->first] += CA.weight[wgt_it->first][*iter];
					}
				}
			}

			//打印输出
			this->summary_info += "From ID: ";
			for (auto iter = startRoads.begin(); iter != startRoads.end(); iter++) {
				if (iter == startRoads.begin())
					this->summary_info += QString::number(*iter);
				else
					this->summary_info += "," + QString::number(*iter);
			}
			this->summary_info += "\n";
			double rate = double(roadNumber) / max(double(FA.roadID.size()), 1.0) * 100;
			this->summary_info += "Reached lines count: " + QString::number(roadNumber) + "(" + QString::number(rate) + "%)\n";
			rate = len / max(FA.length_all, 1.0) * 100;
			this->summary_info += "Reached line length: " + QString::number(len) + "(" + QString::number(rate) + "%)\n";
			if (this->work->ui.radioButton_48->isChecked()) {
				if (this->work->ui.radioButton_43->isChecked() || this->work->ui.lineEdit_77->text().size() > 0) {
					rate = double(jnc) / max(double(FA.jnc_all), 1.0) * 100;
					this->summary_info += "Reached junction count: " + QString::number(jnc) + "(" + QString::number(rate) + "%)\n";
				}
				if (weightAttributesSet.size() > 0) {
					this->summary_info += "Reached line Weight: \n";
					//std::map<std::string, double> wgtvalue = CA.CalculateWgtNet(FA, CA.weight, CA.reachRoad_all[it->first], CA.partInReachRoadLen[it->first]);
					for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
						double weight_all = 0;
						for (auto road_it = FA.roadID.begin(); road_it != FA.roadID.end(); road_it++) {
							weight_all += CA.weight[wgt_it->first][*road_it];
						}
						rate = wgtvalue[wgt_it->first] / max(weight_all, 1.0) * 100;
						this->summary_info += "        " + QString(QString::fromLocal8Bit(wgt_it->first.c_str())) + ": " +
							QString::number(wgtvalue[wgt_it->first]) + "(" + QString::number(rate) + "%)\n";
					}
				}
			}
			this->summary_info += "------------------------------------\n";
		}
	}

	NetInfo = this->summary_info;
	this->displaySummary();
}

void MainWindow::summaryGeo(Calculation &CA, ShapeFileAccessor &FA) {
	this->summary_info = filename_qstr;
	double mean_angle = round(FA.get_Mean_Angle_of_Deviation() * 10) / 10.0;
	if (mean_angle > 0) {
		mean_angle_qstr = QString::number(mean_angle, 10, 1);
		this->summary_info += "Mean Angle of Deviation = " + mean_angle_qstr + "\n\n";
	}
	//this->summary_info += "SUMMARY\n";
	//this->summary_info += "------------------------------------\n";

	//geodesics:按startRoad划分区域
	for (auto it = CA.routeRoad_all.begin(); it != CA.routeRoad_all.end(); it++) {
		this->summary_info += "From ID: " + QString::number(it->first) + "\n";
		this->summary_info += "To ID: ";
		for (auto iter = CA.ToIDVec.begin(); iter != CA.ToIDVec.end(); iter++) {
			if (iter == CA.ToIDVec.begin())
				this->summary_info += QString::number(*iter);
			else
				this->summary_info += "," + QString::number(*iter);
		}
		this->summary_info += "\n";
		double rate = double(it->second.size()) / max(double(FA.roadID.size()), 1.0) * 100;
		this->summary_info += "Path lines count: " + QString::number(it->second.size()) + "(" + QString::number(rate) + "%)\n";

		//计算reach总距离
		double sum = 0;
		std::map<std::string, double> wgtvalue;
		for (auto iter = it->second.begin(); iter != it->second.end(); iter++) {
			sum += FA.Length[*iter];
			if (weightAttributesSet.size() > 0) {
				for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
					wgtvalue[wgt_it->first] += CA.weight[wgt_it->first][*iter];
				}
			}
		}
		rate = sum / max(FA.length_all, 1.0) * 100;
		this->summary_info += "Path line length: " + QString::number(sum) + "(" + QString::number(rate) + "%)\n";

		//判断可选项dc、dd、ddl、wdd、jnc、wgt是否开启
		if (this->work->ui.radioButton_30->isChecked()) {
			for (auto iter = CA.GeoDataCount[it->first].begin(); iter != CA.GeoDataCount[it->first].end(); iter++) {
				if (iter->first == "DD" || iter->first == "DDL" || iter->first == "WDD")
					continue;
				if (iter->first == "DC" && (this->work->ui.lineEdit_79->text().size() > 0 || this->work->ui.radioButton_22->isChecked())) {
					this->summary_info += "Path Directional Changes: " + QString::number(iter->second) + "\n";
				}	
				else if (iter->first == "Jnc"  && (this->work->ui.lineEdit_80->text().size() > 0 || this->work->ui.radioButton_23->isChecked() )) {
					rate = double(iter->second) / max(double(FA.jnc_all), 1.0) * 100;
					this->summary_info += "Path junction count: " + QString::number(iter->second) + "(" + QString::number(rate) + "%)\n";
				}					
			}
			if (weightAttributesSet.size() > 0) {
				/*rate = double(iter->second) / max(double(CA.weight_all), 1.0) * 100;
				this->summary_info += "Path weight: " + QString::number(iter->second) + "(" + QString::number(rate) + "%)\n";*/
				this->summary_info += "Path weight: \n";
				for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
					double weight_all = 0;
					for (auto road_it = FA.roadID.begin(); road_it != FA.roadID.end(); road_it++) {
						weight_all += CA.weight[wgt_it->first][*road_it];
					}
					rate = wgtvalue[wgt_it->first] / max(weight_all, 1.0) * 100;
					this->summary_info += "        " + QString(QString::fromLocal8Bit(wgt_it->first.c_str())) + ": " +
						QString::number(wgtvalue[wgt_it->first]) + "(" + QString::number(rate) + "%)\n";
				}
			}
		}

		this->summary_info += "------------------------------------\n";
	}

	//找出连通的集合序列
	map<int, set<int>> ans;
	findMaxGraph(CA.routeRoad_all, this->global_map.RecordCount, ans);

	//把连通子图中只有1个起点的剔除
	for (auto it = ans.begin(); it != ans.end(); ) {
		if (it->second.size() == 1) {
			it = ans.erase(it);
		}
		else
			it++;
	}

	//对多个起点reach范围有交集的进行处理：计算lenth/Dc/Jnc
	for (auto it = ans.begin(); it != ans.end(); it++) {
		set<int> startRoads(it->second);

		//需要计算的数据：len
		int roadNumber = 0;
		double len = 0;
		int jnc = 0;
		double wgt = 0;

		set<int> roads;
		for (auto iter = startRoads.begin(); iter != startRoads.end(); iter++) {
			roads.insert(CA.routeRoad_all[*iter].begin(), CA.routeRoad_all[*iter].end());
		}
		roadNumber = roads.size();
		for (auto iter = roads.begin(); iter != roads.end(); iter++) {
			len += FA.Length[*iter];
		}

		//判断是否需要计算Jnc、wgt
		std::map<std::string, double> wgtvalue;
		if (this->work->ui.radioButton_30->isChecked()) {
			jnc = CA.CalculateJnc(FA, roads);
			for (auto iter = roads.begin(); iter != roads.end(); iter++) {
				for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
					wgtvalue[wgt_it->first] += CA.weight[wgt_it->first][*iter];
				}
			}
		}

		//打印输出
		this->summary_info += "From ID: ";
		for (auto iter = startRoads.begin(); iter != startRoads.end(); iter++) {
			if (iter == startRoads.begin())
				this->summary_info += QString::number(*iter);
			else
				this->summary_info += "," + QString::number(*iter);
		}
		this->summary_info += "\n";
		this->summary_info += "To ID: ";
		for (auto iter = CA.ToIDVec.begin(); iter != CA.ToIDVec.end(); iter++) {
			if (iter == CA.ToIDVec.begin())
				this->summary_info += QString::number(*iter);
			else
				this->summary_info += "," + QString::number(*iter);
		}
		this->summary_info += "\n";
		double rate = double(roadNumber) / max(double(FA.roadID.size()), 1.0) * 100;
		this->summary_info += "Path lines count: " + QString::number(roadNumber) + "(" + QString::number(rate) + "%)\n";
		rate = len / max(FA.length_all, 1.0) * 100;
		this->summary_info += "Path line length: " + QString::number(len) + "(" + QString::number(rate) + "%)\n";
		if (this->work->ui.radioButton_30->isChecked()) {
			if (this->work->ui.lineEdit_80->text().size() > 0 || this->work->ui.radioButton_23->isChecked()) {
				rate = double(jnc) / max(double(FA.jnc_all), 1.0) * 100;
				this->summary_info += "Path junction count: " + QString::number(jnc) + "(" + QString::number(rate) + "%)\n";
			}
			if (weightAttributesSet.size() > 0) {
				this->summary_info += "Path weight: \n";
				for (auto wgt_it = CA.weight.begin(); wgt_it != CA.weight.end(); wgt_it++) {
					double weight_all = 0;
					for (auto road_it = FA.roadID.begin(); road_it != FA.roadID.end(); road_it++) {
						weight_all += CA.weight[wgt_it->first][*road_it];
					}
					rate = wgtvalue[wgt_it->first] / max(weight_all, 1.0) * 100;
					this->summary_info += "        " + QString(QString::fromLocal8Bit(wgt_it->first.c_str())) + ": " +
						QString::number(wgtvalue[wgt_it->first]) + "(" + QString::number(rate) + "%)\n";
				}
			}
		}
		this->summary_info += "------------------------------------\n";
	}

	GeoInfo = this->summary_info;
	this->displaySummary();
}

void MainWindow::OnSelchangingList()
{
	if (in_FocusGraph) return;

	int row = -1;
	row = m_attrWindow->currentRow();
	if (row > -1 && m_treeDoc) {
		MetaGraph *graph = m_treeDoc->m_meta_graph;
		if (graph->viewingProcessed()) {
			graph->setDisplayedAttribute(row - 1);
			UpdateColorMap();
		}
		m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_FOCUS);
		SetAttributeChecks();
		OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE); // Bug Test TV
	}

	// this *does* work here (but only if they click on a valid attribute):
}

void MainWindow::OnSelchangingTree(QTreeWidgetItem* hItem, int col)
{
	if (in_FocusGraph) return;

	MetaGraph *graph = m_treeDoc->m_meta_graph;
	bool update = false;

	// look it up in the table to see what to do:
	auto iter = m_treegraphmap.find(hItem);
	if (iter != m_treegraphmap.end()) {
		ItemTreeEntry entry = iter->second;
		bool remenu = false;
		if (entry.m_cat != -1) {
			if (entry.m_subcat == -1 && m_indexWidget->isMapColumn(col)) {
				switch (entry.m_type) {
				case 0:
					if (graph->getViewClass() & MetaGraph::VIEWVGA) {
						if (graph->getDisplayedPointMapRef() == entry.m_cat) {
							graph->setViewClass(MetaGraph::SHOWHIDEVGA);
						}
						else {
							graph->setDisplayedPointMapRef(entry.m_cat);
						}
					}
					else {
						graph->setDisplayedPointMapRef(entry.m_cat);
						graph->setViewClass(MetaGraph::SHOWVGATOP);
					}
					remenu = true;
					break;
				case 1:
					if (graph->getViewClass() & MetaGraph::VIEWAXIAL) {
						if (graph->getDisplayedShapeGraphRef() == entry.m_cat) {
							graph->setViewClass(MetaGraph::SHOWHIDEAXIAL);
						}
						else {
							graph->setDisplayedShapeGraphRef(entry.m_cat);
						}
					}
					else {
						graph->setDisplayedShapeGraphRef(entry.m_cat);
						graph->setViewClass(MetaGraph::SHOWAXIALTOP);
					}
					remenu = true;
					break;
				case 2:
					if (graph->getViewClass() & MetaGraph::VIEWDATA) {
						if (graph->getDisplayedDataMapRef() == entry.m_cat) {
							graph->setViewClass(MetaGraph::SHOWHIDESHAPE);
						}
						else {
							graph->setDisplayedDataMapRef(entry.m_cat);
						}
					}
					else {
						graph->setDisplayedDataMapRef(entry.m_cat);
						graph->setViewClass(MetaGraph::SHOWSHAPETOP);
					}
					remenu = true;
					break;
				case 4:
					// slightly different for this one
					break;
				}
				if (remenu) {
					SetGraphTreeChecks();
					m_treeDoc->SetRemenuFlag(QGraphDoc::VIEW_ALL, true);
					OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);
				}
				m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_TABLE);
			}
			else if (entry.m_subcat == -1 && m_indexWidget->isEditableColumn(col)) {
				// hit editable box
				if (entry.m_type == 1) {
					int type = graph->getShapeGraphs()[entry.m_cat]->getMapType();
					if (type != ShapeMap::SEGMENTMAP && type != ShapeMap::ALLLINEMAP) {
						graph->getShapeGraphs()[entry.m_cat]->setEditable(m_indexWidget->isItemSetEditable(hItem));
						update = true;
					}
				}
				if (entry.m_type == 2) {
					graph->getDataMaps()[entry.m_cat].setEditable(m_indexWidget->isItemSetEditable(hItem));
					update = true;
				}
				if (update) {
					// Depending on if the map is displayed you may have to redraw -- I'm just going to redraw *anyway*
					// (it may be worth switching it to topmost when they do click here)
					OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);
				}
			}
			else {
				// They've clicked on the displayed layers
				if (entry.m_type == 1) {
					update = true;
					graph->getShapeGraphs()[entry.m_cat]->setLayerVisible(entry.m_subcat, m_indexWidget->isItemSetVisible(hItem));
				}
				else if (entry.m_type == 2) {
					update = true;
					graph->getDataMaps()[entry.m_cat].setLayerVisible(entry.m_subcat, m_indexWidget->isItemSetVisible(hItem));
				}
				if (update) {
					m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_TABLE);
					OnFocusGraph(m_treeDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);
				}
			}
		}
	}
	else {
		auto iter = m_treedrawingmap.find(hItem);
		if (iter != m_treedrawingmap.end()) {
			ItemTreeEntry entry = iter->second;
			if (entry.m_subcat != -1) {
				if (graph->getLineLayer(entry.m_cat, entry.m_subcat).isShown()) {
					graph->getLineLayer(entry.m_cat, entry.m_subcat).setShow(false);
					graph->redoPointMapBlockLines();
					graph->resetBSPtree();
				}
				else {
					graph->getLineLayer(entry.m_cat, entry.m_subcat).setShow(true);
					graph->redoPointMapBlockLines();
					graph->resetBSPtree();
				}
			}
			m_treeDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_LINESET);
		}
	}
}

void MainWindow::SetGraphTreeChecks()
{
	in_FocusGraph = true;
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	int viewclass = graph->getViewClass();
	for (auto item : m_treegraphmap) {
		QTreeWidgetItem* key = item.first;
		ItemTreeEntry entry = item.second;
		int checkstyle = 7;
		if (entry.m_cat != -1) {
			if (entry.m_subcat == -1) {
				// this is the main type box hit
				switch (entry.m_type) {
				case 0:
					if (viewclass & MetaGraph::VIEWVGA && graph->getDisplayedPointMapRef() == entry.m_cat) {
						checkstyle = 7;
						m_topgraph = key;
					}
					else if (viewclass & MetaGraph::VIEWBACKVGA && graph->getDisplayedPointMapRef() == entry.m_cat) {
						checkstyle = 7;
						m_backgraph = key;
					}
					break;
				case 1:
					if (viewclass & MetaGraph::VIEWAXIAL && graph->getDisplayedShapeGraphRef() == entry.m_cat) {
						checkstyle = 7;
						m_topgraph = key;
					}
					else if (viewclass & MetaGraph::VIEWBACKAXIAL && graph->getDisplayedShapeGraphRef() == entry.m_cat) {
						checkstyle = 7;
						m_backgraph = key;
					}
					break;
				case 2:
					if (viewclass & MetaGraph::VIEWDATA && graph->getDisplayedDataMapRef() == entry.m_cat) {
						checkstyle = 5;
						m_topgraph = key;
					}
					else if (viewclass & MetaGraph::VIEWBACKDATA && graph->getDisplayedDataMapRef() == entry.m_cat) {
						checkstyle = 7;
						m_backgraph = key;
					}
					break;
				}

				if (checkstyle == 5)
					m_indexWidget->setItemVisibility(key, Qt::Checked);
				else if (checkstyle == 6)
					m_indexWidget->setItemVisibility(key, Qt::PartiallyChecked);
				else if (checkstyle == 7)
					m_indexWidget->setItemVisibility(key, Qt::Unchecked);

				// the editable box
				int editable = MetaGraph::NOT_EDITABLE;
				switch (entry.m_type) {
				case 0:
					if (graph->getPointMaps()[entry.m_cat].isProcessed()) {
						editable = MetaGraph::NOT_EDITABLE;
					}
					else {
						editable = MetaGraph::EDITABLE_ON;
					}
					break;
				case 1:
				{
					int type = graph->getShapeGraphs()[entry.m_cat]->getMapType();
					if (type == ShapeMap::SEGMENTMAP || type == ShapeMap::ALLLINEMAP) {
						editable = MetaGraph::NOT_EDITABLE;
					}
					else {
						editable = graph->getShapeGraphs()[entry.m_cat]->isEditable() ? MetaGraph::EDITABLE_ON : MetaGraph::EDITABLE_OFF;
					}
				}
				break;
				case 2:
					editable = graph->getDataMaps()[entry.m_cat].isEditable() ? MetaGraph::EDITABLE_ON : MetaGraph::EDITABLE_OFF;
					break;
				}
				switch (editable) {
				case MetaGraph::NOT_EDITABLE:
					m_indexWidget->setItemReadOnly(key);
					break;
				case MetaGraph::EDITABLE_OFF:
					m_indexWidget->setItemEditability(key, Qt::Unchecked);
					break;
				case MetaGraph::EDITABLE_ON:
					m_indexWidget->setItemEditability(key, Qt::Checked);
					break;
				}
			}
			else {
				// the displayed layers (note that VGA graphs (type 0)
				// do not currently have layers supported
				bool show = false;
				if (entry.m_type == 1) {
					show = graph->getShapeGraphs()[entry.m_cat]->isLayerVisible(entry.m_subcat);
				}
				else if (entry.m_type == 2) {
					show = graph->getDataMaps()[entry.m_cat].isLayerVisible(entry.m_subcat);
				}
				if (show) {
					m_indexWidget->setItemVisibility(key, Qt::Checked);
				}
				else {
					m_indexWidget->setItemVisibility(key, Qt::Unchecked);
				}
			}
		}
	}
	MakeAttributeList();
	in_FocusGraph = false;
}

void MainWindow::SetDrawingTreeChecks()
{
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	int viewclass = graph->getViewClass();
	for (auto iter : m_treedrawingmap) {
		ItemTreeEntry entry = iter.second;
		if (entry.m_subcat != -1) {
			if (graph->getLineLayer(entry.m_cat, entry.m_subcat).isShown()) {
				iter.first->setIcon(0, m_tree_icon[12]);
			}
			else {
				iter.first->setIcon(0, m_tree_icon[13]);
			}
		}
	}
}

// clear the graph tree (not the drawing tree) but also clear the attribute list

void MainWindow::ClearGraphTree()
{
	m_attribute_locked.clear();

	for (int i = 2; i >= 0; i--) {
		if (m_treeroots[i]) {
			m_treeroots[i] = NULL;
		}
	}
	m_treegraphmap.clear();
}

void MainWindow::MakeGraphTree()
{
	MetaGraph *graph = m_treeDoc->m_meta_graph;

	int state = graph->getState();

	if (state & MetaGraph::POINTMAPS) {
		if (!m_treeroots[0]) {
			QTreeWidgetItem* hItem = m_indexWidget->addNewItem(tr("Visibility Graphs"));
			hItem->setIcon(0, m_tree_icon[0]);
			ItemTreeEntry entry(0, -1, -1);
			m_treegraphmap[hItem] = entry;
			m_treeroots[0] = hItem;
		}
		int i = 0;
		for (auto& pointmap : m_treeDoc->m_meta_graph->getPointMaps()) {
			QString name = QString(pointmap.getName().c_str());
			QTreeWidgetItem* hItem = m_indexWidget->addNewItem(name, m_treeroots[0]);
			m_indexWidget->setItemVisibility(hItem, Qt::Unchecked);
			m_indexWidget->setItemEditability(hItem, Qt::Unchecked);
			ItemTreeEntry entry(0, (short)i, -1);
			m_treegraphmap.insert(std::make_pair(hItem, entry));
			i++;
		}
	}
	else if (m_treeroots[0]) {
		m_treeroots[0]->removeChild(m_treeroots[0]);
		auto iter = m_treegraphmap.find(m_treeroots[0]);
		if (iter != m_treegraphmap.end()) {
			m_treegraphmap.erase(iter);
		}
		m_treeroots[0] = NULL;
	}

	if (state & MetaGraph::SHAPEGRAPHS) {
		if (!m_treeroots[1]) {
			QTreeWidgetItem* hItem = m_indexWidget->addNewItem(tr("Shape Graphs"));
			hItem->setIcon(0, m_tree_icon[1]);
			ItemTreeEntry entry(1, -1, -1);
			m_treegraphmap[hItem] = entry;
			m_treeroots[1] = hItem;
		}
		for (size_t i = 0; i < m_treeDoc->m_meta_graph->getShapeGraphs().size(); i++) {
			QString name = QString(m_treeDoc->m_meta_graph->getShapeGraphs()[i]->getName().c_str());
			QTreeWidgetItem* hItem = m_indexWidget->addNewItem(name, m_treeroots[1]);
			m_indexWidget->setItemVisibility(hItem, Qt::Unchecked);
			m_indexWidget->setItemEditability(hItem, Qt::Unchecked);
			ItemTreeEntry entry(1, (short)i, -1);
			m_treegraphmap.insert(std::make_pair(hItem, entry));
			LayerManagerImpl& layers = m_treeDoc->m_meta_graph->getShapeGraphs()[i]->getLayers();
			if (layers.getNumLayers() > 1) {
				for (int j = 0; j < layers.getNumLayers(); j++) {
					QString name = QString(layers.getLayerName(j).c_str());
					QTreeWidgetItem* hNewItem = m_indexWidget->addNewItem(name, hItem);
					ItemTreeEntry entry(1, (short)i, j);
					m_treegraphmap[hNewItem] = entry;
				}
			}
		}
	}
	else if (m_treeroots[1]) {
		m_treeroots[1]->removeChild(m_treeroots[1]);
		auto iter = m_treegraphmap.find(m_treeroots[1]);
		if (iter != m_treegraphmap.end()) {
			m_treegraphmap.erase(iter);
		}
		m_treeroots[1] = NULL;
	}

	if (state & MetaGraph::DATAMAPS) {
		if (!m_treeroots[2]) {
			QTreeWidgetItem* hItem = m_indexWidget->addNewItem(tr("Data Maps"));
			hItem->setIcon(0, m_tree_icon[2]);
			ItemTreeEntry entry(2, -1, -1);
			m_treegraphmap[hItem] = entry;
			m_treeroots[2] = hItem;
		}
		for (size_t i = 0; i < m_treeDoc->m_meta_graph->getDataMaps().size(); i++) {
			QString name = QString(m_treeDoc->m_meta_graph->getDataMaps()[i].getName().c_str());
			QTreeWidgetItem* hItem = m_indexWidget->addNewItem(name, m_treeroots[2]);
			m_indexWidget->setItemVisibility(hItem, Qt::Unchecked);
			m_indexWidget->setItemEditability(hItem, Qt::Unchecked);
			ItemTreeEntry entry(2, (short)i, -1);
			m_treegraphmap[hItem] = entry;

			LayerManagerImpl layers = m_treeDoc->m_meta_graph->getDataMaps()[i].getLayers();
			if (layers.getNumLayers() > 1) {
				for (int j = 0; j < layers.getNumLayers(); j++) {
					QString name = QString(layers.getLayerName(j).c_str());
					QTreeWidgetItem* hNewItem = m_indexWidget->addNewItem(name, hItem);
					m_indexWidget->setItemVisibility(hNewItem, Qt::Unchecked);
					ItemTreeEntry entry(2, (short)i, j);
					m_treegraphmap.insert(std::make_pair(hNewItem, entry));
				}
			}
		}
	}
	else if (m_treeroots[2]) {
		m_treeroots[2]->removeChild(m_treeroots[2]);
		auto iter = m_treegraphmap.find(m_treeroots[2]);
		if (iter != m_treegraphmap.end()) {
			m_treegraphmap.erase(iter);
		}
		m_treeroots[2] = NULL;
	}

	SetGraphTreeChecks();
}

void MainWindow::MakeDrawingTree()
{
	MetaGraph *graph = m_treeDoc->m_meta_graph;

	int state = graph->getState();

	if (state & MetaGraph::LINEDATA) {
		if (m_treeroots[4]) {
			m_treeroots[4] = NULL;
			m_treedrawingmap.clear();
		}
		// we'll do all of these if it works...
		QTreeWidgetItem* root = m_indexWidget->addNewItem(tr("Drawing Layers"));
		root->setIcon(0, m_tree_icon[4]);
		ItemTreeEntry entry(4, 0, -1);
		m_treedrawingmap.insert(std::make_pair(root, entry));
		m_treeroots[4] = root;
		for (int i = 0; i < m_treeDoc->m_meta_graph->getLineFileCount(); i++) {

			QTreeWidgetItem* subroot = m_indexWidget->addNewItem(QString(m_treeDoc->m_meta_graph->getLineFileName(i).c_str()), m_treeroots[4]);
			subroot->setIcon(0, m_tree_icon[8]);
			ItemTreeEntry entry(4, i, -1);
			m_treedrawingmap.insert(std::make_pair(subroot, entry));

			for (int j = 0; j < m_treeDoc->m_meta_graph->getLineLayerCount(i); j++) {
				QString name(m_treeDoc->m_meta_graph->getLineLayer(i, j).getName().c_str());
				QTreeWidgetItem* hItem = m_indexWidget->addNewItem(name, subroot);
				if (m_treeDoc->m_meta_graph->getLineLayer(i, j).isShown()) {
					m_indexWidget->setItemVisibility(hItem, Qt::Checked);
				}
				else {
					m_indexWidget->setItemVisibility(hItem, Qt::Unchecked);
				}
				ItemTreeEntry entry(4, i, j);
				m_treedrawingmap.insert(std::make_pair(hItem, entry));
			}
		}
	}
}

void MainWindow::MakeAttributeList()
{
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (graph == NULL) {
		return;
	}
	auto lock = graph->getLockDeferred();
	if (lock.try_lock()) {

		// just doing this the simple way to start off with
		// (when you add new attributes, list is cleared and re
		m_attribute_locked.clear();
		m_attrWindow->clear();

		int cx = 0;
		QString name;
		if (graph->viewingProcessed()) {
			const AttributeTable& table = graph->getAttributeTable();
			m_attrWindow->addItem(tr("Ref Number"));	
			m_attrWindow->setItemHidden(m_attrWindow->item(0), true);	//隐藏掉Ref列
			m_attribute_locked.push_back(true);

			for (int i = 0; i < table.getNumColumns(); i++) {
				name = QString(table.getColumnName(i).c_str());
				m_attrWindow->addItem(name);
				m_attribute_locked.push_back(table.getColumn(i).isLocked());
				//}
			}
		}
	}

	SetAttributeChecks();
}

void MainWindow::selectAttributes() {
	if (!isOpen) return;
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (graph == NULL) return;

	QListWidgetItem * it;
	if (graph->viewingProcessed()) {
		int image, displayed_attribute = graph->getDisplayedAttribute();

		for (int i = 0; ; i++) {
			it = m_attrWindow->item(i);
			if (!it) break;
			if ((i - 1) == displayed_attribute) {
				if (!m_attribute_locked[i]) {
					image = 9;
				}
				else {
					image = 17;
				}
			}
			else {
				if (!m_attribute_locked[i]) {
					image = 10;
				}
				else {
					image = 18;
				}
			}
			it->setIcon(m_tree_icon[image]);
		}
	}
}

void MainWindow::SetAttributeChecks()
{
	MetaGraph *graph = m_treeDoc->m_meta_graph;
	if (graph == NULL) return;

	QListWidgetItem * it;
	if (graph->viewingProcessed()) {
		int image, displayed_attribute = graph->getDisplayedAttribute();

		for (int i = 0; ; i++) {
			it = m_attrWindow->item(i);
			if (!it) break;
			if ((i - 1) == displayed_attribute) {
				if (!m_attribute_locked[i]) {
					image = 9;
				}
				else {
					image = 17;
				}
			}
			else {
				if (!m_attribute_locked[i]) {
					image = 10;
				}
				else {
					image = 18;
				}
			}
			it->setIcon(m_tree_icon[image]);
		}
	}
}

void MainWindow::OninvertColor()
{
	if (!isOpen) return;
	activeMapDoc()->OnSwapColours();
}

void MainWindow::OnzoomTo()
{
	activeMapView()->OnViewZoomsel();
}

void MainWindow::SelectButtonTriggered()
{
	if (!isOpen) return;
	m_selected_mapbar_item = ID_MAPBAR_ITEM_SELECT;
	activeMapView()->OnEditSelect();
	//this->setCursor(Qt::ArrowCursor);
}

void MainWindow::DragButtonTriggered()
{
	m_selected_mapbar_item = ID_MAPBAR_ITEM_MOVE;
	activeMapView()->OnViewPan();
	//this->setCursor(Qt::OpenHandCursor);
}

void MainWindow::SelectPenTriggered()
{
	m_selected_mapbar_item = ID_MAPBAR_ITEM_PENCIL;
	activeMapView()->OnEditPencil();
}

void MainWindow::AxialMapTriggered()
{
	m_selected_mapbar_item = ID_MAPBAR_ITEM_AL2;
	activeMapView()->OnModeSeedAxial();
}

void MainWindow::StepDepthTriggered()
{
	activeMapDoc()->OnToolsPD();
}

void MainWindow::zoomButtonTriggered()
{
	//int id = zoomInAct->data().value<int>();
	if (m_selected_mapbar_item == ID_MAPBAR_ITEM_ZOOM_IN)
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_ZOOM_IN;
		activeMapView()->OnViewZoomIn();
		//this->setCursor(Qt::PointingHandCursor);
	}
	else
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_ZOOM_OUT;
		activeMapView()->OnViewZoomOut();
		//this->setCursor(Qt::PointingHandCursor);
	}
}

void MainWindow::FillButtonTriggered()
{
	int id;// = qVariantValue<int>(STDFillColorAct->data());
	if (qobject_cast<QAction *>(sender())) { // Not sure // Hack TV
		QAction* temp = qobject_cast<QAction *>(sender());
		id = temp->data().value<int>();
		delete temp;
	}
	else {
		id = STDFillColorAct->data().value<int>();
	}

	if (id == ID_MAPBAR_ITEM_FILL)
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_FILL;
		activeMapView()->OnEditFill();
	}
	else if (id == ID_MAPBAR_ITEM_SEMIFILL)         // AV TV
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_SEMIFILL;
		activeMapView()->OnEditSemiFill();
	}
	else
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_AUGMENT_FILL;
		activeMapView()->OnEditAugmentFill(); // AV TV
	}
}

void MainWindow::LineButtonTriggered()
{
	int id = SelectLineAct->data().value<int>();
	if (id == ID_MAPBAR_ITEM_LINETOOL)
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_LINETOOL;
		activeMapView()->OnEditLineTool();
	}
	else
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_POLYGON;
		activeMapView()->OnEditPolygonTool();
	}
}

void MainWindow::isoButtonTriggered()
{
	int id = MakeIosAct->data().value<int>();
	if (id == ID_MAPBAR_ITEM_ISOVIST)
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_ISOVIST;
		activeMapView()->OnModeIsovist();
	}
	else
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_HALFISOVIST;
		activeMapView()->OnModeTargetedIsovist();
	}
}

void MainWindow::joinButtonTriggered()
{
	int id = JoinAct->data().value<int>();
	if (id == ID_MAPBAR_ITEM_JOIN)
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_JOIN;
		activeMapView()->OnModeJoin();
	}
	else
	{
		m_selected_mapbar_item = ID_MAPBAR_ITEM_UNJOIN;
		activeMapView()->OnModeUnjoin();
	}
}

void MainWindow::zoomModeTriggered()
{
	zoomInAct = qobject_cast<QAction *>(sender());
	if (zoomInAct->data() == ID_MAPBAR_ITEM_ZOOM_IN)
		zoomToolButton->setIcon(QIcon(":/images/win/b-5-3.png"));
	else
		zoomToolButton->setIcon(QIcon(":/images/win/b-5-4.png"));

	zoomToolButton->setChecked(1);
	zoomButtonTriggered();
}

void MainWindow::FillModeTriggered()
{
	fillColorToolButton->setChecked(1);
	FillButtonTriggered();
}

void MainWindow::LineModeTriggered()
{
	SelectLineAct = qobject_cast<QAction *>(sender());
	if (SelectLineAct->data() == ID_MAPBAR_ITEM_LINETOOL)
		lineToolButton->setIcon(QIcon(":/images/win/b-5-10.png"));
	else
		lineToolButton->setIcon(QIcon(":/images/win/b-5-11.png"));
	lineToolButton->setChecked(1);
	LineButtonTriggered();
}

void MainWindow::isoModeTriggered()
{
	MakeIosAct = qobject_cast<QAction *>(sender());
	if (MakeIosAct->data() == ID_MAPBAR_ITEM_ISOVIST)
		newisoToolButton->setIcon(QIcon(":/images/win/b-5-12.png"));
	else
		newisoToolButton->setIcon(QIcon(":/images/win/b-5-13.png"));

	newisoToolButton->setChecked(1);
	isoButtonTriggered();
}

void MainWindow::joinTriggered()
{
	JoinAct = qobject_cast<QAction *>(sender());
	//if (JoinAct->data() == ID_MAPBAR_ITEM_JOIN)
	//	JoinToolButton->setIcon(QIcon(":/images/win/b-5-16.png"));
	//else
	//	JoinToolButton->setIcon(QIcon(":/images/win/b-5-17.png"));

	//JoinToolButton->setChecked(1);
	joinButtonTriggered();
}

void MainWindow::OnFileProperties()
{
	QGraphDoc* gd = activeMapView()->getGraphDoc();
	gd->OnFileProperties();
}

// PlotView message
void MainWindow::OntoggleColor()
{
	QGraphDoc* gd = activeMapDoc();
	if (((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
		((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewColor();
}

void MainWindow::OntoggleOrg()
{
	QGraphDoc* gd = activeMapDoc();
	if (((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
		((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewOrigin();
}

void MainWindow::OnviewTrend()
{
	QGraphDoc* gd = activeMapDoc();
	if (((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
		((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewTrendLine();
}

void MainWindow::OnYX()
{
	QGraphDoc* gd = activeMapDoc();
	if (((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
		((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewEquation();
}

void MainWindow::OnRtwo()
{
	QGraphDoc* gd = activeMapDoc();
	if (((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER]))
		((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->OnViewRsquared();
}

void MainWindow::OnToolsImportTraces()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnToolsImportTraces();
}

void MainWindow::OnAddAgent()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnAddAgent();
}

void MainWindow::OnToolsAgentsPlay()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnToolsAgentsPlay();
}

void MainWindow::OnToolsAgentsPause()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnToolsAgentsPause();
}

void MainWindow::OnToolsAgentsStop()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnToolsAgentsStop();
	updateActiveWindows();
}

void MainWindow::OnAgentTrails()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnAgentTrails();
}

void MainWindow::On3dRot()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->On3dRot();
}

void MainWindow::On3dPan()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->On3dPan();
}

void MainWindow::On3dZoom()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->On3dZoom();
}

void MainWindow::OnPlayLoop()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->OnPlayLoop();
}

void MainWindow::On3dFilled()
{
	QGraphDoc* gd = activeMapDoc();
	if (((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D]))
		((Q3DView*)gd->m_view[QGraphDoc::VIEW_3D])->On3dFilled();
}

///////////////////////////////////////
void MainWindow::createStatusBar()
{
	/*QLabel *locationLabel;
	locationLabel = new QLabel("Ready");*/

	//获取窗口大小
	int nWidth = this->geometry().width();

	/*QProgressBar * m_pConnectProBar = new QProgressBar();*/

	//进度条
	this->m_pConnectProBar->setFormat("%p%");
	this->m_pConnectProBar->setOrientation(Qt::Horizontal);
	this->m_pConnectProBar->setMinimum(this->nMin);  // 最小值  
	this->m_pConnectProBar->setMaximum(this->nMax);  // 最大值  
	this->m_pConnectProBar->setValue(0);  // 当前进度

	int width = min(max(nWidth - 320 - 200, 100)*0.9, nWidth*0.7);
	m_pConnectProBar->setMinimumWidth(width);
	m_pConnectProBar->setMaximumWidth(width);

	this->m_pConnectProBar->setAlignment(Qt::AlignLeft);

	locationLabel->setMinimumWidth(200);
	locationLabel->setMaximumWidth(200);

	//QFont font("Ebrima", 10);
	//locationLabel->setFont(font);

	//Optional
	statusBar()->setStyleSheet(QString("QStatusBar::item{border: 0px}")); // 设置不显示label的边框
	statusBar()->setSizeGripEnabled(false); //设置是否显示右边的大小控制点
	statusBar()->addWidget(m_pConnectProBar);
	statusBar()->addWidget(locationLabel);

	QVBoxLayout * vctLayout = new QVBoxLayout();
	vctLayout->addWidget(m_pConnectProBar);
	vctLayout->addWidget(locationLabel);
	QVBoxLayout * mainLayout = new QVBoxLayout();
	mainLayout->addLayout(vctLayout);
	statusBar()->setLayout(mainLayout);

	statusBar()->showMessage(tr("Ready"));
	g_info_curr = new QLabel;
	g_info_curr->setText("      ");
	statusBar()->addPermanentWidget(g_info_curr);
	g_size = new QLabel;
	g_size->setText("      ");
	statusBar()->addPermanentWidget(g_size);
	g_pos_curr = new QLabel;
	g_pos_curr->setText("      ");
	statusBar()->addPermanentWidget(g_pos_curr);
}

void MainWindow::readSettings()
{
	auto settings = mSettings.getTransaction();
	QPoint pos = settings->readSetting(SettingTag::position, QPoint(200, 200)).toPoint();
	QSize size = settings->readSetting(SettingTag::size, QSize(400, 400)).toSize();
	m_foreground = settings->readSetting(SettingTag::foregroundColour, qRgb(128, 255, 128)).toInt();
	m_background = settings->readSetting(SettingTag::backgroundColour, qRgb(0, 0, 0)).toInt();
	m_simpleVersion = settings->readSetting(SettingTag::simpleVersion, true).toBool();
	m_defaultMapWindowIsLegacy = settings->readSetting(SettingTag::legacyMapWindow, false).toBool();
	if (settings->readSetting(SettingTag::mwMaximised, true).toBool())
	{
		setWindowState(Qt::WindowMaximized);
	}
	else {
		move(pos);
		resize(size);
	}
}

void MainWindow::writeSettings()
{
	auto settings = mSettings.getTransaction();
	settings->writeSetting(SettingTag::position, pos());
	settings->writeSetting(SettingTag::size, size());
	settings->writeSetting(SettingTag::mwMaximised, windowState() == Qt::WindowMaximized);
}

void MainWindow::setCurrentFile(const QString &fileName)
{

	auto settings = mSettings.getTransaction();

	QStringList files = settings->readSetting(SettingTag::recentFileList).toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);
	while (files.size() > MaxRecentFiles)
		files.removeLast();

	settings->writeSetting(SettingTag::recentFileList, files);

	updateRecentFileActions(files);
}

void MainWindow::updateRecentFileActions(const QStringList &files)
{
	int numRecentFiles = qMin(files.size(), MaxRecentFiles);

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
		recentFileActs[j]->setVisible(false);

	//separatorAct->setVisible(numRecentFiles > 0);
}

QString MainWindow::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

void MainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
	{
		QMdiSubWindow *existing = findMapView(action->data().toString());
		if (existing) {
			mdiArea->setActiveSubWindow(existing);
			return;
		}
		MapView *child = createMapView();
		QByteArray ba = action->data().toString().toUtf8(); // quick fix for weird chars (russian filename bug report)
		char *file = ba.data(); // quick fix for weird chars (russian filename bug report)
		if (child->getGraphDoc()->OnOpenDocument(file)) // quick fix for weird chars (russian filename bug report)
		{
			child->setCurrentFile(action->data().toString());
			child->postLoadFile();
			setCurrentFile(action->data().toString());
			statusBar()->showMessage(tr("File loaded"), 2000);
			child->show();
			OnFocusGraph(child->getGraphDoc(), QGraphDoc::CONTROLS_LOADALL);
		}
		else child->close();
	}
}

void MainWindow::RedoPlotViewMenu(QGraphDoc* pDoc)
{
	if (!pDoc->m_view[QGraphDoc::VIEW_SCATTER]) return;
	in_FocusGraph = true;

	// this will be used to distinguish between viewing VGA and axial maps
	int view_class = pDoc->m_meta_graph->getViewClass() & (MetaGraph::VIEWVGA | MetaGraph::VIEWAXIAL | MetaGraph::VIEWDATA);
	int curr_j = 0;

	{
		auto lock = pDoc->m_meta_graph->getLockDeferred();
		if (lock.try_lock()) {
			m_view_map_entries.clear();
			if (view_class == MetaGraph::VIEWVGA) {
				PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();

				const AttributeTable& table = map.getAttributeTable();
				m_view_map_entries.insert(std::make_pair(0, "Ref Number"));
				for (int i = 0; i < table.getNumColumns(); i++) {
					m_view_map_entries.insert(std::make_pair(i+1 , table.getColumnName(i)));
					if (map.getDisplayedAttribute() == i) {
						curr_j = i + 1;
					}
				}
			}
			else if (view_class == MetaGraph::VIEWAXIAL) {
				// using attribute tables is very, very simple...
				const ShapeGraph& map = pDoc->m_meta_graph->getDisplayedShapeGraph();
				const AttributeTable& table = map.getAttributeTable();
				m_view_map_entries.insert(std::make_pair(0, "Ref Number"));
				curr_j = 0;
				for (int i = 0; i < table.getNumColumns(); i++) {
					m_view_map_entries.insert(std::make_pair(i+1 , table.getColumnName(i)));
					if (map.getDisplayedAttribute() == i) {
						curr_j = i + 1;
					}
				}
			}
			else if (view_class == MetaGraph::VIEWDATA) {
				// using attribute tables is very, very simple...
				const ShapeMap& map = pDoc->m_meta_graph->getDisplayedDataMap();
				const AttributeTable& table = map.getAttributeTable();
				m_view_map_entries.insert(std::make_pair(0, "Ref Number"));
				curr_j = 0;
				for (int i = 0; i < table.getNumColumns(); i++) {
					m_view_map_entries.insert(std::make_pair(i+1 , table.getColumnName(i)));
					if (map.getDisplayedAttribute() == i) {
						curr_j = i + 1;
					}
				}
			}
		}
	}

	int t, cur_sel = 0;
	x_coord->clear();
	y_coord->clear();

	int i = 0;
	for (auto view_map_entry : m_view_map_entries) {
		if (curr_j == view_map_entry.first) cur_sel = i;
		x_coord->addItem(QString(view_map_entry.second.c_str()));
		y_coord->addItem(QString(view_map_entry.second.c_str()));
		i++;
	}

	t = ((QPlotView*)pDoc->m_view[QGraphDoc::VIEW_SCATTER])->curr_y;
	if (t != -1) cur_sel = t;
	((QPlotView*)pDoc->m_view[QGraphDoc::VIEW_SCATTER])->SetAxis(1, cur_sel - 1, true);
	y_coord->setCurrentIndex(cur_sel);

	t = ((QPlotView*)pDoc->m_view[QGraphDoc::VIEW_SCATTER])->curr_x;
	if (t != -1) cur_sel = t;
	((QPlotView*)pDoc->m_view[QGraphDoc::VIEW_SCATTER])->SetAxis(0, cur_sel - 1, true);
	x_coord->setCurrentIndex(cur_sel);

	in_FocusGraph = false;
}

void MainWindow::OnSelchangeViewSelector_X(const QString &string)
{
	if (in_FocusGraph) return;

	int i = x_coord->currentIndex();

	QGraphDoc* gd = activeMapDoc();
	((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->SetAxis(0, /*m_view_selection*/i - 1, true);
	((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->curr_x = i;

	// note: this is only attached to a scatter view, and changing the attribute only
	// affects the scatter view, so only send draw to the map:
	gd->SetRedrawFlag(QGraphDoc::VIEW_SCATTER, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_COLUMN);
}

void MainWindow::OnSelchangeViewSelector_Y(const QString &string)
{
	if (in_FocusGraph) return;
	int i = y_coord->currentIndex();

	QGraphDoc* gd = activeMapDoc();
	((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->SetAxis(1, i - 1, true);
	((QPlotView*)gd->m_view[QGraphDoc::VIEW_SCATTER])->curr_y = i;

	// note: this is only attached to a scatter view, and changing the attribute only
	// affects the scatter view, so only send draw to the map:
	gd->SetRedrawFlag(QGraphDoc::VIEW_SCATTER, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_COLUMN);
}


void MainWindow::updateViewMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		RecentAct->setEnabled(0);
		showGridAct->setEnabled(0);
		attributeSummaryAct->setEnabled(0);
		return;
	}
	RecentAct->setEnabled(true);
	showGridAct->setEnabled(true);

	if (m_p->m_meta_graph->m_showgrid) showGridAct->setChecked(true);
	else showGridAct->setChecked(false);

	attributeSummaryAct->setEnabled(true);
	if (!m_p->m_communicator && m_p->m_meta_graph && !m_p->m_meta_graph->viewingNone())
		attributeSummaryAct->setEnabled(true);
	else attributeSummaryAct->setEnabled(0);
}

void MainWindow::updateVisibilitySubMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		SetGridAct->setEnabled(0);
		makeVisibilityGraphAct->setEnabled(0);
		unmakeVisibilityGraphAct->setEnabled(0);
		importVGALinksAct->setEnabled(0);
		makeIsovistPathAct->setEnabled(0);
		runVisibilityGraphAnalysisAct->setEnabled(0);
		convertDataMapLinesAct->setEnabled(0);
		return;
	}
	if (m_p->m_meta_graph->getState() & MetaGraph::LINEDATA || m_p->m_meta_graph->viewingUnprocessedPoints())
		SetGridAct->setEnabled(true);
	else SetGridAct->setEnabled(0);

	if (m_p->m_meta_graph->viewingUnprocessedPoints()) {
		makeVisibilityGraphAct->setEnabled(true);
		unmakeVisibilityGraphAct->setEnabled(false);
	}
	else {
		makeVisibilityGraphAct->setEnabled(false);
		unmakeVisibilityGraphAct->setEnabled(true);
	}

	int state = m_p->m_meta_graph->getState();
	if (state & MetaGraph::LINEDATA)
		makeIsovistPathAct->setEnabled(true);
	else makeIsovistPathAct->setEnabled(0);

	if (m_p->m_meta_graph->viewingProcessedPoints()) {
		importVGALinksAct->setEnabled(true);
		runVisibilityGraphAnalysisAct->setEnabled(true);
	}
	else
	{
		importVGALinksAct->setEnabled(0);
		runVisibilityGraphAnalysisAct->setEnabled(0);
	}

	if (!m_p->m_communicator &&
		m_p->m_meta_graph->viewingProcessedShapes() &&
		(m_p->m_meta_graph->getState() & MetaGraph::POINTMAPS) &&
		m_p->m_meta_graph->getDisplayedPointMap().isProcessed())
		convertDataMapLinesAct->setEnabled(true);
	else convertDataMapLinesAct->setEnabled(0);
}

void MainWindow::updateStepDepthSubMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		visibilityStepAct->setEnabled(0);
		metricStepAct->setEnabled(0);
		angularStepAct->setEnabled(0);
		return;
	}
	if (m_p->m_meta_graph->viewingProcessed() && m_p->m_meta_graph->isSelected())
		visibilityStepAct->setEnabled(true);
	else visibilityStepAct->setEnabled(0);

	if ((m_p->m_meta_graph->viewingProcessedPoints() || (m_p->m_meta_graph->viewingProcessedLines() && m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())) && m_p->m_meta_graph->isSelected())
		metricStepAct->setEnabled(true);
	else metricStepAct->setEnabled(0);

	if (m_p->m_meta_graph->viewingProcessedPoints() && m_p->m_meta_graph->isSelected())
		angularStepAct->setEnabled(true);
	else angularStepAct->setEnabled(0);
}

void MainWindow::updateAgentToolsSubMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		runAgentAnalysisAct->setEnabled(0);
		loadAgentProgramAct->setEnabled(0);
		return;
	}
	if (m_p->m_meta_graph && m_p->m_meta_graph->viewingProcessedPoints() && !m_p->m_communicator)
		runAgentAnalysisAct->setEnabled(true);
	else runAgentAnalysisAct->setEnabled(0);
	if (current_view_type == QGraphDoc::VIEW_3D) loadAgentProgramAct->setEnabled(true);
	else loadAgentProgramAct->setEnabled(0);
}

void MainWindow::updateSegmentSubMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		runAngularSegmentAnalysisAct->setEnabled(0);
		runTopologicalOrMetricAnalysisAct->setEnabled(0);
		return;
	}
	if (m_p->m_meta_graph->viewingProcessedLines() && m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())
		runAngularSegmentAnalysisAct->setEnabled(true);
	else runAngularSegmentAnalysisAct->setEnabled(0);

	if (m_p->m_meta_graph->viewingProcessedLines() && m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())
		runTopologicalOrMetricAnalysisAct->setEnabled(true);
	else runTopologicalOrMetricAnalysisAct->setEnabled(0);
}

void MainWindow::updateSegmentStepDepthSubMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		segmentAngularStepAct->setEnabled(0);
		topologicalStepAct->setEnabled(0);
		segmentMetricStepAct->setEnabled(0);
		return;
	}
	if (m_p->m_meta_graph->viewingProcessed() && m_p->m_meta_graph->isSelected())
		segmentAngularStepAct->setEnabled(true);
	else segmentAngularStepAct->setEnabled(0);

	if (m_p->m_meta_graph->viewingProcessedLines() && m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap() && m_p->m_meta_graph->isSelected())
		topologicalStepAct->setEnabled(true);
	else topologicalStepAct->setEnabled(0);

	if ((m_p->m_meta_graph->viewingProcessedPoints() || (m_p->m_meta_graph->viewingProcessedLines() &&
		m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())) && m_p->m_meta_graph->isSelected())
		segmentMetricStepAct->setEnabled(true);
	else segmentMetricStepAct->setEnabled(0);
}

void MainWindow::updateAxialSubMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		runGraphAnaysisAct->setEnabled(0);
		stepDepthAct->setEnabled(0);
		reduceToFewestLineMapAct->setEnabled(0);
		convertDataMapPointsAct->setEnabled(0);
		loadUnlinksFromFileAct->setEnabled(0);
		return;
	}
	int state = m_p->m_meta_graph->getState();
	// non-segment maps only
	if (state & MetaGraph::SHAPEGRAPHS && !m_p->m_communicator &&
		!m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())
		runGraphAnaysisAct->setEnabled(true);
	else runGraphAnaysisAct->setEnabled(0);

	if (m_p->m_meta_graph->viewingProcessed() && m_p->m_meta_graph->isSelected())
		stepDepthAct->setEnabled(true);
	else stepDepthAct->setEnabled(0);

	state = m_p->m_meta_graph->getState();
	if (state & MetaGraph::SHAPEGRAPHS && !m_p->m_communicator &&
		m_p->m_meta_graph->getDisplayedShapeGraph().isAllLineMap())
		reduceToFewestLineMapAct->setEnabled(true);
	else reduceToFewestLineMapAct->setEnabled(0);

	if (!m_p->m_communicator && m_p->m_meta_graph &&
		m_p->m_meta_graph->viewingProcessedLines() &&
		m_p->m_meta_graph->getDisplayedShapeGraph().getMapType() == ShapeMap::AXIALMAP)
		convertDataMapPointsAct->setEnabled(true);
	else convertDataMapPointsAct->setEnabled(0);

	if (!m_p->m_communicator && m_p->m_meta_graph &&
		m_p->m_meta_graph->viewingProcessedLines() &&
		!m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())
		loadUnlinksFromFileAct->setEnabled(true);
	else loadUnlinksFromFileAct->setEnabled(0);
}

void MainWindow::updateAttributesMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		addColumAct->setEnabled(0);
		updateColumAct->setEnabled(0);
		renameColumnAct->setEnabled(0);
		removeColumAct->setEnabled(0);
		pushValueAct->setEnabled(0);
		columnPropertiesAct->setEnabled(0);
		return;
	}
	if (!m_p->m_communicator && m_p->m_meta_graph->viewingProcessed())
	{
		addColumAct->setEnabled(true);
		columnPropertiesAct->setEnabled(true);
		int col = m_p->m_meta_graph->getDisplayedAttribute();
		if (col == -1 || col == -2 || m_p->m_meta_graph->isAttributeLocked(col))
		{
			renameColumnAct->setEnabled(0);
			updateColumAct->setEnabled(0);
			removeColumAct->setEnabled(0);
			pushValueAct->setEnabled(0);
		}
		else {
			renameColumnAct->setEnabled(true);
			updateColumAct->setEnabled(true);
			removeColumAct->setEnabled(true);
			pushValueAct->setEnabled(true);
		}
	}
	else
	{
		addColumAct->setEnabled(0);
		updateColumAct->setEnabled(0);
		renameColumnAct->setEnabled(0);
		removeColumAct->setEnabled(0);
		pushValueAct->setEnabled(0);
		columnPropertiesAct->setEnabled(0);
	}
}

void MainWindow::updateMapMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		mapNewAct->setEnabled(0);
		deleteAct->setEnabled(0);
		convertActiveMapAct->setEnabled(0);
		convertDrawingMapAct->setEnabled(0);
		convertMapShapesAct->setEnabled(0);
		importAct->setEnabled(0);
		exportAct->setEnabled(0);
		exportGeometryAct->setEnabled(false);
		exportLinksAct->setEnabled(0);
		exportAxialConnectionsDotAct->setEnabled(0);
		exportAxialConnectionsPairAct->setEnabled(0);
		exportSegmentConnectionsPairAct->setEnabled(0);
		return;
	}
	mapNewAct->setEnabled(true);
	importAct->setEnabled(true);
	if (!m_p->m_meta_graph->viewingNone() && !m_p->m_communicator)
		deleteAct->setEnabled(true);
	else deleteAct->setEnabled(0);

	if (!m_p->m_communicator && (m_p->m_meta_graph->viewingProcessedLines() || m_p->m_meta_graph->viewingProcessedShapes()))
		convertActiveMapAct->setEnabled(true);
	else convertActiveMapAct->setEnabled(0);

	if (!m_p->m_communicator && (m_p->m_meta_graph->getState() & MetaGraph::LINEDATA) == MetaGraph::LINEDATA)
		convertDrawingMapAct->setEnabled(true);
	else convertDrawingMapAct->setEnabled(0);

	if (m_p->m_meta_graph && m_p->m_meta_graph->viewingShapes())
		convertMapShapesAct->setEnabled(true);
	else convertMapShapesAct->setEnabled(0);

	if (!m_p->m_meta_graph->viewingNone() && !m_p->m_communicator)
	{
		exportAct->setEnabled(true);
		exportGeometryAct->setEnabled(true);
		exportLinksAct->setEnabled(true);
		exportAxialConnectionsDotAct->setEnabled(true);
		exportAxialConnectionsPairAct->setEnabled(true);
		exportSegmentConnectionsPairAct->setEnabled(true);
	}
	else
	{
		exportAct->setEnabled(0);
		exportGeometryAct->setEnabled(false);
		exportLinksAct->setEnabled(0);
		exportAxialConnectionsDotAct->setEnabled(0);
		exportAxialConnectionsPairAct->setEnabled(0);
		exportSegmentConnectionsPairAct->setEnabled(0);
	}
}


void MainWindow::updateEditMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	if (!m_p)
	{
		copyDataAct->setEnabled(0);
		undoAct->setEnabled(0);
		copyScreenAct->setEnabled(0);
		exportScreenAct->setEnabled(0);
		clearAct->setEnabled(0);
		selectByQueryAct->setEnabled(0);
		//zoomToSelectionAct->setEnabled(0);
		selectionToLayerAct->setEnabled(0);
		return;
	}
	copyScreenAct->setEnabled(true);
	exportScreenAct->setEnabled(true);
	if (m_p->m_meta_graph->isEditable()) clearAct->setEnabled(true);
	else clearAct->setEnabled(0);

	if (m_p->m_meta_graph->canUndo()) undoAct->setEnabled(true);
	else undoAct->setEnabled(0);

	if (m_p->m_meta_graph && !m_p->m_communicator && m_p->m_meta_graph->viewingProcessed())
		selectByQueryAct->setEnabled(true);
	else selectByQueryAct->setEnabled(0);

	if (m_p->m_meta_graph->isSelected())
	{
		//zoomToSelectionAct->setEnabled(true);
		selectionToLayerAct->setEnabled(true);
	}
	else
	{
		//zoomToSelectionAct->setEnabled(0);
		selectionToLayerAct->setEnabled(0);
	}
}

void MainWindow::updateFileMenu()
{
	if (mdiArea->activeSubWindow())
	{
		closeAct->setEnabled(true);
		saveAct->setEnabled(true);
		saveAsAct->setEnabled(true);
		propertiesAct->setEnabled(true);
		if (current_view_type == QGraphDoc::VIEW_3D)
		{
			printAct->setEnabled(0);
			printPreviewAct->setEnabled(0);
		}
		else
		{
			printAct->setEnabled(true);
			printPreviewAct->setEnabled(true);
		}
	}
	else
	{
		closeAct->setEnabled(0);
		saveAct->setEnabled(0);
		saveAsAct->setEnabled(0);
		propertiesAct->setEnabled(0);
		printAct->setEnabled(0);
		printPreviewAct->setEnabled(0);
	}
}

void MainWindow::updateWindowMenu()
{
	QGraphDoc* m_p = activeMapDoc();
	windowMenu->clear();
	windowMenu->addAction(mapAct);

	if (m_p && m_p->m_view[QGraphDoc::VIEW_MAP]) mapAct->setChecked(true);
	else mapAct->setChecked(false);

	windowMenu->addAction(scatterPlotAct);
	if (m_p && m_p->m_view[QGraphDoc::VIEW_SCATTER]) scatterPlotAct->setChecked(true);
	else scatterPlotAct->setChecked(false);

	windowMenu->addAction(tableAct);
	if (m_p && m_p->m_view[QGraphDoc::VIEW_TABLE]) tableAct->setChecked(true);
	else tableAct->setChecked(false);

	windowMenu->addAction(thirdDViewAct);
	if (m_p && m_p->m_view[QGraphDoc::VIEW_3D]) thirdDViewAct->setChecked(true);
	else thirdDViewAct->setChecked(false);

	windowMenu->addAction(glViewAct);
	if (m_p && m_p->m_view[QGraphDoc::VIEW_MAP_GL]) glViewAct->setChecked(true);
	else glViewAct->setChecked(false);

	windowMenu->addSeparator();
	windowMenu->addAction(colourRangeAct);
	windowMenu->addSeparator();
	windowMenu->addAction(cascadeAct);
	windowMenu->addAction(tileAct);
	windowMenu->addAction(arrangeIconsAct);
	windowMenu->addAction(separatorAct);

	if (!m_p)
	{
		mapAct->setEnabled(0);
		scatterPlotAct->setEnabled(0);
		tableAct->setEnabled(0);
		thirdDViewAct->setEnabled(0);
		glViewAct->setEnabled(0);
	}
	else
	{
		thirdDViewAct->setEnabled(true);
		mapAct->setEnabled(true);
		glViewAct->setEnabled(true);
		if (m_p->m_meta_graph && m_p->m_meta_graph->viewingProcessed())
		{
			tableAct->setEnabled(true);
			scatterPlotAct->setEnabled(true);
		}
		else {
			tableAct->setEnabled(0);
			scatterPlotAct->setEnabled(0);
		}
	}

	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	int find_count = 1;
	for (int i = 0; i < windows.size(); ++i) {
		MapView *child = qobject_cast<MapView*>(windows.at(i)->widget());
		if (!child) continue;

		QString text;
		text = tr("&%1 %2").arg(find_count++).arg(child->windowTitle());
		QAction *action = windowMenu->addAction(text);
		action->setCheckable(true);
		action->setChecked(child == activeMapView());
		connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
		windowMapper->setMapping(action, windows.at(i)->widget());
	}
}

void MainWindow::UpdateStatus(QString s1, QString s2, QString s3)
{
	g_info_curr->setText(s1);
	g_info_curr->update();
	g_size->setText(s2);
	g_size->update();
	g_pos_curr->setText(s3);
	g_pos_curr->update();

	//收集线段Ref
	ref_numer = "";
	if (Ref_number_list.size()>0 && Ref_number_list[0] == -1)
		Ref_number_list.erase(Ref_number_list.begin());
	for (int i = 0; i < Ref_number_list.size(); i++)
	{
		if (Ref_number_list[i] == -1)
			continue;
		if (i > 0 && ref_numer.length()>0) ref_numer += ",";
		ref_numer += QString::number(this->Ref_to_Id[Ref_number_list[i]]);
		//为什么i>2000要break
		//if (i > 2000)
		//	break;
	}
	
	bool isSelected = false;
	if (Ref_number_list.size() > 0 && Ref_number_list[0] != -1 && ref_numer != ref_numer_last) {
		isSelected = true;
	}

	ref_numer_last = ref_numer;
}

void MainWindow::updateToolbar()
{
	importAct->setEnabled(0);
	saveAct->setEnabled(0);
	addColumAct->setEnabled(0);
	updateColumAct->setEnabled(0);
	removeColumAct->setEnabled(0);
	pushValueAct->setEnabled(0);
	invertColorAct->setEnabled(0);
	SelectButton->setEnabled(0);
	DragButton->setEnabled(0);
	zoomToolButton->setEnabled(0);
	zoomToAct->setEnabled(0);
	RecentAct->setEnabled(0);
	SetGridAct->setEnabled(0);
	fillColorToolButton->setEnabled(0);
	SelectPenButton->setEnabled(0);
	lineToolButton->setEnabled(0);
	newisoToolButton->setEnabled(0);
	AxialMapButton->setEnabled(0);
	StepDepthButton->setEnabled(0);
	JoinToolButton->setEnabled(0);
	attr_del_button->setEnabled(0);
	attr_add_button->setEnabled(0);

	QGraphDoc* m_p = activeMapDoc();
	MapView* tmpView = activeMapView();
	if (m_p)
	{
		importAct->setEnabled(true);
		saveAct->setEnabled(true);
		if (m_p->m_meta_graph->getDisplayedMapRef() != -1)
			addColumAct->setEnabled(true);
		SelectButton->setEnabled(true);
		DragButton->setEnabled(true);
		RecentAct->setEnabled(true);
		zoomToAct->setEnabled(true);

		editToolBar->setMovable(true);
		editToolBar->move(1000, 50);

		if (m_p->m_meta_graph->isSelected())
			zoomToAct->setEnabled(true);

		if (m_p->m_meta_graph->isShown())   // zoom bug VGA // TV
			zoomToolButton->setEnabled(true);
		if (m_p->m_meta_graph->viewingProcessed())
			zoomToolButton->setEnabled(true);

		if (m_p->m_meta_graph->getState() & MetaGraph::LINEDATA || m_p->m_meta_graph->viewingUnprocessedPoints())
			SetGridAct->setEnabled(true);

		if (m_p->m_meta_graph->viewingUnprocessedPoints())
		{
			fillColorToolButton->setEnabled(true);
			SelectPenButton->setEnabled(true);
		}
		else
		{
			if (tmpView)
			{
				if (m_selected_mapbar_item == ID_MAPBAR_ITEM_FILL
					|| m_selected_mapbar_item == ID_MAPBAR_ITEM_SEMIFILL
					|| m_selected_mapbar_item == ID_MAPBAR_ITEM_PENCIL)
				{
					tmpView->OnEditSelect();
					SelectButton->setChecked(true);
				}
			}
		}

		int type = m_p->m_meta_graph->getDisplayedMapType();
		if ((type == ShapeMap::DATAMAP && m_p->m_meta_graph->getDisplayedDataMap().isEditable()) ||
			((type == ShapeMap::AXIALMAP || type == ShapeMap::CONVEXMAP || type == ShapeMap::PESHMAP) &&
				m_p->m_meta_graph->getDisplayedShapeGraph().isEditable()))
			lineToolButton->setEnabled(true);
		else
		{
			if (tmpView)
			{
				if (m_selected_mapbar_item == ID_MAPBAR_ITEM_LINETOOL
					|| m_selected_mapbar_item == ID_MAPBAR_ITEM_POLYGON)
				{
					tmpView->OnEditSelect();
					SelectButton->setChecked(true);
				}
			}
		}

		type = m_p->m_meta_graph->getState();
		if (!(~type & MetaGraph::LINEDATA))
			newisoToolButton->setEnabled(true);
		else
		{
			if (tmpView)
			{
				if (m_selected_mapbar_item == ID_MAPBAR_ITEM_ISOVIST || m_selected_mapbar_item == ID_MAPBAR_ITEM_HALFISOVIST)
				{
					tmpView->OnEditSelect();
					SelectButton->setChecked(true);
				}
			}
		}

		if ((((m_p->m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) &&
			(m_p->m_meta_graph->getDisplayedPointMap().getFilledPointCount() > 1)) ||
			((m_p->m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) &&
			(m_p->m_meta_graph->getState() & MetaGraph::SHAPEGRAPHS)) &&
				(!m_p->m_meta_graph->getDisplayedShapeGraph().isSegmentMap())))
			JoinToolButton->setEnabled(true);
		else
		{
			if (tmpView)
			{
				if (m_selected_mapbar_item == ID_MAPBAR_ITEM_JOIN || m_selected_mapbar_item == ID_MAPBAR_ITEM_UNJOIN)
				{
					tmpView->OnEditSelect();
					SelectButton->setChecked(true);
				}
			}
		}

		type = m_p->m_meta_graph->getState();
		if (!(~type & MetaGraph::LINEDATA)) AxialMapButton->setEnabled(true);
		else
		{
			if (tmpView)
			{
				if (m_selected_mapbar_item == ID_MAPBAR_ITEM_AL2)
				{
					tmpView->OnEditSelect();
					SelectButton->setChecked(true);
				}
			}
		}

		if (m_p->m_meta_graph->viewingProcessed() && m_p->m_meta_graph->isSelected())
			StepDepthButton->setEnabled(true);

		if (!m_p->m_communicator && m_p->m_meta_graph->viewingProcessed())
		{
			int col = m_p->m_meta_graph->getDisplayedAttribute();
			if (!(col == -1 || col == -2 || m_p->m_meta_graph->isAttributeLocked(col))) {
				renameColumnAct->setEnabled(true);
				updateColumAct->setEnabled(true);
				removeColumAct->setEnabled(true);
				pushValueAct->setEnabled(true);
				invertColorAct->setEnabled(true);
				attr_del_button->setEnabled(true);
				attr_add_button->setEnabled(true);
			}
		}
	}
}

void MainWindow::createActions()
{
	//创建新的Workspace
	newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
	newAct->setShortcut(tr("Ctrl+N"));
	newAct->setStatusTip(tr("Create a new graph workspace\nNew Workspace"));
	connect(newAct, SIGNAL(triggered()), this, SLOT(OnFileNew()));

	//打开文件
	openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
	openAct->setShortcut(tr("Ctrl+O"));
	openAct->setStatusTip(tr("Change the printing options\nPage Setup"));
	connect(openAct, SIGNAL(triggered()), this, SLOT(OnFileOpen()));

	//关闭文件
	closeAct = new QAction(tr("&Close"), this);
	closeAct->setStatusTip(tr("Close the active graph workspace\nClose Workspace"));
	connect(closeAct, SIGNAL(triggered()), this, SLOT(OnFileClose()));

	//保存文件
	saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
	saveAct->setShortcut(tr("Ctrl+S"));
	saveAct->setStatusTip(tr("Save the active graph workspace\nSave workspace"));
	connect(saveAct, SIGNAL(triggered()), this, SLOT(OnFileSave()));

	//文件另存为
	saveAsAct = new QAction(tr("Save &As..."), this);
	saveAsAct->setStatusTip(tr("Save the active graph workspace with a new name\nSave Workspace As"));
	connect(saveAsAct, SIGNAL(triggered()), this, SLOT(OnFileSaveAs()));

	//文件属性
	propertiesAct = new QAction(tr("Properties..."), this);
	propertiesAct->setStatusTip(tr("Edit graph workspace properties\nWorkspace Properties"));
	connect(propertiesAct, SIGNAL(triggered()), this, SLOT(OnFileProperties()));

	//文件打印
	printAct = new QAction(tr("&Print..."), this);
	printAct->setShortcut(tr("Ctrl+P"));
	printAct->setStatusTip(tr("Print the active graph workspace\nPrint Workspace"));
	connect(printAct, SIGNAL(triggered()), this, SLOT(OnFilePrint()));

	//文件打印预览
	printPreviewAct = new QAction(tr("Print Pre&view"), this);
	printPreviewAct->setStatusTip(tr("Display full pages\nPrint Preview"));
	connect(printPreviewAct, SIGNAL(triggered()), this, SLOT(OnFilePrintPreview()));

	//文件打印安装
	printSetupAct = new QAction(tr("P&rint Setup..."), this);
	printSetupAct->setStatusTip(tr("Change the printer and printing options\nPrint Setup"));
	connect(printSetupAct, SIGNAL(triggered()), this, SLOT(OnFilePrintSetup()));

	//打开最近文件
	for (int i = 0; i < MaxRecentFiles; ++i) {
		recentFileActs[i] = new QAction(this);
		recentFileActs[i]->setVisible(false);
		connect(recentFileActs[i], SIGNAL(triggered()),
			this, SLOT(openRecentFile()));
	}

	//关闭文件
	exitAct = new QAction(tr("E&xit"), this);
	exitAct->setStatusTip(tr("Quit the application; prompts to save documents\nExit"));
	connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

	//Edit Menu Actions 编辑菜单动作

	//编辑：撤销
	undoAct = new QAction(tr("&Undo"), this);
	undoAct->setShortcut(tr("Ctrl+Z"));
	undoAct->setStatusTip(tr("Undo the last action\nUndo"));
	connect(undoAct, SIGNAL(triggered()), this, SLOT(OnEditUndo()));

	//编辑：复制数据
	copyDataAct = new QAction(tr("Copy &Data"), this);
	connect(copyDataAct, SIGNAL(triggered()), this, SLOT(OnEditCopyData()));

	//编辑：复制
	copyScreenAct = new QAction(tr("Copy &Screen"), this);
	copyScreenAct->setShortcut(tr("Ctrl+C"));
	copyScreenAct->setStatusTip(tr("Copy the screen contents to clipboard\nCopy Screen"));
	connect(copyScreenAct, SIGNAL(triggered()), this, SLOT(OnEditCopy()));

	//编辑：保存
	exportScreenAct = new QAction(tr("&Export Screen..."), this);
	exportScreenAct->setStatusTip(tr("Export the screen as an Encapsulated Postscript file\nExport Screen"));
	connect(exportScreenAct, SIGNAL(triggered()), this, SLOT(OnEditSave()));

	//编辑：清空
	clearAct = new QAction(tr("&Clear"), this);
	clearAct->setShortcut(tr("Del"));
	clearAct->setShortcutContext(Qt::ApplicationShortcut);
	clearAct->setStatusTip(tr("Erase the selection\nErase"));
	connect(clearAct, SIGNAL(triggered()), this, SLOT(OnEditClear()));

	//编辑：查询
	selectByQueryAct = new QAction(tr("Select by &Query"), this);
	selectByQueryAct->setShortcut(tr("Ctrl+Q"));
	connect(selectByQueryAct, SIGNAL(triggered()), this, SLOT(OnEditQuery()));

	//zoomToSelectionAct = new QAction(tr("&Zoom to Selection"), this);
	//zoomToSelectionAct->setStatusTip(tr("Zoom in around current selection\nZoom to Selection"));
	//connect(zoomToSelectionAct, SIGNAL(triggered()), this, SLOT(OnViewZoomsel()));

	//编辑：选择图层
	selectionToLayerAct = new QAction(tr("Selection to &Layer..."), this);
	selectionToLayerAct->setStatusTip(tr("Convert the current selection to a new map layer"));
	connect(selectionToLayerAct, SIGNAL(triggered()), this, SLOT(OnEditSelectToLayer()));

	//Map Menu Actions 地图菜单动作

	//创建新Map
	mapNewAct = new QAction(tr("&New..."), this);
	mapNewAct->setStatusTip(tr("Create a new map"));
	connect(mapNewAct, SIGNAL(triggered()), this, SLOT(OnLayerNew()));

	//删除Map
	deleteAct = new QAction(tr("&Delete..."), this);
	deleteAct->setStatusTip(tr("Delete the active map"));
	connect(deleteAct, SIGNAL(triggered()), this, SLOT(OnLayerDelete()));

	//Map转换1
	convertActiveMapAct = new QAction(tr("&Convert Active Map..."), this);
	convertActiveMapAct->setStatusTip(tr("Create a new map from the active map"));
	connect(convertActiveMapAct, SIGNAL(triggered()), this, SLOT(OnLayerConvert()));

	//Map转换2
	convertDrawingMapAct = new QAction(tr("Convert Drawing &Map..."), this);
	convertDrawingMapAct->setStatusTip(tr("Create a new map from the displayed drawing maps"));
	connect(convertDrawingMapAct, SIGNAL(triggered()), this, SLOT(OnLayerConvertDrawing()));

	//Map转换3
	convertMapShapesAct = new QAction(tr("Convert Map &Shapes..."), this);
	convertMapShapesAct->setStatusTip(tr("Convert shapes to other shapes within the current map"));
	connect(convertMapShapesAct, SIGNAL(triggered()), this, SLOT(OnConvertMapShapes()));

	//文件导入
	importAct = new QAction(QIcon(":/images/down.png"), tr("&Import..."), this);
	importAct->setShortcut(tr("Ctrl+I"));
	importAct->setStatusTip(tr("Import a DXF or points file\nImport Map"));
	connect(importAct, SIGNAL(triggered()), this, SLOT(OnFileImport()));

	//文件导出
	exportAct = new QAction(tr("&Export map..."), this);
	exportAct->setShortcut(tr("Ctrl+E"));
	exportAct->setStatusTip(tr("Export the active map"));
	connect(exportAct, SIGNAL(triggered()), this, SLOT(OnFileExport()));

	//文件转换导出1
	exportGeometryAct = new QAction(tr("&Export map geometry..."), this);
	exportGeometryAct->setStatusTip(tr("Export the geometry of the active map"));
	connect(exportGeometryAct, SIGNAL(triggered()), this, SLOT(OnFileExportMapGeometry()));

	//文件转换导出2
	exportLinksAct = new QAction(tr("&Export links..."), this);
	exportLinksAct->setStatusTip(tr("Export the links of the active map"));
	connect(exportLinksAct, SIGNAL(triggered()), this, SLOT(OnFileExportLinks()));

	//文件转换导出3
	exportAxialConnectionsPairAct = new QAction(tr("&Axial Connections as CSV..."), this);
	exportAxialConnectionsPairAct->setStatusTip(tr("Export a list of line-line intersections"));
	connect(exportAxialConnectionsPairAct, SIGNAL(triggered()), this, SLOT(OnAxialConnectionsExportAsPairCSV()));

	//文件转换导出4
	exportAxialConnectionsDotAct = new QAction(tr("&Axial Connections as Dot..."), this);
	exportAxialConnectionsDotAct->setStatusTip(tr("Export a list of line-line intersections"));
	connect(exportAxialConnectionsDotAct, SIGNAL(triggered()), this, SLOT(OnAxialConnectionsExportAsDot()));

	//文件转换导出5
	exportSegmentConnectionsPairAct = new QAction(tr("&Segment Connections as CSV..."), this);
	exportSegmentConnectionsPairAct->setStatusTip(tr("Export a list of line-line intersections and weights"));
	connect(exportSegmentConnectionsPairAct, SIGNAL(triggered()), this, SLOT(OnSegmentConnectionsExportAsPairCSV()));

	//文件转换导出6
	exportPointmapConnectionsPairAct = new QAction(tr("Visibility Graph Connections as CSV..."), this);
	exportPointmapConnectionsPairAct->setStatusTip(tr("Export connections between cells in a visibility graph as an adjacency list"));
	connect(exportPointmapConnectionsPairAct, SIGNAL(triggered()), this, SLOT(OnPointmapExportConnectionsAsCSV()));

	//Attributes Menu Actions 属性菜单动作

	//重命名列
	renameColumnAct = new QAction(tr("Rename"), this);
	renameColumnAct->setStatusTip(tr("Rename the currently displayed attribute"));
	connect(renameColumnAct, SIGNAL(triggered()), this, SLOT(OnRenameColumn()));

	//列属性
	columnPropertiesAct = new QAction(tr("Properties"), this);
	columnPropertiesAct->setStatusTip(tr("Summary statistics for the active attribute"));
	connect(columnPropertiesAct, SIGNAL(triggered()), this, SLOT(OnColumnProperties()));

	//Tools Menu Actions 工具菜单动作

	//制图
	makeVisibilityGraphAct = new QAction(tr("Make &Visibility Graph..."), this);
	connect(makeVisibilityGraphAct, SIGNAL(triggered()), this, SLOT(OnToolsMakeGraph()));

	//取消制图
	unmakeVisibilityGraphAct = new QAction(tr("Unmake &Visibility Graph..."), this);
	connect(unmakeVisibilityGraphAct, SIGNAL(triggered()), this, SLOT(OnToolsUnmakeGraph()));

	//导入VGA链接
	importVGALinksAct = new QAction(tr("Import VGA links from file..."), this);
	connect(importVGALinksAct, SIGNAL(triggered()), this, SLOT(OnToolsImportVGALinks()));

	//Isovist路径
	makeIsovistPathAct = new QAction(tr("Make &Isovist Path..."), this);
	connect(makeIsovistPathAct, SIGNAL(triggered()), this, SLOT(OnToolsIsovistpath()));

	//运行
	runVisibilityGraphAnalysisAct = new QAction(tr("&Run Visibility Graph Analysis..."), this);
	connect(runVisibilityGraphAnalysisAct, SIGNAL(triggered()), this, SLOT(OnToolsRun()));

	//PD
	visibilityStepAct = new QAction(tr("&Visibility Step"), this);
	visibilityStepAct->setStatusTip(tr("Step depth from current selection\nStep Depth"));
	connect(visibilityStepAct, SIGNAL(triggered()), this, SLOT(OnToolsPD()));

	//MPD
	metricStepAct = new QAction(tr("&Metric Step"), this);
	metricStepAct->setStatusTip(tr("Distance from current selection\nMetric Depth"));
	connect(metricStepAct, SIGNAL(triggered()), this, SLOT(OnToolsMPD()));

	//APD
	angularStepAct = new QAction(tr("&Angular Step"), this);
	angularStepAct->setStatusTip(tr("Angular distance from current selection\nAngular Depth"));
	connect(angularStepAct, SIGNAL(triggered()), this, SLOT(OnToolsAPD()));

	//点转换为Map
	convertDataMapLinesAct = new QAction(tr("Convert Data Map Lines to Merge Points"), this);
	convertDataMapLinesAct->setStatusTip(tr("Convert displayed data map lines to merge points for current visibility graph"));
	connect(convertDataMapLinesAct, SIGNAL(triggered()), this, SLOT(OnToolsPointConvShapeMap()));

	//代理运行
	runAgentAnalysisAct = new QAction(tr("&Run Agent Analysis"), this);
	connect(runAgentAnalysisAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentRun()));

	//代理加载程序
	loadAgentProgramAct = new QAction(tr("&Load Agent Program"), this);
	connect(loadAgentProgramAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentLoadProgram()));

	//运行Axa
	runGraphAnaysisAct = new QAction(tr("&Run Graph Analysis..."), this);
	runGraphAnaysisAct->setStatusTip(tr("Analyse currently displayed axial line map\nAxial analysis"));
	connect(runGraphAnaysisAct, SIGNAL(triggered()), this, SLOT(OnToolsRunAxa()));

	//PD
	stepDepthAct = new QAction(tr("Step &Depth"), this);
	stepDepthAct->setShortcut(tr("Ctrl+D"));
	stepDepthAct->setStatusTip(tr("Step depth from current selection\nStep Depth"));
	connect(stepDepthAct, SIGNAL(triggered()), this, SLOT(OnToolsPD()));

	//制作最少线Map
	reduceToFewestLineMapAct = new QAction(tr("Reduce to &Fewest Line Map..."), this);
	connect(reduceToFewestLineMapAct, SIGNAL(triggered()), this, SLOT(OnToolsMakeFewestLineMap()));

	//轴向图转为Map
	convertDataMapPointsAct = new QAction(tr("Convert Data Map Points to Unlinks"), this);
	convertDataMapPointsAct->setStatusTip(tr("Convert displayed data map points to unlinks for current axial map"));
	connect(convertDataMapPointsAct, SIGNAL(triggered()), this, SLOT(OnToolsAxialConvShapeMap()));

	//线段加载取消链接
	loadUnlinksFromFileAct = new QAction(tr("Load Unlinks from File..."), this);
	connect(loadUnlinksFromFileAct, SIGNAL(triggered()), this, SLOT(OnToolsLineLoadUnlinks()));

	//运行段
	runAngularSegmentAnalysisAct = new QAction(tr("&Run Angular Segment Analysis..."), this);
	connect(runAngularSegmentAnalysisAct, SIGNAL(triggered()), this, SLOT(OnToolsRunSeg()));

	//顶图
	runTopologicalOrMetricAnalysisAct = new QAction(tr("Run &Topological or Metric Analysis..."), this);
	connect(runTopologicalOrMetricAnalysisAct, SIGNAL(triggered()), this, SLOT(OnToolsTopomet()));

	//PD
	segmentAngularStepAct = new QAction(tr("&Angular Step"), this);
	segmentAngularStepAct->setShortcut(tr("Ctrl+D"));
	segmentAngularStepAct->setStatusTip(tr("Step depth from current selection\nStep Depth"));
	connect(segmentAngularStepAct, SIGNAL(triggered()), this, SLOT(OnToolsPD()));

	//TPD
	topologicalStepAct = new QAction(tr("&Topological Step"), this);
	connect(topologicalStepAct, SIGNAL(triggered()), this, SLOT(OnToolsTPD()));

	//MPD
	segmentMetricStepAct = new QAction(tr("&Metric Step"), this);
	connect(segmentMetricStepAct, SIGNAL(triggered()), this, SLOT(OnToolsMPD()));

	//选项
	optionsAct = new QAction(tr("Options..."), this);
	connect(optionsAct, SIGNAL(triggered()), this, SLOT(OnToolsOptions()));

	//View Menu Actions 视图菜单动作

	//显示网格
	showGridAct = new QAction(tr("Show &Grid"), this);
	showGridAct->setStatusTip(tr("Display grid"));
	showGridAct->setCheckable(true);
	connect(showGridAct, SIGNAL(triggered()), this, SLOT(OnViewShowGrid()));

	//概要视图
	attributeSummaryAct = new QAction(tr("&Attribute Summary..."), this);
	attributeSummaryAct->setStatusTip(tr("Show summarised attribute information for selected points"));
	connect(attributeSummaryAct, SIGNAL(triggered()), this, SLOT(OnViewSummary()));

	//Window Menu Actions 窗口菜单动作

	//Map窗口
	mapAct = new QAction(tr("&Map"), this);
	mapAct->setCheckable(true);
	connect(mapAct, SIGNAL(triggered()), this, SLOT(OnWindowMap()));

	//散点图视图
	scatterPlotAct = new QAction(tr("&Scatter Plot"), this);
	scatterPlotAct->setCheckable(true);
	connect(scatterPlotAct, SIGNAL(triggered()), this, SLOT(OnViewScatterplot()));

	//表格视图
	tableAct = new QAction(tr("&Table"), this);
	tableAct->setCheckable(true);
	connect(tableAct, SIGNAL(triggered()), this, SLOT(OnViewTable()));

	//3D视图
	thirdDViewAct = new QAction(tr("&3D View"), this);
	thirdDViewAct->setCheckable(true);
	connect(thirdDViewAct, SIGNAL(triggered()), this, SLOT(OnWindow3dView()));

	//GL视图
	glViewAct = new QAction(tr("Map (Open&GL)"), this);
	glViewAct->setCheckable(true);
	connect(glViewAct, SIGNAL(triggered()), this, SLOT(OnWindowGLView()));

	//颜色范围视图
	colourRangeAct = new QAction(tr("&Colour Range"), this);
	connect(colourRangeAct, SIGNAL(triggered()), this, SLOT(OnViewColourRange()));

	//级联子窗口
	cascadeAct = new QAction(tr("C&ascade"), this);
	cascadeAct->setStatusTip(tr("Arrange windows so they overlap\nCascade Windows"));
	connect(cascadeAct, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));

	//子窗口标题
	tileAct = new QAction(tr("T&ile"), this);
	tileAct->setStatusTip(tr("Arrange windows as non-overlapping tiles\nTile Windows"));
	connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

	//布置子窗口
	arrangeIconsAct = new QAction(tr("A&rrange Icons"), this);
	arrangeIconsAct->setStatusTip(tr("Arrange icons at the bottom of the window\nArrange Icons"));
	//	connect(arrangeIconsAct, SIGNAL(triggered()), mdiArea, SLOT(arrangeSubWindows()));

		//Help Menu Actions 帮助菜单动作

		//帮助错误
	onlineBugsAct = new QAction(tr("Submit Problems/Ideas"), this);
	onlineBugsAct->setStatusTip(tr("View or Submit Problems and Ideas online"));
	connect(onlineBugsAct, SIGNAL(triggered()), this, SLOT(OnHelpBugs()));

	//帮助菜单
	onlineHandbookAct = new QAction(tr("Get the PDF &Handbook"), this);
	onlineHandbookAct->setStatusTip(tr("View the Original Depthmap Researchers' Handbook"));
	connect(onlineHandbookAct, SIGNAL(triggered()), this, SLOT(OnHelpManual()));

	//帮助教程
	onlineTutorialsAct = new QAction(tr("Online &Tutorials"), this);
	onlineTutorialsAct->setStatusTip(tr("View the Original Depthmap tutorials online"));
	connect(onlineTutorialsAct, SIGNAL(triggered()), this, SLOT(OnHelpTutorials()));

	//帮助手册
	onlineScriptingManualAct = new QAction(tr("Online &Scripting Manual"), this);
	onlineScriptingManualAct->setStatusTip(tr("See the Original SalaScript web page"));
	connect(onlineScriptingManualAct, SIGNAL(triggered()), this, SLOT(OnHelpSalaManual()));

	//软件相关信息
	aboutDepthMapAct = new QAction(tr("About &depthmapX..."), this);
	aboutDepthMapAct->setStatusTip(tr("Display program information, version number and copyright\nAbout"));
	connect(aboutDepthMapAct, SIGNAL(triggered()), this, SLOT(OnAppAbout()));

	// ToolBar actions 工具栏动作

	//颜色转换
	invertColorAct = new QAction(QIcon(":/images/win/b-5-18.png"), tr("Invert Colour Range"), this);
	invertColorAct->setStatusTip(tr("Invert the colour range\nInvert Colour Range"));
	connect(invertColorAct, SIGNAL(triggered()), this, SLOT(OninvertColor()));

	//添加列
	addColumAct = new QAction(tr("&Add"), this);
	addColumAct->setStatusTip(tr("Add column to the active map\nAdd Column"));
	connect(addColumAct, SIGNAL(triggered()), this, SLOT(OnAddColumn()));

	//更新列
	updateColumAct = new QAction(tr("&Update"), this);
	updateColumAct->setStatusTip(tr("Replace column contents using a SalaScript command\nUpdate Column"));
	connect(updateColumAct, SIGNAL(triggered()), this, SLOT(OnUpdateColumn()));

	//删除列
	removeColumAct = new QAction(tr("&Delete"), this);
	removeColumAct->setStatusTip(tr("Delete column from the active map\nRemove column"));
	connect(removeColumAct, SIGNAL(triggered()), this, SLOT(OnRemoveColumn()));

	//添加到图层
	pushValueAct = new QAction(QIcon(":/images/win/b-5-22.png"), tr("&Push Values"), this);
	pushValueAct->setStatusTip(tr("Push values from active map to another map\npushValue"));
	connect(pushValueAct, SIGNAL(triggered()), this, SLOT(OnPushToLayer()));

	//缩放
	zoomToAct = new QAction(QIcon(":/images/win/b-5-5.png"), tr("Color Range"), this);
	zoomToAct->setStatusTip(tr("select color range"));
	connect(zoomToAct, SIGNAL(triggered()), this, SLOT(OnViewColourRange()));

	//中心视图
	RecentAct = new QAction(QIcon(":/images/win/b-5-6.png"), tr("&Recentre View"), this);
	RecentAct->setStatusTip(tr("Fit map to window\nRecentre"));
	connect(RecentAct, SIGNAL(triggered()), this, SLOT(OnViewCentreView()));

	//编辑网格
	SetGridAct = new QAction(QIcon(":/images/win/b-5-7.png"), tr("Set Grid"), this);
	SetGridAct->setStatusTip(tr("Overlay grid on plan\nSet Grid"));
	connect(SetGridAct, SIGNAL(triggered()), this, SLOT(OnEditGrid()));

	//切换颜色
	toggleColor = new QAction(QIcon(":/images/win/b-7-1.png"), tr("Toggle Colour"), this);
	toggleColor->setCheckable(1);
	toggleColor->setStatusTip(tr("Toggle colour display on and off\nToggle Colour"));
	connect(toggleColor, SIGNAL(triggered()), this, SLOT(OntoggleColor()));

	//切换组织
	toggleOrg = new QAction(QIcon(":/images/win/b-7-2.png"), tr("Toggle origin on/off"), this);
	toggleOrg->setCheckable(1);
	toggleOrg->setStatusTip(tr("Toggle graph intersect at origin of X, Y values\nToggle origin on/off"));
	connect(toggleOrg, SIGNAL(triggered()), this, SLOT(OntoggleOrg()));

	//趋势视图
	viewTrend = new QAction(QIcon(":/images/win/b-7-3.png"), tr("View trend line"), this);
	viewTrend->setCheckable(1);
	viewTrend->setStatusTip(tr("Show regression line\nView trend line"));
	connect(viewTrend, SIGNAL(triggered()), this, SLOT(OnviewTrend()));

	//YX
	yx = new QAction(QIcon(":/images/win/b-7-4.png"), tr(""), this);
	yx->setCheckable(1);
	yx->setStatusTip(tr(""));
	connect(yx, SIGNAL(triggered()), this, SLOT(OnYX()));

	//Rtwo
	Rtwo = new QAction(QIcon(":/images/win/b-7-5.png"), tr(""), this);
	Rtwo->setCheckable(1);
	Rtwo->setStatusTip(tr(""));
	connect(Rtwo, SIGNAL(triggered()), this, SLOT(OnRtwo()));

	///////////////////////////////////////////////////////////
	//Popup toolbar 弹出工具栏
	{
		zoomToolButton = new QToolButton;
		zoomToolButton->setPopupMode(QToolButton::MenuButtonPopup);
		QMenu *zoomMenu = new QMenu;
		zoomInAct = new QAction(tr("Zoom in"), this);
		zoomInAct->setStatusTip(tr("Zoom into or out of (using Alt-key) view of map\nZoom In"));
		zoomInAct->setCheckable(1);
		zoomInAct->setChecked(1);
		zoomInAct->setData(ID_MAPBAR_ITEM_ZOOM_IN);
		connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomModeTriggered()));
		zoomMenu->addAction(zoomInAct);
		zoomOutAct = new QAction(tr("Zoom out"), this);
		zoomOutAct->setStatusTip(tr("Zoom out of or into (using Alt-key) view of map\nZoom Out"));
		zoomOutAct->setCheckable(1);
		zoomOutAct->setData(ID_MAPBAR_ITEM_ZOOM_OUT);
		connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomModeTriggered()));
		zoomMenu->addAction(zoomOutAct);
		zoomMenu->setDefaultAction(zoomInAct);
		zoomToolButton->setMenu(zoomMenu);
		zoomToolButton->setIcon(QIcon(":/images/win/b-5-3.png"));
		zoomToolButton->setCheckable(1);
		connect(zoomToolButton, SIGNAL(clicked()), this, SLOT(zoomButtonTriggered()));

		QActionGroup* tGroup = new QActionGroup(this);
		tGroup->addAction(zoomInAct);
		tGroup->addAction(zoomOutAct);
	}
	{
		fillColorToolButton = new QToolButton;
		fillColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
		QMenu *fillColorMenu = new QMenu;

		STDFillColorAct = new QAction(tr("Standard Fill"), this);
		STDFillColorAct->setStatusTip(tr("Standard Fill grid spaces with points\nFill"));
		STDFillColorAct->setCheckable(1);
		STDFillColorAct->setChecked(1);
		STDFillColorAct->setData(ID_MAPBAR_ITEM_FILL);
		connect(STDFillColorAct, SIGNAL(triggered()), this, SLOT(FillModeTriggered()));

		ContextFillColorAct = new QAction(tr("Context Fill"), this);
		ContextFillColorAct->setStatusTip(tr("Context Fill grid spaces with points\nFill"));
		ContextFillColorAct->setCheckable(1);
		ContextFillColorAct->setData(ID_MAPBAR_ITEM_SEMIFILL);
		connect(ContextFillColorAct, SIGNAL(triggered()), this, SLOT(FillModeTriggered()));

		// AV test - TV
		AugmentFillColorAct = new QAction(tr("Augmented Fill"), this);
		AugmentFillColorAct->setStatusTip(tr("Augmented Fill grid spaces with points\nFill"));
		AugmentFillColorAct->setCheckable(1);
		AugmentFillColorAct->setData(ID_MAPBAR_ITEM_AUGMENT_FILL);
		//connect(AugmentFillColorAct, SIGNAL(triggered()), this, SLOT(FillModeTriggered()));

		fillColorMenu->addAction(STDFillColorAct);
		fillColorMenu->addAction(ContextFillColorAct);
		//fillColorMenu->addAction(AugmentFillColorAct); // AV TV
		fillColorMenu->setDefaultAction(STDFillColorAct);
		fillColorToolButton->setMenu(fillColorMenu);
		fillColorToolButton->setIcon(QIcon(":/images/win/b-5-8.png"));
		fillColorToolButton->setCheckable(1);
		connect(fillColorToolButton, SIGNAL(clicked()), this, SLOT(FillButtonTriggered()));

		QActionGroup* tGroup = new QActionGroup(this);
		tGroup->addAction(STDFillColorAct);
		tGroup->addAction(ContextFillColorAct);
		tGroup->addAction(AugmentFillColorAct); // AV TV
	}
	{
		lineToolButton = new QToolButton;
		lineToolButton->setPopupMode(QToolButton::MenuButtonPopup);
		QMenu *lineToolMenu = new QMenu;
		SelectLineAct = new QAction(tr("Line"), this);
		SelectLineAct->setStatusTip(tr("Draw a new line\nLine"));
		SelectLineAct->setCheckable(1);
		SelectLineAct->setChecked(1);
		SelectLineAct->setData(ID_MAPBAR_ITEM_LINETOOL);
		connect(SelectLineAct, SIGNAL(triggered()), this, SLOT(LineModeTriggered()));
		SelectPolyLineAct = new QAction(tr("Polygon"), this);
		SelectPolyLineAct->setStatusTip(tr("Draw a new polygon\nPolygon"));
		SelectPolyLineAct->setCheckable(1);
		SelectPolyLineAct->setData(ID_MAPBAR_ITEM_POLYGON);
		connect(SelectPolyLineAct, SIGNAL(triggered()), this, SLOT(LineModeTriggered()));
		lineToolMenu->addAction(SelectLineAct);
		lineToolMenu->addAction(SelectPolyLineAct);
		lineToolMenu->setDefaultAction(SelectLineAct);
		lineToolButton->setMenu(lineToolMenu);
		lineToolButton->setIcon(QIcon(":/images/win/b-5-10.png"));
		lineToolButton->setCheckable(1);
		connect(lineToolButton, SIGNAL(clicked()), this, SLOT(LineButtonTriggered()));

		QActionGroup* tGroup = new QActionGroup(this);
		tGroup->addAction(SelectLineAct);
		tGroup->addAction(SelectPolyLineAct);
	}
	{
		newisoToolButton = new QToolButton;
		newisoToolButton->setPopupMode(QToolButton::MenuButtonPopup);
		QMenu *isoToolMenu = new QMenu;
		MakeIosAct = new QAction(tr("Isovisit"), this);
		MakeIosAct->setStatusTip(tr("Make a new isovist\nIsovist"));
		MakeIosAct->setCheckable(1);
		MakeIosAct->setChecked(1);
		MakeIosAct->setData(ID_MAPBAR_ITEM_ISOVIST);
		connect(MakeIosAct, SIGNAL(triggered()), this, SLOT(isoModeTriggered()));
		PartialMakeIosAct = new QAction(tr("Partial isovisist"), this);
		PartialMakeIosAct->setStatusTip(tr("Make a new partial isovist\nPartial Isovist"));
		PartialMakeIosAct->setCheckable(1);
		PartialMakeIosAct->setData(ID_MAPBAR_ITEM_HALFISOVIST);
		connect(PartialMakeIosAct, SIGNAL(triggered()), this, SLOT(isoModeTriggered()));
		isoToolMenu->addAction(MakeIosAct);
		isoToolMenu->addAction(PartialMakeIosAct);
		isoToolMenu->setDefaultAction(MakeIosAct);
		newisoToolButton->setMenu(isoToolMenu);
		newisoToolButton->setIcon(QIcon(":/images/win/b-5-12.png"));
		newisoToolButton->setCheckable(1);
		connect(newisoToolButton, SIGNAL(clicked()), this, SLOT(isoButtonTriggered()));

		QActionGroup* tGroup = new QActionGroup(this);
		tGroup->addAction(MakeIosAct);
		tGroup->addAction(PartialMakeIosAct);
	}
	{
		JoinToolButton = new QToolButton;
		JoinToolButton->setPopupMode(QToolButton::MenuButtonPopup);
		QMenu *joinToolMenu = new QMenu;
		JoinAct = new QAction(tr("Link"), this);
		JoinAct->setStatusTip(tr("merge points together\nLink"));
		JoinAct->setCheckable(1);
		JoinAct->setChecked(1);
		JoinAct->setData(ID_MAPBAR_ITEM_JOIN);
		connect(JoinAct, SIGNAL(triggered()), this, SLOT(joinTriggered()));
		JoinUnlinkAct = new QAction(tr("unLink"), this);
		JoinUnlinkAct->setStatusTip(tr("unmerge points\nUnlink"));
		JoinUnlinkAct->setCheckable(1);
		JoinUnlinkAct->setData(ID_MAPBAR_ITEM_UNJOIN);
		connect(JoinUnlinkAct, SIGNAL(triggered()), this, SLOT(joinTriggered()));
		joinToolMenu->addAction(JoinAct);
		joinToolMenu->addAction(JoinUnlinkAct);
		joinToolMenu->setDefaultAction(JoinAct);
		JoinToolButton->setMenu(joinToolMenu);
		JoinToolButton->setIcon(QIcon(":/images/win/b-5-16.png"));
		JoinToolButton->setCheckable(1);
		connect(JoinToolButton, SIGNAL(clicked()), this, SLOT(joinButtonTriggered()));

		QActionGroup* tGroup = new QActionGroup(this);
		tGroup->addAction(JoinAct);
		tGroup->addAction(JoinUnlinkAct);
	}

	SelectButton = new QToolButton;
	SelectButton->setStatusTip(tr("Select a grid point\nSelect"));
	SelectButton->setIcon(QIcon(":/images/win/b-5-1.png"));
	SelectButton->setCheckable(1);
	connect(SelectButton, SIGNAL(clicked()), this, SLOT(SelectButtonTriggered()));
	DragButton = new QToolButton;
	DragButton->setStatusTip(tr("Click and drag to move map\nDrag"));
	DragButton->setIcon(QIcon(":/images/win/b-5-2.png"));
	DragButton->setCheckable(1);
	connect(DragButton, SIGNAL(clicked()), this, SLOT(DragButtonTriggered()));
	SelectPenButton = new QToolButton;
	SelectPenButton->setStatusTip(tr("Fill grid spaces individually (or click on a filled space to clear)\nPencil"));
	SelectPenButton->setIcon(QIcon(":/images/win/b-5-9.png"));
	SelectPenButton->setCheckable(1);
	connect(SelectPenButton, SIGNAL(clicked()), this, SLOT(SelectPenTriggered()));
	AxialMapButton = new QToolButton;
	AxialMapButton->setStatusTip(tr("Construct all line axial map from a seed point\nAxial Map"));
	AxialMapButton->setIcon(QIcon(":/images/win/b-5-14.png"));
	AxialMapButton->setCheckable(1);
	connect(AxialMapButton, SIGNAL(clicked()), this, SLOT(AxialMapTriggered()));
	StepDepthButton = new QToolButton;
	StepDepthButton->setStatusTip(tr("Step depth from current selection\nStep Depth"));
	StepDepthButton->setIcon(QIcon(":/images/win/b-5-15.png"));
	StepDepthButton->setCheckable(1);
	connect(StepDepthButton, SIGNAL(clicked()), this, SLOT(StepDepthTriggered()));

	QButtonGroup* pointerTypeGroup = new QButtonGroup;
	pointerTypeGroup->addButton(JoinToolButton, ID_MAPBAR_JOIN_ITEMS);
	pointerTypeGroup->addButton(zoomToolButton, ID_MAPBAR_ZOOM_ITEMS);
	pointerTypeGroup->addButton(fillColorToolButton, ID_MAPBAR_FILL_ITEMS);
	pointerTypeGroup->addButton(lineToolButton, ID_MAPBAR_DRAW_ITEMS);
	pointerTypeGroup->addButton(newisoToolButton, ID_MAPBAR_ISOVIST_ITEMS);
	pointerTypeGroup->addButton(SelectButton, ID_MAPBAR_ITEM_SELECT);
	pointerTypeGroup->addButton(DragButton, ID_MAPBAR_ITEM_MOVE);
	pointerTypeGroup->addButton(SelectPenButton, ID_MAPBAR_ITEM_PENCIL);
	pointerTypeGroup->addButton(AxialMapButton, ID_MAPBAR_ITEM_AL2);
	pointerTypeGroup->addButton(StepDepthButton, ID_MAPBAR_ITEM_PD);

	{
		toolsImportTracesAct = new QAction(QIcon(":/images/win/b-4-1.png"), tr("Import Traces"), this);
		toolsImportTracesAct->setStatusTip(tr("Import agent traces from a file\nImport Traces"));
		connect(toolsImportTracesAct, SIGNAL(triggered()), this, SLOT(OnToolsImportTraces()));

		addAgentAct = new QAction(QIcon(":/images/win/b-4-2.png"), tr("Add Agent"), this);
		addAgentAct->setCheckable(1);
		connect(addAgentAct, SIGNAL(triggered()), this, SLOT(OnAddAgent()));

		QActionGroup *tGroup = new QActionGroup(this);
		tGroup->addAction(toolsImportTracesAct);
		tGroup->addAction(addAgentAct);
	}
	{
		toolsAgentsPlayAct = new QAction(QIcon(":/images/win/b-4-3.png"), tr("Agents Play"), this);
		toolsAgentsPlayAct->setCheckable(1);
		connect(toolsAgentsPlayAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentsPlay()));

		toolsAgentsPauseAct = new QAction(QIcon(":/images/win/b-4-4.png"), tr("Agents Pause"), this);
		toolsAgentsPauseAct->setCheckable(1);
		connect(toolsAgentsPauseAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentsPause()));

		toolsAgentsStopAct = new QAction(QIcon(":/images/win/b-4-5.png"), tr("Agents Stop"), this);
		connect(toolsAgentsStopAct, SIGNAL(triggered()), this, SLOT(OnToolsAgentsStop()));

		QActionGroup *tGroup = new QActionGroup(this);
		tGroup->addAction(toolsAgentsPlayAct);
		tGroup->addAction(toolsAgentsPauseAct);
		tGroup->addAction(toolsAgentsStopAct);
	}
	agentTrailsAct = new QAction(QIcon(":/images/win/b-4-6.png"), tr("Agent Trails"), this);
	agentTrailsAct->setCheckable(1);
	connect(agentTrailsAct, SIGNAL(triggered()), this, SLOT(OnAgentTrails()));
	{
		thirdRotAct = new QAction(QIcon(":/images/win/b-4-7.png"), tr("3D Rot"), this);
		thirdRotAct->setCheckable(1);
		connect(thirdRotAct, SIGNAL(triggered()), this, SLOT(On3dRot()));

		thirdPanAct = new QAction(QIcon(":/images/win/b-4-8.png"), tr("3D Pan"), this);
		thirdPanAct->setCheckable(1);
		connect(thirdPanAct, SIGNAL(triggered()), this, SLOT(On3dPan()));

		thirdZoomAct = new QAction(QIcon(":/images/win/b-4-9.png"), tr("3D Zoom"), this);
		thirdZoomAct->setCheckable(1);
		connect(thirdZoomAct, SIGNAL(triggered()), this, SLOT(On3dZoom()));

		playLoopAct = new QAction(QIcon(":/images/win/b-4-10.png"), tr("Play Loop"), this);
		playLoopAct->setCheckable(1);
		connect(playLoopAct, SIGNAL(triggered()), this, SLOT(OnPlayLoop()));

		QActionGroup *tGroup = new QActionGroup(this);
		tGroup->addAction(thirdRotAct);
		tGroup->addAction(thirdPanAct);
		tGroup->addAction(thirdZoomAct);
		tGroup->addAction(playLoopAct);
		tGroup->addAction(addAgentAct);
	}
	thirdFilledAct = new QAction(QIcon(":/images/win/b-4-11.png"), tr("3D Filled"), this);
	thirdFilledAct->setCheckable(1);
	connect(thirdFilledAct, SIGNAL(triggered()), this, SLOT(On3dFilled()));
}

void MainWindow::createMenus()
{
	fileMenu = menuBar()->addMenu(tr("&File"));
	//fileMenu->addAction(newAct);
	fileMenu->addAction(openAct);
	//fileMenu->addAction(closeAct);
	fileMenu->addAction(saveAct);
	//fileMenu->addAction(saveAsAct);
	//fileMenu->addSeparator();
	//fileMenu->addAction(propertiesAct);
	//fileMenu->addSeparator();
	//fileMenu->addAction(printAct);
	//fileMenu->addAction(printPreviewAct);
	//fileMenu->addAction(printSetupAct);
	separatorAct = fileMenu->addSeparator();
	//for (int i = 0; i < MaxRecentFiles; ++i)
	//    fileMenu->addAction(recentFileActs[i]);
	//fileMenu->addSeparator();
	//fileMenu->addAction(exitAct);
	//updateRecentFileActions(mSettings.readSetting(SettingTag::recentFileList).toStringList());

	//editMenu = menuBar()->addMenu(tr("&Edit"));
	//editMenu->addAction(undoAct);
	//editMenu->addSeparator();
	//editMenu->addAction(copyDataAct);
	//editMenu->addAction(copyScreenAct);
	//editMenu->addAction(exportScreenAct);
	//editMenu->addSeparator();
	//editMenu->addAction(clearAct);
	//editMenu->addSeparator();
	//editMenu->addAction(selectByQueryAct);
	////editMenu->addAction(zoomToSelectionAct);
	//editMenu->addAction(selectionToLayerAct);

	//mapMenu = menuBar()->addMenu(tr("&Map"));
	//mapMenu->addAction(mapNewAct);
	//mapMenu->addAction(deleteAct);
	//mapMenu->addSeparator();
	//mapMenu->addAction(convertActiveMapAct);
	//mapMenu->addAction(convertDrawingMapAct);
	//mapMenu->addAction(convertMapShapesAct);
	//mapMenu->addSeparator();
	//mapMenu->addAction(importAct);
	//exportSubMenu = mapMenu->addMenu(tr("&Export"));
	//exportSubMenu->addAction(exportAct);
	//exportSubMenu->addAction(exportGeometryAct);
	//exportSubMenu->addAction(exportLinksAct);
	//exportSubMenu->addAction(exportAxialConnectionsDotAct);
	//exportSubMenu->addAction(exportAxialConnectionsPairAct);
	//exportSubMenu->addAction(exportSegmentConnectionsPairAct);
	//exportSubMenu->addAction(exportPointmapConnectionsPairAct);

	//attributesMenu = menuBar()->addMenu(tr("&Attributes"));
	//attributesMenu->addAction(addColumAct);
	//attributesMenu->addSeparator();
	//attributesMenu->addAction(renameColumnAct);
	//attributesMenu->addAction(updateColumAct);
	//attributesMenu->addAction(removeColumAct);
	//attributesMenu->addAction(columnPropertiesAct);
	//attributesMenu->addSeparator();
	//attributesMenu->addAction(pushValueAct);

	//toolsMenu = menuBar()->addMenu(tr("&Tools"));
	//visibilitySubMenu = toolsMenu->addMenu(tr("&Visibility"));
	//visibilitySubMenu->addAction(SetGridAct);
	//visibilitySubMenu->addAction(makeVisibilityGraphAct);
	//visibilitySubMenu->addAction(unmakeVisibilityGraphAct);
	//visibilitySubMenu->addAction(importVGALinksAct);
	//visibilitySubMenu->addAction(makeIsovistPathAct);
	//visibilitySubMenu->addSeparator();
	//visibilitySubMenu->addAction(runVisibilityGraphAnalysisAct);
	//stepDepthSubMenu = visibilitySubMenu->addMenu(tr("Step &Depth"));
	//stepDepthSubMenu->addAction(visibilityStepAct);
	//stepDepthSubMenu->addAction(metricStepAct);
	//stepDepthSubMenu->addAction(angularStepAct);
	//visibilitySubMenu->addSeparator();
	//visibilitySubMenu->addAction(convertDataMapLinesAct);
	//agentToolsSubMenu = toolsMenu->addMenu(tr("&Agent Tools"));
	//agentToolsSubMenu->addAction(runAgentAnalysisAct);
	//agentToolsSubMenu->addAction(loadAgentProgramAct);

	//axialSubMenu = toolsMenu->addMenu(tr("A&xial / Convex / Pesh"));
	//axialSubMenu->addAction(runGraphAnaysisAct);
	//axialSubMenu->addAction(stepDepthAct);
	//axialSubMenu->addSeparator();
	//axialSubMenu->addAction(reduceToFewestLineMapAct);
	//axialSubMenu->addSeparator();
	//axialSubMenu->addAction(convertDataMapPointsAct);
	//axialSubMenu->addAction(loadUnlinksFromFileAct);

	//segmentSubMenu = toolsMenu->addMenu(tr("&Segment"));
	//segmentSubMenu->addAction(runAngularSegmentAnalysisAct);
	//segmentSubMenu->addAction(runTopologicalOrMetricAnalysisAct);
	//segmentStepDepthSubMenu = segmentSubMenu->addMenu(tr("Step &Depth"));
	//segmentStepDepthSubMenu->addAction(segmentAngularStepAct);
	//segmentStepDepthSubMenu->addAction(topologicalStepAct);
	//segmentStepDepthSubMenu->addAction(segmentMetricStepAct);

	//toolsMenu->addSeparator();
	//toolsMenu->addAction(optionsAct);

	//viewMenu = menuBar()->addMenu(tr("&View"));
	//viewMenu->addAction(RecentAct);
	//viewMenu->addAction(showGridAct);
	//viewMenu->addAction(attributeSummaryAct);

	//windowMenu = menuBar()->addMenu(tr("&Window"));
	//windowMenu->addAction(mapAct);
	//windowMenu->addAction(scatterPlotAct);
	//windowMenu->addAction(tableAct);
	//windowMenu->addAction(thirdDViewAct);
	//windowMenu->addAction(glViewAct);
	//windowMenu->addSeparator();
	//windowMenu->addAction(colourRangeAct);
	//windowMenu->addSeparator();
	//windowMenu->addAction(cascadeAct);
	//windowMenu->addAction(tileAct);
	//windowMenu->addAction(arrangeIconsAct);

	//helpMenu = menuBar()->addMenu(tr("&Help"));
	//helpMenu->addAction(onlineBugsAct);
	//helpMenu->addAction(onlineHandbookAct);
	//helpMenu->addAction(onlineTutorialsAct);
	//helpMenu->addAction(onlineScriptingManualAct);
	//helpMenu->addSeparator();
	//helpMenu->addAction(aboutDepthMapAct);

	connect(viewMenu, SIGNAL(aboutToShow()), this, SLOT(updateViewMenu()));
	connect(visibilitySubMenu, SIGNAL(aboutToShow()), this, SLOT(updateVisibilitySubMenu()));
	connect(stepDepthSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateStepDepthSubMenu()));
	connect(agentToolsSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateAgentToolsSubMenu()));
	connect(segmentSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateSegmentSubMenu()));
	connect(segmentStepDepthSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateSegmentStepDepthSubMenu()));
	connect(axialSubMenu, SIGNAL(aboutToShow()), this, SLOT(updateAxialSubMenu()));
	connect(attributesMenu, SIGNAL(aboutToShow()), this, SLOT(updateAttributesMenu()));
	connect(mapMenu, SIGNAL(aboutToShow()), this, SLOT(updateMapMenu()));
	connect(editMenu, SIGNAL(aboutToShow()), this, SLOT(updateEditMenu()));
	connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(updateFileMenu()));
	connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));
}

void MainWindow::createToolBars()
{
	fileToolBar = addToolBar(tr("File"));

	//设置悬浮
	fileToolBar->setFloatable(true);


	//fileToolBar->addAction(newAct);
	fileToolBar->addAction(openAct);
	// fileToolBar->addAction(importAct);
	fileToolBar->addAction(saveAct);
	fileToolBar->addSeparator();
	//fileToolBar->addAction(addColumAct);
	//fileToolBar->addAction(updateColumAct);
	//fileToolBar->addAction(removeColumAct);
	//fileToolBar->addAction(pushValueAct);
	//fileToolBar->addSeparator();
	//fileToolBar->addAction(invertColorAct);
	//fileToolBar->setIconSize(QSize(10,10));
	//fileToolBar->setMovable(0);

	editToolBar = addToolBar(tr("Edit"));
	editToolBar->addWidget(SelectButton);
	editToolBar->addWidget(DragButton);
	editToolBar->addWidget(zoomToolButton);
	editToolBar->addSeparator();
	editToolBar->addAction(RecentAct);
	editToolBar->addSeparator();
	//颜色编辑
	editToolBar->addAction(zoomToAct);
	editToolBar->addSeparator();
	//editToolBar->setFloatable(true);
	//editToolBar->addAction(columnPropertiesAct);
	//editToolBar->addAction(SetGridAct);
	//editToolBar->addWidget(fillColorToolButton);
	//editToolBar->addWidget(SelectPenButton);
	//editToolBar->addWidget(lineToolButton);
	//editToolBar->addSeparator();
	//editToolBar->addWidget(newisoToolButton);
	//editToolBar->addWidget(AxialMapButton);
	//editToolBar->addWidget(StepDepthButton);
	//editToolBar->addWidget(JoinToolButton);
	//editToolBar->setIconSize(QSize(10,10));

	x_coord = new QComboBox(this);
	x_coord->setMinimumContentsLength(20);
	y_coord = new QComboBox(this);
	y_coord->setMinimumContentsLength(20);

	connect(x_coord, SIGNAL(currentIndexChanged(const QString &)),
		this, SLOT(OnSelchangeViewSelector_X(const QString &)));
	connect(y_coord, SIGNAL(currentIndexChanged(const QString &)),
		this, SLOT(OnSelchangeViewSelector_Y(const QString &)));


	QAction* xx = new QAction(tr("X = "), this);
	xx->setEnabled(0);
	QAction* yy = new QAction(tr("Y = "), this);
	yy->setEnabled(0);

	plotToolBar = addToolBar(tr("PlotEdit"));
	plotToolBar->addAction(xx);
	plotToolBar->addWidget(x_coord);
	plotToolBar->addSeparator();
	plotToolBar->addAction(yy);
	plotToolBar->addWidget(y_coord);
	plotToolBar->addSeparator();
	plotToolBar->addAction(toggleColor);
	plotToolBar->addAction(toggleOrg);
	plotToolBar->addAction(viewTrend);
	plotToolBar->addAction(yx);
	plotToolBar->addAction(Rtwo);
	//plotToolBar->setIconSize(QSize(10,10));
	//plotToolBar->setMovable(0);

	thirdViewToolBar = addToolBar(tr("3DView"));
	thirdViewToolBar->addAction(toolsImportTracesAct);
	thirdViewToolBar->addAction(addAgentAct);
	thirdViewToolBar->addSeparator();
	thirdViewToolBar->addAction(toolsAgentsPlayAct);
	thirdViewToolBar->addAction(toolsAgentsPauseAct);
	thirdViewToolBar->addAction(toolsAgentsStopAct);
	thirdViewToolBar->addSeparator();
	thirdViewToolBar->addAction(agentTrailsAct);
	thirdViewToolBar->addSeparator();
	thirdViewToolBar->addAction(thirdRotAct);
	thirdViewToolBar->addAction(thirdPanAct);
	thirdViewToolBar->addAction(thirdZoomAct);
	thirdViewToolBar->addAction(playLoopAct);
	thirdViewToolBar->addSeparator();
	thirdViewToolBar->addAction(thirdFilledAct);
	//thirdViewToolBar->setIconSize(QSize(10,10));
	//thirdViewToolBar->setMovable(0);

	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-1.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-2.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-3.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-4.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-5.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-6.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-7.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-8.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-9.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-10.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-11.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-12.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-13.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-14.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-15.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-16.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-17.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-18.png")));
	m_tree_icon.push_back(QIcon(tr(":/images/win/b-1-19.png")));
}
