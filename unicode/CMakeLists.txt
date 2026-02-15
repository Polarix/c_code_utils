cmake_minimum_required(VERSION 3.10)
project(Unicode VERSION 1.0.0 LANGUAGES C)

# 强制使用 C11 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 使用 GNU 安装目录规范
include(GNUInstallDirs)

# 头文件目录（仅用于构建）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# 库源文件
set(UNICODE_SRC src/unicode_utils.c)
set(UNICODE_HDR inc/unicode_utils.h)

# ============================================================
# 根据 BUILD_SHARED_LIBS 决定构建动态库还是静态库
# ============================================================
# 默认构建静态库，用户可通过 -DBUILD_SHARED_LIBS=ON 切换为动态库
option(BUILD_SHARED_LIBS "Build shared library (.so/.dll) instead of static" OFF)

if(BUILD_SHARED_LIBS)
    add_library(unicode SHARED ${UNICODE_SRC})
    # 动态库需要导出符号，定义 UNICODE_EXPORTS
    target_compile_definitions(unicode PRIVATE UNICODE_EXPORTS)
    # 可选：定义 UNICODE_SHARED 供用户使用（这里未强制）
else()
    add_library(unicode STATIC ${UNICODE_SRC})
endif()

# 设置版本信息
set_target_properties(unicode PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# 头文件搜索路径（构建时 + 安装时）
target_include_directories(unicode PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/unicode>
)

# 指定 PUBLIC_HEADER 属性（便于安装）
set_target_properties(unicode PROPERTIES
    PUBLIC_HEADER "${UNICODE_HDR}"
)

# ============================================================
# 演示程序（始终链接当前构建的 unicode 目标）
# ============================================================
add_executable(unicode_demo demo/unicode_utils_demo.c)
target_link_libraries(unicode_demo unicode)
set_target_properties(unicode_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ============================================================
# 安装规则（适配动态库与静态库）
# ============================================================
install(TARGETS unicode
    EXPORT unicodeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}      # 静态库 / Windows 导入库
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}      # 动态库（非 Windows）
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}      # Windows DLL
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/unicode
)

# 同时安装头文件（确保所有 .h 都被安装）
install(FILES ${UNICODE_HDR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/unicode)

# 导出目标文件
install(EXPORT unicodeTargets
    FILE unicodeTargets.cmake
    NAMESPACE unicode::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/unicode
)

# ============================================================
# 生成 CMake 配置文件
# ============================================================
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/unicodeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/unicodeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/unicode
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/unicodeConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/unicodeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/unicodeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/unicode
)
