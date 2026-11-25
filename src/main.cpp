#include <Geode/Geode.hpp>

#include "engine/LuaEngine.hpp"
#include "ui/ScriptSelectorPopup.hpp"

using namespace geode::prelude;

#include <Geode/modify/EditorUI.hpp>

class $modify(GeometronEditorUI, EditorUI) {
    $override
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

this->addEventListener(
            KeybindSettingPressedEventV3(
                Mod::get(),
                "keybind-script-menu"
            ),
            [this](
                Keybind const& keybind,
                bool down,
                bool repeat,
                double timestamp
            ) {
                if (!down) {
                    return ListenerResult::Propagate;
                }

auto scene = CCScene::get();

if (scene == nullptr) {
                    log::error(
                        "Geometron: current scene is null"
                    );
                    return ListenerResult::Propagate;
                }

CCArrayExt<CCNode*> children =
                    scene->getChildren();

for (auto child : children) {
                    if (
                        child != nullptr &&
                        child->getUserObject(
                            "is-script-selector-popup"
                        )
                    ) {
                        return ListenerResult::Propagate;
                    }
                }

auto popup =
                    ScriptSelectorPopup::create(this);

if (popup == nullptr) {
                    log::error(
                        "Geometron: failed to create "
                        "ScriptSelectorPopup"
                    );
                    return ListenerResult::Propagate;
                }

popup->m_noElasticity = true;
                popup->show();

return ListenerResult::Propagate;
            }
        );

return true;
    }

void onScriptButton(CCObject* object) {
        auto popup =
            ScriptSelectorPopup::create(this);

if (popup == nullptr) {
            log::error(
                "Geometron: failed to create "
                "ScriptSelectorPopup"
            );
            return;
        }

popup->m_noElasticity = true;
        popup->show();
    }

$override
    void createMoveMenu() {
        /*
         * Le bouton personnalisé est désactivé temporairement.
         * L'accès au menu se fait avec Ctrl+B.
         */
        EditorUI::createMoveMenu();
    }
};

#include <Geode/modify/AppDelegate.hpp>

class $modify(GeometronAppDelegate, AppDelegate) {
    $override
    void willSwitchToScene(CCScene* scene) {
        if (LuaEngine::isInitialized()) {
            auto engine = LuaEngine::get();

if (
                engine != nullptr &&
                engine->editor != nullptr
            ) {
                auto editorLayer =
                    scene != nullptr
                        ? scene->getChildByType<LevelEditorLayer>(0)
                        : nullptr;

if (
                    editorLayer == nullptr ||
                    editorLayer->m_editorUI != engine->editor
                ) {
                    engine->forceReset();
                }
            }
        }

AppDelegate::willSwitchToScene(scene);
    }
};
