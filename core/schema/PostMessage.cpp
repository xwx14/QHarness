#include "schema/PostMessage.h"

namespace qh {
namespace schema {

std::string levelToString(Level level) {
    switch (level) {
        case Level::Info:  return "[INFO]";
        case Level::Warn:  return "[WARN]";
        case Level::Error: return "[ERROR]";
        case Level::Chat:  return "[CHAT]";
        case Level::Think: return "[THINK]";
    }
    return "[INFO]";
}

} // namespace schema
} // namespace qh
