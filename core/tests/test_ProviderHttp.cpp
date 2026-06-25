#include "MockHttpServer.h"
#include "provider/ProviderHttp.h"
#include "schema/CancellationToken.h"
#include "TestHarness.h"

QH_TEST(ProviderHttp_post_success) {
    qh::test::MockHttpServer svr;
    QH_CHECK(svr.start());
    svr.onPost("/v1/echo", 200, R"({"ok":true})");

    qh::schema::CancellationToken cancel;
    auto resp = qh::provider::ProviderHttp::post(
        svr.baseUrl(), "/echo", {{"X-Test", "v"}}, R"({"hello":"world"})", cancel);

    QH_CHECK(resp.ok);
    QH_CHECK_EQ(resp.status, 200);
    QH_CHECK_EQ(resp.body, std::string(R"({"ok":true})"));
    QH_CHECK_EQ(svr.lastBody(), std::string(R"({"hello":"world"})"));
}

QH_TEST(ProviderHttp_post_http_error) {
    qh::test::MockHttpServer svr;
    QH_CHECK(svr.start());
    svr.onPost("/v1/bad", 401, R"({"error":{"message":"bad key"}})");

    qh::schema::CancellationToken cancel;
    auto resp = qh::provider::ProviderHttp::post(
        svr.baseUrl(), "/bad", {}, "{}", cancel);

    QH_CHECK(resp.ok);          // 拿到 HTTP 响应
    QH_CHECK_EQ(resp.status, 401);
    QH_CHECK(resp.body.find("bad key") != std::string::npos);
}

QH_TEST(ProviderHttp_post_cancelled) {
    qh::schema::CancellationToken cancel;
    cancel.cancel();
    auto resp = qh::provider::ProviderHttp::post(
        "http://127.0.0.1:1", "/x", {}, "{}", cancel); // 不会真正发请求
    QH_CHECK(!resp.ok);
    QH_CHECK_EQ(resp.error, std::string("cancelled"));
}

QH_TEST(ProviderHttp_post_network_error) {
    qh::schema::CancellationToken cancel;
    // 端口 1 通常无服务 → 连接失败
    auto resp = qh::provider::ProviderHttp::post(
        "http://127.0.0.1:1", "/x", {}, "{}", cancel);
    QH_CHECK(!resp.ok);
    QH_CHECK(!resp.error.empty());
}
