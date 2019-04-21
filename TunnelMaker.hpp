#pragma once

#include "Application.hpp"
#include "Loader.hpp"
#include "CommandLine.hpp"
#include "Topology.hpp"

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
    // BD name -> route ID.
    std::map<std::string, int> bd_routes_;
    Topology* topo_;
    CommandLine* cli_;
    cli_match match_;
    bool was_created_ = false;
    std::string ip_;

    void create_bd();
    void fetch_route_id(std::string name = "");
    void delete_bd(std::string name);
    void check_tunnel_requirements();
    bool add_path(std::string name);
    void delete_path(std::string route_id, std::string path_id);
};

} // namespace runos
