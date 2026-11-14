#pragma once

#include <smpb/BaseComponent.h>
#include <smpb/BaseTask.h>

#include <smgc/display2D.h>
#include <smgc/Mouse.h>

/*#include <sm-gui/BaseWidget.h>
#include <sm-gui/Label.h>
#include <sm-gui/Button.h>
#include <sm-gui/Button2.h>
#include <sm-gui/Spacer.h>
#include <sm-gui/Layout.h>*/

#include <unordered_map>
#include <array>

#include "Map.h"

#define LINESIZE 0
// 0.5

class Scene2D: public BaseComponent
{
public:
    Scene2D();
    int init() override;
    int processTask(std::unique_ptr<BaseTask> task) override;
    int drawLURD(uint32_t texId, const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color, const glm::vec4& offset, const uint32_t t_direction);
    int tick(long long tickCounter) override;
    void death() override;
private:
    int ID_SMGC;

    bool isPressed(int key) const;
    void keyboard();

    std::unordered_map<int, bool> pressedKeys{};

    glm::vec2 standartPlayerPos{400.0f, 700.0f};
    glm::vec2 playerPos = standartPlayerPos;
    float playerSpeed = 300.0f;

    bool outlineEnabled = false;
    bool outlineKeyWasPressed = false;
    bool resetKeyWasPressed = false;
    bool speedMinusKeyWasPressed = false;
    bool speedPlusKeyWasPressed = false;
    bool modeKeyWasPressed = false;
    bool pauseKeyWasPressed = false;

    Map map;
    Map viewMap{true};

    Mouse oMouse;
    bool dirty = true;

    int windowWidth = 0, windowHeight = 0;
    int oWindowWidth = 0, oWindowHeight = 0;

    int speed = 0;
    bool pause = false;

    std::shared_ptr<genom> selectedGenom = nullptr;
    //std::shared_ptr<genom> selectedGenom = nullptr;

    uint32_t mode;
};
