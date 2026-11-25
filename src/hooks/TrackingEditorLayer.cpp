// ReSharper disable CppMemberFunctionMayBeStatic
#include "TrackingEditorLayer.hpp"

#include "TrackedGameObject.hpp"

void TrackingEditorLayer::addSpecial(GameObject* object) {
    if (object == nullptr) {
        return;
    }

LevelEditorLayer::addSpecial(object);

static_cast<TrackedGameObject*>(object)->m_fields->objectExists = true;
}

void TrackingEditorLayer::removeSpecial(GameObject* object) {
    if (object == nullptr) {
        return;
    }

static_cast<TrackedGameObject*>(object)->m_fields->objectExists = false;

LevelEditorLayer::removeSpecial(object);
}
