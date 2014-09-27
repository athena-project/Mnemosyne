CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "DEBUG")
  #SET(CMAKE_BUILD_TYPE "RELEASE")
  #SET(CMAKE_BUILD_TYPE "RELWITHDEBINFO")
  #SET(CMAKE_BUILD_TYPE "MINSIZEREL")
ENDIF()


#Project config
PROJECT(Mnemosyne)
#SET(LIBRARY_OUTPUT_PATH lib/${CMAKE_BUILD_TYPE})

#Configuration de l'exécutable
FILE( GLOB_RECURSE  source_files  lib/*)




#INCLUDE_DIRECTORIES( "/usr/local/mysql/lib")
INCLUDE_DIRECTORIES( "/usr/include/mysql" )


FIND_PACKAGE(Boost 1.54.0)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES("${Boost_INCLUDE_DIRS}" "/usr/include/python3.4")
  SET(Boost_USE_STATIC_LIBS OFF)
  SET(Boost_USE_MULTITHREADED ON)
  SET(Boost_USE_STATIC_RUNTIME OFF)
  FIND_PACKAGE(Boost 1.54.0 COMPONENTS python)

  ADD_LIBRARY(pyRessource SHARED lib/PyRessource.cpp)
  TARGET_LINK_LIBRARIES(pyRessource ${Boost_LIBRARIES})
ELSEIF(NOT Boost_FOUND)
  MESSAGE(FATAL_ERROR "Unable to find correct Boost version. Did you set BOOST_ROOT?")
ENDIF()






IF(CMAKE_COMPILER_IS_GNUCXX)
  ADD_DEFINITIONS("-Wall")
  ADD_DEFINITIONS("-lmysqlpp" "-lmysqlclient")
  ADD_DEFINITIONS("-lboost_filesystem" "-lboost_system" "-lboost_python")
  ADD_DEFINITIONS("-llzma" "-ltar")
  ADD_DEFINITIONS("-std=c++11")
ELSE()
  MESSAGE(FATAL_ERROR "CMakeLists.txt has not been tested/written for your compiler.")
ENDIF()
