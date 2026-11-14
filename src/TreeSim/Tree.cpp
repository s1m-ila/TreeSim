#include <TreeSim/Tree.h>
#include <TreeSim/Map.h>

// Вспомогательная функция для генерации случайных целых чисел
int getRandomInt(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

Tree::Tree(Map* t_map, uint32_t t_index, pos t_pos, Tile* parentTile)
    : map(t_map), index(t_index), mpos(t_pos) {
    
    // Настраиваем диапазоны значений генов и шанс мутации под вашу симуляцию
    const int MIN_GENE_VAL = -1;
    const int MAX_GENE_VAL = GENOM_SIZE - 1;

    // 1. Первое поколение: заполняем геном случайными числами
    myGenom = std::make_shared<genom>();

    for (size_t i = 0; i < GENOM_SIZE; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            (*myGenom)[i][j] = getRandomInt(MIN_GENE_VAL, MAX_GENE_VAL);
        }
    }
    // Случайный стартовый цвет
    col = glm::vec4(
        getRandomInt(20, 100) / 100.0f,
        getRandomInt(50, 100) / 100.0f,
        getRandomInt(20, 100) / 100.0f,
        1.0f
    );

    // Задаем цвет
    //col = genomColor(*myGenom);

    if (parentTile) {
        //*parentTile = Tile{index, true, 0, false, 0, true};
        energy = parentTile->energy;
        if (parentTile->myGenom) myGenom = parentTile->myGenom;

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
    //std::clog << energy << std::endl;
    if (age >= LIFESPAN || energy < 0) {
        for (const auto& tile : tiles) {
            map->killTile(tile);
        }
        if (energy > 0) {
            for (const auto& tile : seedTiles) {
                auto childGenom = myGenom;
                    
                if (myGenom != nullptr) {
                    bool mutated = false;
                    genom tempGenom = *myGenom; 
                    
                    for (size_t i = 0; i < GENOM_SIZE; ++i) {
                        for (size_t j = 0; j < 4; ++j) {
                            if (getRandomInt(1, 100) <= MUTATION_CHANCE) {
                                tempGenom[i][j] = getRandomInt(-1, GENOM_SIZE - 1);
                                mutated = true;
                            }
                        }
                    }

                    if (mutated) {
                        childGenom = std::make_shared<std::remove_reference_t<decltype(*myGenom)>>(std::move(tempGenom));
                    }
                }

                map->birdTile(tile, {true, true, false, 0, UINT32_MAX, childGenom, energy / seedTiles.size()});
            }
        }

        life = false;
        return;
    }
    size_t len = activeTiles.size();
    auto it = activeTiles.begin();

    std::vector<pos> nextActiveTiles;
    nextActiveTiles.reserve(activeTiles.size() * 3);

    for (const auto& tile : activeTiles) {
        Tile* realTile = map->getTile(tile);
        if (!realTile || realTile->seed) continue;
        std::array<pos, 4> place = {{{tile.x - 1, tile.y}, {tile.x, tile.y + 1}, {tile.x + 1, tile.y}, {tile.x, tile.y - 1}}};
        size_t i = 0;
        const auto& genStr = (*myGenom)[realTile->gen % GENOM_SIZE];
        //std::clog << genStr[0] << ';' << genStr[1] << ';' << genStr[2] << ';' << genStr[3] << std::endl;
        bool ageConvert = (genStr[4] >= GENOM_SIZE / 2 && age >= LIFESPAN - 1);
        if (ageConvert || (genStr[0] < 0 && genStr[1] < 0 && genStr[2] < 0 && genStr[3] < 0) ) {
            realTile->seed = true;
            seedTiles.push_back(tile);
        }
        else {
            for (const auto& tile : place) {
                //std::clog << gen % GENOM_SIZE << std::endl;
                const auto& gen = genStr[i % 4];
                if (gen >= 0 && map->birdTile(tile, {true, false, false, static_cast<uint32_t>(gen), index}) >= 0) {
                    tiles.push_back(tile); nextActiveTiles.push_back(tile);
                }
                i++; 
            }
        }
    }

    activeTiles = std::move(nextActiveTiles);
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
    //std::clog << t_energy << ' ' << energy << std::endl;
    return 0;
}