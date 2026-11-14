#include <TreeSim/Scene2D.h>

#include <smpb/Core.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <vector>

static constexpr std::array<int, 63> kKeys = {
    GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT, GLFW_KEY_LEFT_CONTROL, GLFW_KEY_SPACE,
    GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
    GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
    GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4,
    GLFW_KEY_KP_5, GLFW_KEY_KP_6, GLFW_KEY_KP_7, GLFW_KEY_KP_8, GLFW_KEY_KP_9,
    GLFW_KEY_A, GLFW_KEY_B, GLFW_KEY_C, GLFW_KEY_D, GLFW_KEY_E,
    GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_I, GLFW_KEY_J,
    GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_M, GLFW_KEY_N, GLFW_KEY_O,
    GLFW_KEY_P, GLFW_KEY_Q, GLFW_KEY_R, GLFW_KEY_S, GLFW_KEY_T,
    GLFW_KEY_U, GLFW_KEY_V, GLFW_KEY_W, GLFW_KEY_X, GLFW_KEY_Y, GLFW_KEY_Z,
    GLFW_KEY_MINUS, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_SEMICOLON, GLFW_KEY_PERIOD,
    GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_HOME, GLFW_KEY_END,
    GLFW_KEY_BACKSPACE, GLFW_KEY_DELETE, GLFW_KEY_ENTER, GLFW_KEY_KP_ENTER, GLFW_KEY_ESCAPE
};

Scene2D::Scene2D() : BaseComponent("Scene2D") {
}

