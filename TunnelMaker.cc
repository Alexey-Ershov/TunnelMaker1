#include "TunnelMaker.hpp"
#include "SwitchManager.hpp"
#include <runos/core/logging.hpp>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>

#include <boost/format.hpp>

#include <cstdlib>


namespace runos {

using namespace curlpp::Options;
using json = nlohmann::json;

REGISTER_APPLICATION(TunnelMaker, {"command-line",
                                   "topology",
                                   ""})

void TunnelMaker::init(Loader* loader, const Config& config)
{
    cli_ = CommandLine::get(loader);
    topo_ = Topology::get(loader);
    ip_ = config_get(config_cd(config, "tunnel-maker"), "ip", "127.0.0.1");
    SwitchManager* switch_manager = SwitchManager::get(loader);

    connect(switch_manager, &SwitchManager::linkDown,
            this, &TunnelMaker::onLinkDown);

    cli_->register_command(
        cli_pattern(R"(mktun\s+--help)"),
        [=](cli_match const& match)
        {
            cli_->print("{:-^90}", "  TUNNEL MAKER  ");

            cli_->print("mktun "
                        "NAME_OF_BRIDGE_DOMAIN "
                        "FIRST_DPID PORT_NUMBER VLAN_ID "
                        "SECOND_DPID PORT_NUMBER VLAN_ID "
                        "--hops MAX_NUMBER_OF_HOPS "
                        "--freeroute");
            
            cli_->print("{:-^90}", "");
        }
    );

    cli_->register_command(
        cli_pattern(R"(deltun\s+--help)"),
        [=](cli_match const& match)
        {
            cli_->print("{:-^90}", "  TUNNEL MAKER  ");

            cli_->print("deltun NAME_OF_BRIDGE_DOMAIN");
            
            cli_->print("{:-^90}", "");
        }
    );

    cli_->register_command(
        cli_pattern(R"(mktun\s+([-\w]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)(\s+--hops\s+([0-9]+))?(\s+--freeroute)?)"),

        [=](cli_match const& match)
        {
            cli_->print("{:-^90}", "  TUNNEL MAKER  ");

            if (match[match.size() - 1] != "") {
                tun_attrs_[match[1]].free_route = true;
            }

            match_ = match;

            try {
                create_bd();

                if (was_created_) {
                    fetch_route_id(match[1]);

                    if (match[9] != "") {
                        tun_attrs_[match[1]].num_of_hops = std::stoi(match[9]);
                    
                    } else {
                        tun_attrs_[match[1]].num_of_hops = -1;
                    }

                    if (check_tunnel_requirements(match[1])) {
                        while(add_path(match[1]));
                        check_path_collisions(match[1]);
                    }
                }

            } catch (curlpp::LogicError& e) {
                LOG(ERROR) << e.what();
            
            } catch (curlpp::RuntimeError& e) {
                LOG(ERROR) << e.what();
            }
            
            cli_->print("{:-^90}", "");
        }
    );

    cli_->register_command(
        cli_pattern(R"(deltun\s+([-\w]+))"),
        [=](cli_match const& match)
        {
            cli_->print("{:-^90}", "  TUNNEL MAKER  ");

            try {
                delete_bd(match[1]);

            } catch (curlpp::LogicError& e) {
                LOG(ERROR) << e.what();
            
            } catch (curlpp::RuntimeError& e) {
                LOG(ERROR) << e.what();
            }

            tun_attrs_.erase(match[1]);
            
            cli_->print("{:-^90}", "");
        }
    );
}

void TunnelMaker::onLinkDown()
{
    bool paths_were_changed = false;

    for (auto& it: tun_attrs_) {
        it.second.work_path =
                topo_->getFirstWorkPath(tun_attrs_[it.first].route_id);

        auto first_work_path_id =
                topo_->getFirstWorkPathId(tun_attrs_[it.first].route_id);
        
        if (it.second.first_path_id != first_work_path_id) {
            paths_were_changed = true;
            it.second.first_path_id = first_work_path_id;
        }
    }

    if (not paths_were_changed or tun_attrs_.empty()) {
        return;
    }

    was_collision_ = false;

    auto next_to_last_it = --tun_attrs_.end();

    do {
        was_collision_ = false;
        
        for (auto it_i = tun_attrs_.begin(); it_i != next_to_last_it; ++it_i) {
            for (auto it_j = std::next(it_i);
                    it_j != tun_attrs_.end();
                    ++it_j) {

                if ((*it_i).first == (*it_j).first) {
                    continue;
                }

                if (not (*it_i).second.free_route and
                        not (*it_j).second.free_route) {

                    continue;
                }

                cmp_bd(*it_i, *it_j);
            }
        }
    } while (was_collision_);
}


void TunnelMaker::create_bd()
{
    curlpp::Easy request;

    request.setOpt(
            Url(std::string("http://") +
                ip_ +
                std::string(":8080/bridge_domains/")));

    std::string request_data = (boost::format(
    "{"
        "\"name\": \"%s\","
        "\"sw\": ["
            "{"
                "\"dpid\": \"%s\","
                "\"ports\": ["
                    "{"
                        "\"port_num\": \"%s\","
                        "\"stag\": \"%s\""
                    "}"
                "]"
            "},"
            "{"
                "\"dpid\": \"%s\","
                "\"ports\": ["
                    "{"
                        "\"port_num\": \"%s\","
                        "\"stag\": \"%s\""
                    "}"
                "]"
            "}"
        "],"
        "\"type\": \"P2P\""
    "}") % match_[1]
         % match_[2]
         % match_[3]
         % match_[4]
         % match_[5]
         % match_[6]
         % match_[7]).str();

    request.setOpt(new curlpp::options::PostFields(request_data));
    request.setOpt(new curlpp::options::PostFieldSize(request_data.length()));

    request.setOpt(new curlpp::options::CustomRequest{"PUT"});

    std::ostringstream response;
    response.str(std::string());
    response.clear();

    request.setOpt(new curlpp::options::WriteStream(&response));

    request.perform();

    json json_response = json::parse(response.str());
    
    if (json_response["res"] == "ok") {
        was_created_ = true;
        tun_attrs_[match_[1]].first_path_id = 0;
        tun_attrs_[match_[1]].last_path_id = 0;

    } else {
        was_created_ = false;
    }
}

void TunnelMaker::fetch_route_id(std::string bd_name)
{
    curlpp::Easy request;
    request.setOpt(
            Url(std::string("http://") +
                ip_ +
                std::string(":8080/bridge_domains/")));

    std::ostringstream response;
    response.str(std::string());
    response.clear();

    request.setOpt(new curlpp::options::WriteStream(&response));

    request.perform();

    json json_response = json::parse(response.str());
    
    for (auto& item: json_response["array"]) {
        if (item["name"] == bd_name) {
            tun_attrs_[bd_name].route_id =
                    std::stoi(std::string((item["routesId"])[0]));
            break;
        }
    }
}

void TunnelMaker::delete_bd(std::string bd_name)
{
    curlpp::Easy request;
    
    std::string url = std::string("http://") +
                      ip_ +
                      std::string(":8080/bridge_domains/") +
                      bd_name +
                      std::string("/");
    
    request.setOpt(Url(url));
    request.setOpt(new curlpp::options::CustomRequest{"DELETE"});
    
    std::ostringstream response;
    response.str(std::string());
    response.clear();
    request.setOpt(new curlpp::options::WriteStream(&response));
    
    request.perform();
}

bool TunnelMaker::check_tunnel_requirements(std::string bd_name)
{
    auto path = topo_->getFirstWorkPath(tun_attrs_[bd_name].route_id);
    tun_attrs_[bd_name].work_path = path;
    int num_of_hops = tun_attrs_[bd_name].num_of_hops;
    
    if (num_of_hops != -1 and
            static_cast<int>((path.size() - 2) / 2) > num_of_hops - 2) {
        
        LOG(ERROR) << "Can't create tunnel with such "
                      "requirements";
        delete_bd(bd_name);
        tun_attrs_.erase(bd_name);

        return false;
    }
    
    LOG(INFO) << "Tunnel between switches with dpid "
              << match_[2]
              << " and "
              << match_[5]
              << " was made successfully";

    return true;
}

bool TunnelMaker::add_path(std::string bd_name)
{
    curlpp::Easy request;
    
    std::string url =
            std::string("http://") +
            ip_ +
            std::string(":8080/routes/id/") +
            std::to_string(tun_attrs_[bd_name].route_id) +
            std::string("/add-path/");

    using namespace curlpp::Options;
    request.setOpt(Url(url));

    std::string post_data = "{\"broken_flag\": \"true\"}";

    request.setOpt(new curlpp::options::PostFields(
            post_data));
    request.setOpt(new curlpp::options::PostFieldSize(
            post_data.length()));

    std::ostringstream response;
    response.str(std::string());
    response.clear();
    request.setOpt(new curlpp::options::WriteStream(&response));

    request.perform();

    json json_response = json::parse(response.str());
    if (json_response["act"] == "path created") {
        auto route_id = std::stoi(std::string(json_response["route_id"]));
        auto path_id = std::stoi(std::string(json_response["path_id"]));
        auto path = topo_->getPath(route_id, path_id);
        int num_of_hops = tun_attrs_[bd_name].num_of_hops;
    
        if (num_of_hops != -1 and
                static_cast<int>((path.size() - 2) / 2) > num_of_hops - 2) {
            
            delete_path(std::to_string(route_id),
                        std::to_string(path_id));

            return false;
        }

        tun_attrs_[match_[1]].last_path_id += 1;
    
    } else {
        return false;
    }

    return true;
}

void TunnelMaker::delete_path(std::string route_id, std::string path_id)
{
    curlpp::Easy request;

    std::string url = std::string("http://") +
                      ip_ +
                      std::string(":8080/routes/id/") +
                      route_id +
                      std::string("/delete-path/") +
                      path_id +
                      std::string("/");
    
    request.setOpt(Url(url));
    request.setOpt(new curlpp::options::CustomRequest{"DELETE"});
    
    std::ostringstream response;
    response.str(std::string());
    response.clear();
    request.setOpt(new curlpp::options::WriteStream(&response));
    
    request.perform();
}

bool TunnelMaker::change_path(std::string bd_name)
{
    if (tun_attrs_[bd_name].first_path_id ==
            tun_attrs_[bd_name].last_path_id) {
        
        LOG(ERROR) << "Can't change path";
        return false;
    }

    topo_->setUsedPath(tun_attrs_[bd_name].route_id,
                       tun_attrs_[bd_name].first_path_id + 1);

    delete_path(std::to_string(tun_attrs_[bd_name].route_id),
                std::to_string(tun_attrs_[bd_name].first_path_id));

    tun_attrs_[bd_name].last_path_id -= 1;

    tun_attrs_[bd_name].work_path =
            topo_->getFirstWorkPath(tun_attrs_[bd_name].route_id);

    return true;
}

void TunnelMaker::check_path_collisions(std::string bd_name)
{
    if (!was_created_) {
        return;
    }

    was_collision_ = false;

    do {
        was_collision_ = false;

        for (auto& it: tun_attrs_) {
            if (bd_name == it.first) {
                continue;
            }

            if (not tun_attrs_[bd_name].free_route and
                    not tun_attrs_[it.first].free_route) {

                continue;
            }

            cmp_bd(std::make_pair(bd_name, tun_attrs_[bd_name]), it);
        }
    } while (was_collision_);
}

void TunnelMaker::cmp_bd(
        std::pair<std::string, TunnelAttrs> first_bd,
        std::pair<std::string, TunnelAttrs> second_bd)
{
    auto path1 = first_bd.second.work_path;
    auto path2 = second_bd.second.work_path;

    bool was_break = false;
    for (unsigned int i = 0; i < path1.size() - 1; i++) {
        if (path1[i].dpid == path1[i+1].dpid) {
            continue;
        }

        for (unsigned int j = 0; j < path2.size() - 1; j++) {
            if (path2[j].dpid == path2[j+1].dpid) {
                continue;
            }

            if ((path1[i].dpid == path2[j].dpid and
                    path1[i + 1].dpid == path2[j + 1].dpid) or
                    (path1[i].dpid == path2[j + 1].dpid and
                    path1[i + 1].dpid == path2[j].dpid)) {
                
                was_collision_ = true;

                if (first_bd.second.num_of_hops == -1) {
                    if (!change_path(match_[1])) {
                        if (!change_path(second_bd.first)) {
                            LOG(WARNING) << "Can't resolve "
                                            "collision";

                            was_collision_ = false;
                            return;
                        }
                    }

                } else if (second_bd.second.num_of_hops == -1) {
                    if (!change_path(second_bd.first)) {
                        if (!change_path(match_[1])) {
                            LOG(WARNING) << "Can't resolve "
                                            "collision";

                            was_collision_ = false;
                            return;
                        }
                    }
                
                } else if (first_bd.second.num_of_hops >
                        second_bd.second.num_of_hops) {
                    
                    if (!change_path(match_[1])) {
                        if (!change_path(second_bd.first)) {
                            LOG(WARNING) << "Can't resolve "
                                            "collision";

                            was_collision_ = false;
                            return;
                        }
                    }

                } else {
                    if (!change_path(second_bd.first)) {
                        if (!change_path(match_[1])) {
                            LOG(WARNING) << "Can't resolve "
                                            "collision";

                            was_collision_ = false;
                            return;
                        }
                    }
                }

                was_break = true;
                break;
            }
        }
        
        if (was_break) {
            break;
        }
    }
}

} // namespace runos
