# depthmapX 应用程序构建配置

# 应用程序名称
set(APP_NAME UrConnect)

# CMake 配置
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 设置 UI 文件搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/UI)

# 包含目录
include_directories(${URCONNECT_BOOST_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/SNDAApp)

# 链接目录
link_directories(${CMAKE_SOURCE_DIR}/SNDAApp/lib)

# 查找依赖
find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED)
find_package(OpenGL REQUIRED)

# 编译定义
add_compile_definitions(_DEPTHMAP)

# 源文件列表
set(SOURCES
    ../SNDAApp/Calculation.cpp
    ../SNDAApp/Shapefile.cpp
    ../SNDAApp/ShapeFileAccessor.cpp
    ../SNDAApp/stdafx.cpp
    ../SNDAApp/ShapeObject.cpp
    AttributesWin.cpp
    qcolordialogbutton.cpp
    GraphDoc.cpp
    indexWidget.cpp
    mainwindow.cpp
    mdichild.cpp
    renderthread.cpp
    treeWindow.cpp
    mainwindowfactory.cpp
    settingsimpl.cpp
    CalculateUI.cpp
    Display.cpp
    GeoMap.cpp
    GlobalMap.cpp
    message.cpp
    Highlight.cpp
    NetMap.cpp
    newwindow.cpp
    ScaleWin.cpp
    Select.cpp
    setForm.cpp
    Subset.cpp
    UrbanConnectAnalyzer.cpp
    main.cpp
    coreapplication.cpp
    CSVProcess.cpp
)

# 头文件列表（用于 IDE 显示）
set(HEADERS
    Attributes.h
    AttributesWin.h
    CalculateUI.h
    Definition.h
    Display.h
    GeoMap.h
    GlobalMap.h
    GraphDoc.h
    Highlight.h
    NetMap.h
    ScaleWin.h
    Select.h
    Subset.h
    compatibilitydefines.h
    coreapplication.h
    indexWidget.h
    mainwindow.h
    mainwindowfactory.h
    mdichild.h
    message.h
    newwindow.h
    qcolordialogbutton.h
    roadinfo.h
    setForm.h
    settings.h
    settingsimpl.h
    treeWindow.h
    UrbanConnectAnalyzer.h
)

# UI 文件
set(UI_FILES
    UI/TopoMetDlg.ui
    UI/SegmentAnalysisDlg.ui
    UI/RenameObjectDlg.ui
    UI/PushDialog.ui
    UI/PromptReplace.ui
    UI/OptionsDlg.ui
    UI/NewLayerDlg.ui
    UI/MakeOptionsDlg.ui
    UI/MakeLayerDlg.ui
    UI/LicenceDialog.ui
    UI/LayerChooserDlg.ui
    UI/IsovistPathDlg.ui
    UI/InsertColumnDlg.ui
    UI/GridDialog.ui
    UI/FindLocDlg.ui
    UI/FilePropertiesDlg.ui
    UI/FewestLineOptionsDlg.ui
    UI/EditConnectionsDlg.ui
    UI/DepthmapOptionsDlg.ui
    UI/DepthmapAlert.ui
    UI/ConvertShapesDlg.ui
    UI/ColumnPropertiesDlg.ui
    UI/ColourScaleDlg.ui
    UI/AxialAnalysisOptionsDlg.ui
    UI/AttributeSummary.ui
    UI/AttributeChooserDlg.ui
    UI/AgentAnalysisDlg.ui
    UI/AboutDlg.ui
    UI/licenseagreement.ui
    UI/AttributesWin.ui
    UI/CalculateUI.ui
    UI/Highlight.ui
    UI/newwindow.ui
    UI/ScaleWin.ui
    UI/Select.ui
    UI/setForm.ui
    UI/Subset.ui
    UI/UrbanConnectAnalyzer.ui
)

# 资源文件
set(RESOURCES
    resource.qrc
    dialogs/settings/settingsdialog.qrc
)

# 生成 UI 代码
qt5_wrap_ui(UI_HDRS ${UI_FILES})

# 生成资源文件
qt5_add_resources(DM_RSRC ${RESOURCES})

# 创建可执行文件（先创建目标，再添加子目录）
if(MSVC)
    add_executable(${APP_NAME} WIN32 
        ${SOURCES} 
        ${HEADERS} 
        ${UI_HDRS} 
        ${DM_RSRC} 
        icons.rc
    )
    set_target_properties(${APP_NAME} PROPERTIES 
        LINK_FLAGS "/ENTRY:mainCRTStartup /NODEFAULTLIB:libcmt.lib"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
elseif(UNIX AND NOT APPLE)
    add_executable(${APP_NAME} 
        ${SOURCES} 
        ${HEADERS} 
        ${UI_HDRS} 
        ${DM_RSRC}
    )
    set_target_properties(${APP_NAME} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
elseif(APPLE)
    add_executable(${APP_NAME} MACOSX_BUNDLE 
        ${SOURCES} 
        ${HEADERS} 
        ${UI_HDRS} 
        ${DM_RSRC}
    )
    set_target_properties(${APP_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "UrConnect"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.intelligiblecitylab.UrConnect"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()

# 添加子目录（子目录会向已创建的目标添加源文件）
add_subdirectory(dialogs)
add_subdirectory(views)

# 复制图标目录
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/UI/ico"
        "$<TARGET_FILE_DIR:${APP_NAME}>/ico"
    COMMENT "Copying icon directory to output directory"
)

# 复制shp.dll到输出目录
if(WIN32)
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:shp>
            $<TARGET_FILE_DIR:${APP_NAME}>
        COMMENT "Copying shp.dll to output directory"
    )
endif(WIN32)

# 链接依赖
target_link_libraries(${APP_NAME} 
    shp 
    salalib 
    genlib 
    mgraph440 
    Qt5::Core 
    Qt5::Gui 
    Qt5::Widgets 
    Qt5::OpenGL 
    OpenGL::GL 
    OpenGL::GLU
    $<$<PLATFORM_ID:Windows>:ucrt.lib>
    $<$<PLATFORM_ID:Windows>:vcruntime.lib>
    $<$<PLATFORM_ID:Windows>:msvcrt.lib>
    $<$<AND:$<PLATFORM_ID:Windows>,$<CONFIG:Debug>>:ucrtd.lib>
    $<$<AND:$<PLATFORM_ID:Windows>,$<CONFIG:Debug>>:vcruntimed.lib>
    $<$<AND:$<PLATFORM_ID:Windows>,$<CONFIG:Debug>>:msvcrtd.lib>
    $<$<PLATFORM_ID:Windows>:kernel32.lib>
    $<$<PLATFORM_ID:Windows>:user32.lib>
    $<$<PLATFORM_ID:Windows>:gdi32.lib>
    $<$<PLATFORM_ID:Windows>:winspool.lib>
    $<$<PLATFORM_ID:Windows>:comdlg32.lib>
    $<$<PLATFORM_ID:Windows>:advapi32.lib>
    $<$<PLATFORM_ID:Windows>:shell32.lib>
    $<$<PLATFORM_ID:Windows>:ole32.lib>
    $<$<PLATFORM_ID:Windows>:oleaut32.lib>
    $<$<PLATFORM_ID:Windows>:uuid.lib>
    $<$<PLATFORM_ID:Windows>:odbc32.lib>
    $<$<PLATFORM_ID:Windows>:odbccp32.lib>
)

# 设置目标属性
set_target_properties(${APP_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
