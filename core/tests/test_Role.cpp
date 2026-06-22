#include "TestHarness.h"
#include "schema/Message.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

QH_TEST(role_string_roundtrip) {
    QH_CHECK_EQ(qh::schema::roleToString(qh::schema::Role::System), std::string("system"));
    QH_CHECK_EQ(qh::schema::roleToString(qh::schema::Role::User), std::string("user"));
    QH_CHECK_EQ(qh::schema::roleToString(qh::schema::Role::Assistant), std::string("assistant"));
    QH_CHECK(qh::schema::roleFromString("assistant") == qh::schema::Role::Assistant);
}

QH_TEST(role_json_serialization) {
    json j = qh::schema::Role::System;          // to_json
    QH_CHECK_EQ(j.get<std::string>(), std::string("system"));

    auto role = j.get<qh::schema::Role>();      // from_json
    QH_CHECK(role == qh::schema::Role::System);
}
