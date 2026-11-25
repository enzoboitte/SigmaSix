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
            
            // move project with speed and angle
            auto& tr = r.get<Transform2D>(e);
            float a  = tr.angle * DEG2RAD;
            tr.x    += (cosf(a) * p.speed * dt);
            tr.y    += (sinf(a) * p.speed * dt);
            p.life  -= dt;
            if (p.life <= 0) dead.push_back(e);
            
        });
        for (Entity e : dead) r.destroy(e);
    }
    
    void draw(Registry& r) override 
    {
        
    }
};