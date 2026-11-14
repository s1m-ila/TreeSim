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
    Map(uint32_t t_width = MAP_WIDTH, uint32_t t_height = MAP_HEIGHT, bool t_simple = false);
    Map(bool t_simple);
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
    uint32_t getWidth() const;
    uint32_t getHeight() const;
    bool saveToFile(const std::string& filepath) const;
    bool loadFromFile(const std::string& filepath);
private:
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::uniform_real_distribution<double> dis{0.0, 1.0};
    
    uint32_t generation = 0;
    uint32_t width = MAP_WIDTH;
    uint32_t height = MAP_HEIGHT;

    std::vector<std::unique_ptr<Tree>> trees{};
    std::vector<uint32_t> freeSlots{};
    std::vector<Tile> map{};
    std::vector<int> columnCount{};
    std::vector<pos> fallingSeeds{};
    std::vector<pos> nextFallingSeeds{};
    std::vector<std::pair<pos, Tile>> activeFallingSeeds{};
    bool simple = false;
};
