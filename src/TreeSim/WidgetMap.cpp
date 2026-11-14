#include <TreeSim/WidgetMap.h>
#include <algorithm>

WidgetMap::WidgetMap(BaseComponent* t_component, Map* t_map, WidgetMode t_mode)
    : BaseWidget(t_component, t_mode), map(t_map) {
    if (map) {
        minSize = glm::vec2{static_cast<float>(map->getWidth()), static_cast<float>(map->getHeight())};
    }
}

void WidgetMap::setMap(Map* t_map) {
    map = t_map;
    if (map) {
        minSize = glm::vec2{static_cast<float>(map->getWidth()), static_cast<float>(map->getHeight())};
    }
    dirty = true;
}

Map* WidgetMap::getMap() const {
    return map;
}

void WidgetMap::setDisplayMode(uint32_t t_mode) {
    displayMode = t_mode;
    dirty = true;
}

uint32_t WidgetMap::getDisplayMode() const {
    return displayMode;
}

void WidgetMap::setLineSize(float t_lineSize) {
    lineSize = t_lineSize;
    dirty = true;
}

float WidgetMap::getLineSize() const {
    return lineSize;
}

void WidgetMap::setOnTileAction(std::function<void(WidgetMap&, ::pos, int)> t_onTileAction) {
    onTileAction = std::move(t_onTileAction);
}

std::optional<::pos> WidgetMap::getHoverTile() const {
    if (isHovered) return hoverTile;
    return std::nullopt;
}

void WidgetMap::onSizeChanged() {
    dirty = true;
}

int WidgetMap::tick(Mouse& mouse) {
    if (!map) return 0;

    float s = std::min(size.x / static_cast<float>(map->getWidth()), size.y / static_cast<float>(map->getHeight()));
    if (s <= 0.0f) return 0;

    float mapDrawWidth = map->getWidth() * s;
    float mapDrawHeight = map->getHeight() * s;
    float offsetX = (size.x - mapDrawWidth) * 0.5f;
    float offsetY = (size.y - mapDrawHeight) * 0.5f;

    if (mouse.pos.x >= pos.x + offsetX && mouse.pos.x < pos.x + offsetX + mapDrawWidth &&
        mouse.pos.y >= pos.y + offsetY && mouse.pos.y < pos.y + offsetY + mapDrawHeight) {
        int rx = static_cast<int>((mouse.pos.x - (pos.x + offsetX)) / s);
        int fry = static_cast<int>((mouse.pos.y - (pos.y + offsetY)) / s);
        int ry = static_cast<int>(map->getHeight()) - 1 - fry;

        if (rx >= 0 && rx < static_cast<int>(map->getWidth()) &&
            ry >= 0 && ry < static_cast<int>(map->getHeight())) {
            isHovered = true;
            hoverTile = {static_cast<uint32_t>(rx), static_cast<uint32_t>(ry)};
            hoverDrawPos = {pos.x + offsetX + rx * s, pos.y + offsetY + fry * s};

            if (onTileAction) {
                if (mouse.press.x > 0.5f || mouse.state.x > 0.5f) {
                    onTileAction(*this, hoverTile, 0);
                }
                if (mouse.press.y > 0.5f) {
                    onTileAction(*this, hoverTile, 1);
                }
                if (mouse.press.z > 0.5f || mouse.state.z > 0.5f) {
                    onTileAction(*this, hoverTile, 2);
                }
            }
        } else {
            isHovered = false;
        }
    } else {
        isHovered = false;
    }

    return 0;
}

int WidgetMap::draw() {
    if (!component || !map) return -ENOSYS;

    float s = std::min(size.x / static_cast<float>(map->getWidth()), size.y / static_cast<float>(map->getHeight()));
    if (s <= 0.0f) return 0;

    float mapDrawWidth = map->getWidth() * s;
    float mapDrawHeight = map->getHeight() * s;
    float offsetX = (size.x - mapDrawWidth) * 0.5f;
    float offsetY = (size.y - mapDrawHeight) * 0.5f;

    for (uint32_t rx = 0; rx < map->getWidth(); ++rx) {
        for (uint32_t ry = 0; ry < map->getHeight(); ++ry) {
            float x = pos.x + offsetX + rx * s;
            float y = pos.y + offsetY + (map->getHeight() - 1 - ry) * s;
            if (lineSize > 0.0f) {
                component->sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{x, y}, glm::vec2{s, s}, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}), ID_SMGC);
                component->sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{x + lineSize, y + lineSize}, glm::vec2{s - 2.0f * lineSize, s - 2.0f * lineSize}, map->getTileCol({rx, ry}, displayMode)), ID_SMGC);
            } else {
                component->sendTask(std::make_unique<task_SmgcDrawRect>(glm::vec2{x, y}, glm::vec2{s, s}, map->getTileCol({rx, ry}, displayMode)), ID_SMGC);
            }
        }
    }

    if (isHovered) {
        component->sendTask(std::make_unique<task_SmgcDrawRect>(hoverDrawPos, glm::vec2{s, s}, glm::vec4{1.0f, 0.0f, 1.0f, 1.0f}), ID_SMGC);
    }

    return 0;
}