int Scene2D::init() {
    ID_SMGC = Core_getComponentIdxByName("SMGC2D");
    for (int key : kKeys) {
        int errorCode = sendTask(std::make_unique<task_SmgcSubscribeKey>(key), ID_SMGC);
        if (errorCode != 0) return errorCode;
    }

    currentGenomeName = generateRandomGenomeName();

    layout = std::make_unique<Layout>(this, WidgetMode::Normal, true);
    layout->setIndent(0.0f);
    layout->setSpace(2.0f);

    auto mapMain = std::make_unique<WidgetMap>(this, &map, WidgetMode::Maximal);
    widgetMapMain = mapMain.get();
    widgetMapMain->setOnTileAction([this](WidgetMap&, ::pos tilePos, int button) {
        if (editingGene != -1) {
            commitGeneEdit();
        }
        if (button == 0) {
            const auto& tile = map.getTile(tilePos);
            if (tile) {
                const auto& tree = map.getTree(tile->parent);
                if (tree) {
                    selectedGenom = tree->getGenom();
                    currentGenomeName = generateRandomGenomeName();
                    updateNameButtons();
                    std::cout << tile->parent << std::endl;
                    viewMap.reset();
                    viewMap.killTile({viewMap.getWidth() / 2, 0});
                    viewMap.birdTile({viewMap.getWidth() / 2, 0}, {true, true, false, 0, UINT32_MAX, selectedGenom});
                    for (int i = 0; i <= LIFESPAN; i++) {
                        //std::cout << i << std::endl;
                        viewMap.tick();
                    }
                    updateGenomeButtons();
                    dirty = true;
                } else if (tile->myGenom) {
                    selectedGenom = tile->myGenom;
                    currentGenomeName = generateRandomGenomeName();
                    updateNameButtons();
                    viewMap.reset();
                    viewMap.killTile({viewMap.getWidth() / 2, 0});
                    viewMap.birdTile({viewMap.getWidth() / 2, 0}, {true, true, false, 0, UINT32_MAX, selectedGenom});
                    for (int i = 0; i <= LIFESPAN; i++) {
                        //std::cout << i << std::endl;
                        viewMap.tick();
                    }
                    updateGenomeButtons();
                    dirty = true;
                }
            }
        } else if (button == 2) {
            map.birdTile(tilePos, {true, true, false, 0, UINT32_MAX, selectedGenom});
            dirty = true;
        }
    });
    layout->push_back(std::move(mapMain));

    auto bottomLayout = std::make_unique<Layout>(this, WidgetMode::Extending, false);
    bottomLayout->setIndent(2.0f);
    bottomLayout->setSpace(4.0f);

    auto mapView = std::make_unique<WidgetMap>(this, &viewMap, WidgetMode::Extending);
    widgetMapView = mapView.get();
    widgetMapView->setMinSize(static_cast<float>(viewMap.getWidth()), static_cast<float>(viewMap.getHeight()));
    bottomLayout->push_back(std::move(mapView));

    auto rightBottomLayout = std::make_unique<Layout>(this, WidgetMode::Minimal, true);
    rightBottomLayout->setIndent(1.0f);
    rightBottomLayout->setSpace(2.0f);
    rightBottomLayoutPtr = rightBottomLayout.get();

    auto infoLbl = std::make_unique<Label>(this, WidgetMode::Minimal, "Speed: 0; Generation: 0");
    infoLbl->setMinSize(282.0f, 20.0f);
    infoLabel = infoLbl.get();
    rightBottomLayout->push_back(std::move(infoLbl));

    auto buttonsColumnsLayout = std::make_unique<Layout>(this, WidgetMode::Minimal, false);
    buttonsColumnsLayout->setIndent(0.0f);
    buttonsColumnsLayout->setSpace(2.0f);

    auto col1Layout = std::make_unique<Layout>(this, WidgetMode::Minimal, true);
    col1Layout->setIndent(0.0f);
    col1Layout->setSpace(1.0f);

    auto col2Layout = std::make_unique<Layout>(this, WidgetMode::Minimal, true);
    col2Layout->setIndent(0.0f);
    col2Layout->setSpace(1.0f);

    for (int i = 0; i < 15; ++i) {
        auto btn = std::make_unique<Button2>(this, WidgetMode::Minimal, std::format("{}: 0; 0; 0; 0; 0", i));
        btn->setMinSize(140.0f, 20.0f);
        btn->onClick = [this, i](Button&) {
            startEditingGene(i);
        };
        geneButtons[i] = btn.get();
        col1Layout->push_back(std::move(btn));
    }

    for (int i = 15; i < 30; ++i) {
        auto btn = std::make_unique<Button2>(this, WidgetMode::Minimal, std::format("{}: 0; 0; 0; 0; 0", i));
        btn->setMinSize(140.0f, 20.0f);
        btn->onClick = [this, i](Button&) {
            startEditingGene(i);
        };
        geneButtons[i] = btn.get();
        col2Layout->push_back(std::move(btn));
    }

    buttonsColumnsLayout->push_back(std::move(col1Layout));
    buttonsColumnsLayout->push_back(std::move(col2Layout));
    rightBottomLayout->push_back(std::move(buttonsColumnsLayout));

    auto gNameBtn = std::make_unique<Button2>(this, WidgetMode::Minimal, "Gen: " + currentGenomeName);
    gNameBtn->setMinSize(282.0f, 20.0f);
    gNameBtn->onClick = [this](Button&) {
        startEditingName(-2);
    };
    genomeNameBtn = gNameBtn.get();
    rightBottomLayout->push_back(std::move(gNameBtn));

    auto genActionsLayout = std::make_unique<Layout>(this, WidgetMode::Minimal, false);
    genActionsLayout->setIndent(0.0f);
    genActionsLayout->setSpace(2.0f);

    auto sGenBtn = std::make_unique<Button2>(this, WidgetMode::Minimal, "Save Gen");
    sGenBtn->setMinSize(140.0f, 20.0f);
    sGenBtn->onClick = [this](Button&) {
        saveCurrentGenome();
    };
    btnSaveGen = sGenBtn.get();
    genActionsLayout->push_back(std::move(sGenBtn));

    auto lGenBtn = std::make_unique<Button2>(this, WidgetMode::Minimal, "Load Gen");
    lGenBtn->setMinSize(140.0f, 20.0f);
    lGenBtn->onClick = [this](Button&) {
        openFileDialog(FileDialogMode::LoadGenome);
    };
    btnLoadGen = lGenBtn.get();
    genActionsLayout->push_back(std::move(lGenBtn));

    rightBottomLayout->push_back(std::move(genActionsLayout));

    auto mNameBtn = std::make_unique<Button2>(this, WidgetMode::Minimal, "Map: <auto date>");
    mNameBtn->setMinSize(282.0f, 20.0f);
    mNameBtn->onClick = [this](Button&) {
        startEditingName(-3);
    };
    mapNameBtn = mNameBtn.get();
    rightBottomLayout->push_back(std::move(mNameBtn));

    auto mapActionsLayout = std::make_unique<Layout>(this, WidgetMode::Minimal, false);
    mapActionsLayout->setIndent(0.0f);
    mapActionsLayout->setSpace(2.0f);

    auto sMapBtn = std::make_unique<Button2>(this, WidgetMode::Minimal, "Save Map");
    sMapBtn->setMinSize(140.0f, 20.0f);
    sMapBtn->onClick = [this](Button&) {
        saveCurrentMap();
    };
    btnSaveMap = sMapBtn.get();
    mapActionsLayout->push_back(std::move(sMapBtn));

    auto lMapBtn = std::make_unique<Button2>(this, WidgetMode::Minimal, "Load Map");
    lMapBtn->setMinSize(140.0f, 20.0f);
    lMapBtn->onClick = [this](Button&) {
        openFileDialog(FileDialogMode::LoadMap);
    };
    btnLoadMap = lMapBtn.get();
    mapActionsLayout->push_back(std::move(lMapBtn));

    rightBottomLayout->push_back(std::move(mapActionsLayout));

    bottomLayout->push_back(std::move(rightBottomLayout));

    layout->push_back(std::move(bottomLayout));

    const auto& initTile = map.getTile({map.getWidth() / 2, 0});
    if (initTile && initTile->myGenom) {
        selectedGenom = initTile->myGenom;
        viewMap.reset();
        viewMap.killTile({viewMap.getWidth() / 2, 0});
        viewMap.birdTile({viewMap.getWidth() / 2, 0}, {true, true, false, 0, UINT32_MAX, selectedGenom});
        for (int i = 0; i <= LIFESPAN; i++) {
            //std::cout << i << std::endl;
            viewMap.tick();
        }
        updateGenomeButtons();
        updateNameButtons();
    }

    return sendTask(std::make_unique<task_SmgcSetOutline>(outlineEnabled), ID_SMGC);
}

