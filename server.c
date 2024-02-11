#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define milli_sleep(t)                                                         \
  do {                                                                         \
    struct timespec req, rem;                                                  \
    req.tv_sec = t / 1000;                                                     \
    req.tv_nsec = (t % 1000) * 1000000;                                        \
    while (nanosleep(&req, &rem) == -1) {                                      \
      req = rem;                                                               \
    }                                                                          \
  } while (0)

#define warn(msg, ...)                                                         \
  printf("\x1b[38;2;240;190;100m"                                              \
         "[\u26A0] | " msg "\x1b[0m\n",                                        \
         ##__VA_ARGS__);

#define err(msg, ...)                                                          \
  printf("\x1B[33m"                                                            \
         "\x1b[38;2;255;100;100m"                                              \
         "[!] | " msg "\x1B[0m\n",                                             \
         ##__VA_ARGS__)

#define ok(msg, ...)                                                           \
  printf("\x1B[92m"                                                            \
         "[\u2705] | " msg "\x1B[0m\n",                                        \
         ##__VA_ARGS__)

#define info_log(msg, ...) printf("[>] info | " msg "\n", ##__VA_ARGS__)

#define magnitude(v) (sqrt((v.x) * (v.x) + (v.y) * (v.y)))
#define v2_unit(v) ((Vector2){(v.x) / magnitude(v), (v.y) / magnitude(v)})
#define v2_sub(v1, v2) (Vector2Subtract(v1, v2))
#define v2_sum(v1, v2) (Vector2Add(v1, v2))

#define WIN_WIDTH 800
#define WIN_HEIGHT 450
#define PORT 3000
#define ADDRESS "127.0.0.1"

// TYPE DEFINITIONS
typedef int Handle;
typedef struct sockaddr_in Address;
typedef int boolean;

Vector2 vector2, secwin_vector2;

/*
 * x cos 0 - y sin 0
 * x sin 0 + y cos 0
 */
struct Input {
  Vector2 mousePressPos, lastMousePos;
};

struct Ball {
  u_int8_t radius;
  bool is_clicked;
  Vector2 position;
  float velocity;
  struct Input
      input; // i know i was just too lazy to Initialize a new input struct
};

void *render_window(void *arg) {
  // Initialize window and resources
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(WIN_WIDTH, WIN_HEIGHT, "Super Cool Window");
  Camera2D camera = {0};
  camera.zoom = 1.0f;
  SetTargetFPS(60);

  struct Ball ball = {.velocity = 0.0f,
                      .radius = 15,
                      .position =
                          (Vector2){.x = WIN_WIDTH / 2, .y = WIN_HEIGHT / 2}};
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode2D(camera);

    // Calculate the direction vector from the center of the window to
    // secwin_vector2
    Vector2 winpos = GetWindowPosition();
    Vector2 center = (Vector2){.x = WIN_WIDTH / 2, .y = WIN_HEIGHT / 2};

    // Draw the line from the center of the window to the intersection point
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
      info_log("m1 clicked");

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && ball.is_clicked) {
      if (ball.input.lastMousePos.x != GetMousePosition().x &&
          ball.input.lastMousePos.y != GetMousePosition().y) {
        ball.position = GetMousePosition();
        ball.velocity = 1.0f;
      } else {
        ball.velocity = .0f;
      }
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      ball.is_clicked =
          fabs(magnitude(v2_sub(ball.position, GetMousePosition()))) <= 28;
      if (ball.is_clicked)
        ball.input.mousePressPos = GetMousePosition();
    } else {
      ball.is_clicked = false;
      if (ball.velocity > 0) {
        info_log("%f", ball.velocity);
        Vector2 dir = (Vector2){
            .x = log2(magnitude(
                     v2_sub(ball.position, ball.input.mousePressPos))) *
                 v2_unit(v2_sub(ball.position, ball.input.mousePressPos)).x,
            .y = log2(magnitude(
                     v2_sub(ball.position, ball.input.mousePressPos))) *
                 v2_unit(v2_sub(ball.position, ball.input.mousePressPos)).y,
        };
        ball.position = (Vector2){
            .x = ball.position.x + ball.velocity * dir.x,
            .y = ball.position.y + ball.velocity * dir.y,
        };
        ball.velocity *= .95;
      }
    }
    vector2 = (Vector2){
        .x = winpos.x + ball.position.x - WIN_WIDTH / 2,
        .y = winpos.y + ball.position.y - WIN_HEIGHT / 2,
    };
    Vector2 direction = v2_sub(secwin_vector2, vector2);
    float maxX, maxY;
    if (fabs(direction.x) > fabs(direction.y)) {
      maxX = direction.x > 0 ? GetScreenWidth() : 0;
      maxY = ball.position.y +
             direction.y * (maxX - ball.position.x) / direction.x;
    } else {
      maxY = direction.y > 0 ? GetScreenHeight() : 0;
      maxX = ball.position.x +
             direction.x * (maxY - ball.position.y) / direction.y;
    }
    ball.input.lastMousePos = GetMousePosition();
    if (IsKeyDown(KEY_ENTER))
      ball.position = center;
    DrawLineV(ball.position, (Vector2){maxX, maxY}, RED);
    DrawCircleV(ball.position, (float)ball.radius, RED);
    EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return NULL;
}
// address: %s, port: %d\n", inet_ntoa(serv_address.sin_addr),
// ntohs(serv_address.sin_port));

int main() {
  Handle socket_handle = socket(AF_INET, SOCK_STREAM, 0);
  Address serv_address = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = inet_addr(ADDRESS),
      .sin_port = htons(PORT),
  };
  if (socket_handle < 0) {
    err("socket creation failed !");
    exit(EXIT_FAILURE);
  }
  ok("socket successfully created !");
  if (bind(socket_handle, (struct sockaddr *)&serv_address,
           sizeof(serv_address)) == -1) {
    err("binding socket to %s server failed ! errno %d", ADDRESS, errno);
    exit(EXIT_FAILURE);
  }
  if (listen(socket_handle, 100) == -1) {
    err("listening to socket failed !");
    exit(EXIT_FAILURE);
  }
  int clsocket_handle = accept(socket_handle, 0, 0);
  if (clsocket_handle < 0) {
    err("could not accept client socket ! errno %d", errno);
    exit(EXIT_FAILURE);
  }
  ok("successfully connected to %s !", ADDRESS);

  pthread_t t_id;
  pthread_create(&t_id, NULL, (void *)render_window, NULL);
  for (;;) {
    if (recv(clsocket_handle, (void *)&secwin_vector2, sizeof(Vector2), 0) <=
        0) {
      warn("couldnt receive message {%.0f, %.0f}", secwin_vector2.x,
           secwin_vector2.y);
      return 0;
    }
    ok("received {%.0f, %.0f} from client", vector2.x, vector2.y);
    if (send(clsocket_handle, (void *)&vector2, sizeof(Vector2), 0) < 0) {
      err("could not the buffersend to the socket %.0f", errno);
      exit(EXIT_FAILURE);
    }
    ok("sent message {%f, %f} to client !", vector2.x, vector2.y);
    milli_sleep(10);
  }
}
