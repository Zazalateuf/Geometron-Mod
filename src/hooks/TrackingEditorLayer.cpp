#include "TrackingEditorLayer.hpp"
#include "TrackedGameObject.hpp"

// Marque les objets comme « existants » tant qu'ils sont dans l'éditeur.
// gameObjectExists() (LuaUtils.cpp) lit ce drapeau ; s'il reste à false,
// chaque appel Lua prenant un objet échoue avec « object does not exist ».

void TrackingEditorLayer::addSpecial(GameObject* object) {
    LevelEditorLayer::addSpecial(object);

    if (object != nullptr) {
        static_cast<TrackedGameObject*>(object)->m_fields->objectExists = true;
    }
}

void TrackingEditorLayer::removeSpecial(GameObject* object) {
    if (object != nullptr) {
        static_cast<TrackedGameObject*>(object)->m_fields->objectExists = false;
    }

    LevelEditorLayer::removeSpecial(object);
}
