#include "ConfigMgr.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

SectionInfo::SectionInfo() {

}

SectionInfo::~SectionInfo() {
    _section_data.clear();
}

SectionInfo::SectionInfo(const SectionInfo& rhs) {
    _section_data = rhs._section_data;
}
    
SectionInfo& SectionInfo::operator=(const SectionInfo& rhs) {
    if (&rhs == this) {
        return *this;
    }
    _section_data = rhs._section_data;
    return *this;
}

std::string SectionInfo::operator[](const std::string& key) const {
    auto it = _section_data.find(key);
    if (it == _section_data.end()) {
        return "";
    }
    return it->second;
}

ConfigMgr::ConfigMgr() {
    boost::filesystem::path project_path =
         boost::filesystem::current_path().parent_path();
    boost::filesystem::path config_path = project_path / "config.ini";
    std::cout << "config path: " << config_path << std::endl;

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(config_path.string(), pt);

    for (const auto& section_pair : pt) {
        const std::string& section_name = section_pair.first;
        const boost::property_tree::ptree& section_tree = section_pair.second;

        std::unordered_map<std::string, std::string> section_config;
        for (const auto& key_value_pair : section_tree) {
            const std::string& key = key_value_pair.first;
            const std::string& value = 
                key_value_pair.second.get_value<std::string>();
            section_config[key] = value;
        }
        SectionInfo sectioninfo;
        sectioninfo._section_data = section_config;
        _config_map[section_name] = sectioninfo;
    }

    for (const auto& section_entry : _config_map) {
        const std::string& section_name = section_entry.first;
        SectionInfo section_config = section_entry.second;
        std::cout << "[" << section_name << "]" << std::endl;
        for (const auto& key_value_pair : section_config._section_data) {
            std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl;
        }
    }
}

ConfigMgr::~ConfigMgr() {
    _config_map.clear();
}

ConfigMgr::ConfigMgr(const ConfigMgr& rhs) {
    _config_map = rhs._config_map;
}
    
ConfigMgr& ConfigMgr::operator=(const ConfigMgr& rhs) {
    if (&rhs == this) {
        return *this;
    }
    _config_map = rhs._config_map;
    return *this;
}

SectionInfo ConfigMgr::operator[](const std::string& key) {
    auto it = _config_map.find(key);
    if (it == _config_map.end()) {
        return SectionInfo();
    }

    return it->second;
}