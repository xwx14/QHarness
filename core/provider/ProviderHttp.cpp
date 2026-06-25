#include "provider/ProviderHttp.h"
#include <httplib.h>

namespace qh {
namespace provider {

namespace {
struct ParsedUrl {
    bool use_ssl = true;
    std::string host;
    int port = 0;
    std::string basePath;
};

// 解析 baseUrl（形如 https://api.openai.com/v1 或 http://127.0.0.1:19080/v1）
ParsedUrl parseBaseUrl(const std::string& baseUrl) {
    ParsedUrl p;
    std::string url = baseUrl;
    if (url.rfind("https://", 0) == 0) { url = url.substr(8); p.use_ssl = true; }
    else if (url.rfind("http://", 0) == 0) { url = url.substr(7); p.use_ssl = false; }

    auto slash = url.find('/');
    std::string authority = (slash != std::string::npos) ? url.substr(0, slash) : url;
    p.basePath = (slash != std::string::npos) ? url.substr(slash) : "";
    if (!p.basePath.empty() && p.basePath.back() == '/') p.basePath.pop_back();

    auto colon = authority.find(':');
    if (colon != std::string::npos) {
        p.host = authority.substr(0, colon);
        p.port = std::stoi(authority.substr(colon + 1));
    } else {
        p.host = authority;
        p.port = p.use_ssl ? 443 : 80;
    }
    return p;
}
} // namespace

HttpResponse ProviderHttp::post(
    const std::string& baseUrl,
    const std::string& path,
    const std::vector<std::pair<std::string, std::string>>& headers,
    const std::string& body,
    const schema::CancellationToken& cancel) {
    HttpResponse out;
    if (cancel.isCancelled()) { out.error = "cancelled"; return out; }

    ParsedUrl p = parseBaseUrl(baseUrl);
    const std::string fullPath = p.basePath + path;

    httplib::Headers hdrs;
    for (const auto& h : headers) hdrs.emplace(h.first, h.second);

    auto fillFrom = [&](httplib::Result& res) {
        if (!res) { out.error = "网络错误：无法连接或请求失败"; return; }
        out.ok = true;
        out.status = res->status;
        out.body = res->body;
    };

    try {
        if (p.use_ssl) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            httplib::SSLClient cli(p.host, p.port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(120, 0);
            cli.enable_server_certificate_verification(false);
            auto res = cli.Post(fullPath, hdrs, body, "application/json");
            fillFrom(res);
#else
            out.error = "未启用 HTTPS（缺少 OpenSSL）";
#endif
        } else {
            httplib::Client cli(p.host, p.port);
            cli.set_connection_timeout(30, 0);
            cli.set_read_timeout(120, 0);
            auto res = cli.Post(fullPath, hdrs, body, "application/json");
            fillFrom(res);
        }
    } catch (const std::exception& e) {
        out.error = std::string("请求异常：") + e.what();
    } catch (...) {
        out.error = "请求异常：未知错误";
    }

    // Post 返回后再检查一次取消（同步 Post 期间无法中断）
    if (cancel.isCancelled()) {
        out.ok = false;
        out.status = 0;
        out.body.clear();
        out.error = "cancelled";
    }
    return out;
}

} // namespace provider
} // namespace qh
