#pragma once
#include <map>
#include <optional>

class ConfigManager
{
private:
    std::map<std::string, std::string> mData;

private:
    ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

public:
    ~ConfigManager();

    static ConfigManager* getInstance();

    void parseConfig(const std::string& aConfigFilePath);

    std::optional<std::string> getConfigValue(const std::string& aConfigValueName);
};