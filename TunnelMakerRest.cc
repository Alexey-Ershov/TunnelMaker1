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
    Topology* topo = Topology::get(loader);

    cli->register_command(
        cli_pattern(R"(mktun\s+([0-9]+)\s+([0-9]+))"),
        [=](cli_match const& match)
        {
            cli->print("{:-^90}", "  TUNNEL MAKER  ");
    
            std::string url = "http://172.30.7.201:8080/routes/";

            try {
                curlpp::Cleanup cleaner;
                curlpp::Easy request;

                using namespace curlpp::Options;
                request.setOpt(Verbose(true));
                request.setOpt(Url(url));

                request.perform();

                std::string effURL;
                curlpp::infos::EffectiveUrl::get(request, effURL);
                std::cout << "Effective URL: " << effURL << std::endl;

                //other way to retreive URL
                std::cout << std::endl 
                    << "Effective URL: " 
                    << curlpp::infos::EffectiveUrl::get(request)
                    << std::endl;

                std::cout << "Response code: " 
                    << curlpp::infos::ResponseCode::get(request) 
                    << std::endl;

                std::cout << "SSL engines: " 
                    << curlpp::infos::SslEngines::get(request)
                    << std::endl;

            } catch (curlpp::LogicError& e) {
                std::cout << e.what() << std::endl;
            
            } catch (curlpp::RuntimeError& e) {
                std::cout << e.what() << std::endl;
            }

            cli->print("Tunnel between switches with dpid {} and {} was made",
                       match[1], match[2]);
            cli->print("{:-^90}", "");
        }
    );
}

} // namespace runos
