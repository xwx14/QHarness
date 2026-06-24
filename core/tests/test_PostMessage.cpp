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
