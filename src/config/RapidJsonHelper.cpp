#include "config/RapidJsonHelper.h"

namespace config {

std::string RapidJsonHelper::GetString(const rapidjson::Value& value, const char* key, const std::string& defaultValue) {
    if (!value.HasMember(key)) {
        return defaultValue;
    }

    const rapidjson::Value& member = value[key];
    if (!member.IsString()) {
        return defaultValue;
    }

    return member.GetString();
}

int64_t RapidJsonHelper::GetInt64(const rapidjson::Value& value, const char* key, int64_t defaultValue) {
    if (!value.HasMember(key)) {
        return defaultValue;
    }

    const rapidjson::Value& member = value[key];
    if (member.IsInt64()) {
        return member.GetInt64();
    } else if (member.IsInt()) {
        return member.GetInt();
    } else if (member.IsUint64()) {
        return static_cast<int64_t>(member.GetUint64());
    } else if (member.IsDouble()) {
        return static_cast<int64_t>(member.GetDouble());
    }

    return defaultValue;
}

int RapidJsonHelper::GetInt(const rapidjson::Value& value, const char* key, int defaultValue) {
    if (!value.HasMember(key)) {
        return defaultValue;
    }

    const rapidjson::Value& member = value[key];
    if (member.IsInt()) {
        return member.GetInt();
    } else if (member.IsUint()) {
        return static_cast<int>(member.GetUint());
    } else if (member.IsDouble()) {
        return static_cast<int>(member.GetDouble());
    }

    return defaultValue;
}

uint64_t RapidJsonHelper::GetUint64(const rapidjson::Value& value, const char* key, uint64_t defaultValue) {
    if (!value.HasMember(key)) {
        return defaultValue;
    }

    const rapidjson::Value& member = value[key];
    if (member.IsUint64()) {
        return member.GetUint64();
    } else if (member.IsUint()) {
        return member.GetUint();
    } else if (member.IsInt64()) {
        return static_cast<uint64_t>(member.GetInt64());
    } else if (member.IsDouble()) {
        return static_cast<uint64_t>(member.GetDouble());
    }

    return defaultValue;
}

double RapidJsonHelper::GetDouble(const rapidjson::Value& value, const char* key, double defaultValue) {
    if (!value.HasMember(key)) {
        return defaultValue;
    }

    const rapidjson::Value& member = value[key];
    if (member.IsDouble()) {
        return member.GetDouble();
    } else if (member.IsInt()) {
        return static_cast<double>(member.GetInt());
    } else if (member.IsUint()) {
        return static_cast<double>(member.GetUint());
    }

    return defaultValue;
}

bool RapidJsonHelper::GetBool(const rapidjson::Value& value, const char* key, bool defaultValue) {
    if (!value.HasMember(key)) {
        return defaultValue;
    }

    const rapidjson::Value& member = value[key];
    if (!member.IsBool()) {
        return defaultValue;
    }

    return member.GetBool();
}

bool RapidJsonHelper::HasMember(const rapidjson::Value& value, const char* key) {
    return value.HasMember(key);
}

const rapidjson::Value* RapidJsonHelper::GetObject(const rapidjson::Value& value, const char* key) {
    if (!value.HasMember(key)) {
        return nullptr;
    }

    const rapidjson::Value& member = value[key];
    if (!member.IsObject()) {
        return nullptr;
    }

    return &member;
}

} // namespace config
