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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "depthmapX/indexWidget.h"
#include "depthmapX/treeWindow.h"
#include "depthmapX/GraphDoc.h"
#include "depthmapX/compatibilitydefines.h"
#include "depthmapX/settings.h"

#include "depthmapX/dialogs/ColourScaleDlg.h"
#include "depthmapX/views/glview/glview.h"

#include "version.h"

#include <QMainWindow>
#include <QButtonGroup>
#include <QComboBox>
#include <QProgressBar>
#include <string>

#include "ShapeFileAccessor.h"
#include "Calculation.h"

#include "CalculateUI.h"
#include "ui_CalculateUI.h"

#include "UrbanConnectAnalyzer.h"
#include "ui_UrbanConnectAnalyzer.h"

#include "Display.h"
#include "GlobalMap.h"
#include "NetMap.h"
#include "GeoMap.h"

#include "ui_Highlight.h"
#include "Highlight.h"
#include "ui_ScaleWin.h"
#include "ScaleWin.h"
#include "ui_AttributesWin.h"
#include "AttributesWin.h"
#include "ui_Subset.h"
#include "Subset.h"
#include "ui_setForm.h"
#include "setForm.h"
#include "ui_Select.h"
#include "Select.h"

class ItemTreeEntry
{
public:
   ItemTreeEntry() { m_type = -1; m_cat = -1; m_subcat = -1; }
   ItemTreeEntry(char t, short c, short sc)
   { m_type = t; m_cat = c; m_subcat = sc; }
   char m_type;
   short m_cat;
   short m_subcat;
};

class QDepthmapView;
class QGraphDoc;

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
class QSignalMapper;
class QToolButton;
QT_END_NAMESPACE

const int  MaxRecentFiles = 5;

enum { FOCUSGRAPH = 1001, AllTransactionsDone = 1002 };

