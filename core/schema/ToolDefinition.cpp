#include "ToolDefinition.h"

namespace qh {
namespace schema {

void to_json(nlohmann::json& j, const ToolDefinition& td) {
    j = nlohmann::json{
        {"name", td.name},
        {"description", td.description},
        {"input_schema", td.inputSchema}};
}

void from_json(const nlohmann::json& j, ToolDefinition& td) {
    j.at("name").get_to(td.name);
    j.at("description").get_to(td.description);
    j.at("input_schema").get_to(td.inputSchema);
}

} // namespace schema
} // namespace qh
