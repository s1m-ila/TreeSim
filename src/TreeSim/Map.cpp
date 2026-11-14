#include <TreeSim/Map.h>

int mod(int a, int m) {
    int r = a % m;
    return r < 0 ? r + m : r;
}

Map::Map (bool t_simple) : simple(t_simple) {
    // Constructor
    reset();
}
void Map::reset() {
    generation = 0;
    columnCount.fill(0);
    fallingSeeds.clear();
    nextFallingSeeds.clear();
    activeFallingSeeds.clear();
    for (auto& str : map) {
        for (auto& ceil : str) {
            ceil = Tile{};
        }
    }
    map[MAP_WIDTH / 2][0] = Tile{
        true,
        true,
        false,
        0,
        UINT32_MAX,
        std::make_shared<genom>(genom{{
            { 6,  7,  6, -1, GENOM_SIZE - 1}, // 0
            { 2, -1,  4, -1, GENOM_SIZE - 1}, // 1

            {-1,  3, -1, -1, GENOM_SIZE - 1}, // 2
            { 2, -1, -1, -1, GENOM_SIZE - 1}, // 3

            {-1,  5, -1, -1, GENOM_SIZE - 1}, // 4
            {-1, -1,  4, -1, GENOM_SIZE - 1}, // 5

            {-1, -1, -1, -1, GENOM_SIZE - 1}, // 6

            {-1,  8, -1, -1, GENOM_SIZE - 1}, // 7
            {-1,  9, -1, -1, GENOM_SIZE - 1}, // 8
            {-1,  1, -1, -1, GENOM_SIZE - 1}, // 9
        }})
    };
    columnCount[MAP_WIDTH / 2] = 1;
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
    for (size_t i = 0; i < trees.size(); ++i) {
        if (trees[i]) {
            trees[i]->tick();
            if (!trees[i]->isLife()) {
                trees[i].reset();
                freeSlots.push_back(static_cast<uint32_t>(i));
            }
        }
    }

    for (uint32_t x = 0; x < MAP_WIDTH; x++) {
        auto& ceil = map[x][0];

        if (ceil.life && ceil.seed && (ceil.parent == UINT32_MAX || !getTree(ceil.parent))) {
            if (columnCount[x] > 1) {
                ceil = Tile{};
                columnCount[x]--;
                continue;
            }
            growSeed({x, 0});
        }
    }

    if (!fallingSeeds.empty()) {
        nextFallingSeeds.clear();
        activeFallingSeeds.clear();
        for (const auto& p : fallingSeeds) {
            auto& ceil = map[p.x][p.y];
            if (ceil.life && ceil.seed && ceil.parent == UINT32_MAX) {
                activeFallingSeeds.emplace_back(p, std::move(ceil));
                ceil = Tile{};
            }
        }
        for (auto& [p, seed] : activeFallingSeeds) {
            if (!map[p.x][p.y - 1].life) {
                map[p.x][p.y - 1] = std::move(seed);
                if (p.y - 1 > 0) {
                    nextFallingSeeds.push_back({p.x, p.y - 1});
                }
            } else {
                columnCount[p.x]--;
            }
        }
        fallingSeeds.swap(nextFallingSeeds);
    }

    for (uint32_t x = 0; x < MAP_WIDTH; ++x) {
        if (columnCount[x] == 0) continue;
        int solid = SUN_SOLID;
        int level = SUN_LEVELS;
        int remainingLife = columnCount[x];
        for (int y = MAP_HEIGHT - 1; y >= 0; --y) {
            auto& ceil = map[x][y];
            if (y < FOG_HEIGHT) ceil.light = 0;
            else ceil.light = std::max(solid * level, 0);
            if (ceil.life) {
                if (!simple && ceil.parent != UINT32_MAX) {
                    Tree* parent = getTree(ceil.parent);
                    if (parent) parent->addEnergy(ceil.light - CEIL_LOSE_ENERGY);
                }
                level--;
                solid--;
                remainingLife--;
                if (remainingLife == 0 || level <= 0) break;
            }
            else if (solid < SUN_SOLID) {
                solid++;
            }
        }
    }
    generation++;
}

