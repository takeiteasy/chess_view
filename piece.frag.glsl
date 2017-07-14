#version 330

out vec4 fragColor;
in vec2 TexCoords;
uniform int white;

void main() {
  fragColor = vec4(vec3(white), 1.f);
}
