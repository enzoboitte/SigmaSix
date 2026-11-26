#version 330

// Ombres de ligne de vue : un pixel est caché si le segment joueur -> pixel
// traverse un mur. Toutes les positions sont en pixels ÉCRAN (origine en haut à gauche).

#define MAX_WALLS 64

out vec4 finalColor;

uniform vec2  uResolution;              // taille de l'écran
uniform vec2  uPlayerScreenPosition;    // position du joueur à l'écran
uniform vec4  uWalls[MAX_WALLS];        // (centreX, centreY, demiLargeur, demiHauteur)
uniform float uWallAngles[MAX_WALLS];   // rotation de chaque mur, en radians
uniform int   uWallCount;               // nombre de murs réellement envoyés
uniform vec4  uShadowColor;             // couleur de l'ombre (alpha = opacité)

vec2 toWallSpace(vec2 point, vec2 center, float angle) {
    vec2 d = point - center;
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * d.x + s * d.y, -s * d.x + c * d.y);
}

bool insideBox(vec2 p, vec2 halfSize) {
    return all(lessThanEqual(abs(p), halfSize));
}

// Test segment [a, b] contre une boîte centrée en 0 (méthode des slabs).
bool segmentHitsBox(vec2 a, vec2 b, vec2 halfSize) {
    vec2 dir = b - a;
    float tMin = 0.0;
    float tMax = 1.0;

    for (int axis = 0; axis < 2; ++axis) {
        if (abs(dir[axis]) < 1e-6) {
            if (a[axis] < -halfSize[axis] || a[axis] > halfSize[axis]) return false;
        } else {
            float t1 = (-halfSize[axis] - a[axis]) / dir[axis];
            float t2 = ( halfSize[axis] - a[axis]) / dir[axis];
            tMin = max(tMin, min(t1, t2));
            tMax = min(tMax, max(t1, t2));
            if (tMin > tMax) return false;
        }
    }
    return true;
}

void main() {
    vec2 pixel = vec2(gl_FragCoord.x, uResolution.y - gl_FragCoord.y);

    for (int i = 0; i < MAX_WALLS; ++i) {
        if (i >= uWallCount) break;

        vec2 center   = uWalls[i].xy;
        vec2 halfSize = uWalls[i].zw;
        float angle   = uWallAngles[i];

        vec2 localPlayer = toWallSpace(uPlayerScreenPosition, center, angle);
        vec2 localPixel  = toWallSpace(pixel, center, angle);

        // Le joueur collé/à l'intérieur d'un mur ne doit pas tout masquer,
        // et la surface du mur lui-même reste visible.
        if (insideBox(localPlayer, halfSize) || insideBox(localPixel, halfSize)) continue;

        if (segmentHitsBox(localPlayer, localPixel, halfSize)) {
            finalColor = uShadowColor;
            return;
        }
    }

    finalColor = vec4(0.0);
}
