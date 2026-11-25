// ReSharper disable CppMemberFunctionMayBeStatic
#include "TrackingEditorLayer.hpp"

#include "TrackedGameObject.hpp"

void TrackingEditorLayer::addSpecial(GameObject* object) {
    LevelEditorLayer::addSpecial(object);

if (object == nullptr) {
        return;
    }

auto trackedObject = static_cast<TrackedGameObject*>(object);
    if (trackedObject->m_fields != nullptr) {
        trackedObject->m_fields->objectExists = true;
    }
}

void TrackingEditorLayer::removeSpecial(GameObject* object) {
    if (object != nullptr) {
        auto trackedObject = static_cast<TrackedGameObject*>(object);
        if (trackedObject->m_fields != nullptr) {
            trackedObject->m_fields->objectExists = false;
        }
    }

LevelEditorLayer::removeSpecial(object);
}
