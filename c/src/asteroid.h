#ifndef ASTEROID_H
#define ASTEROID_H

#include <SDL.h>
#include "f22.h"
#include "config.h"
#include "wave.h"
#include "player.h"

#define MAX_ASTEROID_POINTS 22
#define MAX_ASTEROIDS 40
#define ASTEROID_BASE_SIZE 40
#define MIN_ASTEROID_SCALE 0.6f
#define MAX_ASTEROID_SCALE 4.4f
#define ASTEROID_SPAWN_BUFFER 150  // min distance from ghost path
#define MIN_ASTEROID_SPACING 60   // min distance between asteroids

typedef struct {
    F22 x;
    F22 y;
    float scale;
    float rotation;
    float rotation_speed;
    bool active;
    SDL_Point points[32];        // increased for more detail
    SDL_Point craters[32];       // new array for crater details
    int num_points;
    int num_crater_points;
} Asteroid;

typedef struct {
    Asteroid asteroids[MAX_ASTEROIDS];
    SDL_Point base_shape[MAX_ASTEROID_POINTS];
    float spawn_timer;
} AsteroidSystem;

AsteroidSystem asteroid_system_init(void);
void asteroid_system_update(AsteroidSystem* system, const WaveGenerator* wave);
void asteroid_system_render(const AsteroidSystem* system, SDL_Renderer* renderer, F22 camera_y_offset);
bool asteroid_system_check_collision(const AsteroidSystem* system, const Player* player);
static void spawn_asteroid(AsteroidSystem* system, const WaveGenerator* wave, bool spawn_above, float layer_multiplier);

#endif
