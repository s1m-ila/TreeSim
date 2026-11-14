#include <TreeSim/Scene2D.h>

#include <smpb/Core.h>

#include <array>
#include <cmath>
#include <format>
#include <filesystem>
#include <iostream>
#include <vector>

static constexpr std::array<int, 40> kKeys = {
    GLFW_KEY_O, GLFW_KEY_R, GLFW_KEY_Q, GLFW_KEY_E, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_LEFT_CONTROL, GLFW_KEY_M, GLFW_KEY_SPACE,
    GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
    GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
    GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4,
    GLFW_KEY_KP_5, GLFW_KEY_KP_6, GLFW_KEY_KP_7, GLFW_KEY_KP_8, GLFW_KEY_KP_9,
    GLFW_KEY_MINUS, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_SEMICOLON,
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

    auto infoLbl = std::make_unique<Label>(this, WidgetMode::Minimal, "Speed: 0; Generation: 0");
    infoLbl->setMinSize(240.0f, 22.0f);
    infoLabel = infoLbl.get();
    rightBottomLayout->push_back(std::move(infoLbl));

    auto buttonsColumnsLayout = std::make_unique<Layout>(this, WidgetMode::Minimal, false);
    buttonsColumnsLayout->setIndent(0.0f);
    buttonsColumnsLayout->setSpace(2.0f);

    auto col1Layout = std::make_unique<Layout>(this, WidgetMode::Minimal, true);
    col1Layout->setIndent(1.0f);
    col1Layout->setSpace(1.0f);

    auto col2Layout = std::make_unique<Layout>(this, WidgetMode::Minimal, true);
    col2Layout->setIndent(1.0f);
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
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) return static_cast<char>('0' + (key - GLFW_KEY_0));
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) return static_cast<char>('0' + (key - GLFW_KEY_KP_0));
    if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) return '-';
    if (key == GLFW_KEY_SEMICOLON) return ';';
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

void Scene2D::commitGeneEdit() {
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

void Scene2D::keyboard() {
    if (editingGene != -1) {
        if (justPressed(GLFW_KEY_ENTER) || justPressed(GLFW_KEY_KP_ENTER)) {
            commitGeneEdit();
            return;
        }
        if (justPressed(GLFW_KEY_ESCAPE)) {
            cancelGeneEdit();
            return;
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
            GLFW_KEY_MINUS, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_SEMICOLON, GLFW_KEY_SPACE
        }) {
            if (justPressed(key)) {
                char c = getCharFromKey(key);
                if (c != '\0' && currentEditText.size() < 35) {
                    currentEditText.insert(cursorPos, 1, c);
                    ++cursorPos;
                }
            }
        }
        if (editingGene >= 0 && editingGene < GENOM_SIZE && geneButtons[editingGene]) {
            std::string displayText = std::format("{}: {}|{}", editingGene, currentEditText.substr(0, cursorPos), currentEditText.substr(cursorPos));
            geneButtons[editingGene]->setText(displayText);
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

        layout->setPos(0.0f, 0.0f);
        layout->setSize(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
        layout->setMinSize(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
        layout->tick(mouse);
        layout->draw();

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
