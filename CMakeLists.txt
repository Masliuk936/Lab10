cmake_minimum_required(VERSION 3.21)
project(Lr10 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SqliteOrm CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_executable(Lr10
    src/main.cpp
    src/SQLiteBuildingRepository.cpp
)

target_include_directories(Lr10
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Lr10
    PRIVATE
        sqlite_orm::sqlite_orm
        unofficial::sqlite3::sqlite3
)

enable_testing()

add_executable(Lr10_tests
    tests/test_patterns.cpp
    src/SQLiteBuildingRepository.cpp
)

target_include_directories(Lr10_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Lr10_tests
    PRIVATE
        sqlite_orm::sqlite_orm
        unofficial::sqlite3::sqlite3
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(Lr10_tests)