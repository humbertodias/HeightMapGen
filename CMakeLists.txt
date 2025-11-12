cmake_minimum_required(VERSION 3.16)  
  
project(HeightMapGenerator VERSION 0.1 LANGUAGES CXX)  
  
set(CMAKE_AUTOUIC ON)  
set(CMAKE_AUTOMOC ON)  
set(CMAKE_AUTORCC ON)  
  
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
  
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools OpenGL OpenGLWidgets)  
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools OpenGL OpenGLWidgets)  
find_package(OpenGL REQUIRED)  
  
set(TS_FILES HeightMapGenerator_es_ES.ts)  
  
set(PROJECT_SOURCES  
        main.cpp  
        mainwindow.cpp  
        mainwindow.h  
        mainwindow.ui  
        openglwidget.cpp  
        openglwidget.h  
        ${TS_FILES}  
)  
  
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)  
    qt_add_executable(HeightMapGenerator  
        MANUAL_FINALIZATION  
        ${PROJECT_SOURCES}  
    )  
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})  
else()  
    if(ANDROID)  
        add_library(HeightMapGenerator SHARED  
            ${PROJECT_SOURCES}  
        )  
    else()  
        add_executable(HeightMapGenerator  
            ${PROJECT_SOURCES}  
        )  
    endif()  
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})  
endif()  
  
target_link_libraries(HeightMapGenerator PRIVATE   
    Qt${QT_VERSION_MAJOR}::Widgets   
    Qt${QT_VERSION_MAJOR}::OpenGL   
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets  
    OpenGL::GL  
)  
  
if(${QT_VERSION} VERSION_LESS 6.1.0)  
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.HeightMapGenerator)  
endif()  
set_target_properties(HeightMapGenerator PROPERTIES  
    ${BUNDLE_ID_OPTION}  
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}  
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}  
    MACOSX_BUNDLE TRUE  
    WIN32_EXECUTABLE TRUE  
)  
  
include(GNUInstallDirs)  
install(TARGETS HeightMapGenerator  
    BUNDLE DESTINATION .  
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}  
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  
)  
  
if(QT_VERSION_MAJOR EQUAL 6)  
    qt_finalize_executable(HeightMapGenerator)  
endif()
