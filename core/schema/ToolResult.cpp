#include "ToolResult.h"

namespace qh {
namespace schema {

void to_json(nlohmann::json& j, const ToolResult& tr) {
    j = nlohmann::json{
        {"tool_call_id", tr.toolCallId},
        {"output", tr.output},
        {"is_error", tr.isError}};
}

void from_json(const nlohmann::json& j, ToolResult& tr) {
    j.at("tool_call_id").get_to(tr.toolCallId);
    j.at("output").get_to(tr.output);
    j.at("is_error").get_to(tr.isError);
}

} // namespace schema
} // namespace qh
