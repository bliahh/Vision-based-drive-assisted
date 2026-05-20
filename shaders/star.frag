#version 330 core
in vec3 vPos;
uniform vec4  uColor;
uniform float uTime;
out vec4 FragColor;
void main(){
    float seed = dot(vPos, vec3(12.9898, 78.233, 45.164));
    float offset = fract(sin(seed) * 43758.5453);
    float blink = 0.7 + 0.3 * sin(uTime * (2.0 + offset) + offset * 6.28);
    FragColor = vec4(uColor.rgb * blink, uColor.a);
}
