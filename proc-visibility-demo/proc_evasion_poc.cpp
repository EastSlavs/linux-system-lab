/**
 * @file proc_evasion_poc.cpp
 * @brief Linux 进程伪装与 procfs 行为监控演示
 * @note 仅供安全排查演练与本地防御测试使用，请勿用于生产环境。
 */

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/prctl.h>

std::atomic<bool> g_is_safe(true);

// 监控目标列表（测试用）
const char* MONITOR_TOOLS[] = {"test_top", "test_ps"};

void monitor_process() {
    char path_buf[64];
    char comm_buf[16]; 

    while (true) {
        bool detected = false;
        DIR* dir = opendir("/proc");
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir))) {
                if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                    snprintf(path_buf, sizeof(path_buf), "/proc/%s/comm", entry->d_name);
                    
                    int fd = open(path_buf, O_RDONLY);
                    if (fd >= 0) {
                        ssize_t bytes = read(fd, comm_buf, sizeof(comm_buf) - 1);
                        if (bytes > 0) {
                            comm_buf[bytes] = '\0';
                            if (comm_buf[bytes - 1] == '\n') comm_buf[bytes - 1] = '\0';
                            
                            for (const char* tool : MONITOR_TOOLS) {
                                if (strcmp(comm_buf, tool) == 0) {
                                    detected = true;
                                    break;
                                }
                            }
                        }
                        close(fd);
                    }
                }
                if (detected) break;
            }
            closedir(dir);
        }

        g_is_safe = !detected;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void cpu_worker() {
    volatile double x = 1.5; 
    while (true) {
        if (g_is_safe) {
            for (int i = 0; i < 50000; ++i) {
                x = x * 1.000001 + 0.001;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
}

int main(int argc, char* argv[]) {
    // 覆盖原始参数，修改进程显示名称
    prctl(PR_SET_NAME, "proc_demo");
    
    if (argc > 0) {
        char* arg_start = argv[0];
        char* arg_end = argv[argc - 1] + strlen(argv[argc - 1]);
        memset(arg_start, 0, arg_end - arg_start);
        strncpy(arg_start, "proc_demo", 9); 
    }

    // 进程监控线程（仅作逻辑展示，默认不启动）
    // std::thread monitor_thread(monitor_process);

    unsigned int cores = std::thread::hardware_concurrency() / 2;
    if (cores == 0) cores = 1;

    std::vector<std::thread> workers;
    for (unsigned int i = 0; i < cores; ++i) {
        workers.emplace_back(cpu_worker);
    }

    for (auto& w : workers) {
        if (w.joinable()) w.join();
    }

    return 0;
}
