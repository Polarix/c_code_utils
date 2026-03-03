cmake_minimum_required(VERSION 3.10)
project(LogUtils VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(GNUInstallDirs)

# 头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# 源文件
set(LOG_UTILS_SRC src/log_utils.c)
set(LOG_UTILS_HDR inc/log_utils.h)

# 选项：是否构建库（如果为 OFF，则只安装头文件，实现 header-only 模式）
option(BUILD_LOG_UTILS_LIB "Build log_utils library (shared or static)" ON)
option(BUILD_SHARED_LIBS "Build shared library" OFF)

if(BUILD_LOG_UTILS_LIB)
    if(BUILD_SHARED_LIBS)
        add_library(log_utils SHARED ${LOG_UTILS_SRC})
        target_compile_definitions(log_utils PRIVATE LOG_UTILS_EXPORTS)
    else()
        add_library(log_utils STATIC ${LOG_UTILS_SRC})
    endif()

    set_target_properties(log_utils PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "${LOG_UTILS_HDR}"
    )

    target_include_directories(log_utils PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/log_utils>
    )

    # 演示程序
    add_executable(log_utils_demo demo/log_utils_demo.c)
    target_link_libraries(log_utils_demo log_utils)
    set_target_properties(log_utils_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # 安装规则
    install(TARGETS log_utils
        EXPORT log_utilsTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log_utils
    )
else()
    # 不构建库，只安装头文件
    install(FILES ${LOG_UTILS_HDR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log_utils)
endif()

# 导出和配置文件（仅在构建库时生成）
if(BUILD_LOG_UTILS_LIB)
    install(EXPORT log_utilsTargets
        FILE log_utilsTargets.cmake
        NAMESPACE log_utils::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/log_utils
    )

    include(CMakePackageConfigHelpers)

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/log_utilsConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/log_utilsConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/log_utils
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/log_utilsConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/log_utilsConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/log_utilsConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/log_utils
    )
endif()