#pragma once

#include <sm-gui/BaseWidget.h>
#include <TreeSim/Map.h>
#include <functional>
#include <optional>

class WidgetMap : public BaseWidget {
public:
    WidgetMap(BaseComponent* t_component, Map* t_map = nullptr, WidgetMode t_mode = WidgetMode::Normal);

    void setMap(Map* t_map);
    Map* getMap() const;

    void setDisplayMode(uint32_t t_mode);
    uint32_t getDisplayMode() const;

    void setLineSize(float t_lineSize);
    float getLineSize() const;

    void setOnTileAction(std::function<void(WidgetMap&, ::pos, int)> t_onTileAction);

    int tick(Mouse& mouse) override;
    int draw() override;

    std::optional<::pos> getHoverTile() const;

protected:
    void onSizeChanged() override;

private:
    Map* map = nullptr;
    uint32_t displayMode = 0;
    float lineSize = 0.0f;
    std::function<void(WidgetMap&, ::pos, int)> onTileAction = nullptr;

    bool isHovered = false;
    ::pos hoverTile{0, 0};
    glm::vec2 hoverDrawPos{0.0f, 0.0f};
};
