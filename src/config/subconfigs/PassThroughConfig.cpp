#include "PassThroughConfig.h"

#include "config/JsonUtils.h"

std::shared_ptr<PassThroughConfig> PassThroughConfig::createFromJson(
    const nlohmann::json& j, ptr<FileManager> /*fileManager*/)
{

    if (j.contains("isEnabled")) {
        CHECK_STATE_JSON(j["isEnabled"].is_boolean(), "isEnabled must be boolean", j);
        if (!j["isEnabled"].get<bool>()) {
            return nullptr;
        }
    }


    CHECK_STATE_JSON(j.contains("target_url"), "Passthrough config must contain target_url", j);


    CHECK_STATE_JSON(j["targetUrl"].is_string(), "targetUrl must be string", j);

    auto targetUrl = j["targetUrl"].get<std::string>();
    return std::shared_ptr<PassThroughConfig>(new PassThroughConfig(targetUrl));
}
