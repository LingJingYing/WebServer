#include "config.h"
#include "log.h"
#include <yaml-cpp/yaml.h>

ljy::ConfigVar<int>::ptr g_int_value_config =
    ljy::Config::Lookup("system.port", (int)8080, "system port");

ljy::ConfigVar<float>::ptr g_int_valuex_config =
    ljy::Config::Lookup("system.port", (float)8080, "system port");

ljy::ConfigVar<float>::ptr g_float_value_config =
    ljy::Config::Lookup("system.value", (float)10.2f, "system value");

ljy::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config =
    ljy::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int vec");

ljy::ConfigVar<std::list<int> >::ptr g_int_list_value_config =
    ljy::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int list");

ljy::ConfigVar<std::set<int> >::ptr g_int_set_value_config =
    ljy::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int set");

ljy::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config =
    ljy::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int uset");

ljy::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config =
    ljy::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k",2}}, "system str int map");

ljy::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config =
    ljy::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k",2}}, "system str int map");


void print_yaml(const YAML::Node& node, int level) {
    if(node.IsScalar()) {
        LJY_LOG_DEBUG(LJY_LOG_ROOT()) << "IsScalar:";
        LJY_LOG_INFO(LJY_LOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) {
        LJY_LOG_DEBUG(LJY_LOG_ROOT()) << "IsNull:";
        LJY_LOG_INFO(LJY_LOG_ROOT()) << std::string(level * 4, ' ')
            << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            LJY_LOG_DEBUG(LJY_LOG_ROOT()) << "IsMap:";
            LJY_LOG_INFO(LJY_LOG_ROOT()) << std::string(level * 4, ' ')
                    << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            LJY_LOG_DEBUG(LJY_LOG_ROOT()) << "IsSequence:";
            LJY_LOG_INFO(LJY_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    YAML::Node node = YAML::LoadFile("/home/wonderful/myserver/WebServer/config/log.yml");
    LJY_LOG_DEBUG(LJY_LOG_ROOT()) << "start";
    print_yaml(node, 0);
    LJY_LOG_DEBUG(LJY_LOG_ROOT()) << node["test"].IsDefined();
    LJY_LOG_DEBUG(LJY_LOG_ROOT()) << node["logs"].IsDefined();
    LJY_LOG_DEBUG(LJY_LOG_ROOT()) << node;
    //LJY_LOG_DEBUG(LJY_LOG_ROOT()) << node.as<std::string>();
}

void test_log() {
    static ljy::Logger::ptr system_log = LJY_LOG_NAME("system");
    LJY_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << ljy::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/wonderful/myserver/WebServer/config/log.yml");
    ljy::Config::LoadFromYaml(root);
    std::cout << "=============" << std::endl;
    std::cout << ljy::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    LJY_LOG_INFO(system_log) << "hello system" << std::endl;

    system_log->setFormatter("%d - %m%n");

    LJY_LOG_INFO(system_log) << "hello system" << std::endl;
}

int main()
{
    //test_yaml();
    test_log();
    return 0;
}