#pragma once

#include "Application.hpp"
#include "Loader.hpp"
#include "CommandLine.hpp"

#include <string>
#include <map>

namespace runos { 

class TunnelMaker : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(TunnelMaker, "tunnel-maker")
public:
    void init(Loader* loader, const Config& config) override;
    void startUp(Loader* loader) override;

private:
    std::map<std::string, int> bd_routes_;
    CommandLine* cli_;
    bool was_created_;

    void create_bd(cli_match const& match);
    void fetch_route_id(std::string name = "");
    void delete_bd(std::string name);
};

} // namespace runos
