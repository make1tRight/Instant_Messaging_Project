#pragma once
#include "const.h"

struct SectionInfo {
public:
    SectionInfo() {}
    ~SectionInfo() { m_sectionData.clear(); }
    SectionInfo(const SectionInfo& src) {
        m_sectionData = src.m_sectionData;
    }
    SectionInfo& operator=(const SectionInfo& src) {
        if (&src == this) { //防备自己拷贝自己
            return *this;
        }
        this->m_sectionData = src.m_sectionData;
        return *this;
    }

    std::string operator[](const std::string& key) {
        if (m_sectionData.find(key) == m_sectionData.end()) {
            return "";
        }
        return m_sectionData[key];
    }

    std::string GetValue(const std::string& key) {
        if (m_sectionData.find(key) == m_sectionData.end()) {
            return "";
        }
        return m_sectionData[key];
    }
public:
    std::map<std::string, std::string> m_sectionData;
};

class ConfigMgr
{
public:
    ~ConfigMgr() {
        m_configMap.clear();
    }
    SectionInfo operator[](const std::string& section) {
        if (m_configMap.find(section) == m_configMap.end()) {
            return SectionInfo();
        }
        return m_configMap[section];
    }

    static ConfigMgr& Inst() {
        static ConfigMgr cfg_mgr;   //(局部静态变量)多次访问只进行一次初始化, 生命周期和进程的生命周期是同步的
        return cfg_mgr;
    }

    ConfigMgr(const ConfigMgr& src) {
        this->m_configMap = src.m_configMap;
    }

    ConfigMgr& operator=(const ConfigMgr& src) {
        if (&src == this) {
            return *this;
        }
        m_configMap = src.m_configMap;
        //return *this;
    }
    std::string GetValue(const std::string& section, const std::string& key);
private:
    ConfigMgr();    //单例的好处: 到哪都能用, 不用避免多次拷贝
    std::map<std::string, SectionInfo> m_configMap;
};