int Scene2D::processTask(std::unique_ptr<BaseTask> task) {
    if (taskIs_SmgcKeyState(task->getIdx())) {
        auto* typed = static_cast<task_SmgcKeyState*>(task.get());
        pressedKeys[typed->key] = typed->pressed;
        return 0;
    }
    return -EINVAL;
}

bool Scene2D::isPressed(int key) const {
    auto it = pressedKeys.find(key);
    return it != pressedKeys.end() && it->second;
}

bool Scene2D::justPressed(int key) {
    bool current = isPressed(key);
    bool prev = wasPressedKeys[key];
    wasPressedKeys[key] = current;
    return current && !prev;
}

char Scene2D::getCharFromKey(int key) {
    bool shift = isPressed(GLFW_KEY_LEFT_SHIFT) || isPressed(GLFW_KEY_RIGHT_SHIFT);
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) return static_cast<char>('0' + (key - GLFW_KEY_0));
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) return static_cast<char>('0' + (key - GLFW_KEY_KP_0));
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        return shift ? static_cast<char>('A' + (key - GLFW_KEY_A)) : static_cast<char>('a' + (key - GLFW_KEY_A));
    }
    if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) return shift ? '_' : '-';
    if (key == GLFW_KEY_SEMICOLON) return ';';
    if (key == GLFW_KEY_PERIOD) return '.';
    if (key == GLFW_KEY_SPACE) return ' ';
    return '\0';
}

std::string Scene2D::formatGene(const gen& g) const {
    return std::format("{}; {}; {}; {}; {}", g[0], g[1], g[2], g[3], g[4]);
}

void Scene2D::updateGenomeButtons() {
    for (size_t i = 0; i < GENOM_SIZE; ++i) {
        if (!geneButtons[i]) continue;
        if (static_cast<int>(i) == editingGene) continue;
        if (selectedGenom) {
            geneButtons[i]->setText(std::format("{}: {}", i, formatGene((*selectedGenom)[i])));
        } else {
            geneButtons[i]->setText(std::format("{}: 0; 0; 0; 0; 0", i));
        }
    }
}

void Scene2D::updateNameButtons() {
    if (genomeNameBtn && editingGene != -2) {
        genomeNameBtn->setText(std::format("Gen: {}", currentGenomeName));
    }
    if (mapNameBtn && editingGene != -3) {
        mapNameBtn->setText(std::format("Map: {}", customMapName.empty() ? "<auto date>" : customMapName));
    }
}

void Scene2D::startEditingGene(int geneIndex) {
    if (editingGene != -1 && editingGene != geneIndex) {
        commitGeneEdit();
    }
    if (!selectedGenom) {
        selectedGenom = std::make_shared<genom>();
        for (auto& g : *selectedGenom) g.fill(0);
        updateGenomeButtons();
    }
    editingGene = geneIndex;
    currentEditText = formatGene((*selectedGenom)[editingGene]);
    cursorPos = currentEditText.size();
    for (const auto& [k, v] : pressedKeys) {
        wasPressedKeys[k] = v;
    }
    if (geneButtons[editingGene]) {
        geneButtons[editingGene]->setText(std::format("{}: {}|", editingGene, currentEditText));
    }
    dirty = true;
}

void Scene2D::startEditingName(int mode) {
    if (editingGene != -1) {
        commitGeneEdit();
    }
    editingGene = mode;
    currentEditText = (mode == -2) ? currentGenomeName : customMapName;
    cursorPos = currentEditText.size();
    for (const auto& [k, v] : pressedKeys) {
        wasPressedKeys[k] = v;
    }
    if (mode == -2 && genomeNameBtn) {
        genomeNameBtn->setText(std::format("Gen: {}|", currentEditText));
    } else if (mode == -3 && mapNameBtn) {
        mapNameBtn->setText(std::format("Map: {}|", currentEditText));
    }
    dirty = true;
}

