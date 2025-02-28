#include "ConfigMgr.h"

ConfigMgr::ConfigMgr() {
    boost::filesystem::path current_path = boost::filesystem::current_path(); //当前文件的路径
    boost::filesystem::path config_path = current_path / "config.ini";
    std::cout << "Config path: " << config_path << std::endl;

    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(config_path.string(), pt);

    for (const auto& section_pair : pt) {       //section_pair就是[GateServer]和它下面的port或ip信息
        const ::std::string& section_name = section_pair.first;
        const boost::property_tree::ptree& section_tree = section_pair.second;

        std::map<std::string, std::string> section_config;
        for (const auto& key_value_pair : section_tree) {   //config.ini每个[]下可能包含ip和address等更多信息
            const std::string& key = key_value_pair.first;
            const std::string& value = key_value_pair.second.get_value<std::string>(); //get_value可以把tree的value转换成string类型
            section_config[key] = value;
        }

        SectionInfo sectionInfo;
        sectionInfo.m_sectionData = section_config;
        m_configMap[section_name] = sectionInfo;
    }

    //输出所有的section和key-value
    for (const auto& section_entry : m_configMap) {
        const std::string& section_name = section_entry.first;
        SectionInfo section_config = section_entry.second;
        std::cout << "[" << section_name << "]" << std::endl;
        for (const auto& key_value_pair : section_config.m_sectionData) {
            std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl;
        }
    }

}