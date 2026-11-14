#pragma once

#include <memory>
#include <ranges>
#include <array>
#include <random>

#include <glm/glm.hpp>

#include "EvoDefinitions.h"
#include "Tile.h"
#include "Tree.h"

class Map {
public:
    Map(bool t_simple = false);
    void reset();
    void tick();
    Tree* getTree(uint32_t i);
    int growSeed(const pos& seedPos);
    int setTile(pos t_pos, Tile tile);
    int birdTile(pos t_pos, Tile tile);
    bool birdBranch(pos t_pos, uint32_t gen, uint32_t parentIndex);
    int killTile(pos t_pos);
    Tile* getTile(pos t_pos);
    glm::vec4 getTileCol(pos t_pos, uint32_t mode = 0);
    uint32_t getGeneration();
private:
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::uniform_real_distribution<double> dis{0.0, 1.0};
    
    uint32_t generation = 0;

    std::vector<std::unique_ptr<Tree>> trees{};
    std::vector<uint32_t> freeSlots{};
    std::array<std::array<Tile, MAP_HEIGHT>, MAP_WIDTH> map;
    std::array<int, MAP_WIDTH> columnCount{};
    std::vector<pos> fallingSeeds{};
    std::vector<pos> nextFallingSeeds{};
    std::vector<std::pair<pos, Tile>> activeFallingSeeds{};
    bool simple = false;
};
