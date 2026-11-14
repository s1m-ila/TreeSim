#include <TreeSim/Tree.h>
#include <TreeSim/Map.h>

static uint64_t g_rngState = []() {
    std::random_device rd;
    return (static_cast<uint64_t>(rd()) << 32) ^ rd();
}();

inline uint32_t fastRand() {
    g_rngState ^= g_rngState >> 12;
    g_rngState ^= g_rngState << 25;
    g_rngState ^= g_rngState >> 27;
    return static_cast<uint32_t>((g_rngState * 0x2545F4914F6CDD1DULL) >> 32);
}

int getRandomInt(int min, int max) {
    uint32_t range = static_cast<uint32_t>(max - min + 1);
    return min + static_cast<int>((static_cast<uint64_t>(fastRand()) * range) >> 32);
}

Tree::Tree(Map* t_map, uint32_t t_index, pos t_pos, Tile* parentTile)
    : map(t_map), index(t_index), mpos(t_pos) {

    col = glm::vec4(
        getRandomInt(20, 100) / 100.0f,
        getRandomInt(50, 100) / 100.0f,
        getRandomInt(20, 100) / 100.0f,
        1.0f
    );

    if (parentTile) {
        energy = parentTile->energy;
        if (parentTile->myGenom) {
            myGenom = parentTile->myGenom;
        } else {
            myGenom = std::make_shared<genom>();
            for (size_t i = 0; i < GENOM_SIZE; ++i) {
                for (size_t j = 0; j < 4; ++j) {
                    (*myGenom)[i][j] = getRandomInt(-1, GENOM_SIZE - 1);
                }
            }
        }

        parentTile->seed = false;
        parentTile->root = true;
        parentTile->parent = index;
        parentTile->energy = 0;
        tiles.push_back(mpos);
        activeTiles.push_back(mpos);
    }
    else {
        life = false;
    }
}

void Tree::tick() {
    if (!map) return;
    if (!life) return;

    if (age >= LIFESPAN || energy < 0) {
        for (const auto& tile : tiles) {
            map->killTile(tile);
        }
        if (energy > 0 && !seedTiles.empty()) {
            float seedEnergy = energy / seedTiles.size();
            for (const auto& tile : seedTiles) {
                auto childGenom = myGenom;

                if (myGenom != nullptr) {
                    bool mutated = false;
                    genom tempGenom;

                    for (size_t i = 0; i < GENOM_SIZE; ++i) {
                        for (size_t j = 0; j < 4; ++j) {
                            if (getRandomInt(1, 100) <= MUTATION_CHANCE) {
                                if (!mutated) {
                                    tempGenom = *myGenom;
                                    mutated = true;
                                }
                                tempGenom[i][j] = getRandomInt(-1, GENOM_SIZE - 1);
                            }
                        }
                    }

                    if (mutated) {
                        childGenom = std::make_shared<genom>(std::move(tempGenom));
                    }
                }

                map->birdTile(tile, {true, true, false, 0, UINT32_MAX, childGenom, seedEnergy});
            }
        }

        life = false;
        return;
    }

    if (activeTiles.empty()) {
        age++;
        return;
    }

    nextActiveTiles.clear();
    nextActiveTiles.reserve(activeTiles.size() * 3);

    for (const auto& tile : activeTiles) {
        Tile* realTile = map->getTile(tile);
        if (!realTile || realTile->seed) continue;

        const auto& genStr = (*myGenom)[realTile->gen % GENOM_SIZE];
        bool ageConvert = (genStr[4] >= GENOM_SIZE / 2 && age >= LIFESPAN - 1);
        if (ageConvert || (genStr[0] < 0 && genStr[1] < 0 && genStr[2] < 0 && genStr[3] < 0)) {
            realTile->seed = true;
            seedTiles.push_back(tile);
        }
        else {
            if (genStr[0] >= 0) {
                pos nextPos = {tile.x > 0 ? tile.x - 1 : map->getWidth() - 1, tile.y};
                if (map->birdBranch(nextPos, static_cast<uint32_t>(genStr[0]), index)) {
                    tiles.push_back(nextPos);
                    nextActiveTiles.push_back(nextPos);
                }
            }
            if (genStr[1] >= 0 && tile.y + 1 < map->getHeight()) {
                pos nextPos = {tile.x, tile.y + 1};
                if (map->birdBranch(nextPos, static_cast<uint32_t>(genStr[1]), index)) {
                    tiles.push_back(nextPos);
                    nextActiveTiles.push_back(nextPos);
                }
            }
            if (genStr[2] >= 0) {
                pos nextPos = {tile.x + 1 < map->getWidth() ? tile.x + 1 : 0, tile.y};
                if (map->birdBranch(nextPos, static_cast<uint32_t>(genStr[2]), index)) {
                    tiles.push_back(nextPos);
                    nextActiveTiles.push_back(nextPos);
                }
            }
            if (genStr[3] >= 0 && tile.y > 0) {
                pos nextPos = {tile.x, tile.y - 1};
                if (map->birdBranch(nextPos, static_cast<uint32_t>(genStr[3]), index)) {
                    tiles.push_back(nextPos);
                    nextActiveTiles.push_back(nextPos);
                }
            }
        }
    }

    activeTiles.swap(nextActiveTiles);
    age++;
}

bool Tree::isLife() const {
    return life;
}
glm::vec4 Tree::getCol() const {
    return col;
}
uint32_t Tree::getIndex() const {
    return index;
}
std::shared_ptr<genom> Tree::getGenom() const {
    return myGenom;
}

int Tree::setCol(glm::vec4 t_col) {
    col = t_col;
    return 0;
}

int Tree::addEnergy(float t_energy) {
    energy += t_energy;
    return 0;
}