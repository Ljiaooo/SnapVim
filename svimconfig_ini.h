#pragma once
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <windows.h>

namespace SnapVimIni {

inline std::string GetConfigDir()
{
    char* appData = nullptr;
    _dupenv_s(&appData, nullptr, "APPDATA");
    std::string dir = std::string(appData ? appData : ".") + "\\SnapVim";
    free(appData);
    return dir;
}

inline std::string GetConfigPath()
{
    return GetConfigDir() + "\\config.ini";
}

inline void EnsureConfigDir()
{
    std::string dir = GetConfigDir();
    CreateDirectoryA(dir.c_str(), nullptr);
}

inline std::map<std::string, std::string> ParseIni(const char* path)
{
    std::map<std::string, std::string> kv;
    std::ifstream file(path);
    if (!file.is_open()) return kv;

    std::string line;
    while (std::getline(file, line))
    {
        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // Skip comments and section headers
        if (line[0] == '#' || line[0] == ';' || line[0] == '[') continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        // Trim key and value
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        size_t vs = val.find_first_not_of(" \t");
        if (vs != std::string::npos) val = val.substr(vs);
        while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\r' || val.back() == '\n')) val.pop_back();

        kv[key] = val;
    }
    return kv;
}

inline void WriteIni(const char* path, const std::map<std::string, std::string>& kv)
{
    std::ofstream file(path);
    if (!file.is_open()) return;
    for (const auto& [key, val] : kv)
        file << key << " = " << val << "\n";
}

}
