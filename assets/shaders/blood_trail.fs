#version 330

// Trace de sang rapide dans une direction donnée : série de taches de tailles variées
// qui s'étirent le long de la direction et disparaissent progressivement.
// Centre transparent. À dessiner en plein écran (DrawRectangle 0,0,w,h WHITE) dans BeginShaderMode.
out vec4 finalColor;

uniform vec2  uResolution;   // taille du framebuffer en pixels (GetRenderWidth/Height)
uniform float uIntensity;    // 0 = aucune trace, 1 = maximum
uniform float uTime;         // GetTime() : contrôle la disparition progressive
uniform float uSeed;         // change la disposition des taches
uniform vec2  uDirection;    // direction de la trace (x,y) normalisée ou non

// uSeed décale tout l'aléatoire.
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

// Projection d'un point sur la direction : retourne (parallèle_perp, perpendiculaire).
// dir doit être normalisé.
vec2 projectDir(vec2 p, vec2 dir) {
    float d = dot(p, dir);
    return vec2(d, length(p - dir * d));
}

// Taches de sang le long de la direction, tailles variées, fading progressif.
// q : coordonnées centrées au milieu de l'écran.
// dir : direction de la trace normalisée.
// Retourne (masque, netteté).
vec2 trailStains(vec2 q, vec2 dir, float scale, float density, float rMin, float rMax, float seed, float age) {
    vec2 id = floor(q * scale);
    float m = 0.0, sharp = 1.0;
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
        vec2 c = id + vec2(x, y);
        float h = hash(c + seed);
        // Position du centre de la tache dans l'espace q
        vec2 center = (c + 0.5 + (vec2(hash(c + seed + 1.3), hash(c + seed + 7.1)) - 0.5) * 0.8) / scale;
        
        // Projection du centre sur la direction (depuis le centre de l'écran)
        vec2 proj = projectDir(center, dir);
        float along = proj.x;
        float perp = proj.y;

        // Limiter les taches à une zone le long de la direction
        // along doit être positif (dans la direction) et pas trop loin
        float maxAlong = 0.4 + 0.6 * age;
        if (along < 0.0 || along > maxAlong) continue;

        // Arrondi progressif à la fin de la traînée
        float alongPos = smoothstep(0.0, 1.0, along / maxAlong);
        // Le bout de la traînée s'arrondit : fade sur les dernières 20%
        float tipRound = 1.0 - smoothstep(0.75, 1.0, alongPos);
        // Étirement spatial pour l'effet arrondi
        float tipDist = 1.0 - alongPos;
        float tipStretch = mix(1.0, 2.0, pow(tipDist, 1.5));
        // Concentration le long de la direction : bande étroite perpendiculaire
        float perpBand = 1.0 - smoothstep(0.0, 0.15, abs(perp));
        
        // Densité : moins de taches au centre, plus vers la fin
        float densityMod = mix(0.2, 1.0, alongPos); // 0.2x au centre, 1x à la fin
        float prob = density * densityMod * perpBand;
        
        // Fading : chaque tache disparaît à un moment différent basé sur son hash h
        // h détermine quand la tache disparaît : taches avec h faible disparaissent en premier
        float fade = clamp((prob - h) / 0.08, 0.0, 1.0);
        // Apparition brutale (effet splash) au début
        float splashAppear = smoothstep(0.0, 0.05, age);
        // Disparition progressive tache par tache : chaque tache a un "temps de mort" unique
        float deathTime = mix(0.5, 5.5, h); // taches avec h=0 meurent à 0.5s, h=1 à 5.5s
        float fadeOut = 1.0 - smoothstep(deathTime - 0.5, deathTime + 0.5, age);
        fade *= splashAppear * fadeOut * tipRound;
        if (fade <= 0.01) continue;

        float r = mix(rMin, rMax, hash(c + seed + 3.7));
        // Déformation : les taches sont étirées dans la direction du mouvement
        vec2 d = q - center;
        vec2 dProj = projectDir(d, dir);
        // Étirement dans la direction : les taches s'allongent le long de la trace
        // + arrondi à la fin de la traînée
        float stretch = (1.0 + 3.0 * age * smoothstep(0.0, 0.5, abs(dProj.x) / 0.5)) * tipStretch;
        // Déformation wobble plus douce pour effet d'éclaboussure
        float wobble = noise(dProj / r * 1.2 + h * 50.0) - 0.5;
        // Distance normalisée : étirée dans la direction, normale perpendiculairement
        float k = sqrt(pow(dProj.x / r * stretch, 2.0) + pow(dProj.y / r + wobble * 0.3, 2.0));
        // Bord plus doux : taches plus arrondies, moins nettes
        float blur = mix(0.15, 0.45, hash(c + seed + 5.9));
        float opacity = 0.8 + 0.2 * hash(c + seed + 6.1);
        float v = (1.0 - smoothstep(1.0 - blur, 1.0 + blur * 0.5, k)) * fade * opacity;
        if (v > m) { m = v; sharp = 0.0; }
    }
    return vec2(m, sharp);
}

