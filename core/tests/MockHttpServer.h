#ifndef QH_TEST_MOCK_HTTP_SERVER_H
#define QH_TEST_MOCK_HTTP_SERVER_H
#include <httplib.h>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

namespace qh {
namespace test {

// 测试用本地 HTTP server：后台线程运行，捕获最近一次请求体供断言
class MockHttpServer {
public:
    MockHttpServer() = default;
    ~MockHttpServer() { stop(); }
    MockHttpServer(const MockHttpServer&) = delete;
    MockHttpServer& operator=(const MockHttpServer&) = delete;

    // 注册某 POST 路径的固定响应；handler 同时捕获请求体
    void onPost(const std::string& pattern, int status, const std::string& respBody) {
        _svr.Post(pattern, [this, status, respBody](const httplib::Request& req, httplib::Response& res) {
            {
                std::lock_guard<std::mutex> lk(_mtx);
                _lastBody = req.body;
            }
            res.status = status;
            res.set_content(respBody, "application/json");
        });
    }

    bool start() {
        // 用端口 0 让 OS 分配可用端口，彻底规避固定端口与系统保留范围冲突
        // （Hyper-V/WSL2 的 excludedportrange 会动态保留大段端口，固定端口列表无法可靠避开）
        const int port = _svr.bind_to_any_port("127.0.0.1");
        if (port <= 0) return false;
        _port = port;
        _thread = std::thread([this] { _svr.listen_after_bind(); });
        _started = true;
        _svr.wait_until_ready();   // httplib 提供，精确等待 listen 就绪（替代固定 sleep）
        return true;
    }

    void stop() {
        if (_started) {
            _svr.stop();
            if (_thread.joinable()) _thread.join();
            _started = false;
        }
    }

    std::string baseUrl() const { return "http://127.0.0.1:" + std::to_string(_port) + "/v1"; }
    std::string lastBody() const {
        std::lock_guard<std::mutex> lk(_mtx);
        return _lastBody;
    }

private:
    httplib::Server _svr;
    std::thread _thread;
    int _port = 0;
    bool _started = false;
    mutable std::mutex _mtx;
    std::string _lastBody;
};

} // namespace test
} // namespace qh
#endif // QH_TEST_MOCK_HTTP_SERVER_H
