#pragma once

#include "Application.hpp"
#include "Loader.hpp"

namespace runos { 

class TunnelMaker : public Application {
    Q_OBJECT
    SIMPLE_APPLICATION(TunnelMaker, "tunnel-maker")
public:
    void init(Loader* loader, const Config& config) override;
};

} // namespace runos
