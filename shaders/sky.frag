#version 330 core
in vec2 vUV;
uniform int   uFogEnabled;
uniform float uFogDensity;
out vec4 fragColor;
void main(){
    float y = vUV.y;
    vec3 ground   = vec3(0.03, 0.02, 0.02);
    vec3 horizon  = vec3(0.85, 0.30, 0.05);
    vec3 mid_sky  = vec3(0.20, 0.05, 0.10);
    vec3 top_sky  = vec3(0.02, 0.02, 0.08);
    float hp = 0.45;
    vec3 color;
    if (y < hp - 0.15) {
        float t = y / (hp - 0.15);
        color = mix(ground, horizon * 0.3, t);
    } else if (y < hp) {
        float t = (y - (hp - 0.15)) / 0.15;
        color = mix(horizon * 0.3, horizon, t * t);
    } else if (y < hp + 0.08) {
        float t = (y - hp) / 0.08;
        color = mix(horizon, mid_sky, t);
    } else if (y < hp + 0.35) {
        float t = (y - hp - 0.08) / 0.27;
        color = mix(mid_sky, top_sky, t);
    } else {
        color = top_sky;
    }
    if (uFogEnabled == 1) {
        vec3 fogColor = vec3(0.15, 0.10, 0.08);
        float fogAmount = uFogDensity / 0.025 * 0.4;
        color = mix(color, fogColor, clamp(fogAmount, 0.0, 0.8));
    }
    fragColor = vec4(color, 1.0);
}