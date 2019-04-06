#pragma once

#include "Application.hpp"
#include "Loader.hpp"

#include <string>
#include <map>

namespace runos { 

class TunnelMaker : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(TunnelMaker, "tunnel-maker")
public:
    void init(Loader* loader, const Config& config) override;

private:
    std::map<std::string, int> bd_routes_;
};

} // namespace runos
