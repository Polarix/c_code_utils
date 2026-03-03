# 日志输出和控制模块

logger是一个简约的日志控制模块，可以通过注册回调函数指定日志的输出。

## 基本特征

- 简约设计：仅依赖标准C的格式化输出。
- 极低消耗：除格式化输出缓冲外，基本不产生内存消耗。
- 独立实例：可以创建不同的实例实现不同log的定向输出。

## 该模块尝试解决以下痛点

- 在编写以库文件形式发布的模块时，需要输出log进行跟踪调试。
- 在模块调试时，修改模块库的log输出等级时只能重新编译。
- 编写的log库与模块耦合，重用时需要与log库解耦，操作麻烦。

## 编译

- 在logger根目录下创建build文件夹并进入：
``` bash 
mkdir build
cd build
```
- 生成编译文件：
``` bash
cmake .. -G "MinGW Makefiles"
```
如果想输出动态库，则添加BUILD_SHARED_LIBS参数：
``` bash
cmake .. -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=ON
```
- 开始编译：
``` bash
cmake --build ./
```
- 运行demo示例程序查看效果：
``` bash
./bin/logger_demo
```

