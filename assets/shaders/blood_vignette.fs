#version 330

// Sang sur le contour de l'écran : liseré irrégulier, taches, gouttelettes et coulures depuis le haut.
// Centre transparent. À dessiner en plein écran (DrawRectangle 0,0,w,h WHITE) dans BeginShaderMode, en coordonnées écran.
out vec4 finalColor;

uniform vec2  uResolution;   // taille du framebuffer en pixels (GetRenderWidth/Height)
uniform float uIntensity;    // 0 = aucun sang, 1 = maximum
uniform float uTime;         // GetTime() : pulsation légère ; 0 = statique
uniform float uSeed;         // change la disposition des taches/coulures (ex. GetRandomValue à chaque blessure)

// uSeed décale tout l'aléatoire (taches, coulures, contour) ; ramené dans [0, 97[ pour garder la précision de sin().
float hash(vec2 p) {
    p += fract(uSeed * vec2(0.1031, 0.1123)) * 97.0;
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x), mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    for (int k = 0; k < 4; k++) { v += a * noise(p); p *= 2.0; a *= 0.5; }
    return v;
}

// Distance au bord le plus proche, en fraction du petit côté (0 = bord, 0.5 = centre).
float edgeDist(vec2 pix) {
    vec2 e = min(pix, uResolution - pix) / min(uResolution.x, uResolution.y);
    return min(e.x, e.y);
}

// Taches rondes déformées, plus fréquentes près des bords. q : coordonnées en petit côté.
// Retourne (masque, netteté de la tache dominante : 1 nette, 0 floue).
vec2 stains(vec2 q, float scale, float density, float rMin, float rMax, float seed, float reach, float minSide) {
    vec2 id = floor(q * scale);
    float m = 0.0, sharp = 1.0;
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
        vec2 c = id + vec2(x, y);
        float h = hash(c + seed);
        vec2 center = (c + 0.5 + (vec2(hash(c + seed + 1.3), hash(c + seed + 7.1)) - 0.5) * 0.8) / scale;
        float prob = density * (1.0 - smoothstep(reach * 0.5, reach * 1.5, edgeDist(center * minSide)));
        float fade = clamp((prob - h) / 0.08, 0.0, 1.0);            // apparition en fondu quand l'intensité monte
        if (fade <= 0.0) continue;
        float r = mix(rMin, rMax, hash(c + seed + 3.7));
        vec2 d = q - center;
        float wobble = noise(d / r * 1.5 + h * 50.0 + uTime * 0.25) - 0.5; // contour éclaboussé qui ondule lentement
        float k = length(d) / r + wobble * 0.6;
        // Flou propre à chaque tache : ~1/3 très floues (projetées hors focus), les autres plus ou moins nettes.
        float hb = hash(c + seed + 5.9);
        float blur = hb < 0.35 ? mix(0.5, 0.9, hb / 0.35) : mix(0.08, 0.25, (hb - 0.35) / 0.65);
        float opacity = hb < 0.35 ? 0.85 : 1.0;
        float v = (1.0 - smoothstep(1.0 - blur, 1.0 + blur * 0.3, k)) * fade * opacity;
        if (v > m) { m = v; sharp = hb < 0.35 ? 0.0 : 1.0; }
    }
    return vec2(m, sharp);
}

