#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

using namespace geode::prelude;

class $modify(TrackingEditorLayer, LevelEditorLayer) {
    // $override est indispensable : sans lui, Geode n'installe pas le hook.
    $override
    void addSpecial(GameObject* object);

    $override
    void removeSpecial(GameObject* object);
};
