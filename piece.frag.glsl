#version 330

out vec4 fragColor;
in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoords;

uniform vec3 viewPos;
uniform int  white;

#define light_position  vec3(0.f, 35.f, 3.f)
#define light_direction vec3(-0.2f, -1.0f, -0.3f)
#define light_ambient   vec3(0.2f, 0.2f, 0.2f)
#define light_diffuse   vec3(0.5f, 0.5f, 0.5f)
#define light_specular  vec3(1.0f, 1.0f, 1.0f)
#define mat_shininess   32.f

void main() {
  vec3 tex = vec3(white);
  
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
