#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
uniform float uTime;
out float vAlpha;
void main(){
    vec3 pos = aPos;
    float speed = 25.0 + abs(aPos.x) * 3.0;
    pos.y = mod(pos.y - uTime * speed, 20.0);
    vAlpha = 0.3;
    gl_Position = uMVP * vec4(pos, 1.0);
}