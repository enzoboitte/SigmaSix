#include <raylib.h>
#include <box2d/box2d.h>

constexpr float PPM = 32.f; // pixels par mètre

int main() {
    InitWindow(800, 600, "Sigmasix");
    SetTargetFPS(60);

    b2World world(b2Vec2(0.f, 9.8f));

    // Sol statique
    b2BodyDef groundDef;
    groundDef.position.Set(400.f / PPM, 580.f / PPM);
    b2Body* ground = world.CreateBody(&groundDef);
    b2PolygonShape groundShape;
    groundShape.SetAsBox(400.f / PPM, 10.f / PPM);
    ground->CreateFixture(&groundShape, 0.f);

    // Boîte dynamique
    b2BodyDef boxDef;
    boxDef.type = b2_dynamicBody;
    boxDef.position.Set(400.f / PPM, 50.f / PPM);
    b2Body* box = world.CreateBody(&boxDef);
    b2PolygonShape boxShape;
    boxShape.SetAsBox(16.f / PPM, 16.f / PPM);
    b2FixtureDef fd;
    fd.shape = &boxShape;
    fd.density = 1.f;
    fd.friction = 0.3f;
    fd.restitution = 0.3f;
    box->CreateFixture(&fd);

    while (!WindowShouldClose()) {
        world.Step(GetFrameTime(), 8, 3);

        b2Vec2 p = box->GetPosition();

        BeginDrawing();
        ClearBackground(Color{30, 30, 40, 255});
        DrawRectangle(0, 570, 800, 20, Color{100, 100, 100, 255});
        DrawRectanglePro(Rectangle{p.x * PPM, p.y * PPM, 32.f, 32.f},
                         Vector2{16.f, 16.f},
                         box->GetAngle() * RAD2DEG, GREEN);
        EndDrawing();
    }
    CloseWindow();
}
