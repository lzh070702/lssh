#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
using namespace std;

int main() {
    pid_t pid = fork();
    if (pid == 0) {
        cout << "子进程:" << endl << getppid() << "   " << getpid() << endl;
    } else if (pid > 0) {
        cout << "父进程:" << endl << getpid() << "   " << pid << endl;
        wait(NULL);
    } else {
        perror("fork");
        return 1;
    }
    return 0;
}