# Process Persistence Demo

演示基于 `fork` 双进程守护与 `/proc/self/exe` 内存恢复的进程驻留机制。

### 核心文件

* `twin_guard.cpp`：包含进程状态互相监控与二进制文件自动重建逻辑的靶机代码。

### 编译与测试

建议在独立的测试环境中运行验证：

```bash
g++ twin_guard.cpp -o twin_guard -std=c++11
./twin_guard