// Gouttelettes projetées perpendiculairement à la direction (effet d'éclaboussure latérale).
float splashes(vec2 q, vec2 dir, float scale, float density, float rMin, float rMax, float seed, float age) {
    vec2 id = floor(q * scale);
    float m = 0.0;
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
        vec2 c = id + vec2(x, y);
        float h = hash(c + seed + 23.0);
        vec2 center = (c + 0.5 + (vec2(hash(c + seed + 25.3), hash(c + seed + 31.1)) - 0.5) * 0.8) / scale;
        vec2 proj = projectDir(center, dir);
        float along = proj.x;
        float perp = proj.y;

        // Limiter aux gouttelettes dans la direction
        float maxAlong = 0.4 + 0.6 * age;
        if (along < 0.0 || along > maxAlong) continue;

        // Arrondi progressif à la fin de la traînée
        float alongPos = smoothstep(0.0, 1.0, along / maxAlong);
        float tipRound = 1.0 - smoothstep(0.75, 1.0, alongPos);

        // Gouttelettes latérales : plus fréquentes vers la fin de la trace
        float densityMod = mix(0.1, 1.0, alongPos);
        // Les éclaboussures sont plus dispersées perpendiculairement
        float perpBand = 1.0 - smoothstep(0.0, 0.20, abs(perp));
        float prob = density * densityMod * perpBand;
        float fade = clamp((prob - h) / 0.06, 0.0, 1.0);
        float splashAppear = smoothstep(0.0, 0.05, age);
        float deathTime = mix(0.5, 5.5, h);
        float fadeOut = 1.0 - smoothstep(deathTime - 0.5, deathTime + 0.5, age);
        fade *= splashAppear * fadeOut * tipRound;
        if (fade <= 0.01) continue;

        float r = mix(rMin, rMax * 0.5, h);
        // Ajouter une déformation irrégulière pour un effet plus naturel
        float deform = noise(center * 20.0 + h * 10.0) * 0.3;
        float k = length((q - center) / (r * (1.0 + deform)));
        // Bord plus doux avec variation aléatoire
        float blur = mix(0.05, 0.20, h);
        float v = (1.0 - smoothstep(blur * 0.5, blur, k)) * fade * 0.6;
        m = max(m, v);
    }
    return m;
}

