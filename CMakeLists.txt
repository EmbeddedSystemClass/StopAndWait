cmake_minimum_required(VERSION 2.6)

##project
project(C_Test C)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/util)
file(GLOB UTIL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.c)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces)
file(GLOB INTERFACE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/*.c)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/protocol)
file(GLOB PROTCOL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol/*.c)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/openfec)
file(GLOB OPENFEC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/openfec/*/*.c)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/rscode)
file(GLOB RSCODE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/rscode/*.c)

add_subdirectory(apps)

link_directories(${LIBRARY_OUTPUT_PATH})

add_executable (Test src/main.c ${UTIL_SOURCES} ${INTERFACE_SOURCES} ${PROTCOL_SOURCES})
add_executable (Tunnel apps/tunnel.c ${UTIL_SOURCES})
add_executable (Net apps/net.c ${UTIL_SOURCES})
add_executable (Tun_Ctrl apps/tun_ctrl.c ${UTIL_SOURCES})
add_executable (RS_Test apps/example.c ${RSCODE_SOURCES})