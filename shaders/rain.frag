#version 330 core
in float vAlpha;
out vec4 fragColor;
void main(){
    fragColor = vec4(0.7, 0.75, 0.85, clamp(vAlpha, 0.05, 0.4));
}