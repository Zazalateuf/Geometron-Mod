#pragma once

#ifndef GEOMETRON_SCRIPTSELECTORPOPUP_HPP
#define GEOMETRON_SCRIPTSELECTORPOPUP_HPP

#include <Geode/Geode.hpp>

enum ScriptExecutionType : int;
enum ScriptYieldType : int;
class LuaEngine;
class EnhancedTextPanel;
class ScrollingWindow;
using namespace geode::prelude;

class ScriptSelectorPopup : public Popup {
    Ref<EditorUI> editor;

    LuaEngine* engine = nullptr;

    CCMenu* scriptTab = nullptr;
    ScrollingWindow* list = nullptr;
    ListBorders* listBorders = nullptr;
    CCMenu* consoleTab = nullptr;
    CCScale9Sprite* consoleBG = nullptr;
    EnhancedTextPanel* consoleText = nullptr;
    ScrollingWindow* consoleWindow = nullptr;
    TextInput* consoleInput = nullptr;
    CCMenuItemSpriteExtra* consoleSendButton = nullptr;

    CCMenu* tabMenu = nullptr;
    CCMenuItemSpriteExtra* scriptTabButton = nullptr;
    CCMenuItemSpriteExtra* consoleTabButton = nullptr;

    CCLabelBMFont* statusLabel = nullptr;
    LoadingCircleSprite* loadingIndicator = nullptr;
    CCMenuItemSpriteExtra* cancelButton = nullptr;

    std::string lastScriptName;
    ScriptExecutionType lastExecutionType = static_cast<ScriptExecutionType>(-1);
    ScriptYieldType lastYieldType = static_cast<ScriptYieldType>(-1);

    std::optional<std::string> errorMessage;

#ifdef GEODE_IS_MOBILE
    // Geode v6 removed TextInputDelegate, so the software keyboard offset is now driven by polling focus
    bool consoleInputShifted = false;
#endif

    bool init(EditorUI* editor);

    ~ScriptSelectorPopup() override;

    void update(float delta) override;

    void addConsoleLine(const std::string& line);
    void sendConsoleInput(CCObject*);

    void setTab(int tab) const;

    void onClickTab(CCObject* object);
    void onClickRun(CCObject* object);

    void onRunInteractive(CCObject* object);

    void onClickCancel(CCObject* object);

    void startScript(const std::string& code, const std::string& name, int flag);

    friend class ConsoleKeyboardHook;

public:
    static ScriptSelectorPopup* create(EditorUI* editor);

    void updateScriptStatus(bool force = false);
    void onConsoleText(const std::string& text);
    void onConsoleLine(const std::string& line);

    void onKeybindRelease();

    void show() override;
    void onClose(CCObject*) override;
};

#endif //GEOMETRON_SCRIPTSELECTORPOPUP_HPP
