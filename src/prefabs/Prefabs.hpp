#pragma once
#include <SigmaSix/ecs/Registry.hpp>
#include <SigmaSix/components/Components.hpp>

// Prefabs = fonctions qui assemblent des composants sur une entité.
inline Entity makeBody(Registry& r, float x, float y, float w, float h, Color c) {
    Entity e = r.create();
    r.add<Transform2D>(e, Transform2D{x, y, 0, 0});
    r.add<Sprite>(e, Sprite{c, w, h});
    r.add<Rigidbody>(e);
    r.add<Collider>(e, Collider{w, h});
    return e;
}
inline Entity makePlayer(Registry& r, float x, float y) {
    Entity e = makeBody(r, x, y, 32, 32, GREEN);
    r.add<PlayerControl>(e);
    r.add<ShaderComponent>(e, ShaderComponent{"outline", {{"thickness", 3.f}}});
    r.add<FogOfWar>(e);
    return e;
}
inline Entity makeEnemy(Registry& r, float x, float y) 
{ 
    Entity e = makeBody(r, x, y, 32, 32, RED);
    // disable collision for the enemy
    r.get<Collider>(e).isTrigger = true;
    return e;
}
inline Entity makeWall(Registry& r, float x, float y, float w, float h) {
    Entity e = makeBody(r, x, y, w, h, GRAY);
    r.get<Rigidbody>(e).isKinematic = true;
    return e;
}
inline Entity makeCamera(Registry& r, float x, float y, Entity target) {
    Entity e = r.create();
    r.add<Transform2D>(e, Transform2D{x, y, 0, 0});
    r.add<FollowTarget>(e, FollowTarget{target, 0.f});
    return e;
}
inline Entity makeProjectile(Registry& r, float x, float y, float w, float h, Color c) 
{
    Entity e = makeBody(r, x, y, w, h, c);
    r.get<Collider>(e).isTrigger = true;
    return e;
}