void Scene2D::commitGeneEdit() {
    if (editingGene == -2) {
        currentGenomeName = currentEditText;
        if (currentGenomeName.empty()) {
            currentGenomeName = generateRandomGenomeName();
        }
        editingGene = -1;
        updateNameButtons();
        dirty = true;
        return;
    }
    if (editingGene == -3) {
        customMapName = currentEditText;
        editingGene = -1;
        updateNameButtons();
        dirty = true;
        return;
    }
    if (editingGene < 0 || editingGene >= GENOM_SIZE || !selectedGenom) {
        editingGene = -1;
        return;
    }
    std::vector<int> vals;
    size_t start = 0;
    while (start < currentEditText.size()) {
        size_t next = currentEditText.find(';', start);
        if (next == std::string::npos) next = currentEditText.size();
        std::string token = currentEditText.substr(start, next - start);
        size_t first = token.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = token.find_last_not_of(" \t\r\n");
            std::string trimmed = token.substr(first, last - first + 1);
            try {
                vals.push_back(std::stoi(trimmed));
            } catch (...) {
                vals.push_back(0);
            }
        } else {
            vals.push_back(0);
        }
        start = next + 1;
    }
    while (vals.size() < 5) vals.push_back(0);
    for (size_t k = 0; k < 5; ++k) {
        (*selectedGenom)[editingGene][k] = vals[k];
    }
    viewMap.reset();
    viewMap.killTile({viewMap.getWidth() / 2, 0});
    viewMap.birdTile({viewMap.getWidth() / 2, 0}, {true, true, false, 0, UINT32_MAX, selectedGenom});
    for (int i = 0; i <= LIFESPAN; ++i) {
        //std::cout << i << std::endl;
        viewMap.tick();
    }
    if (geneButtons[editingGene]) {
        geneButtons[editingGene]->setText(std::format("{}: {}", editingGene, formatGene((*selectedGenom)[editingGene])));
    }
    editingGene = -1;
    dirty = true;
}

void Scene2D::cancelGeneEdit() {
    if (editingGene == -2 || editingGene == -3) {
        editingGene = -1;
        updateNameButtons();
        dirty = true;
        return;
    }
    if (editingGene >= 0 && editingGene < GENOM_SIZE && geneButtons[editingGene]) {
        if (selectedGenom) {
            geneButtons[editingGene]->setText(std::format("{}: {}", editingGene, formatGene((*selectedGenom)[editingGene])));
        } else {
            geneButtons[editingGene]->setText(std::format("{}: 0; 0; 0; 0; 0", editingGene));
        }
    }
    editingGene = -1;
    dirty = true;
}

std::string Scene2D::generateRandomGenomeName() {
    static const char* prefixes[] = {
        "Acorn", "Bonsai", "Cedar", "Dryad", "Elm", "Flora", "Grove", "Hazel",
        "Ivy", "Juniper", "Kelp", "Lotus", "Maple", "Nectar", "Olive", "Pine",
        "Quercus", "Root", "Spruce", "Timber", "Ulmus", "Vine", "Willow", "Xylem", "Yew", "Zinnia"
    };
    int p = rand() % (sizeof(prefixes) / sizeof(prefixes[0]));
    int num = rand() % 1000;
    return std::format("{}_{:03d}", prefixes[p], num);
}

std::string Scene2D::generateDefaultMapName() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &tm);
    return std::string(buf);
}

bool Scene2D::saveGenome(const std::string& path, const std::string& name, const genom& g) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << "Name: " << name << "\n";
    for (size_t i = 0; i < GENOM_SIZE; ++i) {
        out << i << ": " << g[i][0] << "; " << g[i][1] << "; " << g[i][2] << "; " << g[i][3] << "; " << g[i][4] << "\n";
    }
    return true;
}

