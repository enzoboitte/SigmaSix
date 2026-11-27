#pragma once
#include <raylib.h>
#include <cstdlib>

// Caméra de scène basée sur Camera3D.
class SceneCamera {
public:
    float x = 0, y   = 0;        // point regardé (monde)
    float z          = 0;        // couche regardée
    float distance   = 500.f;    // distance caméra -> point regardé
    float zoom       = 1.f;      // ortho : zoom ; perspective : ignoré
    float fov        = 60.f;     // perspective : champ de vision vertical (degrés)
    bool perspective = false;    // true = perspective, false = orthographique

    void lookAt(float px, float py) { x = px; y = py; }

    // Construit la caméra 3D de base.
    Camera3D baseCamera3D() const {
        Camera3D cam{};
        cam.position = Vector3{x, y, -z - distance};
        cam.target   = Vector3{x, y, -z};
        cam.up       = Vector3{0, -1, 0};
        cam.projection = perspective ? CAMERA_PERSPECTIVE : CAMERA_ORTHOGRAPHIC;
        cam.fovy     = perspective ? fov : (float)GetScreenHeight() / zoom;
        return cam;
    }

    // Appeler à la place de BeginMode2D.
    void begin() {
        Camera3D cam = baseCamera3D();

        // Applique le shake à la position de la caméra
        if (shakeTime > 0.f) {
            float shakeIntensity = (shakeTime / shakeMax) * 5.f;  // décroît de 1 à 0
            float shakeOffsetX = ((float)(rand() % 200 - 100) / 100.f) * shakeIntensity * shakeMax;
            float shakeOffsetY = ((float)(rand() % 200 - 100) / 100.f) * shakeIntensity * shakeMax;
            
            cam.position       = Vector3{cam.position.x + shakeOffsetX, cam.position.y + shakeOffsetY, cam.position.z};
            cam.target         = Vector3{cam.target.x + shakeOffsetX, cam.target.y + shakeOffsetY, cam.target.z};
            
            // Décrémente le temps restant
            shakeTime -= 1.0f / 60.0f;
            if (shakeTime < 0.f) shakeTime = 0.f;
        }

        BeginMode3D(cam);
    }

    void end() { EndMode3D(); }

    Vector2 worldToScreen(Vector3 position) const 
    {
        return GetWorldToScreen(position, baseCamera3D());
    }

    // Convertit une position écran en monde (couche z=0, ortho uniquement).
    Vector2 screenToWorld(Vector2 s) const 
    {
        float viewHeight = (float)GetScreenHeight() / zoom;
        float viewWidth = (float)GetScreenWidth() / zoom;
        return Vector2{
            x + (s.x - GetScreenWidth() / 2.f) * (viewWidth / GetScreenWidth()),
            y + (s.y - GetScreenHeight() / 2.f) * (viewHeight / GetScreenHeight())
        };
    }

    // Couche z -> profondeur GPU (plus grand z = plus proche de la caméra).
    static float depth(float layer) { return -layer; }

    void addShake(float intensity) {
        this->shakeMax = intensity;
        this->shakeTime = intensity;
    }

private:
    float shakeTime = 0;
    float shakeMax  = 0;
};
