# views 模块构建配置

# 自动生成 MOC 文件
set(CMAKE_AUTOMOC ON)

# 向主应用程序添加源文件
target_sources(depthmapX
    PRIVATE
        mapview.cpp
        viewhelpers.cpp
        glview/gllinesuniform.cpp
        glview/glview.cpp
        glview/gllines.cpp
        glview/glrastertexture.cpp
        glview/glpolygons.cpp
        glview/glutriangulator.cpp
        glview/gltrianglesuniform.cpp
        glview/glpointmap.cpp
        glview/glshapegraph.cpp
        glview/glshapemap.cpp
        glview/gldynamicrect.cpp
        glview/gldynamicline.cpp
        plotview/plotview.cpp
        tableview/tableview.cpp
        depthmapview/depthmapview.cpp
        3dview/3dview.cpp
        glview/glregularpolygons.cpp
        glview/gltriangles.cpp
    PUBLIC
        mapview.h
        viewhelpers.h
        glview/gllinesuniform.h
        glview/glview.h
        glview/gllines.h
        glview/glrastertexture.h
        glview/glpolygons.h
        glview/glutriangulator.h
        glview/gltrianglesuniform.h
        glview/glpointmap.h
        glview/glshapegraph.h
        glview/glshapemap.h
        glview/gldynamicrect.h
        glview/gldynamicline.h
        plotview/plotview.h
        tableview/tableview.h
        depthmapview/depthmapview.h
        glview/glregularpolygons.h
        glview/gltriangles.h
        3dview/3dview.h
)

