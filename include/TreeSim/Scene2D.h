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

#include <sm-gui/BaseWidget.h>
#include <sm-gui/Label.h>
#include <sm-gui/Button.h>
#include <sm-gui/Button2.h>
#include <sm-gui/Spacer.h>
#include <sm-gui/Layout.h>
#include <TreeSim/WidgetMap.h>

#include <unordered_map>
#include <array>
#include <memory>
#include <string>

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
    static bool saveGenome(const std::string& path, const std::string& name, const genom& g);
    static bool loadGenome(const std::string& path, std::string& name, genom& g);
private:
    enum class FileDialogMode {
        None,
        LoadGenome,
        LoadMap
    };

    int ID_SMGC;

    bool isPressed(int key) const;
    void keyboard();
    bool justPressed(int key);
    char getCharFromKey(int key);

    void startEditingGene(int geneIndex);
    void startEditingName(int mode);
    void commitGeneEdit();
    void cancelGeneEdit();
    void updateGenomeButtons();
    void updateNameButtons();
    std::string formatGene(const gen& g) const;

    void saveCurrentGenome();
    void saveCurrentMap();
    void openFileDialog(FileDialogMode mode);
    void closeFileDialog();
    std::string generateRandomGenomeName();
    std::string generateDefaultMapName();

    std::unordered_map<int, bool> pressedKeys{};
    std::unordered_map<int, bool> wasPressedKeys{};

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
    Map viewMap{2 * LIFESPAN + 1, LIFESPAN + 1, true};

    Mouse oMouse;
    bool dirty = true;

    int windowWidth = 0, windowHeight = 0;
    int oWindowWidth = 0, oWindowHeight = 0;

    int speed = 0;
    bool pause = false;

    std::shared_ptr<genom> selectedGenom = nullptr;
    //std::shared_ptr<genom> selectedGenom = nullptr;

    uint32_t mode = 0;

    std::unique_ptr<Layout> layout = nullptr;
    WidgetMap* widgetMapMain = nullptr;
    WidgetMap* widgetMapView = nullptr;
    Label* infoLabel = nullptr;
    Layout* rightBottomLayoutPtr = nullptr;
    std::array<Button2*, GENOM_SIZE> geneButtons{};

    Button2* genomeNameBtn = nullptr;
    Button2* mapNameBtn = nullptr;
    Button2* btnSaveGen = nullptr;
    Button2* btnLoadGen = nullptr;
    Button2* btnSaveMap = nullptr;
    Button2* btnLoadMap = nullptr;

    std::string currentGenomeName;
    std::string customMapName;

    FileDialogMode fileDialogMode = FileDialogMode::None;
    std::unique_ptr<Layout> fileDialogLayout = nullptr;

    int editingGene = -1;
    std::string currentEditText = "";
    size_t cursorPos = 0;
};
