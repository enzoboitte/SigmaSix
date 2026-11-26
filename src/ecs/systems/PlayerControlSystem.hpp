#pragma once
#include "raylib.h"
#include "raymath.h"
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/scene/Camera.hpp>
#include <SigmaSix/components/Components.hpp>
#include <SigmaSix/prefabs/Prefabs.hpp>
#include <SigmaSix/ecs/systems/InterpolationSystem.hpp>
#include <memory>

// Déplacement du joueur (flèches / ZQSD) -> vitesse du Rigidbody.
struct PlayerControlSystem : System 
{
    std::shared_ptr<SceneCamera> camera = nullptr;

    // Les appuis ponctuels sont lus à chaque frame, puis consommés au pas physique suivant.
    void update(Registry&, float) override
    {
        if (IsKeyPressed(KEY_SPACE)) m_fireRequested = true;
    }
    
    void fixedUpdate(Registry& r, float) override 
    {   
        float dx = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) - (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
        float dy = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) - (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
        r.each<PlayerControl>([&](Entity e, PlayerControl& c)
        {
            if (!r.has<Rigidbody>(e) || !r.has<Transform2D>(e)) return;
            auto& rb = r.get<Rigidbody>(e);
            rb.vx = dx * c.speed;
            rb.vy = dy * c.speed;
            
            auto& tr = r.get<Transform2D>(e);
            Vector2 mouse = camera->screenToWorld(GetMousePosition());
            tr.angle = atan2f(mouse.y - tr.y, mouse.x - tr.x) * RAD2DEG;
            
            if (m_fireRequested)
            {
                // spawn projectile
                auto& src = r.get<Transform2D>(e);
                Entity p  = makeProjectile(r, src.x, src.y, 8, 8, YELLOW);
                r.add<Transform2D>(p, Transform2D{src.x, src.y, src.angle, 0});
                r.add<Projectile>(p, Projectile{400.f, 2.f, e});
            }
        });
        m_fireRequested = false;
    }

    // Ligne de visée vers la souris (le dessin doit être fait dans draw).
    void draw(Registry& r) override 
    {
        r.each<PlayerControl>([&](Entity e, PlayerControl&) 
        {
            if (!r.has<Transform2D>(e)) return;
            const Vector2 pos = renderPosition(r, e);
            Vector2 mouse = camera->screenToWorld(GetMousePosition());
            DrawLine((int)pos.x, (int)pos.y, (int)mouse.x, (int)mouse.y, RED);
        });
    }

private:
    bool m_fireRequested = false;
};
