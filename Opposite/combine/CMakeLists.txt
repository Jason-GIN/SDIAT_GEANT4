cmake_minimum_required(VERSION 3.16...3.21)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(MyTest)

find_package(ROOT REQUIRED)

include(${ROOT_USE_FILE})
include_directories(
    include
    ${PRO}
    )

file(GLOB_RECURSE SOURCES "src/*.cc")

file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

# 创建静态库 
add_library(mylib STATIC ${SOURCES})
#或是这样
#add_library(mylib STATIC
#    src/utils.cpp
#    src/calculator.cpp
#    src/filter.cpp
#    # 可以继续添加其他源文件
#)

target_link_libraries(mylib ${ROOT_LIBRARIES})


set(PROGRAMS 
    dataAnalyseTest
    #dataAnalyse2
    #dataAnalyse3
    #TestFieldLine
)

foreach(prog ${PROGRAMS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${prog}.cc")
        add_executable(${prog} ${prog}.cc)
        target_link_libraries(${prog} mylib ${ROOT_LIBRARIES} -static-libstdc++ -static-libgcc)
        message(STATUS "Added: ${prog}")
    endif()
endforeach()

# 创建可执行文件（假设你的主程序是main.cpp）
# add_executable(dataAnalyseTest dataAnalyseTest.cc)
# 将你的静态库链接到可执行文件
# target_link_libraries(dataAnalyseTest mylib)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")