#include <raylib.h>
#include <SigmaSix/entity/Scene.hpp>
#include <SigmaSix/entity/prefabs/Player.hpp>
#include <SigmaSix/entity/prefabs/Enemy.hpp>
#include <SigmaSix/entity/prefabs/Wall.hpp>

int main() {
    InitWindow(800, 600, "Sigmasix");
    SetTargetFPS(60);

    Scene scene;
    scene.spawn<Player>(400, 300);
    scene.spawn<Enemy>(200, 150);
    scene.spawn<Wall>(400, 580, 800, 20);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        scene.fixedUpdate(dt);
        scene.update(dt);

        BeginDrawing();
        ClearBackground(Color{30, 30, 40, 255});
        scene.draw();
        EndDrawing();
    }
    CloseWindow();
}
