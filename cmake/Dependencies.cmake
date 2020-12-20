# therion dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(PROJ REQUIRED IMPORTED_TARGET proj)
string(FIND ${PROJ_VERSION} "." MVER_SEP)
string(SUBSTRING ${PROJ_VERSION} 0 ${MVER_SEP} PROJ_MVER)

option(USE_BUNDLED_SHAPELIB "Build bundled version of shapelib." ON)
if (USE_BUNDLED_SHAPELIB)
    add_subdirectory(extern/shapelib)
else()
    pkg_check_modules(SHAPELIB REQUIRED IMPORTED_TARGET shapelib)
endif()

option(USE_BUNDLED_CATCH2 "Use bundled version of Catch2." ON)
if (USE_BUNDLED_CATCH2)
    add_subdirectory(extern/Catch2)
else()
    find_package(Catch2 REQUIRED)
endif()

# loch dependencies
find_package(wxWidgets REQUIRED COMPONENTS core base gl xml html)
find_package(VTK REQUIRED COMPONENTS 
    vtkCommonExecutionModel
    vtkCommonDataModel
    vtkCommonCore
    vtkFiltersCore
    vtkFiltersHybrid
    vtkIOLegacy
    vtkjpeg
    vtkpng
)
find_package(Freetype REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
