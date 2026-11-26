#pragma once
#include "raylib.h"
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/components/Components.hpp>
#include <vector>

// Déplacement du joueur (flèches / ZQSD) -> vitesse du Rigidbody.
struct ProjectileSystem : System 
{
    void fixedUpdate(Registry& r, float dt) override
    {
        std::vector<Entity> dead;
        r.each<Projectile>([&](Entity e, Projectile& p)
        {
            if (!r.has<Transform2D>(e)) return;

            auto& tr = r.get<Transform2D>(e);
            float a  = tr.angle * DEG2RAD;
            if (r.has<Rigidbody>(e)) {
                auto& rb = r.get<Rigidbody>(e);
                rb.vx = cosf(a) * p.speed;
                rb.vy = sinf(a) * p.speed;
            }
            p.life  -= dt;
            if (p.life <= 0) dead.push_back(e);
            
        });
        for (Entity e : dead) r.destroy(e);
    }
    
    void draw(Registry& r) override 
    {
        
    }
};