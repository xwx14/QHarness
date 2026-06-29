#include "tool/WinBashTool.h"
#include "tool/PathCodec.h"
#include "tool/ContentCodec.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace qh {
namespace tool {

namespace fs = std::filesystem;

static constexpr std::size_t kMaxLen = 8000;

WinBashTool::WinBashTool(std::string workDir) {
    _workDir = std::move(workDir);
    _definition._name = "bash";
    _definition._description = "在工作目录运行 Windows 命令行命令(cmd)。请提供 command。";
    _definition._inputSchema = nlohmann::json::object();
    _definition._inputSchema["type"] = "object";
    _definition._inputSchema["properties"] = nlohmann::json::object();
    _definition._inputSchema["properties"]["command"]["type"] = "string";
    _definition._inputSchema["properties"]["command"]["description"] = "要执行的命令行";
    _definition._inputSchema["properties"]["timeout_ms"]["type"] = "integer";
    _definition._inputSchema["properties"]["timeout_ms"]["description"] = "超时毫秒，默认 30000";
    _definition._inputSchema["required"] = nlohmann::json::array({ "command" });
}

schema::ToolResult WinBashTool::execute(const schema::ToolCall& call) {
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

#ifdef _WIN32
    // 管道捕获 stdout+stderr
    SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        result._output = "CreatePipe 失败";
        result._isError = true;
        return result;
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{}; si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    PROCESS_INFORMATION pi{};

    // 命令行与工作目录均转 GBK（CreateProcessA 用本地代码页）
    std::string cmdLine = "cmd.exe /c " + pathCodec::utf8ToGbk(command);
    const std::string workDirGbk = pathCodec::utf8ToGbk(base.u8string());

    BOOL ok = CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW, nullptr,
                             workDirGbk.empty() ? nullptr : workDirGbk.c_str(),
                             &si, &pi);
    CloseHandle(hWrite);   // 关闭写端，否则子进程结束后 ReadFile 仍阻塞
    if (!ok) {
        CloseHandle(hRead);
        result._output = "CreateProcess 失败: " + std::to_string(GetLastError());
        result._isError = true;
        return result;
    }

    // 并发读取输出（否则子进程写满管道会阻塞，WaitForSingleObject 永等不到结束）
    std::string output;
    std::thread reader([&] {
        char buf[4096]; DWORD readN = 0;
        while (ReadFile(hRead, buf, sizeof(buf), &readN, nullptr) && readN > 0) {
            output.append(buf, readN);
            if (output.size() > kMaxLen * 2) break;
        }
    });
    const DWORD wait = WaitForSingleObject(pi.hProcess, static_cast<DWORD>(timeoutMs));
    const bool timedOut = (wait == WAIT_TIMEOUT);
    if (timedOut) TerminateProcess(pi.hProcess, 1);
    reader.join();   // 子进程结束/被杀后写端关闭，ReadFile 返回 0，reader 退出
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hRead);

    // cmd 输出按本地代码页(GBK)转 UTF-8，再截断
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
    result._output = "WinBashTool 仅在 Windows 编译";
    result._isError = true;
    return result;
#endif
}

} // namespace tool
} // namespace qh
