#include "TunnelMaker.hpp"
#include "Topology.hpp"
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
    Topology* topo = Topology::get(loader);

    cli_->register_command(
        cli_pattern(R"(mktun\s+([-\w]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+--hops\s+([0-9]+))"),
        [=](cli_match const& match)
        {
            cli_->print("{:-^90}", "  TUNNEL MAKER  ");

            try {
                create_bd(match);

                fetch_route_id();

                /*DLOG(INFO) << "AFTER ADDING NEW BD";
                for (auto& it: bd_routes_) {
                    DLOG(INFO) << it.first << ": " << it.second;
                }*/

            } catch (curlpp::LogicError& e) {
                LOG(ERROR) << e.what();
            
            } catch (curlpp::RuntimeError& e) {
                LOG(ERROR) << e.what();
            }

            auto path = topo->getFirstWorkPath(bd_routes_[match[1]]);
            /*DLOG(INFO) << "DEBUG: path.size = " << path.size();*/
            if (static_cast<int>((path.size() - 2) / 2) >
                    std::stoi(match[8]) - 2) {
                
                LOG(ERROR) << "Can't create tunnel with such "
                              "requirements";
                delete_bd(match[1]);
            
            } else if (was_created_) {
                LOG(INFO) << "Tunnel between switches with dpid "
                          << match[2]
                          << " and "
                          << match[5]
                          << " was made successfully";
            }

            
            /*try {
                curlpp::Cleanup cleaner;
                curlpp::Easy request;
                
                std::string url =
                        std::string("http://172.30.7.201:8080/routes/id/") +
                        std::string(match[1]) +
                        std::string("/add-path/");

                using namespace curlpp::Options;
                // request.setOpt(Verbose(true));
                request.setOpt(Url(url));

                std::string post_data = "{\"broken_flag\": \"true\"}";

                DLOG(INFO) << "DEBUG: " << post_data.length();
                
                request.setOpt(new curlpp::options::PostFields(
                        post_data));
                request.setOpt(new curlpp::options::PostFieldSize(
                        post_data.length()));

                request.perform();

            } catch (curlpp::LogicError& e) {
                LOG(ERROR) << e.what();
            
            } catch (curlpp::RuntimeError& e) {
                LOG(ERROR) << e.what();
            }*/

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


void TunnelMaker::create_bd(cli_match const& match)
{
    curlpp::Easy request;

    request.setOpt(
            Url("http://172.30.7.201:8080/bridge_domains/"));

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
    "}") % match[1]
         % match[2]
         % match[3]
         % match[4]
         % match[5]
         % match[6]
         % match[7]).str();

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
            Url("http://172.30.7.201:8080/bridge_domains/"));

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
    
    std::string url = std::string("http://172.30.7.201:8080/bridge_domains/") +
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

} // namespace runos
