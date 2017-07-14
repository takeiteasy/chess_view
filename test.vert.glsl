#version 330
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coord;

uniform mat4 projection, view, model;
out vec2 TexCoords;

void main() {
  gl_Position = projection * view * model * vec4(position, 1.f);
  TexCoords = tex_coord;
}
