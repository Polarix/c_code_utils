# MoParser
这是一个轻量化的gettext实现，能够动态的解析和查询mo文件内容。

# 查找策略
MoParse提供三种查找策略。
- 线性查找策略：使用最普通的遍历查找，时间复杂度为O(n)，平均查找长度为(n+1)/2，如果数据量大，则会有严重的性能问题。这种模式多用于数据校验。

- 二分查找策略：加载后将数据排序，然后按二分查找，时间复杂度为O(log n)，平均查找长度为log 2n，这种方式需要对数据进行预处理，但是会大幅度改善查找效率。

- 哈希查找：加载数据后会先创建数据的哈希表，时间复杂度为O(1)，平均查找长度视哈希碰撞情况而定。此种方式具有最高的检索效率，但是哈希表会造成相对较大的内存占用。

### 编译
使用线性查找策略：
```shell
cmake -G "MinGW Makefiles" ../ -DMO_SEARCH_METHOD=LINEAR -B ./build-linear
cmake --build ./build-linear
```

使用二分查找策略：
```shell
cmake -G "MinGW Makefiles" ../ -DMO_SEARCH_METHOD=BINARY -B build-binary
cmake --build build-binary
```

使用哈希查找策略
```shell
cmake -G "MinGW Makefiles" ../ -DMO_SEARCH_METHOD=HASH -B build-hash
cmake --build build-hash
```

带性能统计的哈希表版本
```shell
cmake -G "MinGW Makefiles" ../ -DMO_SEARCH_METHOD=HASH -DMO_ENABLE_STATS=ON -B build-hash-stats
cmake --build build-hash-stats
```
