#include "TunnelMaker.hpp"
#include "CommandLine.hpp"
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
                curlpp::Cleanup cleaner;
                curlpp::Easy request;

                using namespace curlpp::Options;
                
                // request.setOpt(Verbose(true));
                request.setOpt(
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

                request.setOpt(new curlpp::options::PostFields(
                        put_request_data));
                request.setOpt(new curlpp::options::PostFieldSize(
                        put_request_data.length()));

                request.setOpt(new curlpp::options::CustomRequest{"PUT"});

                request.perform();

            } catch (curlpp::LogicError& e) {
                LOG(ERROR) << e.what();
            
            } catch (curlpp::RuntimeError& e) {
                LOG(ERROR) << e.what();
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

            /*cli->print("Tunnel between switches with dpid {} and {} was made",
                       match[1], match[2]);*/
            cli->print("{:-^90}", "");
        }
    );

}

} // namespace runos
