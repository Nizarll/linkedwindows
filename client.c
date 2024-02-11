#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define magnitude(v) (sqrt((v.x) * (v.x) + (v.y) * (v.y)))
#define v2_sub(v1, v2) ((Vector2){(v1.x) - (v2.x), (v1.y) - (v2.y)})
#define v2_sum(v1, v2) (v2_sub(v1, (Vector2){-(v2.x), -(v2.y)}))
#define magnitude(v) (sqrt((v.x) * (v.x) + (v.y) * (v.y)))
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

#define WIN_WIDTH 800
#define WIN_HEIGHT 450
#define PORT 3000
#define ADDRESS "127.0.0.1"

typedef int Handle;
typedef struct sockaddr_in Address;

Vector2 vector2, mainwin_vector2 = {.0f};

void render_window() {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(WIN_WIDTH, WIN_HEIGHT, "super cool window");

  Camera2D camera = {0};
  camera.zoom = 1.0f;
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    vector2 = GetWindowPosition();
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode2D(camera);
    Vector2 center = (Vector2){.x = WIN_WIDTH / 2, .y = WIN_HEIGHT / 2};
    Vector2 direction = v2_sub(mainwin_vector2, vector2);
    float maxX, maxY;
    if (fabs(direction.x) > fabs(direction.y)) {
      maxX = direction.x > 0 ? GetScreenWidth() : 0;
      maxY = center.y + direction.y * (maxX - center.x) / direction.x;
    } else {
      maxY = direction.y > 0 ? GetScreenHeight() : 0;
      maxX = center.x + direction.x * (maxY - center.y) / direction.y;
    }

    // Draw the line from the center of the window to the intersection point
    DrawLineV(center, (Vector2){maxX, maxY}, RED);
    DrawCircle(WIN_WIDTH / 2, WIN_HEIGHT / 2, 30, RED);
    EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  pthread_exit(NULL);
}

int main() {

  Handle socket_handle = socket(AF_INET, SOCK_STREAM, 0);
  Address serv_address = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = inet_addr(ADDRESS),
      .sin_port = htons(PORT),
  };
  if (socket_handle < 0) {
    err("Error during socket creation !");
    exit(EXIT_FAILURE);
  }
  ok("socket successfully created !");
  if (connect(socket_handle, (struct sockaddr *)&serv_address,
              sizeof(serv_address)) < 0) {
    err("Connection to %s server failed !", ADDRESS);
    exit(EXIT_FAILURE);
  }
  ok("successfully connected to %s !", ADDRESS);

  pthread_t t_id;
  pthread_create(&t_id, NULL, (void *)render_window, NULL);
  for (;;) {
    if (send(socket_handle, (void *)&vector2, sizeof(Vector2), 0) <= 0) {
      err("could not the buffersend to the socket %d", errno);
      exit(EXIT_FAILURE);
    }
    ok("sent message {%.2f, %.2f} to server", vector2.x, vector2.y);
    if (recv(socket_handle, &mainwin_vector2, sizeof(Vector2), 0) == 0) {
      err("Receive failed");
      exit(EXIT_FAILURE);
    }
    ok("received message : {%2f, %.2f} to server", mainwin_vector2.x,
       mainwin_vector2.y);
    milli_sleep(10);
  }

  return 0; // Added for completeness
}
