CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
SET(PRJ test_lookup_table)
PROJECT(${PRJ})
SET(BOOST_ROOT "/usr/local/share/")
FIND_PACKAGE(Boost COMPONENTS thread regex program_options system REQUIRED)


INCLUDE_DIRECTORIES("../inc/")
INCLUDE_DIRECTORIES("/usr/local/include/")
INCLUDE_DIRECTORIES("/usr/local/share/include/")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11 -O0 -g -DTEST_PRINT")

##
SET(TEST_SOURCE_FILES
        single_thread_test.cc
        multi_thread_test.cc
        multi_thread_17bit_test.cc
        multi_thread_18bit_test.cc
        multi_thread_20bit_test.cc
        basic_test.cc
        )

ADD_EXECUTABLE(${PRJ} ${TEST_SOURCE_FILES})
TARGET_LINK_LIBRARIES(${PRJ} gtest gtest_main)
TARGET_LINK_LIBRARIES(${PRJ} pthread)
TARGET_LINK_LIBRARIES(${PRJ} ${Boost_LIBRARIES})
##
ADD_TEST(lookup ${PRJ})
if (UNIX AND NOT APPLE)
    target_link_libraries(${PRJ} rt)
endif()