class QmyEvent : public QEvent
{
public:
    void* wparam;
    int lparam;
    QmyEvent(Type type, void* wp, int lp);

};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
	//构造函数
    MainWindow(const QString &fileToLoad, Settings &settings);
    ~MainWindow() override;
    //编码
    std::locale m_loc;
	//状态栏
	QLabel *locationLabel = new QLabel("Ready");
	QProgressBar * m_pConnectProBar = new QProgressBar();
	int nPos = 0, nMin = 0, nMax = 10000;

    //图分析选项
    QString m_formula_cache;	//公式缓存字符串
    Options m_options;			//平均深度计算的选项参数结构体
    QRgb m_foreground;			//前景色
    QRgb m_background;			//背景色

    bool m_simpleVersion;		//替换编译定义的布尔

    void RedoPlotViewMenu(QGraphDoc* pDoc);		//重写图视图菜单
    void updateToolbar();		//更新工具栏
    void update3DToolbar();		//更新3D工具栏
    void showContextMenu(QPoint &point);	//显示上下文菜单
    void UpdateStatus(QString s1, QString s2, QString s3);	//更新状态
	void UpdateLinesID() { emit(this->pDisplaydow->ui->Button_start_scan_8->clicked()); };
    void updateGLWindows(bool datasetChanged, bool recentreView);	//跟新GL窗口
    void loadFile(QString fileName);	//加载文件

	//计算模块
	CalculateUI *pDisplaydow = nullptr;
	UrbanConnectAnalyzer *work = nullptr;
	Highlight* highlight_window = nullptr;
	ScaleWin* scaleWin = nullptr;
	Select* selectWin = nullptr;
	AttributesWin* attributesWin = nullptr;
	Subset* subsetWin = nullptr;
	setForm* setFormWin = nullptr;

	//增加
	std::string whichOpen();	//查出哪个计算功能被勾选
	std::map<std::string, int> String_to_Index;
	void generateIndex();
	static volatile bool run_start;
	static volatile bool needOver;

	//目录树模块
	QString shpFileName;
	std::string infilepath;
	std::string dbfFilePath;
    std::string csvFilePath;
	QString csvFileName;
    QString lastDirectory; // 用于保存上次打开的文件夹路径

	//单线程函数
	Calculation CA;
	ShapeFileAccessor FA;

	void calculateMR_Single_thread();
	void calculateDR_Single_thread();
	void calculateDDL_Single_thread();
	void calculateMDR_Single_thread();
	void calculateJnR_Single_thread();

	void Net_calculateMR_Single_thread();
	void Net_calculateDR_Single_thread();
	void Net_calculateJnR_Single_thread();
	void Net_calculateMDR_Single_thread();

	void Geo_calculateMR_Single_thread();
	void Geo_calculateDR_Single_thread();
	void Geo_calculateJnR_Single_thread();

	void Net_calculateMD_Single_thread();
	void Net_calculateDD_Single_thread();
	void Net_calculateJnD_Single_thread();

	//多线程函数
	static int thread_num;
	static clock_t T_timeBegin, T_timeEnd;
	static std::vector<Calculation> CAVec;

	static void output_calculate_Data(MainWindow* temp);
	static void start_Multi_thread_for_calculate(MainWindow* temp);
	static void output_calculate_Data_DD(MainWindow* temp);
	static void start_Multi_thread_for_calculate_DD(MainWindow* temp);

	static void output_NetGeo_Data(MainWindow* temp);
	static void output_shp_data(MainWindow* temp);
	static void start_Multi_thread_for_NetGeo(MainWindow* temp);
	static void timeCount();
	//static void progress(MainWindow* temp);

	bool init_for_MR();
	void calculateMR_Multi_thread();	//MR
	bool init_for_DR(std::string txAngleThreshold);
	void calculateDR_Multi_thread();	//DR
	bool init_for_MDR(std::string txAngleThreshold);
	void calculateMDR_Multi_thread();	//MDR
	bool init_for_JnR();
	void calculateJnR_Multi_thread();	//JnR

	bool init_for_Net_MR();
	void Net_calculateMR_Multi_thread();	//Net_MR
	bool init_for_Net_DR();
	void Net_calculateDR_Multi_thread();	//Net_DR
	bool init_for_Net_MDR();
	void Net_calculateMDR_Multi_thread();	//Net_MDR
	bool init_for_Net_JnR();
	void Net_calculateJnR_Multi_thread();	//Net_JnR

	bool init_for_Geo_MR();
	void Geo_calculateMR_Multi_thread();	//Geo_MR
	bool init_for_Geo_DR();
	void Geo_calculateDR_Multi_thread();	//Geo_DR
	bool init_for_Geo_JnR();
	void Geo_calculateJnR_Multi_thread();	//Geo_JnR

	bool init_for_MD();
	void calculateMD_Multi_thread();	//MD
	bool init_for_DDL(std::string txAngleThreshold);
	void calculateDDL_Multi_thread();	//DDL
	bool init_for_JnDDL();
	void calculateJnDDL_Multi_thread();	//JnDDL

	bool init_for_Net_MD();
	void Net_calculateMD_Multi_thread();	//StedDepth_MR
	bool init_for_Net_DD();
	void Net_calculateDD_Multi_thread();	//StedDepth_DR
	bool init_for_Net_JnD();
	void Net_calculateJnD_Multi_thread();	//StedDepth_JnR

	//绘图
	void drawTxtFile(std::string shpfilename, std::string dbffilename, std::string txtfilename, bool needDelete);
	void AddLines(std::string shpfilename, std::string dbffilename);	//新增线段
	void RemoveLines(std::vector<int> &ids);	//删除线段

	//图层数据
	int global_roads_num, netreach_roads_num, geodesics_roads_num, nowCount = 0;
	bool global_view = false, netreach_view = false, geodesics_view = false;
	std::string global_file_path = "global.txt", netreach_file_path = "netreach.txt", geodesics_file_path = "geodesics.txt";
	std::map<int, int> Global_Ref_to_Id;	//value=<Ref, id>
	std::map<int, int> Global_Id_to_Ref;
	std::map<int, int> Netreach_Ref_to_Id;
	std::map<int, int> Netreach_Id_to_Ref;
	std::map<int, int> Geodesics_Ref_to_Id;
	std::map<int, int> Geodesics_Id_to_Ref;
	std::map<int, int> Ref_to_Id;
	std::map<int, int> Id_to_Ref;

	//load file
	bool loadFilePathOver = false;

	//检查输入
	bool CheckOpenFile();

	bool CheckReachMRInput();
	bool CheckReachDRInput();
	bool CheckReachJnRInput();
	bool CheckReachMDRInput();

	bool CheckReachMDInput();
	bool CheckReachDDLInput();
	bool CheckReachJnDDLInput();

	bool CheckNetreachIDInput();
	bool CheckNetReachMRInput();
	bool CheckNetReachDRInput();
	bool CheckNetReachJnRInput();
	bool CheckNetReachMDRInput();

	bool CheckDistanceModeInput();
	//bool CheckDistanceRadiusInput();
	bool CheckDistanceDRInput();
	bool CheckDistanceJnRInput();

	bool CheckGeodesicsDRInput();
	bool CheckGeodesicsJnRInput();
	bool CheckGeodesicsFromIDInput();
	bool CheckGeodesicsToIDInput();
	bool CheckGeodesicsCSVInput();

	//bool CheckSearchInput();

	bool CheckThreadNumberInput();

	//收集每次的属性名称
    std::set<std::string> ColNames;
	void modifyColNames(std::vector<std::string> &SourceNames); 

	//更新属性字段
	void modifyTXT(std::map<int, double> &Result, std::string fieldName, std::string fileName);

	//属性栏
	QDockWidget *AttributesListDock;
	void addAttribute(std::string new_column_name, std::map<int,double> &data);
	void updateAttributes(Calculation &CA);

	void addNetMap(NetMap &subNetMap);
	void addNetMapsAll(Calculation &CA, ShapeFileAccessor &FA);
	void cancelNetReach();	//取消NetReach绘制
	void reDrawNetReach();	//重绘NetReach

	void addGeoMap(GeoMap &subGeoMap);
	void addGeoMapsAll(Calculation &CA, ShapeFileAccessor &FA);
	void cancelGeoReach();	//取消GeoReach绘制
	void reDrawGeoReach();	//重绘GeoReach

	//subset
	void addSubsetNetMap(NetMap &subNetMap);
	void cancelSubsetNetReach();	//取消NetReach绘制
	void reDrawSubsetNetReach(std::vector<std::string> &items);	//重绘NetReach入口

	void addSubsetGeoMap(GeoMap &subGeoMap);
	void cancelSubsetGeoReach();	//取消GeoReach绘制
	void reDrawSubsetGeoReach(std::vector<std::string> &items);	//重绘GeoReach入口

	//others
	//void reDrawGlobalMap();	//重绘全局地图
	//void deleteAllMap();	//删除所有线段，无论是全局、Net、Geo
	void deleteGlobal();

	//data
	DisPlay display;
	GlobalMap global_map;
	set<int> NetFromIDs;
	set<int> GeoFromIDs, GeoToIDs;
	set<int> NetRoadsAll, GeoRoadsAll;
	QString NetInfo, GeoInfo;
	map<int, NetMap> net_map_all;	//key是起点坐标
	map<int, GeoMap> geo_map_all;	//key是起点坐标

	//subset
	set<int> subset_NetFromIDs;
	set<int> subset_GeoFromIDs, subset_GeoToIDs;
	set<int> subset_NetRoadsAll, subset_GeoRoadsAll;
	QString subset_NetInfo, subset_GeoInfo;
	map<int, NetMap> subset_net_map_all;	//key是起点坐标
	map<int, GeoMap> subset_geo_map_all;	//key是起点坐标

	//信息栏
	QString summary_info;
	void summaryFirstLoad();
	void summaryGlobal(Calculation &CA);
	bool judgeConnect(Calculation &CA, ShapeFileAccessor &FA, int start_road_1, int start_road_2);
	void findConnect(Calculation &CA, ShapeFileAccessor &FA, map<int, set<int>> &connect);
	void summaryNet(Calculation &CA, ShapeFileAccessor &FA);
	void summaryGeo(Calculation &CA, ShapeFileAccessor &FA);

	//颜色重载
	void ModifyLinesColor();
	void CancelModifyLinesColor();

	static volatile bool calculate_over;
	static int process_pos;
	static QString process_str;

	//清理数据
	void clearOldData();

	//是否已经导入文件
	bool isOpen = false;

	//多个角度阈值
	int finished_angle_count = 0;
	int last_finished_angle_count = 0;

	//线程同步
	bool get_run_start();
	bool get_needOver();
	bool get_NeedStop();
	bool get_calculate_over();
	void set_run_start(bool flag);
	void set_needOver(bool flag);
	void set_NeedStop(bool flag);
	void set_calculate_over(bool flag);

	//subsets analysis
	std::map<int, std::set<int>> SubsetIDs;
	bool getWeightDataByName(std::string WgtLimitStr, std::map<int, double> &data);
	void summarySubset(Calculation &CA, ShapeFileAccessor &FA);
	void outputCsvFiles(Calculation &CA, ShapeFileAccessor &FA);
	std::map<int, PafColor> SubsetColorMap;
	std::map<int, double> SubsetThicknessMap;
	bool isFirstSelect = true;		
	//subset参数
	bool isInputValid = false;
	double mr_MRLimit = -1;
	double dr_DRLimit = -1, dr_Angle = -1;
	double jnr_JnRLimit = -1, jnr_Degree = -1;
	double mdr_MRLimit = -1, mdr_DRLimit = -1, mdr_Angle = -1;
	std::string fileDir;

