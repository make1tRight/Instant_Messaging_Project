#ifndef CONFIGMGR_H
#define CONFIGMGR_H
#include <unordered_map>
#include <string>

struct SectionInfo {
public:
    SectionInfo();

    ~SectionInfo();

    SectionInfo(const SectionInfo& rhs);
    
    SectionInfo& operator=(const SectionInfo& rhs);

    std::string operator[](const std::string& key) const;
public:
    std::unordered_map<std::string, std::string> _section_data;
};


class ConfigMgr {
public:
    ConfigMgr();

    ~ConfigMgr();

    ConfigMgr(const ConfigMgr& rhs);
    
    ConfigMgr& operator=(const ConfigMgr& rhs);

    SectionInfo operator[](const std::string& key);
private:
    std::unordered_map<std::string, SectionInfo> _config_map;
};
#endif // CONFIGMGR_H