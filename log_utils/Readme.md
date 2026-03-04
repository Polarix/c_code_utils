# 日志输出和控制模块

logger是一个简约的日志控制模块，可以通过注册回调函数指定日志的输出。

## 基本特征

- 简约设计：仅依赖标准C的格式化输出。
- 极低消耗：除格式化输出缓冲外，基本不产生内存消耗。
- 独立实例：可以创建不同的实例实现不同log的定向输出。

## 该模块尝试解决以下痛点

- 在编写以库文件形式发布的模块时，需要输出log进行跟踪调试。
- 如果简单的使用标准IO输出Log，在模块调试时，修改模块库的log输出等级时只能重新编译。
- 如果使用现成的log库，编写的库文件将同时依赖log库，造成编写的库与log模块耦合，重用时需要与log库解耦，操作麻烦。

## 解决方式

- #### 内部包含

这种方式下，log_utils组件以head-only的形式，组入到各个库文件中，由内部的静态全局实例实现log等级的控制和输出回调的托管。

由于在head-only模式下，log_utils组件的函数实现均被static inline形修饰，会被打包至库的实现内部，不同的库之间也不会产生符号重复的问题。

> - 优点：log_utils组件被作为库内容的一部分编译，对外提供控制接口的同时也不再对外部的log输出接口或库产生依赖。
> - 缺点：不同的库都引用了一份log_utils的实现，在引用了多个使用log_utils组件的库时，会不可避免的造成编译输出的体积膨胀。

- #### 全局引用

这种方式下，log_utils组件会以动态库或静态库的形式存在，各个库仅需自行托管log的控制实例即可。

> - 优点：不同的库之间都是用了同一份log_utils组件的实装，缩减了编译输出的体积。
> - 缺点：使用utils_log的库存在额外的依赖，在复用和分离时依然需要解耦。

- #### 使用建议

从设计初衷来说，推荐使用**内部包含**的方式使用log_utils库，且log_utils库本身就是简化设计，编译后的二进制内容体积有限，如果不是及大量使用，带来的代码膨胀也十分有限，整体对环境的影响不大。

## 编译和使用

#### 编译参数
log_utils库提供了CMake的编译配置并设置了三个编译参数：

 - LOG_UTILS_HEAD_ONLY：是否使用head-only模式，默认开启。
 - BUILD_SHARED_LIBS：编译静态库还是动态库，如果LOG_UTILS_HEAD_ONLY为ON的话，此选项将被无视。
 - BUILD_LOG_UTILS_DEMO：是否编译演示程序，默认关闭。

#### 示例

在logger根目录下创建build文件夹并进入：
``` bash 
mkdir build
cd build
```

生成编译文件：
- 编译静态库
``` bash
cmake .. -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=OFF -DLOG_UTILS_HEAD_ONLY=OFF
```

- 编译动态库，并编译演示程序
``` bash
cmake .. -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=ON -DLOG_UTILS_HEAD_ONLY=OFF -DBUILD_LOG_UTILS_DEMO=ON
```

- 使用Head-Only模式，并编译演示程序
``` bash
cmake .. -G "MinGW Makefiles" -DLOG_UTILS_HEAD_ONLY=ON -DBUILD_LOG_UTILS_DEMO=ON
```

开始编译：
``` bash
cmake --build ./
```

如果选择了编译演示程序，可以运行并查看效果：
``` bash
./bin/log_utils_demo
```

- #### 使用
如果使用head-lony模式，需要在生成库的编译环境中**定义LOG_UTILS_HEAD_ONLY**宏以确保接口的声明和定义均被正确包含。
