#include <stdio.h>

#include "3rdparty/glad.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "3rdparty/linalgb.h"
#define SLIM_HASH_IMPLEMENTATION
#include "3rdparty/slim_hash.h"
#include "obj.h"
#include "helpers.h"

static const int SCREEN_WIDTH = 640, SCREEN_HEIGHT = 480, FBO_SIZE = 1024;

static SDL_Window* window;
static SDL_GLContext context;

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"
#define BOARD_STEP 6.f
#define BOARD_TOP -21.f
#define DRAG_SPEED 15.f

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

static char grid[64];

SH_GEN_DECL(dict, char, obj_t*);
SH_GEN_HASH_IMPL(dict, char, obj_t*);
static struct dict piece_map;

void fen_to_grid(const char* fen) {
  int cur_row = 0, cur_col = 0;
  for (int i = 0; i < strlen(fen); ++i) {
    char c = fen[i];
    if (c == ' ')
      break;
    
    if (c == '/') {
      cur_row += 1;
      cur_col  = 0;
      continue;
    }
    
    if (c >= '1' && c <= '8') {
      int v = (int)c - 48;
      for (int j = 0; j < v; ++j) {
        grid[cur_row * 8 + cur_col++] = ' ';
      }
    } else {
      grid[cur_row * 8 + cur_col++] = c;
    }
  }
}

int main(int argc, const char* argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Failed to initalize SDL!\n");
    return -1;
  }
  
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
  
  SDL_GL_SetSwapInterval(1);
  
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
  
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POLYGON_SMOOTH);
  glEnable(GL_MULTISAMPLE);
  
  GLuint quad_vao, quad_vbo;
  float quad_verts[] = {
    // positions        // texture Coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
  };
  // setup plane VAO
  glGenVertexArrays(1, &quad_vao);
  glGenBuffers(1, &quad_vbo);
  glBindVertexArray(quad_vao);
  glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), &quad_verts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  
  mat4 proj = mat4_perspective(45.f, .1f, 1000.f, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
  mat4 view = mat4_view_look_at(vec3_new(0.f, 25.f, -45.f),
                                vec3_new(0.f, -2.f, 0.f),
                                vec3_new(0.f, 1.f, 0.f));
  mat4 board_world = mat4_id();
  
  GLuint board_shader = load_shader_file("default.vert.glsl", "board.frag.glsl");
  GLuint piece_shader = load_shader_file("default.vert.glsl", "piece.frag.glsl");
  
  dict_new(&piece_map);
  
#define LOAD_PIECE(x, c) \
obj_t x; \
load_obj(&x, "res/" #x ".obj"); \
if (c) { \
  dict_put(&piece_map, (char)c, &x); \
  dict_put(&piece_map, (char)(c - 32), &x); \
}
  
  LOAD_PIECE(board,  0);
  LOAD_PIECE(pawn,   'p');
  LOAD_PIECE(bishop, 'b');
  LOAD_PIECE(knight, 'n');
  LOAD_PIECE(rook,   'r');
  LOAD_PIECE(king,   'k');
  LOAD_PIECE(queen,  'q');
  
  fen_to_grid(DEFAULT_FEN);
  
  SDL_bool running = SDL_TRUE, dragging = SDL_FALSE;
  SDL_Event e;
  
  Uint32 now = SDL_GetTicks();
  Uint32 then;
  float  delta;
  
  while (running) {
    then = now;
    now = SDL_GetTicks();
    delta = (float)(now - then) / 1000.0f;
    
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          running = SDL_FALSE;
          break;
        case SDL_MOUSEBUTTONDOWN:
          if (e.button.button == SDL_BUTTON_LEFT)
            dragging = SDL_TRUE;
          break;
        case SDL_MOUSEBUTTONUP:
          if (e.button.button == SDL_BUTTON_LEFT)
            dragging = SDL_FALSE;
          break;
        case SDL_MOUSEMOTION:
          if (dragging) {
            view = mat4_mul_mat4(view, mat4_rotation_y(DEG2RAD((float)e.motion.xrel * DRAG_SPEED) * delta));
          }
          break;
      }
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(board_shader);
    
    glUniformMatrix4fv(glGetUniformLocation(board_shader, "projection"),  1, GL_FALSE, &proj.m[0]);
    glUniformMatrix4fv(glGetUniformLocation(board_shader, "view"),  1, GL_FALSE, &view.m[0]);
    glUniformMatrix4fv(glGetUniformLocation(board_shader, "model"),  1, GL_FALSE, &board_world.m[0]);
    glUniform3f(glGetUniformLocation(board_shader, "viewPos"), view.xw, view.yw, view.zw);
    
    draw_obj(&board);
    
    glUseProgram(piece_shader);
    
    glUniformMatrix4fv(glGetUniformLocation(piece_shader, "projection"),  1, GL_FALSE, &proj.m[0]);
    glUniformMatrix4fv(glGetUniformLocation(piece_shader, "view"),  1, GL_FALSE, &view.m[0]);
    
    mat4 top_left = mat4_mul_mat4(mat4_id(), mat4_translation(vec3_new(-21.f, 0.f, -21.f)));
    for (int i = 0; i < 8; ++i) {
      for (int j = 0; j < 8; ++j) {
        char grid_c = grid[i * 8 + j];
        if (grid_c == ' ')
          continue;
        
        glUniformMatrix4fv(glGetUniformLocation(piece_shader, "model"),  1, GL_FALSE, &top_left.m[0]);
        glUniform3f(glGetUniformLocation(piece_shader, "viewPos"), view.xw, view.yw, view.zw);
        glUniform1i(glGetUniformLocation(piece_shader, "white"), ((int)grid_c < 98));
        
        draw_obj(dict_get(&piece_map, grid_c, NULL));
        top_left = mat4_mul_mat4(top_left, mat4_translation(vec3_new(BOARD_STEP, 0.f, 0.f)));
      }
      top_left = mat4_mul_mat4(mat4_id(), mat4_translation(vec3_new(BOARD_TOP, 0.f, BOARD_TOP + ((i + 1) * BOARD_STEP))));
    }
    
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
  glDeleteVertexArrays(1, &quad_vao);
  glDeleteBuffers(1, &quad_vbo);
  glDeleteProgram(board_shader);
  glDeleteProgram(piece_shader);
  dict_destroy(&piece_map);
  
  return 0;
}
