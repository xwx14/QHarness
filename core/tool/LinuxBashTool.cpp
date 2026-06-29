#include "tool/LinuxBashTool.h"
#include "tool/ContentCodec.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <chrono>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#endif

namespace qh {
namespace tool {

namespace fs = std::filesystem;

static constexpr std::size_t kMaxLen = 8000;

LinuxBashTool::LinuxBashTool(std::string workDir) {
    _workDir = std::move(workDir);
    _definition._name = "bash";
    _definition._description = "在工作目录运行 Linux bash 命令。请提供 command。";
    _definition._inputSchema = nlohmann::json::object();
    _definition._inputSchema["type"] = "object";
    _definition._inputSchema["properties"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["command"]["type"] = "string";
    _definition._inputSchema["properties"]["timeout_ms"]["type"] = "integer";
    _definition._inputSchema["required"] = nlohmann::json::array({ "command" });
}

schema::ToolResult LinuxBashTool::execute(const schema::ToolCall& call) {
    schema::ToolResult result;
    result._toolCallId = call._id;

    const nlohmann::json args = call.parseArguments();
    if (!args.is_object() || !args.contains("command") || !args["command"].is_string()) {
        result._output = "参数解析失败: 缺少字符串字段 command";
        result._isError = true;
        return result;
    }
    const std::string command = args["command"].get<std::string>();
    long long timeoutMs = 30000;
    if (args.contains("timeout_ms") && args["timeout_ms"].is_number_integer()) {
        timeoutMs = args["timeout_ms"].get<long long>();
    }
    const fs::path base = resolveWorkBase();
    if (base.empty()) {
        result._output = "工作目录解析失败: " + _workDir;
        result._isError = true;
        return result;
    }

#ifndef _WIN32
    int fds[2];
    if (pipe(fds) != 0) {
        result._output = "pipe 失败"; result._isError = true; return result;
    }
    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]); close(fds[1]);
        result._output = "fork 失败"; result._isError = true; return result;
    }
    if (pid == 0) {
        // 子进程：重定向 stdout/stderr 到管道，chdir 到工作目录，exec /bin/sh -c
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        dup2(fds[1], STDERR_FILENO);
        close(fds[1]);
        if (chdir(base.c_str()) != 0) _exit(127);
        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);   // exec 失败
    }
    // 父进程：读输出
    close(fds[1]);
    std::string output;
    char buf[4096]; ssize_t n;
    while ((n = read(fds[0], buf, sizeof(buf))) > 0) {
        output.append(buf, static_cast<size_t>(n));
        if (output.size() > kMaxLen * 2) break;
    }
    close(fds[0]);

    // 超时轮询等待
    bool timedOut = false;
    int status = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (true) {
        const pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid || r < 0) break;
        if (std::chrono::steady_clock::now() >= deadline) {
            kill(pid, SIGKILL);
            timedOut = true;
            waitpid(pid, &status, 0);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    const int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    output = contentCodec::toUtf8(output);
    if (output.size() > kMaxLen) {
        output = contentCodec::truncateUtf8Safe(output, kMaxLen) +
            "\n\n...[输出过长，已截断至前 " + std::to_string(kMaxLen) + " 字节]...";
    }
    if (timedOut) output += "\n[命令超时(" + std::to_string(timeoutMs) + "ms)已被终止]";
    output += "\n[exit code: " + std::to_string(exitCode) + "]";

    result._output = std::move(output);
    result._isError = (timedOut || exitCode != 0);
    return result;
#else
    result._output = "LinuxBashTool 仅在 Linux 编译";
    result._isError = true;
    return result;
#endif
}

} // namespace tool
} // namespace qh
