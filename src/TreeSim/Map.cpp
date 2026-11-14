#include <TreeSim/Map.h>
#include <fstream>
#include <filesystem>

int mod(int a, int m) {
    int r = a % m;
    return r < 0 ? r + m : r;
}

Map::Map(uint32_t t_width, uint32_t t_height, bool t_simple)
    : width(t_width), height(t_height), simple(t_simple) {
    // Constructor
    map.resize(static_cast<size_t>(width) * height);
    columnCount.resize(width, 0);
    reset();
}

Map::Map(bool t_simple) : Map(MAP_WIDTH, MAP_HEIGHT, t_simple) {
}

void Map::reset() {
    generation = 0;
    columnCount.assign(width, 0);
    fallingSeeds.clear();
    nextFallingSeeds.clear();
    activeFallingSeeds.clear();
    std::fill(map.begin(), map.end(), Tile{});
    map[static_cast<size_t>(width / 2) * height + 0] = Tile{
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
    columnCount[width / 2] = 1;
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

    for (uint32_t x = 0; x < width; x++) {
        auto& ceil = map[static_cast<size_t>(x) * height + 0];

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
            auto& ceil = map[static_cast<size_t>(p.x) * height + p.y];
            if (ceil.life && ceil.seed && ceil.parent == UINT32_MAX) {
                activeFallingSeeds.emplace_back(p, std::move(ceil));
                ceil = Tile{};
            }
        }
        for (auto& [p, seed] : activeFallingSeeds) {
            auto& dest = map[static_cast<size_t>(p.x) * height + (p.y - 1)];
            if (!dest.life) {
                dest = std::move(seed);
                if (p.y - 1 > 0) {
                    nextFallingSeeds.push_back({p.x, p.y - 1});
                }
            } else {
                columnCount[p.x]--;
            }
        }
        fallingSeeds.swap(nextFallingSeeds);
    }

    for (uint32_t x = 0; x < width; ++x) {
        if (columnCount[x] == 0) continue;
        int solid = SUN_SOLID;
        int level = SUN_LEVELS;
        int remainingLife = columnCount[x];
        for (int y = static_cast<int>(height) - 1; y >= 0; --y) {
            auto& ceil = map[static_cast<size_t>(x) * height + y];
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
    if (t_pos.y >= height) return -1;
    if (t_pos.x >= width) {
        int x = static_cast<int>(t_pos.x) % static_cast<int>(width);
        if (x < 0) x += static_cast<int>(width);
        t_pos.x = static_cast<uint32_t>(x);
    }
    if (!tile.seed && tile.parent != UINT32_MAX && !getTree(tile.parent)) return -3;
    size_t idx = static_cast<size_t>(t_pos.x) * height + t_pos.y;
    if (map[idx].life) columnCount[t_pos.x]--;
    if (tile.life) columnCount[t_pos.x]++;
    if (tile.life && tile.seed && tile.parent == UINT32_MAX && t_pos.y > 0) {
        fallingSeeds.push_back(t_pos);
    }
    map[idx] = std::move(tile);
    return 0;
}
int Map::birdTile(pos t_pos, Tile tile) {
    if (t_pos.y >= height) return -1;
    if (t_pos.x >= width) {
        int x = static_cast<int>(t_pos.x) % static_cast<int>(width);
        if (x < 0) x += static_cast<int>(width);
        t_pos.x = static_cast<uint32_t>(x);
    }
    size_t idx = static_cast<size_t>(t_pos.x) * height + t_pos.y;
    if (map[idx].life) return -2;
    if (!tile.seed && tile.parent != UINT32_MAX && !getTree(tile.parent)) return -3;
    if (tile.life) columnCount[t_pos.x]++;
    if (tile.life && tile.seed && tile.parent == UINT32_MAX && t_pos.y > 0) {
        fallingSeeds.push_back(t_pos);
    }
    map[idx] = std::move(tile);
    return 0;
}
bool Map::birdBranch(pos t_pos, uint32_t gen, uint32_t parentIndex) {
    if (t_pos.y >= height) return false;
    if (t_pos.x >= width) {
        int x = static_cast<int>(t_pos.x) % static_cast<int>(width);
        if (x < 0) x += static_cast<int>(width);
        t_pos.x = static_cast<uint32_t>(x);
    }
    size_t idx = static_cast<size_t>(t_pos.x) * height + t_pos.y;
    Tile& cell = map[idx];
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
    if (t_pos.y >= height) return -1;
    if (t_pos.x >= width) {
        int x = static_cast<int>(t_pos.x) % static_cast<int>(width);
        if (x < 0) x += static_cast<int>(width);
        t_pos.x = static_cast<uint32_t>(x);
    }
    size_t idx = static_cast<size_t>(t_pos.x) * height + t_pos.y;
    if (map[idx].life) {
        columnCount[t_pos.x]--;
        map[idx] = Tile{};
    }
    return 0;
}
Tile* Map::getTile(pos t_pos) {
    if (t_pos.y >= height) return nullptr;
    if (t_pos.x >= width) {
        int x = static_cast<int>(t_pos.x) % static_cast<int>(width);
        if (x < 0) x += static_cast<int>(width);
        t_pos.x = static_cast<uint32_t>(x);
    }
    return &map[static_cast<size_t>(t_pos.x) * height + t_pos.y];
}
glm::vec4 Map::getTileCol(pos t_pos, uint32_t mode) {
    if (t_pos.y >= height) return {0.0f, 0.0f, 0.0f, 1.0f};
    if (t_pos.x >= width) {
        int x = static_cast<int>(t_pos.x) % static_cast<int>(width);
        if (x < 0) x += static_cast<int>(width);
        t_pos.x = static_cast<uint32_t>(x);
    }
    const auto& tile = map[static_cast<size_t>(t_pos.x) * height + t_pos.y];
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
uint32_t Map::getWidth() const {
    return width;
}
uint32_t Map::getHeight() const {
    return height;
}

bool Map::saveToFile(const std::string& filepath) const {
    std::filesystem::create_directories(std::filesystem::path(filepath).parent_path());
    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) return false;

    char magic[8] = {'T', 'R', 'E', 'E', 'S', 'I', 'M', '\0'};
    out.write(magic, 8);
    uint32_t version = 1;
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));

    out.write(reinterpret_cast<const char*>(&width), sizeof(width));
    out.write(reinterpret_cast<const char*>(&height), sizeof(height));
    out.write(reinterpret_cast<const char*>(&generation), sizeof(generation));
    uint8_t simp = simple ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&simp), sizeof(simp));

    uint32_t colSize = static_cast<uint32_t>(columnCount.size());
    out.write(reinterpret_cast<const char*>(&colSize), sizeof(colSize));
    if (colSize > 0) {
        out.write(reinterpret_cast<const char*>(columnCount.data()), colSize * sizeof(int));
    }

    uint32_t mapSize = static_cast<uint32_t>(map.size());
    out.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));
    for (size_t i = 0; i < mapSize; ++i) {
        const auto& tile = map[i];
        uint8_t flags = (tile.life ? 1 : 0) |
                        ((tile.seed ? 1 : 0) << 1) |
                        ((tile.root ? 1 : 0) << 2) |
                        ((tile.myGenom != nullptr ? 1 : 0) << 3);
        out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
        out.write(reinterpret_cast<const char*>(&tile.gen), sizeof(tile.gen));
        out.write(reinterpret_cast<const char*>(&tile.parent), sizeof(tile.parent));
        out.write(reinterpret_cast<const char*>(&tile.energy), sizeof(tile.energy));
        out.write(reinterpret_cast<const char*>(&tile.light), sizeof(tile.light));
        if (tile.myGenom) {
            out.write(reinterpret_cast<const char*>(tile.myGenom->data()), sizeof(genom));
        }
    }

    uint32_t treeCount = static_cast<uint32_t>(trees.size());
    out.write(reinterpret_cast<const char*>(&treeCount), sizeof(treeCount));
    for (size_t i = 0; i < treeCount; ++i) {
        uint8_t exists = (trees[i] != nullptr) ? 1 : 0;
        out.write(reinterpret_cast<const char*>(&exists), sizeof(exists));
        if (exists) {
            const auto& tr = *trees[i];
            out.write(reinterpret_cast<const char*>(&tr.index), sizeof(tr.index));
            out.write(reinterpret_cast<const char*>(&tr.mpos), sizeof(tr.mpos));
            uint8_t life = tr.life ? 1 : 0;
            out.write(reinterpret_cast<const char*>(&life), sizeof(life));
            uint64_t age = tr.age;
            out.write(reinterpret_cast<const char*>(&age), sizeof(age));
            out.write(reinterpret_cast<const char*>(&tr.energy), sizeof(tr.energy));
            out.write(reinterpret_cast<const char*>(&tr.col), sizeof(tr.col));

            uint8_t hasGenom = (tr.myGenom != nullptr) ? 1 : 0;
            out.write(reinterpret_cast<const char*>(&hasGenom), sizeof(hasGenom));
            if (hasGenom) {
                out.write(reinterpret_cast<const char*>(tr.myGenom->data()), sizeof(genom));
            }

            uint32_t nTiles = static_cast<uint32_t>(tr.tiles.size());
            out.write(reinterpret_cast<const char*>(&nTiles), sizeof(nTiles));
            if (nTiles > 0) {
                out.write(reinterpret_cast<const char*>(tr.tiles.data()), nTiles * sizeof(pos));
            }

            uint32_t nActive = static_cast<uint32_t>(tr.activeTiles.size());
            out.write(reinterpret_cast<const char*>(&nActive), sizeof(nActive));
            if (nActive > 0) {
                out.write(reinterpret_cast<const char*>(tr.activeTiles.data()), nActive * sizeof(pos));
            }

            uint32_t nSeed = static_cast<uint32_t>(tr.seedTiles.size());
            out.write(reinterpret_cast<const char*>(&nSeed), sizeof(nSeed));
            if (nSeed > 0) {
                out.write(reinterpret_cast<const char*>(tr.seedTiles.data()), nSeed * sizeof(pos));
            }
        }
    }

    uint32_t freeCount = static_cast<uint32_t>(freeSlots.size());
    out.write(reinterpret_cast<const char*>(&freeCount), sizeof(freeCount));
    if (freeCount > 0) {
        out.write(reinterpret_cast<const char*>(freeSlots.data()), freeCount * sizeof(uint32_t));
    }

    uint32_t fallCount = static_cast<uint32_t>(fallingSeeds.size());
    out.write(reinterpret_cast<const char*>(&fallCount), sizeof(fallCount));
    if (fallCount > 0) {
        out.write(reinterpret_cast<const char*>(fallingSeeds.data()), fallCount * sizeof(pos));
    }

    return true;
}

