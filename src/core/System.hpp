#pragma once
#include <SigmaSix/ecs/Registry.hpp>

// Interface de base des systèmes (input, physique, rendu...).
class System {
public:
    virtual ~System() = default;
    virtual void fixedUpdate(Registry&, float /*dt*/) {}
    virtual void update(Registry&, float /*dt*/) {}
    virtual void draw(Registry&) {}
};
