#include "TunnelMaker.hpp"
#include "CommandLine.hpp"
#include "Topology.hpp"
#include <runos/core/logging.hpp>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>

#include <cstdlib>


namespace runos {

REGISTER_APPLICATION(TunnelMaker, {"command-line", ""})

void TunnelMaker::init(Loader* loader, const Config& config)
{
    CommandLine* cli = CommandLine::get(loader);
    /*Topology* topo = Topology::get(loader);*/

    cli->register_command(
        cli_pattern(R"(mktun\s+([0-9]+)\s+([0-9]+))"),
        [=](cli_match const& match)
        {
            cli->print("{:-^90}", "  TUNNEL MAKER  ");

            try {
                curlpp::Cleanup cleaner;
                curlpp::Easy request;
                
                std::string url =
                        std::string("http://172.30.7.201:8080/routes/id/") +
                        std::string(match[1]) +
                        std::string("/delete-path/1/");

                using namespace curlpp::Options;
                /*request.setOpt(Verbose(true));*/
                request.setOpt(Url(url));

                /*std::string post_data = "{\"broken_flag\": \"true\"}";

                DLOG(INFO) << "DEBUG: " << post_data.length();
                
                request.setOpt(new curlpp::options::PostFields(
                        post_data));
                request.setOpt(new curlpp::options::PostFieldSize(
                        post_data.length()));*/

                request.perform();

            } catch (curlpp::LogicError& e) {
                LOG(ERROR) << e.what();
            
            } catch (curlpp::RuntimeError& e) {
                LOG(ERROR) << e.what();
            }

            /*RouteSelector selector {
                route_selector::app=ServiceFlag::None,
                route_selector::metrics=MetricsFlag::Hop
            };

            uint32_t route_id = topo->newRoute(std::stoi(match[1]),
                                               std::stoi(match[2]),
                                               selector);

            if (!route_id) {
                LOG(WARNING) << "[TunnelMaker] Can't make tunnel.";
                return;
            }

            DLOG(INFO) << "[TunnelMaker] route_id = " << route_id;

            data_link_route dl_route = topo->getFirstWorkPath(route_id);
            for (auto& it: dl_route) {
                DLOG(INFO) << "[TunnelMaker] dpid = " << it.dpid
                           << " port = " << it.port;
            }
            
            uint8_t path_id = 15;
            path_id = topo->newPath(route_id, selector);

            if (path_id == max_path_id) {
                LOG(WARNING) << "[TunnelMaker] Can't add dynamic path.";
                return;
            }

            DLOG(INFO) << "[TunnelMaker] path_id = " << path_id;

            topo->addDynamic(route_id, selector);

            if (topo->getFirstWorkPathId(route_id) == max_path_id) {
                LOG(WARNING) << "[TunnelMaker] Can't add dynamic path.";
                return;
            }*/

            cli->print("Tunnel between switches with dpid {} and {} was made",
                       match[1], match[2]);
            cli->print("{:-^90}", "");
        }
    );

}

} // namespace runos
