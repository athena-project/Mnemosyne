CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "DEBUG")
  #SET(CMAKE_BUILD_TYPE "RELEASE")
  #SET(CMAKE_BUILD_TYPE "RELWITHDEBINFO")
  #SET(CMAKE_BUILD_TYPE "MINSIZEREL")
ENDIF()


#Project config
#PROJECT(Mnemosyne)
#SET(LIBRARY_OUTPUT_PATH lib/${CMAKE_BUILD_TYPE})

#Configuration de l'exécutable
FILE( GLOB_RECURSE  source_files  ../lib/*)




INCLUDE_DIRECTORIES( "/usr/local/mysql/lib" 
						"/usr/include/mysql"
						"/usr/include/python3.4" 
						"/home/toor/Downloads/boost_1_56_0/stage/lib" 
						"/home/toor/Github/Mnemosyne/trunk/lib" 
					)
SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREADED ON)
SET(Boost_USE_STATIC_RUNTIME OFF)






ADD_DEFINITIONS("-Wall")
ADD_DEFINITIONS("-std=c++11")






ADD_LIBRARY(pyRessource SHARED ../lib/PyRessource.cpp)

TARGET_LINK_LIBRARIES(pyRessource  "/home/toor/Downloads/boost_1_56_0/stage/lib/libboost_python3.so" 
									"/home/toor/Downloads/boost_1_56_0/stage/lib/libboost_system.so" 
									"/home/toor/Downloads/mysql++-3.2.1/libmysqlpp.so"
									"/usr/lib/x86_64-linux-gnu/liblzma.so")

