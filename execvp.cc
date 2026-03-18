#include <unistd.h>
#include <iostream>
using namespace std;

int main() {
    char* home = getenv("HOME");
    char* args[] = {(char*)"ls", (char*)"-l", home, nullptr};
    cout << "准备执行 ls -l ..." << endl;
    execvp("ls", args);
    perror("execvp");
    return 1;
}