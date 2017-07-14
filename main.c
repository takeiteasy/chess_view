#include <stdio.h>

#include "3rdparty/glad.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "3rdparty/linalgb.h"
#include "obj.h"
#include "helpers.h"

static const int SCREEN_WIDTH = 640, SCREEN_HEIGHT = 480;

static SDL_Window* window;
static SDL_GLContext context;

#undef GLAD_DEBUG

#ifdef GLAD_DEBUG
void pre_gl_call(const char *name, void *funcptr, int len_args, ...) {
  printf("Calling: %s (%d arguments)\n", name, len_args);
}
#endif

char* glGetError_str(GLenum err) {
  switch (err) {
    case GL_INVALID_ENUM:                  return "INVALID_ENUM"; break;
    case GL_INVALID_VALUE:                 return "INVALID_VALUE"; break;
    case GL_INVALID_OPERATION:             return "INVALID_OPERATION"; break;
    case GL_STACK_OVERFLOW:                return "STACK_OVERFLOW"; break;
    case GL_STACK_UNDERFLOW:               return "STACK_UNDERFLOW"; break;
    case GL_OUT_OF_MEMORY:                 return "OUT_OF_MEMORY"; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "INVALID_FRAMEBUFFER_OPERATION"; break;
    default:
      return "Unknown Error";
  }
}

void post_gl_call(const char *name, void *funcptr, int len_args, ...) {
  GLenum err = glad_glGetError();
  if (err != GL_NO_ERROR) {
    fprintf(stderr, "ERROR %d (%s) in %s\n", err, glGetError_str(err), name);
    abort();
  }
}

void cleanup() {
  SDL_DestroyWindow(window);
  SDL_GL_DeleteContext(context);
  printf("Goodbye!\n");
}

int main(int argc, const char* argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Failed to initalize SDL!\n");
    return -1;
  }
  
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  
  window = SDL_CreateWindow(argv[0],
                            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            SCREEN_WIDTH, SCREEN_HEIGHT,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
  if (!window) {
    fprintf(stderr, "Failed to create SDL window!\n");
    return -1;
  }
  
  context = SDL_GL_CreateContext(window);
  if (!context) {
    fprintf(stderr, "Failed to create OpenGL context!\n");
    return -1;
  }
  
  if (!gladLoadGL()) {
    fprintf(stderr, "Failed to load GLAD!\n");
    return -1;
  }
  
#ifdef GLAD_DEBUG
  glad_set_pre_callback(pre_gl_call);
#endif
  
  glad_set_post_callback(post_gl_call);
  
  printf("Vendor:   %s\n", glGetString(GL_VENDOR));
  printf("Renderer: %s\n", glGetString(GL_RENDERER));
  printf("Version:  %s\n", glGetString(GL_VERSION));
  printf("GLSL:     %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  
  glClearColor(0.93, 0.93, 0.93, 1.0f);
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glCullFace(GL_BACK);
  
  mat4 proj = mat4_perspective(45.f, .1f, 1000.f, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
  mat4 view = mat4_view_look_at(vec3_new(0.f, 5.f, 10.f),
                                vec3_new(0.f, 2.f, 0.f),
                                vec3_new(0.f, 1.f, 0.f));
  mat4 model = mat4_id();
  
  GLuint shader = load_shader_file("test.vert.glsl", "test.frag.glsl");
  
#define LOAD_PIECE(x) \
obj_t x; \
load_obj(&x, "res/" #x ".obj");
  
  LOAD_PIECE(board);
  LOAD_PIECE(bishop);
  LOAD_PIECE(knight);
  LOAD_PIECE(rook);
  LOAD_PIECE(king);
  LOAD_PIECE(queen);
  
  SDL_bool running = SDL_TRUE;
  SDL_Event e;
  
  Uint32 now = SDL_GetTicks();
  Uint32 then;
  float  delta;
  
  while (running) {
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          running = SDL_FALSE;
          break;
      }
    }
    
    then = now;
    now = SDL_GetTicks();
    delta = (float)(now - then) / 1000.0f;
    
    model = mat4_mul_mat4(model, mat4_rotation_y(DEG2RAD(10.f) * delta));
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(shader);
    
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"),  1, GL_FALSE, &proj.m[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"),  1, GL_FALSE, &view.m[0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"),  1, GL_FALSE, &model.m[0]);
    
    draw_obj(&board);
    
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    SDL_GL_SwapWindow(window);
  }
  
  free_obj(&board);
  free_obj(&bishop);
  free_obj(&knight);
  free_obj(&rook);
  free_obj(&king);
  free_obj(&queen);
  glDeleteProgram(shader);
  
  return 0;
}
