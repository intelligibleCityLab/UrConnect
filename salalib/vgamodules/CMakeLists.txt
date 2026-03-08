# vgamodules 模块构建配置

# 向 salalib 库添加源文件
target_sources(salalib
    PRIVATE
       vgaisovist.cpp 
       vgaangular.cpp
       vgametric.cpp
       vgathroughvision.cpp
       vgavisuallocal.cpp
       vgavisualglobal.cpp
       vgaangulardepth.cpp
       vgametricdepth.cpp
       vgavisualglobaldepth.cpp
    PUBLIC
       vgaangular.h
       vgametric.h
       vgavisualglobal.h
       vgaangulardepth.h
       vgametricdepth.h
       vgavisualglobaldepth.h
       vgaisovist.h
       vgathroughvision.h
       vgavisuallocal.h
)
