########################################################
# Top-level CMakeLists.txt to build Raspberry Libraries
# @author LAGARDE Francois
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
########################################################

# ----- Project name and version -----
project( RaspberrySensors)
set( ${PROJECT_NAME}_VERSION_MAJOR 1 )
set( ${PROJECT_NAME}_VERSION_MINOR 0 )
set( ${PROJECT_NAME}_VERSION_PATCH 0 )

# set the C++ standard to C++ 11
set(CMAKE_CXX_STANDARD 11)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wl,-rpath=${CMAKE_SOURCE_DIR}/bin/lib/" )
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath=${CMAKE_SOURCE_DIR}/bin/lib/" )
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/bin/lib)

macro( set_if_not )
    if( NOT ${ARGV0} )
        set( ${ARGN} )
    endif()
endmacro()

set_if_not( jsoncpp_repository "https://github.com/RPClab/jsoncpp" )
set_if_not( jsoncpp_version "master" )
set_if_not( mariadbpp_repository "https://github.com/RPClab/mariadbpp" )
set_if_not( mariadbpp_version "master" )
set_if_not( BME280_repository "https://github.com/RPClab/BME280_driver.git" )
set_if_not( BME280_version "master" )

include(ExternalProject)

    # dependencies
    find_package(MariaDBClient REQUIRED)

    # ----- mariadb++ package -----
    ExternalProject_Add( 
                        mariadbpp
                        GIT_REPOSITORY ${mariadbpp_repository}
                        GIT_TAG ${mariadbpp_version}
                        GIT_PROGRESS TRUE
                        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/mariadbpp-prefix
                        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/mariadbpp
                        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
                        LIST_SEPARATOR %
                        UPDATE_COMMAND ""
                        )
    # ----- jsoncpp package -----
    ExternalProject_Add( 
                        jsoncpp
                        GIT_REPOSITORY ${jsoncpp_repository}
                        GIT_TAG ${jsoncpp_version}
                        GIT_PROGRESS TRUE
                        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/jsoncpp-prefix
                        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/jsoncpp
                        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
                        LIST_SEPARATOR %
                        UPDATE_COMMAND ""
                        )
    # ----- BME280 package -----
    ExternalProject_Add( 
                        bme280
                        GIT_REPOSITORY ${BME280_repository}
                        GIT_TAG ${BME280_version}
                        GIT_PROGRESS TRUE
                        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/bme280-prefix
                        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/bme280
                        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
                        LIST_SEPARATOR %
                        UPDATE_COMMAND ""
                        )
add_subdirectory(source)
