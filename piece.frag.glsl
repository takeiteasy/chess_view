#version 330

out vec4 fragColor;
in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoords;

uniform vec3 viewPos;
uniform int  white;

#define light_position  vec3(40.f, 35.f, 40.f)
#define light_direction vec3(-0.2f, -1.0f, -0.3f)

#define light_ambient  (white_b ? vec3(0.25f, 0.20725f, 0.20725f) : vec3(0.05375f, 0.05f, 0.06625f))
#define light_diffuse  (white_b ? vec3(0.829f, 0.829f, 0.829f) : vec3(0.18275f, 0.17f, 0.22525f))
#define light_specular (white_b ? vec3(0.296648f, 0.296648f, 0.296648f) : vec3(0.332741f, 0.328634f, 0.346435f))
#define mat_shininess  (white_b ? 0.088f : 0.3f)

void main() {
  bool white_b = !(white > 0);
  vec3 tex = vec3((white_b ? 1.f : .5f));
  
  vec3 ambient = light_ambient * tex;
  
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(light_position - FragPos);;
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light_diffuse * diff * tex;
  
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);
  vec3 specular = light_specular * spec * tex;
  
  fragColor = vec4(ambient + diffuse + specular, 1.0);
}
