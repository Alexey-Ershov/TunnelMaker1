#include "TunnelMaker.hpp"
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
        cli_pattern(R"(mktun\s+([-\w]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+--hops\s+([0-9]+))"),
        [=](cli_match const& match)
        {
            cli_->print("{:-^90}", "  TUNNEL MAKER  ");

            match_ = match;

            try {
                create_bd();

                if (was_created_) {
                    fetch_route_id();

                    /*DLOG(INFO) << "AFTER ADDING NEW BD";
                    for (auto& it: tun_attrs_) {
                        DLOG(INFO) << it.first << ": " << it.second;
                    }*/

                    tun_attrs_[match[1]].num_of_hops = std::stoi(match[8]);

                    check_tunnel_requirements();

                    while(add_path(match[1]));

                    check_path_collisions();

                    /*change_path(match[1]);*/

                    DLOG(INFO) << "TUN ATTR";
                    for (auto& it: tun_attrs_) {
                        DLOG(INFO) << "name = " << it.first;
                        DLOG(INFO) << "num_of_hops = "
                                   << it.second.num_of_hops;
                        
                        DLOG(INFO) << "WORK PATH";
                        for (auto& path: it.second.work_path) {
                            DLOG(INFO) << path.dpid << ":" << path.port;
                        }

                        DLOG(INFO) << "first_path_id = "
                                   << it.second.first_path_id;

                        DLOG(INFO) << "last_path_id = "
                                   << it.second.last_path_id;
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

void TunnelMaker::startUp(Loader* loader)
{
    /*DLOG(INFO) << "DEBUG1";
    fetch_route_id();
    DLOG(INFO) << "DEBUG2";

    DLOG(INFO) << "REGULAR FETCH";
    for (auto& it: tun_attrs_) {
        DLOG(INFO) << it.first << ": " << it.second;
    }*/
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

void TunnelMaker::fetch_route_id(std::string name)
{
    /*DLOG(INFO) << "DEBUG3: name = " << name;*/

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
        if (name == "") {
            tun_attrs_[item["name"]].route_id =
                    std::stoi(std::string((item["routesId"])[0]));
        
        } else {
            if (item["name"] == name) {
                tun_attrs_[name].route_id =
                        std::stoi(std::string((item["routesId"])[0]));
                break;
            }
        }
    }
}

void TunnelMaker::delete_bd(std::string name)
{
    curlpp::Easy request;
    
    std::string url = std::string("http://") +
                      ip_ +
                      std::string(":8080/bridge_domains/") +
                      name +
                      std::string("/");
    
    request.setOpt(Url(url));
    request.setOpt(new curlpp::options::CustomRequest{"DELETE"});
    
    std::ostringstream response;
    response.str(std::string());
    response.clear();
    request.setOpt(new curlpp::options::WriteStream(&response));
    
    request.perform();
}

void TunnelMaker::check_tunnel_requirements()
{
    auto path = topo_->getFirstWorkPath(tun_attrs_[match_[1]].route_id);
    tun_attrs_[match_[1]].work_path = path;
    /*DLOG(INFO) << "DEBUG: path.size = " << path.size();*/
    if (static_cast<int>((path.size() - 2) / 2) >
            std::stoi(match_[8]) - 2) {
        
        LOG(ERROR) << "Can't create tunnel with such "
                      "requirements";
        delete_bd(match_[1]);

    } else {
        LOG(INFO) << "Tunnel between switches with dpid "
                  << match_[2]
                  << " and "
                  << match_[5]
                  << " was made successfully";
    }
}

bool TunnelMaker::add_path(std::string name)
{
    curlpp::Easy request;
    
    std::string url =
            std::string("http://") +
            ip_ +
            std::string(":8080/routes/id/") +
            std::to_string(tun_attrs_[name].route_id) +
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

    // DLOG(INFO) << "### RESPONSE ###\n" << response.str();

    json json_response = json::parse(response.str());
    if (json_response["act"] == "path created") {
        auto route_id = std::stoi(std::string(json_response["route_id"]));
        auto path_id = std::stoi(std::string(json_response["path_id"]));

        auto path = topo_->getPath(route_id, path_id);
        /*DLOG(INFO) << "### PATH ###";
        for (auto& it: path) {
            DLOG(INFO) << it.dpid << ":" << it.port;
        }*/

        if (static_cast<int>((path.size() - 2) / 2) >
                std::stoi(match_[8]) - 2) {
            
            // LOG(ERROR) << "Can't add path";
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

void TunnelMaker::change_path(std::string bd_name)
{
    if (tun_attrs_[bd_name].first_path_id ==
            tun_attrs_[bd_name].last_path_id) {
        
        LOG(ERROR) << "Can't change path";
        return;
    }

    topo_->setUsedPath(tun_attrs_[bd_name].route_id,
                       tun_attrs_[bd_name].first_path_id + 1);

    delete_path(std::to_string(tun_attrs_[bd_name].route_id),
                std::to_string(tun_attrs_[bd_name].first_path_id));

    tun_attrs_[bd_name].last_path_id -= 1;

    tun_attrs_[bd_name].work_path =
            topo_->getFirstWorkPath(tun_attrs_[bd_name].route_id);
}

void TunnelMaker::check_path_collisions()
{
    if (!was_created_) {
        return;
    }

    bool was_collision = false;

    do {
        was_collision = false;

        for (auto& it: tun_attrs_) {
            if (match_[1] != it.first) {
                auto current_tun_attrs = tun_attrs_[match_[1]];
                auto path1 = current_tun_attrs.work_path;
                auto path2 = it.second.work_path;

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
                            
                            LOG(ERROR) << "COLLISION!";

                            was_collision = true;

                            if (current_tun_attrs.num_of_hops >
                                    it.second.num_of_hops) {
                                
                                change_path(match_[1]);

                            } else {
                                change_path(it.first);
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
        }
    } while (was_collision);
}

} // namespace runos
