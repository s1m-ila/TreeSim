#include <TreeSim/Map.h>

int mod(int a, int m) {
    return (a % m + m) % m;
}

Map::Map (bool t_simple) : simple(t_simple) {
    // Constructor
    reset();
}
void Map::reset() {
    generation = 0;
    for (auto& str : map) {
        for (auto& ceil : str) {
            ceil = Tile{};
        }
    }
    /*
    map[MAP_WIDTH / 2][0] = Tile{
        true,
        true,
        false,
        0,
        UINT32_MAX,
        // 1. Геном ТУИ
        std::make_shared<genom>(genom{{
            {-1,  5, -1, -1, 0}, // 0
            {-1, -1, -1, -1, 0}, // 1
            { 2, -1,  2, -1, 0}, // 2
            { 4, -1,  4, -1, GENOM_SIZE - 1}, // 3
            { 3,  1,  3, -1, GENOM_SIZE - 1}, // 4
            { 4,  7,  4, -1, 0}, // 5
            { 2,  7,  2, -1, 0}, // 6
            {-1,  6, -1, -1, 0}  // 7
        }})
    };
    */
    map[MAP_WIDTH / 2][0] = Tile{
        true,
        true,
        false,
        0,
        UINT32_MAX,
        // 3. Геном деревца
        std::make_shared<genom>(genom{{
            { 6,  1,  6, -1, GENOM_SIZE - 1}, // 0
            { 2, -1,  4, -1, GENOM_SIZE - 1}, // 1

            {-1,  3, -1, -1, GENOM_SIZE - 1}, // 2
            { 2, -1, -1, -1, GENOM_SIZE - 1}, // 3

            {-1,  5, -1, -1, GENOM_SIZE - 1}, // 4
            {-1, -1,  4, -1, GENOM_SIZE - 1}, // 5

            {-1, -1, -1, -1, GENOM_SIZE - 1}, // 6
        }})
    };
    trees.clear();
    freeSlots.clear();
}
int Map::growSeed(const pos& seedPos) {
    uint32_t slot;
    if (!freeSlots.empty()) {
        slot = freeSlots.back();
        freeSlots.pop_back();
    } else {
        slot = static_cast<uint32_t>(trees.size());
        trees.emplace_back();
    }
    trees[slot] = std::make_unique<Tree>(this, slot, seedPos, getTile(seedPos));
    return 0;
}

void Map::tick() {
    for (auto& tree : trees) {
        if (tree) tree->tick();
    }

    for (size_t i = 0; i < trees.size(); ++i) {
        if (trees[i] && !trees[i]->isLife()) {
            trees[i].reset();
            freeSlots.push_back(static_cast<uint32_t>(i));
        }
    }

    for (uint32_t x = 0; x < MAP_WIDTH; x++) {
        auto& ceil = map[x][0];

        if (ceil.life && ceil.seed && !getTree(ceil.parent)) {
            bool blocked = false;
            for (int y = 1; y < MAP_HEIGHT; y++) {
                if (map[x][y].life) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) {
                ceil = Tile{};
                continue;
            }
            
            growSeed({x, 0});
        }
    }

    std::vector<Tile> fallingSeeds{};
    std::vector<pos> fallingSeedsPositions{};
    for (uint32_t x = 0; x < MAP_WIDTH; x++) {
        for (uint32_t y = 1; y < MAP_HEIGHT; y++) {
            auto& ceil = map[x][y];
            if (ceil.life && ceil.seed && !getTree(ceil.parent)) {
                fallingSeeds.push_back(std::move(ceil));
                fallingSeedsPositions.push_back({x, y - 1});
                ceil = Tile{};
            }
        }
    }
    for (size_t i = 0; i < fallingSeeds.size(); i++) {
        birdTile(fallingSeedsPositions[i], std::move(fallingSeeds[i]));
    }

    for (auto& str : map) {
        int solid = SUN_SOLID;
        int level = SUN_LEVELS;
        for (auto& ceil : str | std::views::reverse) {
            ceil.light = std::max(solid * level, 0);
            if (ceil.life) {
                auto parent = getTree(ceil.parent);
                if (parent && !simple) parent->addEnergy(ceil.light - CEIL_LOSE_ENERGY);
                level--;
                solid--;
            }
            else if (solid < SUN_SOLID) {
                solid = std::min(solid + 1, SUN_SOLID);
            }
            if (level <= 0) break;
        }
    }
    /*for (auto& str : map) {
        for (auto& ceil : str) {
            ceil.life = rand() & 1;
        }
    }*/
    generation++;
}

Tree* Map::getTree(uint32_t i) {
    if (i >= trees.size()) return nullptr;
    return trees[i].get();
}
int Map::setTile(pos t_pos, Tile tile) {
    t_pos.x = mod(t_pos.x, MAP_WIDTH);
    if (t_pos.x >= MAP_WIDTH || t_pos.y >= MAP_HEIGHT) return -1;
    if (!tile.seed && tile.parent != UINT32_MAX && !getTree(tile.parent)) return -3;
    map[t_pos.x][t_pos.y] = std::move(tile);
    return 0;
}
int Map::birdTile(pos t_pos, Tile tile) {
    t_pos.x = mod(t_pos.x, MAP_WIDTH);
    if (t_pos.x >= MAP_WIDTH || t_pos.y >= MAP_HEIGHT) return -1;
    if (map[t_pos.x][t_pos.y].life) return -2;
    if (!tile.seed && tile.parent != UINT32_MAX && !getTree(tile.parent)) return -3;
    map[t_pos.x][t_pos.y] = std::move(tile);
    return 0;
}
int Map::killTile(pos t_pos) {
    t_pos.x = mod(t_pos.x, MAP_WIDTH);
    if (t_pos.x >= MAP_WIDTH || t_pos.y >= MAP_HEIGHT) return -1;
    map[t_pos.x][t_pos.y] = Tile{};
    return 0;
}
Tile* Map::getTile(pos t_pos) {
    t_pos.x = mod(t_pos.x, MAP_WIDTH);
    if (t_pos.x >= MAP_WIDTH || t_pos.y >= MAP_HEIGHT) return nullptr;
    return &map[t_pos.x][t_pos.y];
}
glm::vec4 Map::getTileCol(pos t_pos, uint32_t mode) {
    t_pos.x = mod(t_pos.x, MAP_WIDTH);
    if (t_pos.x >= MAP_WIDTH || t_pos.y >= MAP_HEIGHT) return {0.0f, 0.0f, 0.0f, 1.0f};
    const auto& tile = map[t_pos.x][t_pos.y];
    mode = mode % 2;
    if (tile.life) {
        float light = tile.light / float(SUN_LEVELS * SUN_SOLID);
        if (mode == 1) return {light, light * 0.5f, 0.0f, 1.0f};
        if (tile.root) return {0.545f, 0.271f, 0.075f, 1.0f};
        if (tile.seed) return {1.0f, 1.0f, 1.0f, 1.0f};
        auto parent = getTree(tile.parent);
        if (parent) return parent->getCol();
        return {0.5f, 0.5f, 0.5f, 1.0f};
    }
    return {0.25f, 0.25f, 0.25f, 1.0f};
}
uint32_t Map::getGeneration() {
    return generation;
}