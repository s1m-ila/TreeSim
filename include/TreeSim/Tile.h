#pragma once

#include "EvoDefinitions.h"
#include <memory>
#include <cstdint>

class Tree;

struct Tile {
    bool life = false;
    bool seed = false;
    bool root = false;
    uint32_t gen = 0;
    uint32_t parent = UINT32_MAX;
    std::shared_ptr<genom> myGenom = nullptr;
    float energy = START_ENERGY;
    int light = 0;
};