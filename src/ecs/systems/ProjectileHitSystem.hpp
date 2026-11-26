#pragma once
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/components/Components.hpp>
#include <SigmaSix/ecs/systems/CollisionSystem.hpp>

struct ProjectileHitSystem : System {
    explicit ProjectileHitSystem(CollisionSystem& collisions)
        : m_collisions(collisions) {}

    void fixedUpdate(Registry& r, float) override {
        std::vector<Entity> projectilesToDestroy;

        for (const CollisionEvent& event : m_collisions.events()) {
            if (!event.began) continue;

            Entity projectile = NULL_ENTITY;
            Entity target = NULL_ENTITY;
            if (r.has<Projectile>(event.a)) {
                projectile = event.a;
                target = event.b;
            } else if (r.has<Projectile>(event.b)) {
                projectile = event.b;
                target = event.a;
            }

            if (projectile == NULL_ENTITY) continue;
            if (r.get<Projectile>(projectile).owner == target) continue;

            projectilesToDestroy.push_back(projectile);
        }

        for (Entity projectile : projectilesToDestroy) {
            if (r.has<Projectile>(projectile)) r.destroy(projectile);
        }
    }

private:
    CollisionSystem& m_collisions;
};