protected:
    QGraphDoc* m_treeDoc;	//画图类						
    void closeEvent(QCloseEvent *event);	//结束事件响应
    bool eventFilter(QObject *object, QEvent *e);	//事件过滤器
    virtual void actionEvent( QActionEvent * event );	//事件响应函数
	void resizeEvent(QResizeEvent *event);

private slots:
	//文件导出
	void transferToSegment();

	//工具栏
	void select_button();
	void drag_button();
	void zoomIn_button();
	void zoonOut_button();
	void maxView_button();
	void colorRange_button();

	//属性栏
	void showAttributes();
	void saveAttributes();
    void saveAllAttributesToCSV();
	void selectWeights();
	void infoRecv(QString);
	void saveToCSV();
	void collectWeights();
	void selectAttributes();
	void adaptiveWidth();

	//信息栏
	void showSummaryInfo();
	void displaySummary();

	//Run
	void Run();
	void run_calculate();
	void OnCSVFileOpen();
	void progressCount();
	void setProcessPos();
	void recover();
	void drawGraph();
	//void displayResult(int);

	//重绘
	void reDraw();
	//void reDrawGlobal();
	void reDrawNet(bool checked);
	void reDrawGeo(bool checked);
	void newHighlightWin();
	void ChangeThickness();
	void CloseHighlightWin();
	void ChangeColor(QColor newColor);
	void setBackground(QColor newColor);
	void ResetVisual();
	void UpdateColorMap();

	//subset
	void NewSubsetWin(bool checked);
	void SelectSubsetByInfo();
	bool SelectSubsetByInfoSingle(std::string info_str);
	void SelectSubsetByMouse();
	void NewSetFormWin();
	bool CheckSetForm();
	void CalculateSubset();
	void setMRPara(bool checked);
	void subsetProgressCount();
	void DrawSubsetReach();

	//缩放
	void newScaleWin();
	void ChangeScale();
	void CloseScaleWin();

	//选择线条
	void newSelectWin();
	void selectLinesByAttribute();
	void CloseSelectWin();

	//stop
	void setStop();

	//进度条
	void setProcessWidth(int width);

	//from id、to id
	void PushFromID(bool checked);
	void PushToID(bool checked);

	//void on_Button_start_scan_clicked();

	void fun2();
	void InputError();
	void GlobalOpen();
	void NetReachOpen();
	void GeodesicsOPen();
	void NetReachClose();
	void GeodesicsClose();
	void ChangeView();
	//void VisualEdit(bool);

	void progressOpenFile();  //deprecated

	void searchId();
	void showSelectedLines();

	//Visualize
    void updateActiveWindows();		//更改激活窗口
    void updateSubWindowTitles(QString newTitle);	//更改子窗口标题
    void updateWindowMenu();	//更改窗口菜单
    void setActiveSubWindow(QWidget *window);	//设置激活窗口
    void OnSelchangingTree(QTreeWidgetItem* item, int col);		//响应目录树
    void OnSelchangingList();	//响应列表
    void OnFileNew();		//创建新文件
    void OnFileImport();	//导入文件
    void OnFileOpen();		//打开文件
    void OnFileClose();		//关闭文件
    void OnFileSave();		//保存文件
    void OnFileSaveAs();	//文件另存为
    void OnFileProperties();	//文件属性
    void OnFilePrint();			//文件打印
    void OnFilePrintPreview();	//文件打印预览
    void OnFilePrintSetup();	//文件打印安装
    void OnEditUndo();			//编辑：撤销
    void OnEditCopyData();		//编辑：拷贝数据
    void OnEditCopy();			//编辑：复制
    void OnEditSave();			//编辑：保存
    void OnEditClear();			//编辑：清空
    void OnEditQuery();			//编辑：查询
    void OnViewZoomsel();		//缩放
    void OnEditSelectToLayer();	//编辑：选择图层
    void OnAppAbout();			//软件相关信息
    void OnLayerNew();			//创建新图层
    void OnLayerDelete();		//删除图层
    void OnLayerConvert();		//图层变换
    void OnLayerConvertDrawing();	//图层变换绘图
    void OnConvertMapShapes();		//转换为Map形状
    void OnFileExport();			//文件导出
	void OnExportNetGeo();			//导出netreach/geodesics文件
    void OnFileExportMapGeometry();	//文件导出为Map几何形状
    void OnFileExportLinks();		//文件导出链接
    void OnAxialConnectionsExportAsDot();		//轴向连接导出为点
    void OnAxialConnectionsExportAsPairCSV();	//轴向连接导出为CSV对
    void OnSegmentConnectionsExportAsPairCSV();	//段连接导出为CSV对
    void OnPointmapExportConnectionsAsCSV();	//点地图将连接导出为CSV
    void OnAddColumn();			//添加列
    void OnRenameColumn();		//重命名列
    void OnUpdateColumn();		//更新列
    void OnRemoveColumn();		//删除列
    void OnColumnProperties();	//列属性
    void OnPushToLayer();		//推入图层
    void OnToolsMakeGraph();			//工具：制作图
    void OnToolsUnmakeGraph();			//工具：取消图表
    void OnToolsImportVGALinks();		//工具：导入VGA链接
    void OnToolsIsovistpath();			//工具：Isovist路径
    void OnToolsAgentLoadProgram();		//工具：代理加载程序
    void OnToolsRunAxa();				//工具：运行Axa
    void OnToolsPD();					//工具：PD
    void OnToolsAPD();					//工具：APD
    void OnToolsMakeFewestLineMap();	//工具：制作最少线图
    void OnToolsAxialConvShapeMap();	//工具：轴向转换形状图
    void OnToolsLineLoadUnlinks();		//工具：线路加载取消链接
    void OnToolsRunSeg();				//工具：运行线段图分析
    void OnToolsTopomet();				//工具：拓扑
    void OnToolsTPD();					//工具：TPD
    void OnToolsMPD();					//工具：MPD
    void OnToolsPointConvShapeMap();	//工具：点转换形状图
    void OnToolsOptions();				//工具：选项
    void OnViewCentreView();		//视图：中心视图
    void OnViewShowGrid();			//视图：显示网格
    void OnViewSummary();			//视图：概要视图
    void OnViewColourRange();		//视图：颜色范围
    void OnHelpBugs();			//错误帮助
    void OnHelpManual();		//菜单帮助
    void OnHelpTutorials();		//讲解帮助
    void OnHelpSalaManual();	//Sala菜单帮助
    void OnEditGrid();			//编辑网格
    void OnWindowMap();			//地图窗口
    void OnViewTable();			//表格视图
    void OnWindow3dView();		//3D视图窗口
    void OnWindowGLView();		//GL视图窗口
    void OnViewScatterplot();	//散点图视图
    void OnToolsRun();			//工具：运行
    void OnToolsAgentRun();		//工具：代理运行

	// MapView message 地图显示消息
    void zoomModeTriggered();	//缩放模式触发
    void FillModeTriggered();	//填充模式触发
    void LineModeTriggered();	//线路模式触发
    void isoModeTriggered();	//iso模式触发
    void joinTriggered();		//加入触发
    void zoomButtonTriggered();	//缩放按钮触发
    void FillButtonTriggered();	//填充按钮触发
    void LineButtonTriggered();	//线路按钮触发
    void isoButtonTriggered();	//iso按钮触发
    void joinButtonTriggered();	//加入按钮触发
    void openRecentFile();		//打开最近文件
    void StepDepthTriggered();	//步长触发
    void AxialMapTriggered();	//轴向图触发
    void SelectPenTriggered();	//选择笔触发
    void DragButtonTriggered();	//拖动按钮触发
    void SelectButtonTriggered();	//选择按钮触发
    void OnSelchangeViewSelector_X(const QString &string);	//选择变更视图选择器X
    void OnSelchangeViewSelector_Y(const QString &string);	//选择变更视图选择器Y
    void OninvertColor();	//反转颜色
    void OnzoomTo();		//放大到

	// PlotView message 节点显示消息
    void OntoggleColor();	//切换颜色
    void OntoggleOrg();		//切换组织
    void OnviewTrend();		//查看趋势
    void OnYX();			//YX
    void OnRtwo();			//Rtwo

	//Menu Update slots 菜单更新槽函数
    void updateViewMenu();					//更新视图菜单
    void updateAttributesMenu();			//更新属性菜单
    void updateMapMenu();					//更新地图菜单
    void updateEditMenu();					//更新编辑菜单
    void updateFileMenu();					//更新文件菜单
    void updateVisibilitySubMenu();			//更新能见度子菜单
    void updateStepDepthSubMenu();			//更新步长深度子菜单
    void updateSegmentStepDepthSubMenu();	//段步长深度子菜单
    void updateAgentToolsSubMenu();			//更新代理工具子菜单
    void updateSegmentSubMenu();			//更新段子菜单
    void updateAxialSubMenu();				//更新轴向子菜单

	//3D View ToolBar slots 3D工具栏槽函数
    void OnToolsImportTraces();		//工具导入跟踪
    void OnAddAgent();				//添加代理
    void OnToolsAgentsPlay();		//工具代理运行
    void OnToolsAgentsPause();		//工具代理暂停
    void OnToolsAgentsStop();		//工具代理终止
    void OnAgentTrails();			//代理轨迹
    void On3dRot();					//3d旋转
    void On3dPan();					//3d平移
    void On3dZoom();				//3d缩放
    void OnPlayLoop();				//循环运行
    void On3dFilled();				//3d填充