void main() {
    float intensity = clamp(uIntensity, 0.0, 1.0);
    if (intensity <= 0.0) discard;

    vec2 pix = gl_FragCoord.xy;
    float minSide = min(uResolution.x, uResolution.y);
    vec2 center = uResolution * 0.5;
    vec2 q = (pix - center) / minSide; // coordonnées centrées, normalisées par le petit côté

    // Direction normalisée
    vec2 dir = normalize(uDirection);

    // Age de la trace : commence à 0, augmente avec le temps
    float age = uTime;
    float life = 12.0; // durée de vie en secondes
    // Splash initial : apparition brutale puis décroissance progressive lente
    float lifeFade = smoothstep(0.0, 0.08, age) * (1.0 - smoothstep(life * 0.5, life, age));
    if (lifeFade <= 0.0) discard;

    float reach = mix(0.05, 0.30, intensity); // étendue de la trace

    // 1. Taches principales le long de la direction (grandes)
    vec2 bigS = trailStains(q, dir, 8.0, 0.20 + 0.70 * intensity, 0.015, 0.060, 11.0, age);

    // 2. Taches moyennes
    vec2 midS = trailStains(q, dir, 20.0, 0.25 + 0.55 * intensity, 0.006, 0.025, 29.0, age);

    // 3. Petites taches / éclaboussures
    vec2 smallS = trailStains(q, dir, 35.0, 0.30 + 0.60 * intensity, 0.002, 0.010, 47.0, age);

    // Debug : afficher les taches principales en blanc pour vérifier
    // bigS = vec2(0.8, 0.0);

    // 4. Gouttelettes latérales (éclaboussures perpendiculaires)
    float splash = splashes(q, dir, 28.0, 0.15 + 0.40 * intensity, 0.002, 0.008, 61.0, age);

    // 5. Traînée diffuse (trace floue derrière les taches principales)
    float trail = 0.0;
    {
        float scale = 12.0;
        vec2 id = floor(q * scale);
        for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++) {
            vec2 c = id + vec2(float(x), float(y));
            float h = hash(c + 73.0);
            vec2 center = (c + 0.5 + (vec2(hash(c + 75.3), hash(c + 79.1)) - 0.5) * 0.8) / scale;
            vec2 proj = projectDir(center, dir);
            float along = proj.x;
            float perp = proj.y;
            
            // Limiter à la zone le long de la direction
            float maxAlong = 0.4 + 0.6 * age;
            if (along < 0.0 || along > maxAlong) continue;
            
            // Arrondi progressif à la fin de la traînée
            float alongPos = smoothstep(0.0, 1.0, along / maxAlong);
            float tipRound = 1.0 - smoothstep(0.75, 1.0, alongPos);
            
            // Densité croissante le long de la direction
            float densityMod = mix(0.2, 1.0, alongPos);
            
            // Bande étroite perpendiculaire pour la traînée
            float perpBand = 1.0 - smoothstep(0.0, 0.10, abs(perp));
            float trailLen = 0.6 * (1.0 + 0.8 * h);
            float dist = length(vec2(along / trailLen, perp * 3.0));
            float density2 = 0.12 + 0.25 * intensity;
            float fade = clamp((density2 * densityMod - h) / 0.07, 0.0, 1.0) * perpBand;
            float splashAppear = smoothstep(0.0, 0.05, age);
            float deathTime = mix(0.5, 5.5, h);
            float fadeOut = 1.0 - smoothstep(deathTime - 0.5, deathTime + 0.5, age);
            fade *= splashAppear * fadeOut * tipRound;
            if (fade <= 0.01) continue;
            float v = (1.0 - smoothstep(0.3, 0.7, dist)) * fade * 0.50;
            trail = max(trail, v);
        }
    }

    // 6. Grosse tache irrégulière au point d'impact (à la fin de la traînée)
    // Position : à l'extrémité de la direction (au lieu du centre de l'écran)
    float impactR = 0.08 + 0.04 * intensity;
    // Le centre de l'impact est à la fin de la traînée, pas au centre de l'écran
    vec2 impactCenter = dir * reach;
    float distImpact = length(q - impactCenter);
    // Forme irrégulière : bruit déformant le cercle
    float deform = fbm(q * 25.0 + 200.0) * 0.35 + noise(q * 45.0 + 300.0) * 0.15;
    float impactRadius = impactR * (1.0 + deform);
    float impactMask = 1.0 - smoothstep(impactR * 0.6, impactRadius * 1.2, distImpact);
    // La tache d'impact apparaît instantanément et persiste jusqu'à la fin du cycle
    float impactFade = smoothstep(0.0, 0.02, age) * (1.0 - smoothstep(life * 0.6, life, age));
    impactMask *= impactFade;

    // Combiner tous les éléments
    float mask = max(max(max(max(max(bigS.x, midS.x), smallS.x), splash), trail), impactMask);
    if (mask <= 0.01) discard;

    // === COULEUR DE SANG RÉALISTE ===
    // Variations locales de couleur avec bruit
    float colorNoise = fbm(q * 35.0 + fbm(q * 60.0) * 0.5);
    float thickNoise = fbm(q * 8.0 + vec2(0.0, 1.7));
    
    // Palette de couleurs de sang réaliste
    // Sang frais : rouge vif avec reflets orangés
    vec3 bloodFresh1 = vec3(0.65, 0.05, 0.04);    // rouge vif
    vec3 bloodFresh2 = vec3(0.55, 0.08, 0.06);    // rouge avec reflets
    vec3 bloodFresh3 = vec3(0.70, 0.12, 0.08);    // rouge orangé (bords)
    
    // Sang partiellement séché : brun-rouge foncé
    vec3 bloodDried1 = vec3(0.25, 0.04, 0.03);    // brun foncé
    vec3 bloodDried2 = vec3(0.35, 0.06, 0.04);    // brun rougeâtre
    
    // Sang très sec : presque noir
    vec3 bloodVeryDried = vec3(0.12, 0.01, 0.015);
    
    // Mélange dynamique basé sur la position et l'âge
    float ageFactor = smoothstep(0.0, 2.5, age);
    float dryAmount = mix(0.0, 0.7, ageFactor);
    
    // Variations spatiales de couleur
    vec3 baseColor;
    if (thickNoise > 0.6) {
        // Zones épaisses : sang plus foncé et moins sec
        baseColor = mix(bloodFresh2, bloodDried1, dryAmount * 0.5);
    } else if (thickNoise < 0.3) {
        // Zones fines : sang plus clair et plus sec
        baseColor = mix(bloodFresh3, bloodDried2, dryAmount);
    } else {
        // Mélange intermédiaire
        baseColor = mix(bloodFresh1, bloodDried1, dryAmount);
    }
    
    // Appliquer les variations de couleur locales
    baseColor = mix(baseColor, bloodFresh3, colorNoise * 0.3);
    baseColor = mix(baseColor, bloodVeryDried, dryAmount * colorNoise * 0.4);
    
    // === EFFET D'HUMIDITÉ / BRILLANCE ===
    // Simuler le reflet spéculaire du sang humide
    float wetness = 1.0 - smoothstep(0.0, 2.5, age) * 0.8;
    float specular = pow(mask, 3.0) * wetness * 0.4;
    
    // Ajout de reflets directionnels (simule la lumière sur le sang humide)
    float lightAngle = dot(normalize(q), vec2(0.7, 0.7));
    specular += pow(max(0.0, lightAngle), 4.0) * wetness * 0.2;
    
    // Combiner la couleur de base avec la brillance
    vec3 color = baseColor + vec3(specular * 0.8, specular * 0.3, specular * 0.2);
    
    // === TRANSITIONS DE COULEUR SUR LES BORDS ===
    // Les bords des taches de sang ont tendance à être plus clairs (effet de séchage)
    float edgeFactor = smoothstep(0.0, 0.3, mask) * (1.0 - smoothstep(0.3, 0.7, mask));
    color = mix(color, baseColor * 1.3, edgeFactor * 0.4);
    
    // === ALPHA AVEC VARIATIONS RÉALISTES ===
    // Le sang n'est pas uniformément opaque
    float alphaBase = mask * mix(0.60, 0.85, intensity) * lifeFade;
    
    // Variations locales d'opacité
    float alphaNoise = fbm(q * 50.0 + 100.0);
    float alphaMod = mix(0.7, 1.0, alphaNoise);
    
    // Les zones plus épaisses sont plus opaques
    float thicknessMod = smoothstep(0.0, 0.5, mask);
    alphaMod = mix(alphaMod, 1.0, thicknessMod);
    
    float alpha = clamp(alphaBase * alphaMod, 0.0, 0.95);
    
    // === EFFET DE SÉCHAGE SUR LES BORDS ===
    // Les bords des taches deviennent plus foncés et plus translucides
    float borderDry = smoothstep(0.0, 0.4, mask) * (1.0 - smoothstep(0.4, 0.8, mask));
    color = mix(color, bloodDried1 * 0.8, borderDry * dryAmount * 0.6);
    
    // === FINETUNING FINAL ===
    // Ajouter un peu de variation aléatoire à la couleur
    float fineNoise = fbm(q * 80.0);
    color += (fineNoise - 0.5) * 0.05 * (1.0 - dryAmount);
    
    // Assombrir légèrement le sang en fonction de l'âge
    color *= mix(1.0, 0.85, dryAmount * 0.5);

    finalColor = vec4(color, alpha);
}
