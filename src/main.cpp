#include <memory>
#include <raylib.h>
#include <SigmaSix/ecs/Registry.hpp>
#include <SigmaSix/scene/Camera.hpp>
#include <SigmaSix/ecs/systems/Systems.hpp>
#include <SigmaSix/prefabs/Prefabs.hpp>

int main() {
    InitWindow(800, 600, "Sigmasix");
    SetTargetFPS(60);

    Registry reg;
    Entity player = makePlayer(reg, 400, 300);
    makeEnemy(reg, 200, 150);
    makeWall(reg, 400, 580, 800, 20);
    Entity cam = makeCamera(reg, 400, 300, player);

    std::vector<std::shared_ptr<System>> systems;
    auto pc = std::make_shared<PlayerControlSystem>();
    PlayerControlSystem* playerControl = pc.get();
    auto collisions = std::make_shared<CollisionSystem>();
    systems.push_back(pc);
    systems.push_back(std::make_shared<ProjectileSystem>());
    systems.push_back(std::make_shared<PhysicsSystem>(collisions));
    systems.push_back(collisions);
    systems.push_back(std::make_shared<ProjectileHitSystem>(collisions));
    auto interpolation = std::make_shared<InterpolationSystem>();
    systems.push_back(interpolation);
    systems.push_back(std::make_shared<FollowSystem>());
    systems.push_back(std::make_shared<RenderSystem>());

    auto camera = std::make_shared<SceneCamera>();
    FogOfWarSystem fogOfWar(player, camera);
    const float step = 1.f / 50.f;
    float acc = 0;

    WallShadowSystem wallShadow(player, camera);

    playerControl->camera = camera;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.25f) 
            dt = 0.25f;

        for (acc += dt; acc >= step; acc -= step) {
            for (auto& s : systems) 
                s->fixedUpdate(reg, step);
        }
        interpolation->alpha = acc / step;

        for (auto& s : systems) 
            s->update(reg, dt);
            
        if (reg.has<Transform2D>(cam)) 
            camera->lookAt(reg.get<Transform2D>(cam).x, reg.get<Transform2D>(cam).y);

        BeginDrawing();
            ClearBackground(Color{30, 30, 40, 255});
            camera->begin();
                for (auto& s : systems) s->draw(reg);
            camera->end();
            wallShadow.draw(reg);
            fogOfWar.draw(reg);
        EndDrawing();
    }
    CloseWindow();
}
