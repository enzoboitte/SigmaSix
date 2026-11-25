#pragma once
#include <SigmaSix/entity/Registry.hpp>

// Interface de base des systèmes (physique, rendu, input...).
class System {
public:
    virtual ~System() = default;
    virtual void update(Registry&, float /*dt*/) {}
    virtual void draw(Registry&) {}
};
