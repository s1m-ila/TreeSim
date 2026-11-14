#pragma once

#include <iostream>
#include <array>
#include <vector>
#include <random>
#include <algorithm>

#include <glm/glm.hpp>

#include "EvoDefinitions.h"
#include "Tile.h"

#include <unordered_set>

inline uint32_t hashGenom(const std::array<gen, GENOM_SIZE>& g) {
    uint32_t h = 2166136261u;
    for (const auto& gene : g) {
        for (int v : gene) {
            uint32_t u = static_cast<uint32_t>(v);
            // 4 байта
            for (int b = 0; b < 4; ++b) {
                h ^= (u >> (b * 8)) & 0xFF;
                h *= 16777619u;
            }
        }
    }
    return h;
}

inline glm::vec3 hsv2rgb(float h, float s, float v) {
    float r, g, b;
    int i = int(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        default:r=v; g=p; b=q; break;
    }
    return {r, g, b};
}

inline glm::vec4 genomColor(const std::array<gen, GENOM_SIZE>& g) {
    uint32_t h = hashGenom(g);

    float hue = float(h & 0xFFFF) / 65535.0f;              // 0..1 полный круг
    float satJ = float((h >> 16) & 0xFF) / 255.0f;         // «случайность»
    float valJ = float((h >> 24) & 0xFF) / 255.0f;

    float sat = 0.55f + 0.35f * satJ;   // 0.55..0.90
    float val = 0.60f + 0.35f * valJ;   // 0.60..0.95

    glm::vec3 rgb = hsv2rgb(hue, sat, val);
    return {rgb.r, rgb.g, rgb.b, 1.0f};
}

class Map;

class Tree {
public:
    Tree(Map* t_map = nullptr, uint32_t t_index = UINT32_MAX, pos t_pos = {0, 0}, Tile* parentTile = nullptr);
    void init();
    void tick();
    bool isLife() const;
    glm::vec4 getCol() const;
    uint32_t getIndex() const;
    std::shared_ptr<genom> getGenom() const;
    int setCol(glm::vec4 t_col);
    int addEnergy(float t_energy);
private:
    std::shared_ptr<genom> myGenom = nullptr;
    uint32_t index;
    Map* map = nullptr;
    pos mpos{};
    bool life = true;
    size_t age = 0;
    float energy = 0;
    glm::vec4 col{};
    std::vector<pos> tiles{};
    std::vector<pos> activeTiles{};
    std::vector<pos> seedTiles{};
};