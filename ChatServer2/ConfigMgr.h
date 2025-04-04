#ifndef CONFIGMGR_H
#define CONFIGMGR_H
#include "Singleton.h"
#include <string>
#include <map>

struct SectionInfo {
public:
    SectionInfo();
    ~SectionInfo();
    SectionInfo(const SectionInfo& rhs); 
    SectionInfo& operator=(const SectionInfo& rhs);
    std::string operator[](const std::string& key) const;
    std::string GetValue(const std::string& key);
public:
    std::map<std::string, std::string> _section_data;
};


class ConfigMgr : public Singleton<ConfigMgr> {
    friend class Singleton<ConfigMgr>;
public:
    ~ConfigMgr();
    SectionInfo operator[](const std::string& key);
    std::string GetValue(const std::string& section, const std::string& key);
private:
    ConfigMgr();
    ConfigMgr(const ConfigMgr&) = delete;
    ConfigMgr& operator=(const ConfigMgr&) = delete;
    std::map<std::string, SectionInfo> _config_map;
};
#endif // CONFIGMGR_H