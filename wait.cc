#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
using namespace std;

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // 子进程
        cout << "子进程运行中,PID = " << getpid() << endl;
        sleep(2);
        cout << "子进程结束" << endl;
    } else if (pid > 0) {
        // 父进程
        cout << "父进程等待子进程..." << endl;
        int status;
        waitpid(pid, &status, 0);  // 阻塞等待
        cout << "子进程已结束，状态码: " << WEXITSTATUS(status) << endl;
    } else {
        perror("fork");
    }
    return 0;
}