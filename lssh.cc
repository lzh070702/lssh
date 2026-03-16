#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

// 定义字体
#define BOLD "\033[1m"     // 加粗
#define RED "\033[91m"     // 红色
#define GREEN "\033[92m"   // 绿色
#define CYAN "\033[1;96m"  // 青色加粗
#define RESET "\033[0m"    // 重置

struct job {
    int num;      // 作业号
    string line;  // 后台命令
};

string cmd_line;                // 命令
string prev_dir;                // 上次目录
string current_dir;             // 当前目录
bool prev_cmd_status = true;    // 上次执行状态
int job_num = 0;                // 作业计数
vector<pair<pid_t, job>> jobs;  // pid--job

void sigchld_handler(int signum);                  // 后台子进程结束信号处理
string get_current_dir();                          // 获取当前目录
void init_dirs();                                  // 初始化目录
void splash_screen();                              // 启动界面
void clear_zom_proc();                             // 清理僵尸进程
void print_cmd_rompt();                            // 打印命令提示符
vector<string> split(const string& line, char c);  // 分割命令行
bool parse_cmd(string& cmd_line,
               vector<vector<string>>& cmd_args,
               string& in_file,
               string& out_file,
               int& append,
               bool& background);                       // 解析参数
bool is_home_subdir(const string& dir, int& pos);       // 判断家目录子目录
bool exec_cd_cmd(const vector<string>& cd_cmd);         // 执行cd命令
vector<char*> string_char(const vector<string>& args);  // 类型转换
bool exec_pipe_cmd(const vector<vector<string>>& cmd_args,
                   const string& in_file,
                   const string& out_file,
                   const int& append,
                   const bool& background);  // 执行pipe命令

int main() {
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, sigchld_handler);
    init_dirs();
    system("clear");
    splash_screen();
    system("clear");
    while (true) {
        clear_zom_proc();
        print_cmd_rompt();
        cmd_line.clear();
        if (!getline(cin, cmd_line)) {
            break;
        }
        if (cmd_line.empty()) {
            continue;
        }
        vector<vector<string>> cmd_args;
        string in_file, out_file;
        int append = O_TRUNC;
        bool background = false;
        if (!parse_cmd(cmd_line, cmd_args, in_file, out_file, append,
                       background)) {
            prev_cmd_status = false;
            continue;
        }
        if (cmd_args.size() == 1) {
            if (cmd_args[0][0] == "exit") {
                break;
            } else if (cmd_args[0][0] == "cd") {
                if (!exec_cd_cmd(cmd_args[0])) {
                    prev_cmd_status = false;
                }
                continue;
            }
        }
        if (!exec_pipe_cmd(cmd_args, in_file, out_file, append, background)) {
            prev_cmd_status = false;
            continue;
        }
    }
    system("clear");
    return 0;
}

void sigchld_handler(int signum) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        job j;
        size_t i;
        for (i = 0; i < jobs.size(); i++) {
            if (jobs[i].first == pid) {
                j = jobs[i].second;
                break;
            }
        }
        cout << endl;
        cout << "[" << j.num << "] ";
        if (i == jobs.size() - 1) {
            cout << " + ";
        } else if (i == jobs.size() - 2) {
            cout << " - ";
        } else {
            cout << "   ";
        }
        cout << pid << " done       " << j.line << endl;
        jobs.erase(jobs.begin() + i);
        --job_num;
        if (!WIFEXITED(status)) {
            prev_cmd_status = false;
        }
        print_cmd_rompt();
    }
}

string get_current_dir() {
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf)) != nullptr) {
        return string(buf);
    }
    return "";
}

void init_dirs() {
    current_dir = get_current_dir();
    prev_dir = current_dir;
}

void splash_screen() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int len = (w.ws_col - 42) / 2;
    cout << BOLD "欢迎使用lzh-super-shell" RESET << endl;
    usleep(300000);
    cout << string(len, ' ') << "██╗        ████████╗  ████████╗  ██╗   ██╗"
         << endl;
    usleep(300000);
    cout << string(len, ' ') << "██║        ██╔═════╝  ██╔═════╝  ██║   ██║"
         << endl;
    usleep(300000);
    cout << string(len, ' ') << "██║        ████████╗  ████████╗  ████████║"
         << endl;
    usleep(300000);
    cout << string(len, ' ') << "██║        ╚═════██║  ╚═════██║  ██╔═══██║"
         << endl;
    usleep(300000);
    cout << string(len, ' ') << "████████╗  ████████║  ████████║  ██║   ██║"
         << endl;
    usleep(300000);
    cout << string(len, ' ') << "╚═══════╝  ╚═══════╝  ╚═══════╝  ╚═╝   ╚═╝"
         << endl;
    usleep(300000);
    cout << BOLD "请按回车键继续. . ." RESET << endl;
    cin.get();
}

void clear_zom_proc() {
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
        ;
    }
}

void print_cmd_rompt() {
    if (prev_cmd_status) {
        cout << GREEN "➜  " RESET;
    } else {
        cout << RED "➜  " RESET;
    }
    // 重置上次执行状态
    prev_cmd_status = true;
    if (current_dir.size() == 1) {
        cout << CYAN "/" RESET;
        return;
    }
    string home_dir = getenv("HOME");
    if (current_dir == home_dir) {
        cout << CYAN "~ " << RESET;
        return;
    }
    auto i = current_dir.size();
    while (i != 0) {
        if (current_dir[--i] == '/') {
            break;
        }
    }
    cout << CYAN << current_dir.substr(i + 1) << " " << RESET;
    cout.flush();
}

