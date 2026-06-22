#include "Role.h"

namespace qh {
namespace schema {

std::string roleToString(Role role) {
    switch (role) {
        case Role::System:    return "system";
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
    }
    return "user"; // 兜底
}

Role roleFromString(const std::string& str) {
    if (str == "system")    return Role::System;
    if (str == "assistant") return Role::Assistant;
    return Role::User;
}

} // namespace schema
} // namespace qh
