#version 330
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coord;

uniform mat4 projection, view, model;
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main() {
  FragPos = vec3(model * vec4(position, 1.0));
  Normal = mat3(transpose(inverse(model))) * normal;
  TexCoords = tex_coord;
  
  gl_Position = projection * view * vec4(FragPos, 1.0);
}
