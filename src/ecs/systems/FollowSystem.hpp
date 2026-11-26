#pragma once
#include <cmath>
#include <raymath.h>
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/components/Components.hpp>
#include <SigmaSix/ecs/systems/InterpolationSystem.hpp>

static float minSpeed         = 70;
static float minEffectLength  = 4;
static float fractionSpeed    = 8.f;

// Caméra / entité suiveuse.
struct FollowSystem : System 
{
    void update(Registry& r, float dt) override 
    {
        r.each<FollowTarget>([&](Entity e, FollowTarget& f) 
        {
            if (!r.has<Transform2D>(e) || !r.has<Transform2D>(f.target)) return;
            
            auto& t  = r.get<Transform2D>(e);
            const Vector2 tt = renderPosition(r, f.target);
            
            Vector2 diff = { tt.x - t.x, tt.y - t.y };
            float length = Vector2Length(diff);
            if (length > minEffectLength) 
            {
                float speed = fmaxf(fractionSpeed * length, minSpeed);
                t.x += diff.x * speed * dt / length;
                t.y += diff.y * speed * dt / length;
            }
        });
    }
};