Tree* Map::getTree(uint32_t i) {
    if (i >= trees.size()) return nullptr;
    return trees[i].get();
}
int Map::setTile(pos t_pos, Tile tile) {
    if (t_pos.y >= MAP_HEIGHT) return -1;
    if (t_pos.x >= MAP_WIDTH) {
        int x = static_cast<int>(t_pos.x) % MAP_WIDTH;
        if (x < 0) x += MAP_WIDTH;
        t_pos.x = static_cast<uint32_t>(x);
    }
    if (!tile.seed && tile.parent != UINT32_MAX && !getTree(tile.parent)) return -3;
    if (map[t_pos.x][t_pos.y].life) columnCount[t_pos.x]--;
    if (tile.life) columnCount[t_pos.x]++;
    if (tile.life && tile.seed && tile.parent == UINT32_MAX && t_pos.y > 0) {
        fallingSeeds.push_back(t_pos);
    }
    map[t_pos.x][t_pos.y] = std::move(tile);
    return 0;
}
int Map::birdTile(pos t_pos, Tile tile) {
    if (t_pos.y >= MAP_HEIGHT) return -1;
    if (t_pos.x >= MAP_WIDTH) {
        int x = static_cast<int>(t_pos.x) % MAP_WIDTH;
        if (x < 0) x += MAP_WIDTH;
        t_pos.x = static_cast<uint32_t>(x);
    }
    if (map[t_pos.x][t_pos.y].life) return -2;
    if (!tile.seed && tile.parent != UINT32_MAX && !getTree(tile.parent)) return -3;
    if (tile.life) columnCount[t_pos.x]++;
    if (tile.life && tile.seed && tile.parent == UINT32_MAX && t_pos.y > 0) {
        fallingSeeds.push_back(t_pos);
    }
    map[t_pos.x][t_pos.y] = std::move(tile);
    return 0;
}
bool Map::birdBranch(pos t_pos, uint32_t gen, uint32_t parentIndex) {
    if (t_pos.y >= MAP_HEIGHT) return false;
    if (t_pos.x >= MAP_WIDTH) {
        int x = static_cast<int>(t_pos.x) % MAP_WIDTH;
        if (x < 0) x += MAP_WIDTH;
        t_pos.x = static_cast<uint32_t>(x);
    }
    Tile& cell = map[t_pos.x][t_pos.y];
    if (cell.life) return false;

    cell.life = true;
    cell.seed = false;
    cell.root = false;
    cell.gen = gen;
    cell.parent = parentIndex;
    cell.myGenom = nullptr;
    cell.energy = START_ENERGY;
    cell.light = 0;
    columnCount[t_pos.x]++;
    return true;
}
int Map::killTile(pos t_pos) {
    if (t_pos.y >= MAP_HEIGHT) return -1;
    if (t_pos.x >= MAP_WIDTH) {
        int x = static_cast<int>(t_pos.x) % MAP_WIDTH;
        if (x < 0) x += MAP_WIDTH;
        t_pos.x = static_cast<uint32_t>(x);
    }
    if (map[t_pos.x][t_pos.y].life) {
        columnCount[t_pos.x]--;
        map[t_pos.x][t_pos.y] = Tile{};
    }
    return 0;
}
Tile* Map::getTile(pos t_pos) {
    if (t_pos.y >= MAP_HEIGHT) return nullptr;
    if (t_pos.x >= MAP_WIDTH) {
        int x = static_cast<int>(t_pos.x) % MAP_WIDTH;
        if (x < 0) x += MAP_WIDTH;
        t_pos.x = static_cast<uint32_t>(x);
    }
    return &map[t_pos.x][t_pos.y];
}
glm::vec4 Map::getTileCol(pos t_pos, uint32_t mode) {
    if (t_pos.y >= MAP_HEIGHT) return {0.0f, 0.0f, 0.0f, 1.0f};
    if (t_pos.x >= MAP_WIDTH) {
        int x = static_cast<int>(t_pos.x) % MAP_WIDTH;
        if (x < 0) x += MAP_WIDTH;
        t_pos.x = static_cast<uint32_t>(x);
    }
    const auto& tile = map[t_pos.x][t_pos.y];
    mode = mode % 2;
    glm::vec4 col = {0.25f, 0.25f, 0.25f, 1.0f};
    if (tile.life) {
        if (mode == 1) {
            float light = tile.light / float(SUN_LEVELS * SUN_SOLID);
            col = {light, light * 0.5f, 0.0f, 1.0f};
        }
        else {
            if (tile.root) col = {0.545f, 0.271f, 0.075f, 1.0f};
            else if (tile.seed) col = {1.0f, 1.0f, 1.0f, 1.0f};
            else {
                auto parent = getTree(tile.parent);
                if (parent) col = parent->getCol();
                else col = {0.5f, 0.5f, 0.5f, 1.0f};
            }
        }
    }
    if (t_pos.y < FOG_HEIGHT) col += glm::vec4{0.05f, 0.05f, 0.2f, 0.0f};
    return col;
}
uint32_t Map::getGeneration() {
    return generation;
}