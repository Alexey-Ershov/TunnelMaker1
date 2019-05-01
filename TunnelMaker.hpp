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
    bool free_route = false;
};

class TunnelMaker : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(TunnelMaker, "tunnel-maker")
public:
    void init(Loader* loader, const Config& config) override;

protected slots:
    void onLinkDown();

private:
    // BD name -> Tunnel attributes.
    std::map<std::string, TunnelAttrs> tun_attrs_;
    Topology* topo_;
    CommandLine* cli_;
    cli_match match_;
    bool was_created_ = false;
    std::string ip_;
    bool was_collision_ = false;

    void create_bd();
    void fetch_route_id(std::string bd_name);
    void delete_bd(std::string bd_name);
    bool check_tunnel_requirements(std::string bd_name);
    bool add_path(std::string bd_name);
    void delete_path(std::string route_id, std::string path_id);
    bool change_path(std::string bd_name);
    void check_path_collisions(std::string bd_name);
    void cmp_bd(std::pair<std::string, TunnelAttrs> first_bd,
                std::pair<std::string, TunnelAttrs> second_bd);
};

} // namespace runos
