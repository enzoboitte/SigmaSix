#include <memory>
#include <raylib.h>
#include <SigmaSix/ecs/Registry.hpp>
#include <SigmaSix/scene/Camera.hpp>
#include <SigmaSix/ecs/systems/Systems.hpp>
#include <SigmaSix/prefabs/Prefabs.hpp>
#include <SigmaUi/Ui.hpp>
#include <SigmaSix/core/ConfigFile.hpp>

// ─── Helpers ────────────────────────────────────────────────────────────────

static void drawFrame(Registry& reg, const std::shared_ptr<SceneCamera>& camera,
                      const std::vector<std::shared_ptr<System>>& systems,
                      const std::vector<std::shared_ptr<System>>& systems_shaders,
                      ui::Screen& pause)
{
    BeginDrawing();
    ClearBackground(Color{30, 30, 40, 255});

    camera->begin();
        for (auto& s : systems) s->draw(reg);
    camera->end();

    for (auto& s : systems_shaders) s->draw(reg);

    if (pause.visible)
        pause.draw();

    EndDrawing();
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main()
{
    // ── 1. Configuration ────────────────────────────────────────────────────
    const ConfigFile config("assets/config.yml");
    const SigmaConfig& cfg = config.data;

    InitWindow(cfg.window.width, cfg.window.height, cfg.window.title.c_str());
    SetTargetFPS(cfg.window.fps);
    InitAudioDevice();
    SetMasterVolume(cfg.audio.volume / 100.f);

    // ── 2. UI ───────────────────────────────────────────────────────────────
    ui::loadStyles("assets/ui/styles.yml");
    ui::Screen pause("assets/ui/pause_menu.yml");
    pause.setValue("volume", static_cast<float>(cfg.audio.volume));
    pause.visible = false;

    bool quit = false;
    pause.on("Resume", [&](ui::Element&) { pause.visible = false; })
         .on("Quit",     [&](ui::Element&) { quit = true; })
         .on("VolumeChanged", [](ui::Element& e) {
             if (IsAudioDeviceReady())
                 SetMasterVolume(e.value / 100.f);
         });

    // ── 3. World & Entities ─────────────────────────────────────────────────
    Registry reg;

    const Entity player = makePlayer(reg, 400, 300);
    makeEnemy(reg, 200, 150);
    makeWall(reg, 400, 580, 800, 20);
    const Entity cam    = makeCamera(reg, 400, 300, player);

    // ── 4. Systems ──────────────────────────────────────────────────────────
    std::vector<std::shared_ptr<System>> systems;
    std::vector<std::shared_ptr<System>> systems_shaders;

    // Core systems
    auto playerControl = std::make_shared<PlayerControlSystem>();
    auto collisions    = std::make_shared<CollisionSystem>();
    auto interpolation = std::make_shared<InterpolationSystem>();

    systems.push_back(playerControl);
    systems.push_back(std::make_shared<ProjectileSystem>());
    systems.push_back(std::make_shared<PhysicsSystem>(collisions));
    systems.push_back(collisions);
    systems.push_back(std::make_shared<ProjectileHitSystem>(collisions));
    systems.push_back(interpolation);
    systems.push_back(std::make_shared<FollowSystem>());
    systems.push_back(std::make_shared<RenderSystem>());

    const auto camera = std::make_shared<SceneCamera>();
    playerControl->camera = camera;

    // Shader-based systems
    systems_shaders.push_back(std::make_shared<FogOfWarSystem>(player, camera));
    systems_shaders.push_back(std::make_shared<WallShadowSystem>(player, camera));
    systems_shaders.push_back(std::make_shared<BloodVignetteSystem>(player, camera));

    // ── 5. Game loop ────────────────────────────────────────────────────────
    const float step = 1.f / 50.f;
    float acc      = 0.f;

    while (!WindowShouldClose() && !quit) {
        // Input
        if (IsKeyPressed(KEY_P))
            pause.visible = !pause.visible;

        const float dt = pause.visible ? 0.f : std::min(GetFrameTime(), 0.25f);

        // Fixed update
        for (acc += dt; acc >= step; acc -= step) {
            for (auto& s : systems)
                s->fixedUpdate(reg, step);
        }
        interpolation->alpha = acc / step;

        // Update
        for (auto& s : systems)
            s->update(reg, dt);
        for (auto& s : systems_shaders)
            s->update(reg, dt);

        // Camera follow
        if (reg.has<Transform2D>(cam))
            camera->lookAt(reg.get<Transform2D>(cam).x, reg.get<Transform2D>(cam).y);

        // Draw
        drawFrame(reg, camera, systems, systems_shaders, pause);
    }

    // ── 6. Cleanup ──────────────────────────────────────────────────────────
    CloseAudioDevice();
    CloseWindow();
}
