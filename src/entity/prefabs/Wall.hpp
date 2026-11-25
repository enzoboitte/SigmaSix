#pragma once
#include <SigmaSix/entity/Registry.hpp>
#include <SigmaSix/components/Components.hpp>

// Crée un mur (rectangle gris) de taille w x h.
inline Entity createWall(Registry& r, float x, float y, float w, float h) {
    Entity e = r.create();
    r.add<Transform>(e, x, y, 0.f);
    r.add<Sprite>(e, GRAY, w, h);
    return e;
}
