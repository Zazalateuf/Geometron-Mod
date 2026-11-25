#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

using namespace geode::prelude;

class $modify(TrackingEditorLayer, LevelEditorLayer) {
    void addSpecial(GameObject* object);
    void removeSpecial(GameObject* object);
};
