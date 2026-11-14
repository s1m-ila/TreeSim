#pragma once

#define MAP_WIDTH 800 // 200
#define MAP_HEIGHT 52 // 40

#define SUN_SOLID 3
#define SUN_LEVELS 6
#define CEIL_LOSE_ENERGY 10

#define GENOM_SIZE 30
#define START_ENERGY 5000
#define LIFESPAN 50//10
#define MUTATION_CHANCE 1
#define FOG_HEIGHT 4 // 6*6 - 10x < 0

struct pos {
    uint32_t x = 0, y = 0;
};
using gen = std::array<int, 5>; // TILE LEFT, TILE UP, TILE RIGHT, TILE DOWN, SEED_IN_END
using genom = std::array<gen, GENOM_SIZE>;