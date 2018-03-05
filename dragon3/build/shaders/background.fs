#version 420 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube background;

uniform float lodLevel;

void main()
{
  vec3 color = textureLod(background, WorldPos, lodLevel).rgb;
  FragColor = vec4(0.53,0.81,0.92, 1.0);
}