#pragma once

#include "Application.hpp"
#include "Loader.hpp"
#include "CommandLine.hpp"
#include "Topology.hpp"

#include <string>
#include <map>

namespace runos {

struct TunnelAttrs
{
    int route_id;
    int num_of_hops;
    int first_path_id;
    int last_path_id;
    data_link_route work_path;
};

class TunnelMaker : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(TunnelMaker, "tunnel-maker")
public:
    void init(Loader* loader, const Config& config) override;
    void startUp(Loader* loader) override;

private:
    // BD name -> Tunnel attributes.
    std::map<std::string, TunnelAttrs> tun_attrs_;
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
    void change_path(std::string bd_name);
    void check_path_collisions();
};

} // namespace runos