bool Map::loadFromFile(const std::string& filepath) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in.is_open()) return false;

    char magic[8];
    in.read(magic, 8);
    if (std::string(magic, 7) != "TREESIM") return false;

    uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1) return false;

    in.read(reinterpret_cast<char*>(&width), sizeof(width));
    in.read(reinterpret_cast<char*>(&height), sizeof(height));
    in.read(reinterpret_cast<char*>(&generation), sizeof(generation));
    uint8_t simp = 0;
    in.read(reinterpret_cast<char*>(&simp), sizeof(simp));
    simple = (simp != 0);

    uint32_t colSize = 0;
    in.read(reinterpret_cast<char*>(&colSize), sizeof(colSize));
    columnCount.resize(colSize);
    if (colSize > 0) {
        in.read(reinterpret_cast<char*>(columnCount.data()), colSize * sizeof(int));
    }

    uint32_t mapSize = 0;
    in.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));
    map.clear();
    map.resize(mapSize);
    for (size_t i = 0; i < mapSize; ++i) {
        auto& tile = map[i];
        uint8_t flags = 0;
        in.read(reinterpret_cast<char*>(&flags), sizeof(flags));
        tile.life = (flags & 1) != 0;
        tile.seed = (flags & 2) != 0;
        tile.root = (flags & 4) != 0;
        bool hasGenom = (flags & 8) != 0;

        in.read(reinterpret_cast<char*>(&tile.gen), sizeof(tile.gen));
        in.read(reinterpret_cast<char*>(&tile.parent), sizeof(tile.parent));
        in.read(reinterpret_cast<char*>(&tile.energy), sizeof(tile.energy));
        in.read(reinterpret_cast<char*>(&tile.light), sizeof(tile.light));
        if (hasGenom) {
            tile.myGenom = std::make_shared<genom>();
            in.read(reinterpret_cast<char*>(tile.myGenom->data()), sizeof(genom));
        } else {
            tile.myGenom = nullptr;
        }
    }

    uint32_t treeCount = 0;
    in.read(reinterpret_cast<char*>(&treeCount), sizeof(treeCount));
    trees.clear();
    trees.resize(treeCount);
    for (size_t i = 0; i < treeCount; ++i) {
        uint8_t exists = 0;
        in.read(reinterpret_cast<char*>(&exists), sizeof(exists));
        if (exists) {
            auto tr = std::make_unique<Tree>();
            tr->map = this;
            in.read(reinterpret_cast<char*>(&tr->index), sizeof(tr->index));
            in.read(reinterpret_cast<char*>(&tr->mpos), sizeof(tr->mpos));
            uint8_t life = 0;
            in.read(reinterpret_cast<char*>(&life), sizeof(life));
            tr->life = (life != 0);
            uint64_t age = 0;
            in.read(reinterpret_cast<char*>(&age), sizeof(age));
            tr->age = age;
            in.read(reinterpret_cast<char*>(&tr->energy), sizeof(tr->energy));
            in.read(reinterpret_cast<char*>(&tr->col), sizeof(tr->col));

            uint8_t hasGenom = 0;
            in.read(reinterpret_cast<char*>(&hasGenom), sizeof(hasGenom));
            if (hasGenom) {
                tr->myGenom = std::make_shared<genom>();
                in.read(reinterpret_cast<char*>(tr->myGenom->data()), sizeof(genom));
            } else {
                tr->myGenom = nullptr;
            }

            uint32_t nTiles = 0;
            in.read(reinterpret_cast<char*>(&nTiles), sizeof(nTiles));
            tr->tiles.resize(nTiles);
            if (nTiles > 0) {
                in.read(reinterpret_cast<char*>(tr->tiles.data()), nTiles * sizeof(pos));
            }

            uint32_t nActive = 0;
            in.read(reinterpret_cast<char*>(&nActive), sizeof(nActive));
            tr->activeTiles.resize(nActive);
            if (nActive > 0) {
                in.read(reinterpret_cast<char*>(tr->activeTiles.data()), nActive * sizeof(pos));
            }

            uint32_t nSeed = 0;
            in.read(reinterpret_cast<char*>(&nSeed), sizeof(nSeed));
            tr->seedTiles.resize(nSeed);
            if (nSeed > 0) {
                in.read(reinterpret_cast<char*>(tr->seedTiles.data()), nSeed * sizeof(pos));
            }

            trees[i] = std::move(tr);
        } else {
            trees[i] = nullptr;
        }
    }

    uint32_t freeCount = 0;
    in.read(reinterpret_cast<char*>(&freeCount), sizeof(freeCount));
    freeSlots.resize(freeCount);
    if (freeCount > 0) {
        in.read(reinterpret_cast<char*>(freeSlots.data()), freeCount * sizeof(uint32_t));
    }

    uint32_t fallCount = 0;
    in.read(reinterpret_cast<char*>(&fallCount), sizeof(fallCount));
    fallingSeeds.resize(fallCount);
    if (fallCount > 0) {
        in.read(reinterpret_cast<char*>(fallingSeeds.data()), fallCount * sizeof(pos));
    }
    nextFallingSeeds.clear();
    activeFallingSeeds.clear();

    return true;
}