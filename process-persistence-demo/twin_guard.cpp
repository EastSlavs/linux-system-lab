#include <fstream>
#include <string>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

constexpr int BUSY_US = 8000;
constexpr int IDLE_US = 2000;
constexpr auto CHECK_INTERVAL = std::chrono::seconds(1);

void restoreBinary(const std::string& path) {
    const std::string temp_path = path + ".tmp";

    std::ifstream src("/proc/self/exe", std::ios::binary);
    std::ofstream dst(temp_path, std::ios::binary);
    
    if (!src || !dst) return;
    
    dst << src.rdbuf();
    dst.close();

    if (!dst) {
        unlink(temp_path.c_str());
        return;
    }

    chmod(temp_path.c_str(), 0755);

    if (rename(temp_path.c_str(), path.c_str()) == -1) {
        unlink(temp_path.c_str());
    }
}

int main() {
    char buf[1024];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len == -1) return 1;
    buf[len] = '\0';
    
    const std::string exec_path(buf);

    while (true) {
        pid_t pid = fork();
        
        if (pid > 0) {
            waitpid(pid, nullptr, 0); 
        } 
        else if (pid == 0) {
            pid_t monitor_pid = getppid();
            auto last_check = std::chrono::steady_clock::now();
            
            while (true) {
                // 父进程发生变化，当前进程跳出，于外层晋升为Monitor拉起新Worker
                const pid_t current_ppid = getppid();
                if (current_ppid != monitor_pid || current_ppid == 1) {
                    break; 
                }
                
                auto now = std::chrono::steady_clock::now();
                if (now - last_check >= CHECK_INTERVAL) {
                    if (access(exec_path.c_str(), F_OK) == -1) {
                        restoreBinary(exec_path);
                    }
                    last_check = now;
                }
                
                auto start = std::chrono::steady_clock::now();
                while (std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - start).count() < BUSY_US) {
                    __asm__ volatile("nop"); 
                }
                usleep(IDLE_US); 
            }
        } 
        else if (pid < 0) {
            sleep(1); 
        }
    }
    return 0;
}