// Coulures verticales partant du bord haut, terminées par une goutte.
float drips(vec2 q, float topDist, float reach, float intensity) {
    const float scale = 22.0;
    float col = floor(q.x * scale);
    float m = 0.0;
    for (int i = -1; i <= 1; i++) {
        float c = col + float(i);
        float h = hash(vec2(c, 17.0));
        float fade = clamp((0.15 + 0.5 * intensity - h) / 0.05, 0.0, 1.0);
        if (fade <= 0.0) continue;
        float xc = (c + 0.5 + (hash(vec2(c, 3.0)) - 0.5) * 0.6) / scale;
        float len = reach * (0.5 + 1.0 * hash(vec2(c, 9.0)));
        float w = (0.0025 + 0.004 * hash(vec2(c, 5.0))) * (0.7 + 0.5 * intensity);
        float x = q.x - xc + sin(topDist * 35.0 + h * 20.0) * 0.0015; // trajet légèrement ondulé
        float taper = mix(1.0, 0.55, clamp(topDist / len, 0.0, 1.0));
        float body = (1.0 - smoothstep(w * taper * 0.8, w * taper, abs(x))) * step(topDist, len);
        float drop = 1.0 - smoothstep(w * 1.4, w * 1.8, length(vec2(x, topDist - len)));
        // Goutte qui glisse le long de la coulure puis repart du haut (vitesse propre à chaque coulure).
        float slide = fract(uTime * (0.08 + 0.10 * hash(vec2(c, 23.0))) + h) * len;
        float bead = (1.0 - smoothstep(w * 1.0, w * 1.4, length(vec2(x, topDist - slide)))) * step(slide, len * 0.97);
        m = max(m, max(max(body, drop), bead) * fade);
    }
    return m;
}

void main() {
    float intensity = clamp(uIntensity, 0.0, 1.0);
    if (intensity <= 0.0) discard;

    vec2 pix = gl_FragCoord.xy;
    float minSide = min(uResolution.x, uResolution.y);
    vec2 q = pix / minSide;
    // Aucune tache n'apparaît/disparaît avec le temps : il anime seulement les contours, les gouttes et un battement.
    float beat = pow(abs(sin(uTime * 2.2)), 12.0);
    float pulse = 1.0 + 0.10 * beat * intensity;
    float reach = mix(0.05, 0.30, intensity);   // jusqu'où le sang avance vers le centre

    // 1. Liseré continu au bord, contour déchiqueté ; reste fin même à intensité max (les taches portent l'effet).
    float rimReach = mix(0.015, 0.045, intensity);
    float d = edgeDist(pix) + (fbm(q * 6.0 + vec2(0.0, uTime * 0.06)) - 0.5) * 0.04; // contour du liseré qui coule
    float rim = 1.0 - smoothstep(rimReach * 0.6, rimReach, d);

    // 2. Grosses taches et 3. gouttelettes projetées, 4. coulures depuis le haut.
    vec2 bigS   = stains(q, 9.0,  0.10 + 0.70 * intensity, 0.020, 0.055, 11.0, reach, minSide);
    vec2 smallS = stains(q, 32.0, 0.10 + 0.60 * intensity, 0.003, 0.010, 47.0, reach * 1.2, minSide);
    float big = bigS.x, small = smallS.x;
    float drip  = drips(q, (uResolution.y - pix.y) / minSide, reach * 0.9, intensity);

    float mask = max(max(rim, big), max(small, drip));
    if (mask <= 0.01) discard;

    // Couleur : rouge vif humide à l'intérieur, presque noir sur le bord des taches (sang qui sèche).
    float tone = fbm(q * 18.0);
    vec3 fresh = vec3(0.55, 0.02, 0.03), dried = vec3(0.16, 0.0, 0.01);
    vec3 color = mix(fresh, dried, 0.35 + 0.4 * tone);
    // Netteté du pixel : une tache floue qui domine reste rouge vif (pas de bord séché, qui la rendait brune).
    float sharp = max(big, small) >= max(rim, drip) ? (big >= small ? bigS.y : smallS.y) : 1.0;
    color = mix(mix(fresh, vec3(0.42, 0.01, 0.02), tone), color, sharp);
    float border = smoothstep(0.0, 0.5, mask) * (1.0 - smoothstep(0.5, 1.0, mask));
    color = mix(color, dried * 0.6, border * 0.8 * sharp);
    color = mix(color, dried, (1.0 - smoothstep(0.0, rimReach, edgeDist(pix))) * 0.6); // bord écran plus sombre

    finalColor = vec4(color, clamp(mask * mix(0.75, 0.95, intensity) * pulse, 0.0, 1.0));
}
