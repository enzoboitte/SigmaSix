#pragma once
#include <raylib.h>
#include <cstdlib>

// Caméra de scène basée sur Camera3D.
class SceneCamera {
public:
    float x = 0, y = 0;        // point regardé (monde)
    float z = 0;               // couche regardée
    float distance = 500.f;    // distance caméra -> point regardé
    float zoom = 1.f;          // ortho : zoom ; perspective : ignoré
    float fov = 60.f;          // perspective : champ de vision vertical (degrés)
    bool perspective = false;

    void lookAt(float px, float py) { x = px; y = py; }

    Camera3D camera3D() const {
        Camera3D cam{};
        // caméra derrière le plan (z négatif) et regardant +Z, up = -Y => x droite, y bas
        cam.position = Vector3{x, y, -z - distance};
        cam.target = Vector3{x, y, -z};
        cam.up = Vector3{0, -1, 0};
        cam.projection = perspective ? CAMERA_PERSPECTIVE : CAMERA_ORTHOGRAPHIC;
        cam.fovy = perspective ? fov : (float)GetScreenHeight() / zoom;
        return cam;
    }

    // Appeler à la place de BeginMode2D.
    void begin() {
        const Camera3D cam = camera3D();

        // Applique le shake à la position de la caméra

        BeginMode3D(cam);
    }

    void end() { EndMode3D(); }

    Vector2 worldToScreen(Vector3 position) const {
        return GetWorldToScreen(position, camera3D());
    }

    // Convertit une position écran en monde sur la couche `layer` (ortho uniquement).
    Vector2 screenToWorld(Vector2 s) const {
        return Vector2{x + (s.x - GetScreenWidth() / 2.f) / zoom,
                       y + (s.y - GetScreenHeight() / 2.f) / zoom};
    }

    // Couche z -> profondeur GPU (plus grand z = plus proche de la caméra).
    static float depth(float layer) { return -layer; }

    void addShake(float intensity) {
        int shakeX = (int)(((float)rand() / RAND_MAX - 0.5f) * intensity);
        int shakeY = (int)(((float)rand() / RAND_MAX - 0.5f) * intensity);
        // Déplace la caméra de manière aléatoire autour du point regardé
        this->shakeX = shakeX;
        this->shakeY = shakeY;
    }

private:
    float shakeX = 0;
    float shakeY = 0;
};
