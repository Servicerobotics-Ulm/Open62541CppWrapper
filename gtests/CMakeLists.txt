CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

PROJECT(Open62541CppWrapperTests)

FIND_PACKAGE(GTest QUIET)

FILE(GLOB SRCS ${PROJECT_SOURCE_DIR}/*.cc)

IF(GTest_FOUND)
ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Open62541CppWrapper gtest gtest_main)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 14
)
ENDIF(GTest_FOUND)
