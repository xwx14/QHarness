#ifndef QH_PROVIDER_HTTP_H
#define QH_PROVIDER_HTTP_H
#include <string>
#include <vector>
#include <utility>
#include "schema/CancellationToken.h"
#include "qh_export.h"

namespace qh {
namespace provider {

struct QH_API HttpResponse {
    bool ok = false;          // 是否拿到 HTTP 响应（false=网络错误/取消/超时）
    int status = 0;           // HTTP 状态码（ok=true 时有效）
    std::string body;         // 响应体
    std::string error;        // ok=false 时的错误描述
};

class QH_API ProviderHttp {
public:
    static HttpResponse post(
        const std::string& baseUrl,
        const std::string& path,
        const std::vector<std::pair<std::string, std::string>>& headers,
        const std::string& body,
        const schema::CancellationToken& cancel);
};

} // namespace provider
} // namespace qh
#endif // QH_PROVIDER_HTTP_H
