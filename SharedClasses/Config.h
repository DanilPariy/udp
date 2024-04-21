#pragma once
#include <map>
#include <optional>

class Config
{
private:
    std::map<std::string, std::string> mData;

private:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

public:
    Config();
    ~Config();

    void parseConfig(const std::string& aConfigFilePath);

    std::optional<std::string> getConfigValue(const std::string& aConfigValueName);
};