vector<string> split(const string& line, char c) {
    vector<string> tokens;
    string token;
    for (auto ch : line) {
        if (ch != c) {
            token += ch;
        } else if (!token.empty()) {
            tokens.push_back(token);
            token.clear();
        }
    }
    if (!token.empty())
        tokens.push_back(token);
    return tokens;
}

bool parse_cmd(string& cmd_line,
               vector<vector<string>>& cmd_args,
               string& in_file,
               string& out_file,
               int& append,
               bool& background) {
    // 判断是否后台运行
    for (auto i = cmd_line.size() - 1; cmd_line[i] == ' ' || cmd_line[i] == '&';
         i--) {
        if (cmd_line[i] == '&') {
            background = true;
        }
        cmd_line.pop_back();
    }
    // 补全管道内容
    while (cmd_line.back() == '|') {
        cout << "pipe>";
        string pipe_cmd_line;
        getline(cin, pipe_cmd_line);
        cmd_line.insert(cmd_line.end(), pipe_cmd_line.begin(),
                        pipe_cmd_line.end());
    }
    // 按管道符分割命令行
    vector<string> cmds = split(cmd_line, '|');
    if (cmds.empty()) {
        return false;
    }
    // 输入文件
    for (auto i = 0; i < cmds[0].size(); i++) {
        if (cmds[0][i] == '<') {
            if (i < cmds[0].size() - 1) {
                in_file = cmds[0].substr(i + 1);
                cmds[0].erase(i);
                break;
            } else {
                return false;
            }
        }
    }
    if (!in_file.empty()) {
        in_file = split(in_file, ' ').back();
    }
    // 输出文件
    for (auto i = 0; i < cmds.back().size(); i++) {
        if (cmds.back()[i] == '>') {
            if (cmds.back()[i + 1] == '>') {
                if (i < cmds.back().size() - 2) {
                    out_file = cmds.back().substr(i + 2);
                    cmds.back().erase(i);
                    append = O_APPEND;
                } else {
                    return false;
                }
            } else {
                if (i < cmds.back().size() - 1) {
                    out_file = cmds.back().substr(i + 1);
                    cmds.back().erase(i);
                } else {
                    return false;
                }
            }
            break;
        }
    }
    if (!out_file.empty()) {
        out_file = split(out_file, ' ').back();
    }
    // 按空格分割命令行
    for (auto i : cmds) {
        vector<string> args = split(i, ' ');
        if (args.empty()) {
            return false;
        }
        cmd_args.push_back(args);
    }
    return true;
}

bool is_home_subdir(const string& dir, int& pos) {
    string home_dir = getenv("HOME");
    if (dir.size() < home_dir.size() + 1) {
        return false;
    }
    while (pos < home_dir.size()) {
        if (dir[pos] != home_dir[pos]) {
            return false;
        }
        pos++;
    }
    return true;
}

bool exec_cd_cmd(const vector<string>& cd_cmd) {
    string target_dir;
    string home_dir = getenv("HOME");
    if (cd_cmd.size() == 1) {
        target_dir = home_dir;
    } else if (cd_cmd.size() == 2) {
        target_dir = cd_cmd[1];
        if (cd_cmd[1] == "-") {
            target_dir = prev_dir;
            int pos = 0;
            if (target_dir == home_dir) {
                cout << "~" << endl;
            } else if (is_home_subdir(prev_dir, pos)) {
                cout << "~" << prev_dir.substr(pos) << endl;
            } else {
                cout << prev_dir << endl;
            }
        } else if (cd_cmd[1] == "~") {
            target_dir = home_dir;
        }
    } else {
        cout << "cd: string not in pwd: " << cd_cmd[1] << endl;
        return false;
    }
    if (chdir(target_dir.c_str()) == 0) {
        target_dir = get_current_dir();
        prev_dir = current_dir;
        current_dir = target_dir;
    } else {
        cout << "cd: 没有那个文件或目录: " << cd_cmd[1] << endl;
        return false;
    }
    return true;
}

vector<char*> string_char(const vector<string>& args) {
    vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

bool exec_pipe_cmd(const vector<vector<string>>& cmd_args,
                   const string& in_file,
                   const string& out_file,
                   const int& append,
                   const bool& background) {
    int cmd_num = cmd_args.size();
    int prev_pipe_read = -1;
    vector<pid_t> children;
    for (int i = 0; i < cmd_num; i++) {
        int pipefd[2] = {-1, -1};
        if (i < cmd_num - 1) {
            if (pipe(pipefd) == -1) {
                return false;
            }
        }
        pid_t pid = fork();
        if (pid == 0) {
            if (i == 0) {
                if (!in_file.empty()) {
                    int input_fd = open(in_file.c_str(), O_RDONLY);
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }
            } else {
                dup2(prev_pipe_read, STDIN_FILENO);
                close(prev_pipe_read);
            }
            if (i == cmd_num - 1) {
                if (!out_file.empty()) {
                    int output_fd = open(out_file.c_str(),
                                         O_WRONLY | O_CREAT | append, 0644);
                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                }
            } else {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }
            vector<char*> argv = string_char(cmd_args[i]);
            execvp(argv[0], argv.data());
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            children.push_back(pid);
            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }
            if (i < cmd_num - 1) {
                close(pipefd[1]);
                prev_pipe_read = pipefd[0];
            } else {
                prev_pipe_read = -1;
            }
        } else {
            return false;
        }
    }
    if (background) {
        jobs.push_back({children[0], {++job_num, cmd_line}});
        cout << "[" << jobs.back().second.num << "] " << children[0] << endl;
    } else {
        for (pid_t p : children) {
            int status;
            waitpid(p, &status, 0);
            if (status == EXIT_FAILURE) {
                prev_cmd_status = false;
            }
        }
    }
    return true;
}