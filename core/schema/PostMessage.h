#ifndef QH_SCHEMA_POSTMESSAGE_H
#define QH_SCHEMA_POSTMESSAGE_H
#include "qh_export.h"
#include <string>

namespace qh {
namespace schema {

// 消息级别（日志/通知的严重性）
enum class Level { Info, Warn, Error };

QH_API std::string levelToString(Level level);

// 实时消息发送抽象：core 类持有其指针，运行时向 Qt 窗口或其他接口推送日志/通知
// 纯 C++17 抽象，不依赖 Qt；app 提供 QPostMessage（QObject）实现
class QH_API PostMessage {
public:
    virtual ~PostMessage() = default;
    virtual void post(Level level, const std::string& message) = 0;
};

// 持有 PostMessage 的 mixin：core 类继承它获得「注入实时消息发送器」能力（DRY）
class QH_API PostMessageInterface {
public:
    virtual ~PostMessageInterface() = default;
    void setPostMessage(PostMessage* pm) { _postMessage = pm; }
protected:
    PostMessage* _postMessage = nullptr;  // 非拥有，由 app 注入
};

} // namespace schema
} // namespace qh
#endif // QH_SCHEMA_POSTMESSAGE_H
