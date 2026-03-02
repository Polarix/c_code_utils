cmake_minimum_required(VERSION 3.10)
project(ThreadWrapper LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 指定头文件路径（假设 thread.h 在 include/thread 下，thread.cpp 在 src 下）
# 这里我们将所有文件放在同一目录，稍作调整
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# 查找 pthread 库
find_package(Threads REQUIRED)

# 定义源文件
set(LIB_SRC src/thread/thread.cpp)
set(LIB_HDR inc/thread/thread.h)
set(LIB_HDR src/log/misc_log.cpp)

# 创建静态库
add_library(threadwrapper STATIC ${LIB_SRC} ${LIB_HDR})
target_link_libraries(threadwrapper PRIVATE Threads::Threads)

# 如果是 MinGW，可能需要额外链接
if(MINGW)
    target_link_libraries(threadwrapper PRIVATE -lpthread)
endif()

# 创建测试可执行文件
add_executable(test_thread test/test_thread.cpp)
target_link_libraries(test_thread PRIVATE threadwrapper Threads::Threads)

# 设置输出目录（可选）
set_target_properties(threadwrapper PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
set_target_properties(test_thread PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 安装规则（可选）
install(TARGETS threadwrapper test_thread
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES thread.h DESTINATION include/thread)