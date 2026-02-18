cmake_minimum_required(VERSION 3.10)
project(mo_parser C)

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 项目信息
set(PROJECT_NAME "mo_parser")
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

# 定义查找策略选项，默认使用哈希查找。
set(MO_SEARCH_METHOD "HASH" CACHE STRING "Search method: LINEAR, BINARY, or HASH")
set_property(CACHE MO_SEARCH_METHOD PROPERTY STRINGS LINEAR BINARY HASH)

# 定义库文件
add_library(${PROJECT_NAME} STATIC
    src/mo_parser.c
)

# 根据选择的查找策略定义宏
if(MO_SEARCH_METHOD STREQUAL "LINEAR")
    target_compile_definitions(${PROJECT_NAME} PRIVATE MO_SEARCH_METHOD_LINEAR=1)
    message(STATUS "Using LINEAR search method")
elseif(MO_SEARCH_METHOD STREQUAL "BINARY")
    target_compile_definitions(${PROJECT_NAME} PRIVATE MO_SEARCH_METHOD_BINARY=1)
    message(STATUS "Using BINARY search method")
elseif(MO_SEARCH_METHOD STREQUAL "HASH")
    target_compile_definitions(${PROJECT_NAME} PRIVATE MO_SEARCH_METHOD_HASH=1)
    message(STATUS "Using HASH search method")
else()
    message(FATAL_ERROR "Unknown search method: ${MO_SEARCH_METHOD}. Use LINEAR, BINARY, or HASH")
endif()

# 可选：启用性能统计
option(MO_ENABLE_STATS "Enable performance statistics" OFF)
if(MO_ENABLE_STATS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MO_ENABLE_STATS=1)
endif()

# 可选：启用详细日志
option(MO_ENABLE_LOGGING "Enable detailed logging" OFF)
if(MO_ENABLE_LOGGING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MO_ENABLE_LOGGING=1)
endif()

# 设置库的属性
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>
)

# 根据平台设置属性
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(${PROJECT_NAME} PRIVATE kernel32 user32)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE _GNU_SOURCE)
endif()

# 创建测试程序
add_executable(test_${PROJECT_NAME}
    demo/test_mo.c
)

target_link_libraries(test_${PROJECT_NAME} PRIVATE ${PROJECT_NAME})

# 安装配置
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES inc/mo_parser.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 导出配置
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# 创建配置文件
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# 测试配置
enable_testing()
add_test(
    NAME basic_test
    COMMAND test_${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# 可选：创建示例程序
option(BUILD_EXAMPLES "Build example programs" OFF)
if(BUILD_EXAMPLES)
    add_executable(example_mo examples/example.c)
    target_link_libraries(example_mo PRIVATE ${PROJECT_NAME})
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
)

# 可选：创建不同查找策略的测试
add_executable(test_search_comparison demo/search_comparison.c)
target_link_libraries(test_search_comparison PRIVATE ${PROJECT_NAME})
