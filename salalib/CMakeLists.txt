# salalib 静态库构建配置

# 库名称
set(LIBRARY_NAME salalib)

# 源文件列表
set(SOURCES
    axialmap.cpp
    connector.cpp
    isovist.cpp
    mgraph.cpp
    ngraph.cpp
    pointdata.cpp
    salaprogram.cpp
    shapemap.cpp
    spacepix.cpp
    sparksieve2.cpp
    entityparsing.cpp
    linkutils.cpp
    gridproperties.cpp
    attributetable.cpp
    layermanagerimpl.cpp
    attributetableview.cpp
    geometrygenerators.cpp
    point.cpp
    pafcolor.cpp
    spacepixfile.cpp
    alllinemap.cpp
    axialminimiser.cpp
    axialpolygons.cpp
    tidylines.cpp
    mapconverter.cpp
    importutils.cpp
    attributetableindex.cpp
)

# 头文件列表（用于 IDE 显示）
set(HEADERS
    axialmap.h
    connector.h
    isovist.h
    mgraph.h
    ngraph.h
    pointdata.h
    salaprogram.h
    shapemap.h
    spacepix.h
    sparksieve2.h
    entityparsing.h
    linkutils.h
    gridproperties.h
    attributetable.h
    layermanagerimpl.h
    attributetableview.h
    geometrygenerators.h
    point.h
    pafcolor.h
    spacepixfile.h
    alllinemap.h
    axialminimiser.h
    axialpolygons.h
    tidylines.h
    mapconverter.h
    importutils.h
    attributetableindex.h
    layermanager.h
    displayparams.h
    fileproperties.h
    iaxial.h
    importtypedefs.h
    isegment.h
    isovistdef.h
    ivga.h
    options.h
    pixelref.h
    mgraph_consts.h
    tolerances.h
    attributetablehelpers.h
)

# 编译定义
add_compile_definitions(_DEPTHMAP SALALIB_LIBRARY)

# 创建静态库（先创建目标，再添加子目录）
add_library(${LIBRARY_NAME} STATIC 
    ${SOURCES} 
    ${HEADERS}
)

# 添加子目录（子目录会向已创建的目标添加源文件）
add_subdirectory(vgamodules)
add_subdirectory(axialmodules)
add_subdirectory(segmmodules)
add_subdirectory(parsers)
add_subdirectory(agents)

# 设置库属性
set_target_properties(${LIBRARY_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
)



