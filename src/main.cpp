// ReSharper disable CppHidingFunction
// ReSharper disable CppLocalVariableMayBeConst
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef
// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppFunctionalStyleCast

#include <Geode/Geode.hpp>

#include "engine/LuaEngine.hpp"
#include "ui/ScriptSelectorPopup.hpp"

using namespace geode::prelude;

#include <Geode/modify/EditorUI.hpp>

class $modify(GeometronEditorUI, EditorUI) {
    static void luaPanic(sol::optional<std::string> msg) {
        if (msg) {
            log::error("Lua panic triggered: {}", msg.value());
        } else {
            log::error("Lua panic triggered with no error info");
        }

*reinterpret_cast<volatile int*>(0) = 1;
    }

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

CCArrayExt<CCNode*> children = scene->getChildren();

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

auto popup = ScriptSelectorPopup::create(this);

if (popup == nullptr) {
                    log::error(
                        "Geometron: could not initialize script selector"
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
        auto popup = ScriptSelectorPopup::create(this);

if (popup == nullptr) {
            log::error(
                "Geometron: could not initialize script selector"
            );
            return;
        }

popup->m_noElasticity = true;
        popup->show();
    }

$override
    void createMoveMenu() {
        /*
         * Important:
         * We intentionally do not add the custom Geometron button here.
         *
         * The previous implementation accessed:
         *
         *   m_editButtonBar->m_buttonArray
         *   m_editButtonBar->reloadItems(...)
         *
         * while the editor UI was still being initialized.
         * This caused Geometry Dash to crash when opening the editor.
         *
         * The script menu remains accessible with Ctrl+B, as defined
         * in mod.json.
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
