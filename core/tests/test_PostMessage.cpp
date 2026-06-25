#include "TestHarness.h"
#include "schema/PostMessage.h"
#include <string>
#include <vector>

namespace {

class FakePostMessage : public qh::schema::PostMessage {
public:
    void post(qh::schema::Level level, const std::string& msg) override {
        _levels.push_back(level);
        _messages.push_back(msg);
    }
    std::vector<qh::schema::Level> _levels;
    std::vector<std::string> _messages;
};

} // namespace

QH_TEST(postmessage_level_to_string) {
    QH_CHECK_EQ(qh::schema::levelToString(qh::schema::Level::Info), std::string("[INFO]"));
    QH_CHECK_EQ(qh::schema::levelToString(qh::schema::Level::Warn), std::string("[WARN]"));
    QH_CHECK_EQ(qh::schema::levelToString(qh::schema::Level::Error), std::string("[ERROR]"));
    QH_CHECK_EQ(qh::schema::levelToString(qh::schema::Level::Chat), std::string("[CHAT]"));
    QH_CHECK_EQ(qh::schema::levelToString(qh::schema::Level::Think), std::string("[THINK]"));
}

QH_TEST(postmessage_post_collects_messages) {
    FakePostMessage pm;
    pm.post(qh::schema::Level::Info, "hello");
    pm.post(qh::schema::Level::Error, "boom");
    QH_CHECK_EQ(pm._messages.size(), static_cast<size_t>(2));
    QH_CHECK_EQ(pm._messages[0], std::string("hello"));
    QH_CHECK(pm._levels[1] == qh::schema::Level::Error);
}

// 验证 PostMessageInterface 注入：继承它的类获得 setPostMessage + _postMessage
namespace {
class Holder : public qh::schema::PostMessageInterface {
public:
    qh::schema::PostMessage* posted() { return _postMessage; }
};
} // namespace

QH_TEST(postmessage_interface_injection) {
    Holder h;
    QH_CHECK(h.posted() == nullptr);  // 默认空
    FakePostMessage pm;
    h.setPostMessage(&pm);
    QH_CHECK(h.posted() == &pm);  // 注入后可取回
}

// 验证 PostMessageInterface::post 转发到注入的 PostMessage
QH_TEST(postmessage_interface_forwards_post) {
    Holder h;
    FakePostMessage pm;
    h.setPostMessage(&pm);
    h.post(qh::schema::Level::Warn, "direct");
    QH_CHECK_EQ(pm._messages.size(), static_cast<size_t>(1));
    QH_CHECK_EQ(pm._messages[0], std::string("direct"));
    QH_CHECK(pm._levels[0] == qh::schema::Level::Warn);
}

// 验证 info/warn/error 便利方法映射到对应 Level
QH_TEST(postmessage_interface_info_warn_error_levels) {
    Holder h;
    FakePostMessage pm;
    h.setPostMessage(&pm);
    h.info("i");
    h.warn("w");
    h.error("e");
    QH_CHECK_EQ(pm._messages.size(), static_cast<size_t>(3));
    QH_CHECK(pm._levels[0] == qh::schema::Level::Info);
    QH_CHECK(pm._levels[1] == qh::schema::Level::Warn);
    QH_CHECK(pm._levels[2] == qh::schema::Level::Error);
    QH_CHECK_EQ(pm._messages[2], std::string("e"));
}

// 验证 chat/think 便利方法映射到 Level::Chat/Level::Think
QH_TEST(postmessage_interface_chat_think_levels) {
    Holder h;
    FakePostMessage pm;
    h.setPostMessage(&pm);
    h.chat("c");
    h.think("t");
    QH_CHECK_EQ(pm._messages.size(), static_cast<size_t>(2));
    QH_CHECK(pm._levels[0] == qh::schema::Level::Chat);
    QH_CHECK(pm._levels[1] == qh::schema::Level::Think);
    QH_CHECK_EQ(pm._messages[0], std::string("c"));
}

// 验证 _postMessage 为空时调用不崩溃（null 安全）
QH_TEST(postmessage_interface_null_safe) {
    Holder h;  // 不注入，_postMessage 为 nullptr
    h.info("x");
    h.warn("y");
    h.error("z");
    h.post(qh::schema::Level::Info, "p");
    QH_CHECK(h.posted() == nullptr);  // 仍未注入
}
