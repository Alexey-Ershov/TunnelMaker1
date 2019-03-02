#include "TunnelMaker.hpp"
#include "CommandLine.hpp"
#include <runos/core/logging.hpp>

namespace runos {

REGISTER_APPLICATION(TunnelMaker, {"command-line", ""})

void TunnelMaker::init(Loader* loader, const Config& config)
{
    LOG(INFO) << "TunnelMaker is up!";

    auto cli = CommandLine::get(loader);

    cli->register_command(
        cli_pattern(R"(tunnel\s+([0-9]+)\s+([0-9]+))"),
        [=](cli_match const& match)
        {
            cli->print("{:-^90}", "  TUNNEL MAKER  ");
            
            for (const auto& mtch: match) {
                cli->print("{:<50}", mtch);
            }
            
            cli->print("{:<50} {:^16}", "Some info", "I am the Walrus");
            cli->print("{:-^90}", "");
        }
    );

}

} // namespace runos
