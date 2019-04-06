#include "TunnelMaker.hpp"
#include "CommandLine.hpp"
#include "Topology.hpp"
#include <runos/core/logging.hpp>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>

/*#include <nlohmann/json.hpp>*/

#include <boost/format.hpp>

#include <cstdlib>


namespace runos {

REGISTER_APPLICATION(TunnelMaker, {"command-line", ""})

void TunnelMaker::init(Loader* loader, const Config& config)
{
    CommandLine* cli = CommandLine::get(loader);
    /*Topology* topo = Topology::get(loader);*/

    cli->register_command(
        cli_pattern(R"(mktun\s+([-\w]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+))"),
        [=](cli_match const& match)
        {
            cli->print("{:-^90}", "  TUNNEL MAKER  ");

            try {
                // Create Bridge Domain.
                curlpp::Cleanup cleaner;
                curlpp::Easy put_request;

                using namespace curlpp::Options;
                using json = nlohmann::json;
                
                // put_request.setOpt(Verbose(true));
                put_request.setOpt(
                        Url("http://172.30.7.201:8080/bridge_domains/"));

                std::string put_request_data = (boost::format(
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

                put_request.setOpt(new curlpp::options::PostFields(
                        put_request_data));
                put_request.setOpt(new curlpp::options::PostFieldSize(
                        put_request_data.length()));

                put_request.setOpt(new curlpp::options::CustomRequest{"PUT"});

                std::ostringstream response;
                put_request.setOpt(
                        new curlpp::options::WriteStream(&response));

                put_request.perform();

                json json_response = json::parse(response.str());
                
                if (json_response["res"] == "ok") {
                    cli->print("Tunnel between switches with dpid {} and {} "
                               "was made successfully",
                               match[2], match[5]);
                }

                // Get route id.
                curlpp::Easy get_request;
                get_request.setOpt(
                        Url("http://172.30.7.201:8080/bridge_domains/"));

                response.str(std::string());
                response.clear();

                get_request.setOpt(
                        new curlpp::options::WriteStream(&response));

                get_request.perform();

                /*DLOG(INFO) << "response.str() =\n" << response.str();
                DLOG(INFO) << "JSON dump =\n" << json_response.dump(4);*/

                json_response = json::parse(response.str());
                for (auto& item: json_response["array"]) {
                    if (item["name"] == match[1]) {
                        bd_routes_[item["name"]] =
                                std::stoi(std::string((item["routesId"])[0]));
                        break;
                    }
                }

            } catch (curlpp::LogicError& e) {
                LOG(ERROR) << e.what();
            
            } catch (curlpp::RuntimeError& e) {
                LOG(ERROR) << e.what();
            }

            /*try {
                curlpp::Cleanup cleaner;
                curlpp::Easy put_request;
                
                std::string url =
                        std::string("http://172.30.7.201:8080/routes/id/") +
                        std::string(match[1]) +
                        std::string("/add-path/");

                using namespace curlpp::Options;
                // put_request.setOpt(Verbose(true));
                put_request.setOpt(Url(url));

                std::string post_data = "{\"broken_flag\": \"true\"}";

                DLOG(INFO) << "DEBUG: " << post_data.length();
                
                put_request.setOpt(new curlpp::options::PostFields(
                        post_data));
                put_request.setOpt(new curlpp::options::PostFieldSize(
                        post_data.length()));

                put_request.perform();

            } catch (curlpp::LogicError& e) {
                LOG(ERROR) << e.what();
            
            } catch (curlpp::RuntimeError& e) {
                LOG(ERROR) << e.what();
            }*/

            cli->print("{:-^90}", "");
        }
    );

}

} // namespace runos
