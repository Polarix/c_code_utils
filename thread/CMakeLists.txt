cmake_minimum_required(VERSION 3.10)
project(ThreadWrapper LANGUAGES CXX)

# 选项：是否使用原子状态（默认关闭）
option(USE_ATOMIC_THREAD_STATE "Use atomic for thread state" OFF)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# 查找 pthread
find_package(Threads REQUIRED)

# 源文件
set(LIB_SRC
    src/thread/thread.cpp
    src/log/misc_log.cpp   # 假设存在日志实现，若没有可移除
)
set(LIB_HDR
    inc/thread/thread.h
)

# 根据 USE_ATOMIC_THREAD_STATE 添加编译定义
if(USE_ATOMIC_THREAD_STATE)
    add_compile_definitions(USE_ATOMIC_THREAD_STATE)
    message(STATUS "Thread state using atomic")
else()
    message(STATUS "Thread state using mutex")
endif()

# 创建库（动态或静态，由 BUILD_SHARED_LIBS 控制）
add_library(threadwrapper ${LIB_SRC} ${LIB_HDR})
target_link_libraries(threadwrapper PRIVATE Threads::Threads)

if(MINGW)
    target_link_libraries(threadwrapper PRIVATE -lpthread)
endif()

# 创建测试可执行文件
add_executable(test_thread test/test_thread.cpp)
target_link_libraries(test_thread PRIVATE threadwrapper Threads::Threads)

# 设置输出目录
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
install(FILES inc/thread/thread.h DESTINATION include/thread)