bool Scene2D::loadGenome(const std::string& path, std::string& name, genom& g) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    std::string line;
    if (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        if (line.starts_with("Name: ")) {
            name = line.substr(6);
        } else {
            name = line;
        }
    }
    size_t rowIdx = 0;
    while (std::getline(in, line) && rowIdx < GENOM_SIZE) {
        size_t colon = line.find(':');
        std::string valsPart = (colon != std::string::npos) ? line.substr(colon + 1) : line;
        std::vector<int> vals;
        size_t start = 0;
        while (start < valsPart.size()) {
            size_t next = valsPart.find(';', start);
            if (next == std::string::npos) next = valsPart.size();
            std::string token = valsPart.substr(start, next - start);
            size_t first = token.find_first_not_of(" \t\r\n");
            if (first != std::string::npos) {
                size_t last = token.find_last_not_of(" \t\r\n");
                std::string trimmed = token.substr(first, last - first + 1);
                try {
                    vals.push_back(std::stoi(trimmed));
                } catch (...) {
                    vals.push_back(0);
                }
            } else {
                vals.push_back(0);
            }
            start = next + 1;
        }
        while (vals.size() < 5) vals.push_back(0);
        for (size_t k = 0; k < 5; ++k) {
            g[rowIdx][k] = vals[k];
        }
        rowIdx++;
    }
    return true;
}

void Scene2D::saveCurrentGenome() {
    if (editingGene != -1) commitGeneEdit();
    if (!selectedGenom) {
        selectedGenom = std::make_shared<genom>();
        for (auto& g : *selectedGenom) g.fill(0);
    }
    std::filesystem::create_directories("saves/genomes");
    if (currentGenomeName.empty()) {
        currentGenomeName = generateRandomGenomeName();
        updateNameButtons();
    }
    std::string filename = currentGenomeName;
    if (!filename.ends_with(".gen")) filename += ".gen";
    std::string filepath = "saves/genomes/" + filename;
    saveGenome(filepath, currentGenomeName, *selectedGenom);
}

void Scene2D::saveCurrentMap() {
    if (editingGene != -1) commitGeneEdit();
    std::filesystem::create_directories("saves/maps");
    std::string filename = customMapName.empty() ? generateDefaultMapName() : customMapName;
    if (!filename.ends_with(".map")) filename += ".map";
    std::string filepath = "saves/maps/" + filename;
    map.saveToFile(filepath);
}

void Scene2D::openFileDialog(FileDialogMode mode) {
    if (editingGene != -1) commitGeneEdit();
    fileDialogMode = mode;
    fileDialogLayout = std::make_unique<Layout>(this, WidgetMode::Normal, true);
    fileDialogLayout->setIndent(2.0f);
    fileDialogLayout->setSpace(2.0f);

    auto headerLayout = std::make_unique<Layout>(this, WidgetMode::Minimal, false);
    headerLayout->setIndent(0.0f);
    headerLayout->setSpace(2.0f);

    std::string title = (mode == FileDialogMode::LoadGenome) ? "Load Genome" : "Load Map";
    auto titleLbl = std::make_unique<Label>(this, WidgetMode::Minimal, title);
    titleLbl->setMinSize(180.0f, 20.0f);
    headerLayout->push_back(std::move(titleLbl));

    auto cancelBtn = std::make_unique<Button2>(this, WidgetMode::Minimal, "[Cancel]");
    cancelBtn->setMinSize(90.0f, 20.0f);
    cancelBtn->onClick = [this](Button&) {
        closeFileDialog();
    };
    headerLayout->push_back(std::move(cancelBtn));

    fileDialogLayout->push_back(std::move(headerLayout));

    std::string dirPath = (mode == FileDialogMode::LoadGenome) ? "saves/genomes" : "saves/maps";
    std::string ext = (mode == FileDialogMode::LoadGenome) ? ".gen" : ".map";
    std::filesystem::create_directories(dirPath);

    struct FileEntry {
        std::filesystem::path path;
        std::filesystem::file_time_type time;
    };
    std::vector<FileEntry> files;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ext) {
            files.push_back({entry.path(), entry.last_write_time()});
        }
    }
    std::sort(files.begin(), files.end(), [](const FileEntry& a, const FileEntry& b) {
        return a.time > b.time;
    });

    if (files.empty()) {
        auto emptyLbl = std::make_unique<Label>(this, WidgetMode::Minimal, "No saves found");
        emptyLbl->setMinSize(270.0f, 20.0f);
        fileDialogLayout->push_back(std::move(emptyLbl));
    } else {
        size_t showCount = std::min(files.size(), size_t(14));
        for (size_t i = 0; i < showCount; ++i) {
            std::string stem = files[i].path.stem().string();
            std::filesystem::path p = files[i].path;
            auto btn = std::make_unique<Button2>(this, WidgetMode::Minimal, stem);
            btn->setMinSize(274.0f, 20.0f);
            btn->onClick = [this, p, mode](Button&) {
                if (mode == FileDialogMode::LoadGenome) {
                    std::string loadedName;
                    genom loadedGen;
                    if (loadGenome(p.string(), loadedName, loadedGen)) {
                        currentGenomeName = loadedName.empty() ? p.stem().string() : loadedName;
                        if (!selectedGenom) {
                            selectedGenom = std::make_shared<genom>();
                        }
                        *selectedGenom = loadedGen;
                        viewMap.reset();
                        viewMap.killTile({viewMap.getWidth() / 2, 0});
                        viewMap.birdTile({viewMap.getWidth() / 2, 0}, {true, true, false, 0, UINT32_MAX, selectedGenom});
                        for (int k = 0; k <= LIFESPAN; ++k) {
                            viewMap.tick();
                        }
                        updateGenomeButtons();
                        updateNameButtons();
                    }
                } else if (mode == FileDialogMode::LoadMap) {
                    if (map.loadFromFile(p.string())) {
                        customMapName = p.stem().string();
                        updateNameButtons();
                    }
                }
                closeFileDialog();
                dirty = true;
            };
            fileDialogLayout->push_back(std::move(btn));
        }
        if (files.size() > showCount) {
            auto moreLbl = std::make_unique<Label>(this, WidgetMode::Minimal, std::format("+{} more files...", files.size() - showCount));
            moreLbl->setMinSize(274.0f, 18.0f);
            fileDialogLayout->push_back(std::move(moreLbl));
        }
    }
    dirty = true;
}

