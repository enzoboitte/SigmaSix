#version 330

out vec4 finalColor;

uniform vec2 uResolution;
uniform vec2 uPlayerScreenPosition;
uniform float uVisibilityRadius;
uniform float uFeather;
uniform vec4 uFogColor;

void main() {
    vec2 pixelPosition = vec2(gl_FragCoord.x, uResolution.y - gl_FragCoord.y);
    float distanceFromPlayer = distance(pixelPosition, uPlayerScreenPosition);
    float feather = max(uFeather, 0.001);
    float fogAmount = smoothstep(
        max(uVisibilityRadius - feather, 0.0),
        uVisibilityRadius,
        distanceFromPlayer
    );

    finalColor = vec4(uFogColor.rgb, uFogColor.a * fogAmount);
}
