#include "ConfigManager.h"

#include <fstream>
#include <string>
#include <iostream>

ConfigManager::ConfigManager()
{

}

ConfigManager::~ConfigManager()
{
    
}

ConfigManager* ConfigManager::getInstance()
{
    static ConfigManager instance;
    return &instance;
}

void ConfigManager::parseConfig(const std::string& aConfigFilePath)
{
    std::ifstream file(aConfigFilePath);

    if (!file.is_open())
        return;

    std::string line;
    while (std::getline(file, line))
    {
        auto findIt = line.find("=");
        if (findIt != std::string::npos)
        {
            mData[line.substr(0, findIt)] = line.substr(findIt + 1);
        }
    }
    file.close();
}

std::optional<std::string> ConfigManager::getConfigValue(const std::string& aConfigValueName)
{
    std::optional<std::string> result = std::nullopt;

    auto findIt = mData.find(aConfigValueName);
    if (findIt != mData.end())
    {
        result = findIt->second;
    }

    return result;
}