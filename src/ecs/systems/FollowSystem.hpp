#pragma once
#include <cmath>
#include <raymath.h>
#include <SigmaSix/core/System.hpp>
#include <SigmaSix/components/Components.hpp>

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
            auto& tt = r.get<Transform2D>(f.target);
            
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


// void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
// {
//     static float minSpeed = 30;
//     static float minEffectLength = 10;
//     static float fractionSpeed = 0.8f;

//     camera->offset = (Vector2){ width/2.0f, height/2.0f };
//     Vector2 diff = Vector2Subtract(player->position, camera->target);
//     float length = Vector2Length(diff);

//     if (length > minEffectLength)
//     {
//         float speed = fmaxf(fractionSpeed*length, minSpeed);
//         camera->target = Vector2Add(camera->target, Vector2Scale(diff, speed*delta/length));
//     }
// }