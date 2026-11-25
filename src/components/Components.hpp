#pragma once
#include <map>
#include <raylib.h>
#include <SigmaSix/ecs/Entity.hpp>
#include <string>

// Composants = données pures, sans logique.
struct Transform2D     { float x = 0, y = 0, angle = 0; int z = 0; };
struct Sprite        { Color color = WHITE; float w = 32, h = 32; };
struct Rigidbody     { float vx = 0, vy = 0, mass = 1.f, gravityScale = 0.f, drag = 0.f; bool isKinematic = false; };
struct Collider      { float w = 32, h = 32, offsetX = 0, offsetY = 0; bool isTrigger = false; };
struct PlayerControl { float speed = 200.f; };
struct FollowTarget  { Entity target = NULL_ENTITY; float smoothing = 0.f; };  // 0 = collé
struct Projectile    { float speed = 400.f; float life = 2.f; };
struct ShaderComponent { std::string name; std::map<std::string, float> uniforms; };