private:
    int OnFocusGraph(QGraphDoc* pDoc, int lParam);			//聚焦图
    void setCurrentFile(const QString &fileName);			//设置当前文件
    void updateRecentFileActions(const QStringList &files);	//更新最近文件动作
    QString strippedName(const QString &fullFileName);		//剥离名称

    void createActions();	//创建动作响应
    void createMenus();		//创建菜单
    void createToolBars();	//创建工具栏
    void createStatusBar();	//创建状态栏

    // Settings Files 设置文件
    Settings &mSettings;	//设置(setting)结构体
    void readSettings();	//读取设置(setting)
    void writeSettings();	//写入设置(setting)

    bool m_defaultMapWindowIsLegacy;	//默认的地图窗口为旧版

    QWidget * setupAttributesListWidget();	//设置属性列表小部件
    MapView *createMapView();	//创建地图视图
    MapView *activeMapView();	//激活地图视图
    QGraphDoc *activeMapDoc();	//激活地图文件
    QMdiSubWindow *findMapView(const QString &fileName);	//找出地图视图

	//treeContorl 树控件
    QVector<QIcon> m_tree_icon;		//图标树
    std::map<int, std::string> m_view_map_entries;	//视图条目

    std::vector<bool> m_attribute_locked;	//属性锁
    std::map<QTreeWidgetItem*, ItemTreeEntry> m_treegraphmap;	//树状图
    std::map<QTreeWidgetItem*, ItemTreeEntry> m_treedrawingmap;	//树状绘图
    QTreeWidgetItem* m_topgraph;		//顶图
    QTreeWidgetItem* m_backgraph;		//底图
    QTreeWidgetItem* m_treeroots[5];	//树根

    void MakeTree();			//创建树
	void TestAddColumn();
    void MakeGraphTree();		//创建图树
    void MakeDrawingTree();		//创建绘图树
    void ClearGraphTree();		//清除图树
    void MakeAttributeList();	//创建属性表
    void SetAttributeChecks();	//设置属性检查
    void SetDrawingTreeChecks();//设置绘图树检查
    void SetGraphTreeChecks();	//设置图树检查

