#include "provider/ProviderHttp.h"
#include <httplib.h>

namespace qh {
namespace provider {

namespace {
// SSL spike：验证 httplib::SSLClient 在 OpenSSL 3.6.3 + MSVC 下可编译链接。
// 必须放在 core（带 CPPHTTPLIB_OPENSSL_SUPPORT）；测试 target 无此宏，放测试处 #ifdef 会被跳过。
void sslCompileSpike() {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient c("api.openai.com", 443);
    c.enable_server_certificate_verification(false);
    (void)c;
#endif
}
} // namespace

HttpResponse ProviderHttp::post(
    const std::string& /*baseUrl*/,
    const std::string& /*path*/,
    const std::vector<std::pair<std::string, std::string>>& /*headers*/,
    const std::string& /*body*/,
    const schema::CancellationToken& /*cancel*/) {
    sslCompileSpike(); // 占位期内触发 SSLClient 实例化做编译验证；Task 2 实现后此调用随占位一起替换
    return HttpResponse{};
}

} // namespace provider
} // namespace qh
