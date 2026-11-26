#include <memory>
#include <raylib.h>
#include <SigmaSix/ecs/Registry.hpp>
#include <SigmaSix/scene/Camera.hpp>
#include <SigmaSix/ecs/systems/Systems.hpp>
#include <SigmaSix/prefabs/Prefabs.hpp>
#include <SigmaUi/Ui.hpp>

int main() {
    InitWindow(800, 600, "Sigmasix");
    SetTargetFPS(60);

    ui::loadStyles("assets/ui/styles.yml");
    ui::Screen pause("assets/ui/pause_menu.yml");
    pause.visible = false;
    bool quit = false;
    pause.on("Resume", [&](ui::Element&) { pause.visible = false; })
         .on("Quit", [&](ui::Element&) { quit = true; })
         .on("VolumeChanged", [](ui::Element& e) { if (IsAudioDeviceReady()) SetMasterVolume(e.value / 100.f); });

    Registry reg;
    Entity player = makePlayer(reg, 400, 300);
    makeEnemy(reg, 200, 150);
    makeWall(reg, 400, 580, 800, 20);
    Entity cam = makeCamera(reg, 400, 300, player);

    std::vector<std::shared_ptr<System>> systems;
    std::vector<std::shared_ptr<System>> systems_shaders;

    // Core systems
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

    // Shader-based systems
    systems_shaders.push_back(std::make_shared<FogOfWarSystem>(player, camera));
    systems_shaders.push_back(std::make_shared<WallShadowSystem>(player, camera));

    const float step      = 1.f / 50.f;
    float       acc       = 0;
    playerControl->camera = camera;

    while (!WindowShouldClose() && !quit) {
        if (IsKeyPressed(KEY_P))
            pause.visible = !pause.visible;
        float dt = pause.visible ? 0.f : GetFrameTime();
        if (dt > 0.25f) 
            dt = 0.25f;

        for (acc += dt; acc >= step; acc -= step) {
            for (auto& s : systems) 
                s->fixedUpdate(reg, step);
        }
        interpolation->alpha = acc / step;

        for (auto& s : systems) 
            s->update(reg, dt);
        for (auto& s : systems_shaders)
            s->update(reg, dt);
            
        if (reg.has<Transform2D>(cam)) 
            camera->lookAt(reg.get<Transform2D>(cam).x, reg.get<Transform2D>(cam).y);

        BeginDrawing();
            ClearBackground(Color{30, 30, 40, 255});
            camera->begin();
                for (auto& s : systems) s->draw(reg);
            camera->end();

            for (auto& s : systems_shaders) s->draw(reg);
        EndDrawing();
    }
    CloseWindow();
}
