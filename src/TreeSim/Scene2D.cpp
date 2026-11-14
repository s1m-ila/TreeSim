#include <TreeSim/Scene2D.h>

#include <smpb/Core.h>

#include <array>
#include <cmath>
#include <format>

#include <filesystem>

static constexpr std::array<int, 8> kKeys = {
    GLFW_KEY_O, GLFW_KEY_R, GLFW_KEY_Q, GLFW_KEY_E, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_LEFT_CONTROL, GLFW_KEY_M, GLFW_KEY_SPACE
};

Scene2D::Scene2D() : BaseComponent("Scene2D") {
}

int Scene2D::init() {
    ID_SMGC = Core_getComponentIdxByName("SMGC2D");
    for (int key : kKeys) {
        int errorCode = sendTask(std::make_unique<task_SmgcSubscribeKey>(key), ID_SMGC);
        if (errorCode != 0) return errorCode;
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

void Scene2D::keyboard() {
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
        
        const glm::vec4& color = {1.0f, 0.942f, 0.786f, 1.0f};
        
        float windowMinSize = std::min(windowWidth, windowHeight);
        float mapMinSize = std::min(MAP_WIDTH, MAP_HEIGHT);
        float s = std::min((float) windowWidth / (float) MAP_WIDTH, ((float) windowHeight - 2.0f) / (float) MAP_HEIGHT / 2.0f);
        //std::clog << viewMapSize << std::endl;

        for (int rx = 0; rx < MAP_WIDTH; rx++) {
            for (int ry = 0; ry < MAP_HEIGHT; ry++) {
                float x = rx * s, y = (MAP_HEIGHT - 1 - ry) * s;
                sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{x, y}, glm::vec2{s, s}, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}), ID_SMGC);
                sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{x + LINESIZE, y + LINESIZE}, glm::vec2{s - 2 * LINESIZE, s - 2 * LINESIZE}, map.getTileCol({rx, ry}, mode)), ID_SMGC);
            }
        }
        for (int rx = 0; rx < MAP_WIDTH; rx++) {
            for (int ry = 0; ry < MAP_HEIGHT; ry++) {
                float x = rx * s, y = (MAP_HEIGHT - 1 - ry) * s + MAP_HEIGHT * s + 2.0f;
                sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{x, y}, glm::vec2{s, s}, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}), ID_SMGC);
                sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{x + LINESIZE, y + LINESIZE}, glm::vec2{s - 2 * LINESIZE, s - 2 * LINESIZE}, viewMap.getTileCol({rx, ry}, mode)), ID_SMGC);
            }
        }
        if (mouse.press.x && mouse.pos.y < windowMinSize) {
            int rx = mouse.pos.x / s; int fry = mouse.pos.y / s; int ry = MAP_HEIGHT - 1 - fry;
            sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{rx * s, fry * s}, glm::vec2{s, s}, glm::vec4{1.0f, 0.0f, 1.0f, 1.0f}), ID_SMGC);
            const auto& tile = map.getTile({rx, ry});
            if (tile) {
                sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{rx * s, fry * s}, glm::vec2{s, s}, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}), ID_SMGC);
                const auto& tree = map.getTree(tile->parent);
                std::cout << tile->parent << std::endl;
                if (tree) {
                    sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{rx * s, fry * s}, glm::vec2{s, s}, glm::vec4{0.0f, 0.0f, 1.0f, 1.0f}), ID_SMGC);
                    auto selectedGenom = tree->getGenom();
                    viewMap.reset();
                    viewMap.killTile({MAP_WIDTH / 2, 0});
                    viewMap.birdTile({MAP_WIDTH / 2, 0}, {true,  true, false, 0, UINT32_MAX, selectedGenom});
                    for (int i = 0; i < LIFESPAN - 2; i++) {
                        //std::cout << i << std::endl;
                        viewMap.tick();
                    }
                }
            }
        }

        sendTask(std::make_unique<task_SmgcDrawText>(std::string("Speed: ") + std::to_string(speed) + std::string("; Generation: ") + std::to_string(map.getGeneration()), glm::vec2{0.0f, 0.0f}, 30.0f, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}), ID_SMGC);
        
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