void Scene2D::closeFileDialog() {
    fileDialogMode = FileDialogMode::None;
    dirty = true;
}

void Scene2D::keyboard() {
    if (fileDialogMode != FileDialogMode::None) {
        if (justPressed(GLFW_KEY_ESCAPE)) {
            closeFileDialog();
        }
        return;
    }

    if (editingGene != -1) {
        if (justPressed(GLFW_KEY_ENTER) || justPressed(GLFW_KEY_KP_ENTER)) {
            commitGeneEdit();
            return;
        }
        if (justPressed(GLFW_KEY_ESCAPE)) {
            cancelGeneEdit();
            return;
        }
        if (editingGene >= 0 && justPressed(GLFW_KEY_R)) {
            currentEditText = "0; 0; 0; 0; 0";
            cursorPos = currentEditText.size();
            if (selectedGenom) {
                (*selectedGenom)[editingGene] = {0, 0, 0, 0, 0};
                viewMap.reset();
                viewMap.killTile({viewMap.getWidth() / 2, 0});
                viewMap.birdTile({viewMap.getWidth() / 2, 0}, {true, true, false, 0, UINT32_MAX, selectedGenom});
                for (int i = 0; i <= LIFESPAN; ++i) {
                    viewMap.tick();
                }
            }
            dirty = true;
        }
        if (justPressed(GLFW_KEY_LEFT)) {
            if (cursorPos > 0) --cursorPos;
        }
        if (justPressed(GLFW_KEY_RIGHT)) {
            if (cursorPos < currentEditText.size()) ++cursorPos;
        }
        if (justPressed(GLFW_KEY_HOME)) {
            cursorPos = 0;
        }
        if (justPressed(GLFW_KEY_END)) {
            cursorPos = currentEditText.size();
        }
        if (justPressed(GLFW_KEY_BACKSPACE)) {
            if (cursorPos > 0) {
                currentEditText.erase(cursorPos - 1, 1);
                --cursorPos;
            }
        }
        if (justPressed(GLFW_KEY_DELETE)) {
            if (cursorPos < currentEditText.size()) {
                currentEditText.erase(cursorPos, 1);
            }
        }
        for (int key : {
            GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
            GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
            GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4,
            GLFW_KEY_KP_5, GLFW_KEY_KP_6, GLFW_KEY_KP_7, GLFW_KEY_KP_8, GLFW_KEY_KP_9,
            GLFW_KEY_A, GLFW_KEY_B, GLFW_KEY_C, GLFW_KEY_D, GLFW_KEY_E,
            GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_I, GLFW_KEY_J,
            GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_M, GLFW_KEY_N, GLFW_KEY_O,
            GLFW_KEY_P, GLFW_KEY_Q, GLFW_KEY_R, GLFW_KEY_S, GLFW_KEY_T,
            GLFW_KEY_U, GLFW_KEY_V, GLFW_KEY_W, GLFW_KEY_X, GLFW_KEY_Y, GLFW_KEY_Z,
            GLFW_KEY_MINUS, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_SEMICOLON, GLFW_KEY_PERIOD, GLFW_KEY_SPACE
        }) {
            if (justPressed(key)) {
                char c = getCharFromKey(key);
                if (c != '\0' && currentEditText.size() < 35) {
                    bool allowed = false;
                    if (editingGene >= 0) {
                        allowed = (c >= '0' && c <= '9') || c == '-' || c == ';' || c == ' ';
                    } else if (editingGene == -2 || editingGene == -3) {
                        allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
                    }
                    if (allowed) {
                        currentEditText.insert(cursorPos, 1, c);
                        ++cursorPos;
                    }
                }
            }
        }
        if (editingGene >= 0 && editingGene < GENOM_SIZE && geneButtons[editingGene]) {
            std::string displayText = std::format("{}: {}|{}", editingGene, currentEditText.substr(0, cursorPos), currentEditText.substr(cursorPos));
            geneButtons[editingGene]->setText(displayText);
        } else if (editingGene == -2 && genomeNameBtn) {
            std::string displayText = std::format("Gen: {}|{}", currentEditText.substr(0, cursorPos), currentEditText.substr(cursorPos));
            genomeNameBtn->setText(displayText);
        } else if (editingGene == -3 && mapNameBtn) {
            std::string displayText = std::format("Map: {}|{}", currentEditText.substr(0, cursorPos), currentEditText.substr(cursorPos));
            mapNameBtn->setText(displayText);
        }
    } else {
        bool outlineKeyPressed = isPressed(GLFW_KEY_O);
        bool resetKeyPressed = isPressed(GLFW_KEY_R);
        bool speedMinusKeyPressed = isPressed(GLFW_KEY_Q);
        bool speedPlusKeyPressed = isPressed(GLFW_KEY_E);
        bool shiftKeyPressed = isPressed(GLFW_KEY_LEFT_SHIFT);
        bool ctrlKeyPressed = isPressed(GLFW_KEY_LEFT_CONTROL);
        bool modeKeyPressed = isPressed(GLFW_KEY_M);
        bool pauseKeyPressed = isPressed(GLFW_KEY_SPACE);

        int speedStep = ctrlKeyPressed ? 100 : (shiftKeyPressed ? 10 : 1);

        if (outlineKeyPressed && !outlineKeyWasPressed) {
            outlineEnabled = !outlineEnabled;
            sendTask(std::make_unique<task_SmgcSetOutline>(outlineEnabled), ID_SMGC);
            dirty = true;
        }
        if (resetKeyPressed && !resetKeyWasPressed) {
            map.reset();
            dirty = true;
        }
        if (speedMinusKeyPressed && !speedMinusKeyWasPressed) {
            speed -= speedStep;
        }
        if (speedPlusKeyPressed && !speedPlusKeyWasPressed) {
            speed += speedStep;
        }
        if (modeKeyPressed && !modeKeyWasPressed) {
            mode++;
            if (widgetMapMain) widgetMapMain->setDisplayMode(mode);
            if (widgetMapView) widgetMapView->setDisplayMode(mode);
            dirty = true;
        }
        if (pauseKeyPressed && !pauseKeyWasPressed) {
            pause = !pause;
        }
        outlineKeyWasPressed = outlineKeyPressed;
        resetKeyWasPressed = resetKeyPressed;
        speedMinusKeyWasPressed = speedMinusKeyPressed;
        speedPlusKeyWasPressed = speedPlusKeyPressed;
        modeKeyWasPressed = modeKeyPressed;
        pauseKeyWasPressed = pauseKeyPressed;
    }
}

