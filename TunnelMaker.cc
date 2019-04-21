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

REGISTER_APPLICATION(TunnelMaker, {"command-line", ""})

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
                    for (auto& it: bd_routes_) {
                        DLOG(INFO) << it.first << ": " << it.second;
                    }*/

                    check_tunnel_requirements();

                    while(add_path(match[1]));
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
    for (auto& it: bd_routes_) {
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
            bd_routes_[item["name"]] =
                    std::stoi(std::string((item["routesId"])[0]));
        
        } else {
            if (item["name"] == name) {
                bd_routes_[name] =
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
    auto path = topo_->getFirstWorkPath(bd_routes_[match_[1]]);
    /*DLOG(INFO) << "DEBUG: path.size = " << path.size();*/
    if (static_cast<int>((path.size() - 2) / 2) >
            std::stoi(match_[8]) - 2) {
        
        LOG(ERROR) << "Can't create tunnel with such "
                      "requirements";
        delete_bd(match_[1]);

    } else if (was_created_) {
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
            std::to_string(bd_routes_[name]) +
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

} // namespace runos
