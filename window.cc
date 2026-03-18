#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>

using namespace std;

int main() {
    // 检查是否已经在新终端中运行
    const char* in_new_terminal = getenv("IN_NEW_TERMINAL");
    
    if (!in_new_terminal || string(in_new_terminal) != "1") {
        // 获取当前程序路径
        char program_path[1024];
        ssize_t len = readlink("/proc/self/exe", program_path, sizeof(program_path) - 1);
        
        if (len != -1) {
            program_path[len] = '\0';
            
            // 构建在新终端中运行的命令
            string cmd = "gnome-terminal -- bash -c 'export IN_NEW_TERMINAL=1 && \"" + 
                         string(program_path) + "\"; exec bash'";
            
            // 执行命令打开新终端
            int result = system(cmd.c_str());
            
            if (result == 0) {
                cout << "已在新的终端窗口中打开程序" << endl;
            } else {
                cerr << "错误：无法打开新终端" << endl;
                return 1;
            }
            return 0;
        } else {
            cerr << "错误：无法获取程序路径" << endl;
            return 1;
        }
    }
    
    // 以下是主程序逻辑（在新终端中执行的部分）
    cout << "这是在新终端中运行的程序" << endl;
    cout << "按回车键退出..." << endl;
    cin.get();
    
    return 0;
}