/*int Scene2D::drawButton(glm::vec2 pos, glm::vec2 size, bool pressed) {
    int lineSize = 10.0f;
    int fontSize = 30.0f;
    glm::vec4 colA = {0.75f, 0.75f, 0.75f, 1.0f};
    glm::vec4 colB = {0.5f, 0.5f, 0.5f, 1.0f};
    glm::vec4 colC = {0.25f, 0.25f, 0.25f, 1.0f};

    glm::vec4 col;
    if (pressed) {
        col = colA;
        colA = colC;
        colC = col;
    }

    sendTask(std::make_unique<task_SmgcDrawRect>(pos + glm::vec2{0.0f, 0.0f}, glm::vec2{size.x, size.y}, colB), ID_SMGC);
    sendTask(std::make_unique<task_SmgcDrawRect>(pos + glm::vec2{0.0f, 0.0f}, glm::vec2{size.x - lineSize, size.y - lineSize}, colA), ID_SMGC);
    sendTask(std::make_unique<task_SmgcDrawRect>(pos + glm::vec2{lineSize, lineSize}, glm::vec2{size.x - lineSize, size.y - lineSize}, colC), ID_SMGC);
    sendTask(std::make_unique<task_SmgcDrawRect>(pos + glm::vec2{lineSize, lineSize}, glm::vec2{size.x - lineSize * 2, size.y - lineSize * 2}, colB), ID_SMGC);
    return sendTask(std::make_unique<task_SmgcDrawText>(
        "Я кнопка?", pos + glm::vec2{size.x / 2.0f, size.y / 2.0f - fontSize * 0.5f}, fontSize, glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}, TextAlign::Center), ID_SMGC);
    
}*/

