#pragma once
#include <SigmaSix/entity/Registry.hpp>
#include <SigmaSix/components/Components.hpp>

// Crée un ennemi (carré rouge) à la position donnée.
inline Entity createEnemy(Registry& r, float x, float y) {
    Entity e = r.create();
    r.add<Transform>(e, x, y, 0.f);
    r.add<Sprite>(e, RED, 32.f, 32.f);
    return e;
}
