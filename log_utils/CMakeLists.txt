cmake_minimum_required(VERSION 3.10)
project(LogUtils VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(GNUInstallDirs)

# 选项：header-only 模式（不构建库）
option(LOG_UTILS_HEAD_ONLY "Use header-only mode (no library build)" OFF)
# 当构建库时，由 BUILD_SHARED_LIBS 控制动态/静态
option(BUILD_SHARED_LIBS "Build shared library" OFF)

# 头文件
set(LOG_UTILS_HDR inc/log_utils.h)

# 安装头文件（任何模式下都安装）
install(FILES ${LOG_UTILS_HDR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log_utils)

if(NOT LOG_UTILS_HEAD_ONLY)
    # ========== 构建库模式 ==========
    set(LOG_UTILS_SRC src/log_utils.c)

    if(BUILD_SHARED_LIBS)
        add_library(log_utils SHARED ${LOG_UTILS_SRC})
        # 动态库需要导出符号
        target_compile_definitions(log_utils PRIVATE LOG_UTILS_BUILD)
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

    # 安装库和导出配置
    install(TARGETS log_utils
        EXPORT log_utilsTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log_utils
    )

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
else()
    message(STATUS "Header-only mode: no library will be built. Only headers installed.")
endif()

# ========== 演示程序（始终构建） ==========
add_executable(log_utils_demo demo/log_utils_demo.c)
target_include_directories(log_utils_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

if(NOT LOG_UTILS_HEAD_ONLY)
    # 独立库模式：链接库
    target_link_libraries(log_utils_demo log_utils)
else()
    # Header-only 模式：需要定义宏以获取函数实现
    target_compile_definitions(log_utils_demo PRIVATE LOG_UTILS_HEAD_ONLY)
endif()

set_target_properties(log_utils_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