////////////////////////////////////////////////////////////界面控件

    QMdiArea *mdiArea;					//用于提供多窗口文档(MDI)的显示区域
    QSignalMapper *windowMapper;		//信号的翻译和转发器，它可以把一个无参数的信号翻译成带int参数、QString参数、QObject*参数或者QWidget*参数的信号，并将之转发
    IndexWidget* m_indexWidget;
    AttribWindow* m_attrWindow;
    CColourScaleDlg m_wndColourScale;	//color range dialog
	//Dialog m_MRDlg;		//MR参数组

    QLabel *g_size;			//提供了文本或图像的显示
    QLabel *g_pos_curr;		
    QLabel *g_info_curr;

    QMenu *fileMenu;		//菜单
    QMenu *editMenu;
    QMenu *mapMenu;
    QMenu *exportSubMenu;
    QMenu *attributesMenu;
    QMenu *toolsMenu;
    QMenu *visibilitySubMenu;
    QMenu *stepDepthSubMenu;
    QMenu *agentToolsSubMenu;
    QMenu *axialSubMenu;
    QMenu *segmentSubMenu;
    QMenu *segmentStepDepthSubMenu;
    QMenu *viewMenu;
    QMenu *windowMenu;
    QMenu *helpMenu;

    QToolBar *fileToolBar;	
    QToolBar *editToolBar;
    QToolBar *plotToolBar;
    QToolBar *thirdViewToolBar;
    QToolButton *fillColorToolButton;
    QToolButton *zoomToolButton;
    QToolButton *lineToolButton;
    QToolButton *newisoToolButton;
    QToolButton *JoinToolButton;
    QToolButton *SelectButton;
    QToolButton *DragButton;
    QToolButton *SelectPenButton;
    QToolButton *AxialMapButton;
    QToolButton *StepDepthButton;
    QToolButton *attr_add_button;
    QToolButton *attr_del_button;

