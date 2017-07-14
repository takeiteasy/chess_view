#version 330

out vec4 fragColor;
in vec2 TexCoords;

void main() {
  fragColor = vec4(vec3(step(0.5, (TexCoords.st + fract(gl_FragCoord.xy * vec2(64.f))).x)), 1.0);
}
