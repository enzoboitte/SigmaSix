#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform vec2 size;          // taille totale du quad (corps + contour) en pixels
uniform float thickness;    // épaisseur du contour en pixels
uniform vec4 outlineColor;

void main() {
    vec2 p = fragTexCoord * size;
    bool inside = all(greaterThanEqual(p, vec2(thickness))) &&
                  all(lessThanEqual(p, size - vec2(thickness)));
    finalColor = inside ? fragColor : outlineColor;
}