////////////////////////////////////////////////////////////动作响应函数

    //File Menu Actions 文件菜单操作
    QAction *newAct;
    QAction *openAct;
    QAction *closeAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *propertiesAct;
    QAction *printAct;
    QAction *printPreviewAct;
    QAction *printSetupAct;
    QAction *recentFileActs[MaxRecentFiles];
    QAction *separatorAct;
    QAction *exitAct;

    //Edit Menu Actions 编辑菜单操作
    QAction *undoAct;
    QAction *copyDataAct;
    QAction *copyScreenAct;
    QAction *exportScreenAct;
    QAction *clearAct;
    QAction *selectByQueryAct;
    QAction *selectionToLayerAct;

    //Map Menu Actions 地图菜单动作
    QAction *mapNewAct;
    QAction *deleteAct;
    QAction *convertActiveMapAct;
    QAction *convertDrawingMapAct;
    QAction *convertMapShapesAct;
    QAction *importAct;
    QAction *exportAct;
    QAction *exportGeometryAct;
    QAction *exportLinksAct;
    QAction *exportAxialConnectionsDotAct;
    QAction *exportAxialConnectionsPairAct;
    QAction *exportSegmentConnectionsPairAct;
    QAction *exportPointmapConnectionsPairAct;

    //Attributes Menu Actions 属性菜单操作
    QAction *renameColumnAct;
    QAction *columnPropertiesAct;

    //Tools Menu Actions 工具菜单操作
    QAction *makeVisibilityGraphAct;
    QAction *unmakeVisibilityGraphAct;
    QAction *importVGALinksAct;
    QAction *makeIsovistPathAct;
    QAction *runVisibilityGraphAnalysisAct;
    QAction *visibilityStepAct;
    QAction *metricStepAct;
    QAction *angularStepAct;
    QAction *convertDataMapLinesAct;
    QAction *runAgentAnalysisAct;
    QAction *loadAgentProgramAct;
    QAction *runGraphAnaysisAct;
    QAction *stepDepthAct;
    QAction *reduceToFewestLineMapAct;
    QAction *convertDataMapPointsAct;
    QAction *loadUnlinksFromFileAct;
    QAction *runAngularSegmentAnalysisAct;
    QAction *runTopologicalOrMetricAnalysisAct;
    QAction *segmentAngularStepAct;
    QAction *topologicalStepAct;
    QAction *segmentMetricStepAct;
    QAction *optionsAct;

    //View Menu Actions 视图菜单操作
    QAction *showGridAct;
    QAction *attributeSummaryAct;

    //Window Menu Actions 窗口菜单动作
    QAction *mapAct;
    QAction *scatterPlotAct;
    QAction *tableAct;
    QAction *thirdDViewAct;
    QAction *glViewAct;
    QAction *colourRangeAct;
    QAction *cascadeAct;
    QAction *tileAct;
    QAction *arrangeIconsAct;

    //Help Menu Actions 帮助菜单操作
    QAction *onlineBugsAct;
    QAction *onlineHandbookAct;
    QAction *onlineTutorialsAct;
    QAction *onlineScriptingManualAct;
    QAction *aboutDepthMapAct;

	//depthmapX Contorl depthmapX控件
    QAction *addColumAct;
    QAction *updateColumAct;
    QAction *removeColumAct;
    QAction *pushValueAct;
    QAction *invertColorAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *zoomToAct;
    QAction *RecentAct;
    QAction *SetGridAct;
    QAction *STDFillColorAct;
    QAction *ContextFillColorAct;
    QAction *AugmentFillColorAct; // AV test - TV
    QAction *SelectLineAct;
    QAction *SelectPolyLineAct;
    QAction *MakeIosAct;
    QAction *PartialMakeIosAct;
    QAction *JoinAct;
    QAction *JoinUnlinkAct;
	
	//PlotMap control PlotMap控件
    QComboBox *x_coord;
    QComboBox *y_coord;
    QAction *toggleColor;
    QAction *toggleOrg;
    QAction *viewTrend;
    QAction *yx;
    QAction *Rtwo;

	//3D View ToolBar 3D视图工具栏
    QAction *toolsImportTracesAct;
    QAction *addAgentAct;
    QAction *toolsAgentsPlayAct;
    QAction *toolsAgentsPauseAct;
    QAction *toolsAgentsStopAct;
    QAction *agentTrailsAct;
    QAction *thirdRotAct;
    QAction *thirdPanAct;
    QAction *thirdZoomAct;
    QAction *playLoopAct;
    QAction *thirdFilledAct;

	////////////////////////////////////////////////////////////地图栏项

    int m_selected_mapbar_item = -1;	//选定的地图栏项

    enum {
       ID_MAPBAR_ZOOM_ITEMS = 2,	//地图栏缩放项目
       ID_MAPBAR_FILL_ITEMS = 8,	//地图栏填充项目
       ID_MAPBAR_DRAW_ITEMS = 10,	//地图栏绘图项目
       ID_MAPBAR_ISOVIST_ITEMS = 12,//地图栏ISOVIST项目
       ID_MAPBAR_JOIN_ITEMS = 15	//地图栏连接项目
    };

    enum {
       ID_MAPBAR_ITEM_SELECT = 0,		//选择
       ID_MAPBAR_ITEM_MOVE = 1,			//移动
       ID_MAPBAR_ITEM_ZOOM_IN = 2,		//放大
       ID_MAPBAR_ITEM_ZOOM_OUT = 3,		//缩小
       ID_MAPBAR_ITEM_FILL = 7,			//填充
       ID_MAPBAR_ITEM_SEMIFILL = 8,		//半填充
       ID_MAPBAR_ITEM_PENCIL = 9,		//画笔
       ID_MAPBAR_ITEM_LINETOOL = 10,	//线段工具
       ID_MAPBAR_ITEM_POLYGON = 11,		//折线
       ID_MAPBAR_ITEM_ISOVIST = 12,		//ISOVIST
       ID_MAPBAR_ITEM_HALFISOVIST = 13,	//半ISOVIST
       ID_MAPBAR_ITEM_AL2 = 14,			//AL2
       ID_MAPBAR_ITEM_PD = 15,			//PD
       ID_MAPBAR_ITEM_JOIN = 16,		//连接
       ID_MAPBAR_ITEM_UNJOIN = 17,		//取消连接
       ID_MAPBAR_ITEM_AUGMENT_FILL = 18 //补足 AV test - TV
    };
	
};
//移动到cpp
//bool MainWindow::run_start = false;
//bool MainWindow::needOver = false;
//bool MainWindow::calculate_over = true;
//int MainWindow::process_pos = 0;
//QString MainWindow::process_str = "";
//
////多线程
//int MainWindow::maxNum = 4;
//clock_t MainWindow::T_timeBegin;
//clock_t MainWindow::T_timeEnd;
//std::vector<Calculation> MainWindow::CAVec;

#endif