int Scene2D::drawLURD(const uint32_t texId, const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color, const glm::vec4& offset, const uint32_t t_direction) {
    int width = 0, height = 0;
    sendTask(std::make_unique<task_SmgcGetTextureSize>(texId, &width, &height), ID_SMGC);
    
    int hW = width >> 1, hH = height >> 1;
    return sendTask (
        std::make_unique<task_SmgcDrawTexture> (
            texId,
            pos + (t_direction % 2 > 0 ? glm::vec2{offset.x, offset.y} : glm::vec2{offset.z, offset.w}) * size * glm::vec2{(t_direction > 1 ? -1.0f : 1.0f), 1.0f},
            size,
            color,
            glm::vec4 {
                hW * (t_direction % 2),
                hH * (t_direction % 4 > 1),
                hW,
                hH
            }
        ),
        ID_SMGC
    );
}

int Scene2D::tick(long long tickCounter) {
    keyboard();

    sendTask(std::make_unique<task_SmgcGetWindowSize>(&windowWidth, &windowHeight), ID_SMGC);

    Mouse mouse;
    sendTask(std::make_unique<task_SmgcGetMouse>(&mouse), ID_SMGC);

    uint32_t TPS = Core_getTPS();

    if (!pause) {
        size_t s = std::max(abs(speed) + 1, 1);
        if (tickCounter % s == 0) {
            size_t c = std::max(speed + 1, 1);
            for (size_t i = 0; i < c; i++) {
                map.tick();
                //viewMap.tick();
            }
        }
    }
    //dirty = dirty || tickCounter % TPS == 0;
    if (windowWidth > 0 && windowHeight > 0) {///*oMouse != mouse || */oWindowWidth != windowWidth || oWindowHeight != windowHeight || dirty) {
        sendTask(std::make_unique<task_SmgcClearObjects>(), ID_SMGC);
        //sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{0.0f, 0.0f}, glm::vec2{windowWidth, windowHeight}, glm::vec4{0.5f, 0.5f, 0.5f, 1.0f}), ID_SMGC);
        sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{0.0f, 0.0f}, glm::vec2{windowWidth, windowHeight}, glm::vec4{0.12f, 0.14f, 0.16f, 1.0f}), ID_SMGC);

        if (infoLabel) {
            infoLabel->setText(std::format("Speed: {}; Generation: {}", speed, map.getGeneration()));
        }

        if (widgetMapMain) {
            float mapScale = static_cast<float>(windowWidth) / static_cast<float>(map.getWidth());
            float mainMapHeight = std::ceil(static_cast<float>(map.getHeight()) * mapScale);
            widgetMapMain->setMinSize(static_cast<float>(windowWidth), mainMapHeight);
        }

        Mouse layoutMouse = mouse;
        if (fileDialogMode != FileDialogMode::None && rightBottomLayoutPtr) {
            if (mouse.pos.x >= rightBottomLayoutPtr->getPos().x &&
                mouse.pos.x <= rightBottomLayoutPtr->getPos().x + rightBottomLayoutPtr->getSize().x &&
                mouse.pos.y >= rightBottomLayoutPtr->getPos().y &&
                mouse.pos.y <= rightBottomLayoutPtr->getPos().y + rightBottomLayoutPtr->getSize().y) {
                layoutMouse.press = glm::vec3{0.0f};
                layoutMouse.release = glm::vec3{0.0f};
            }
        }

        layout->setPos(0.0f, 0.0f);
        layout->setSize(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
        layout->setMinSize(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
        layout->tick(layoutMouse);
        layout->draw();

        if (fileDialogMode != FileDialogMode::None && fileDialogLayout && rightBottomLayoutPtr) {
            fileDialogLayout->setPos(rightBottomLayoutPtr->getPos());
            fileDialogLayout->setSize(rightBottomLayoutPtr->getSize());
            fileDialogLayout->tick(mouse);
            if (fileDialogMode != FileDialogMode::None) {
                sendTask(std::make_unique<task_SmgcDrawRect>(fileDialogLayout->getPos(), fileDialogLayout->getSize(), glm::vec4{0.12f, 0.14f, 0.16f, 1.0f}), ID_SMGC);
                fileDialogLayout->draw();
            }
        }

        //std::clog << viewMapSize << std::endl;

        sendTask(std::make_unique<task_SmgcSwapObjects>(), ID_SMGC);
    }

    oMouse = mouse;
    oWindowWidth = windowWidth, oWindowHeight = windowHeight;
    dirty = false;

    return 0;
}

void Scene2D::death() {
    for (int key : kKeys) sendTask(std::make_unique<task_SmgcUnsubscribeKey>(key), ID_SMGC);
}
