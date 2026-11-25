// ReSharper disable CppParameterMayBeConstPtrOrRef
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppLocalVariableMayBeConst
// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppHidingFunction
#include <Geode/Geode.hpp>

#include "ScriptSelectorPopup.hpp"

#include "../engine/LuaEngine.hpp"
#include "EnhancedTextPanel.hpp"
#include "ScrollingWindow.hpp"

#include "../utils/ScriptLoader.hpp"
#include "../utils/Settings.hpp"

using namespace geode::prelude;

volatile int lastTabIndex = 0;

//enter is handled through a keyboard dispatcher hook (TextInputDelegate no longer exists in Geode v6)
#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(ConsoleKeyboardHook, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown, bool isKeyRepeat, double idk) {
        if (key == KEY_Enter && isKeyDown) {
            auto console = LuaEngine::get()->getCurrentConsole();
            if (console != nullptr && console->consoleTab->isVisible()) {
                console->sendConsoleInput(nullptr);
                return true;
            }
        }

        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat, idk);
    }
};

inline static float FONT_SIZE_MULT = 0.6f;
inline static std::string FONT_NAME = "geode.loader/mdFontMono.fnt";//"AdwaitaMono-Regular.fnt"_spr;
bool ScriptSelectorPopup::init(EditorUI* editor) {
    if (!Popup::init(380.0f, 280.0f))
        return false;

    setUserObject("is-script-selector-popup", CCBool::create(true));

    this->editor = editor;

    auto popupSize = m_mainLayer->getContentSize();
    if (Settings::interfaceTransparentScriptMenu()) {
        NineSlice* newBG = NineSlice::create("square02_001.png");
        newBG->setOpacity(125);
        newBG->setContentSize(m_bgSprite->getContentSize());
        newBG->setScaleMultiplier(m_bgSprite->getScaleMultiplier());
        newBG->setPosition(m_bgSprite->getPosition());
        m_mainLayer->addChild(newBG);
        m_bgSprite->removeFromParentAndCleanup(true);
        m_bgSprite = newBG;
    }

    auto scriptLoader = ScriptLoader::get();
    auto scripts = scriptLoader->getScripts();
    if (!scriptLoader->isLoaded()) {
        errorMessage = "ScriptLoader failed to initialize properly.\nCheck the logs for more details.";
        return true;
    }
    engine = LuaEngine::get();
    if (!engine) {
        errorMessage = "LuaEngine failed to initialize properly.\nCheck the logs for more details.";
        return true;
    }

    tabMenu = CCMenu::create();
    tabMenu->setPosition({popupSize.width / 2, popupSize.height - 22});
    tabMenu->setContentSize({popupSize.width - 80, 25});
    tabMenu->setZOrder(1);
    //this seems to work i guess???
    tabMenu->setTouchPriority(-503);
    tabMenu->setID("tab-menu"_spr);

    scriptTabButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Scripts", 100, 0, 0.55, true, "bigFont.fnt", "GJ_button_04.png", 24.0),
        this, menu_selector(ScriptSelectorPopup::onClickTab));
    scriptTabButton->setTag(0);
    scriptTabButton->setID("tab-button-scripts"_spr);
    consoleTabButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Console", 100, 0, 0.55, true, "bigFont.fnt", "GJ_button_04.png", 24.0),
        this, menu_selector(ScriptSelectorPopup::onClickTab));
    consoleTabButton->setTag(1);
    consoleTabButton->setID("tab-button-console"_spr);

    tabMenu->addChild(scriptTabButton);
    tabMenu->addChild(consoleTabButton);
    tabMenu->setLayout(RowLayout::create());
    m_mainLayer->addChild(tabMenu);

    scriptTab = CCMenu::create();
    scriptTab->setContentSize(popupSize);
    scriptTab->setPosition(popupSize / 2);
    scriptTab->ignoreAnchorPointForPosition(false);
    scriptTab->setID("tab-container-scripts"_spr);
    consoleTab = CCMenu::create();
    consoleTab->setContentSize(popupSize);
    consoleTab->setPosition(popupSize / 2);
    consoleTab->ignoreAnchorPointForPosition(false);
    consoleTab->setID("tab-container-console"_spr);

    //both the script tab and the console tab have scrolling layers and use the scroll wheel
    //if both are present at the same time the scrolling layers would fight for input causing weird issues
    //so when one tab is active the other is removed, meaning both tabs should be manually retained
    scriptTab->retain();
    consoleTab->retain();

    setTab(lastTabIndex);
    {
        auto listSize = CCSize(300.0f, 200.0f);
        auto listPosition = CCPoint(popupSize / 2);
        auto rowSize = CCSize(listSize.width, 50);

        CCArrayExt<CCMenu*> rows;
        {
            auto row = CCMenu::create();
            row->setContentSize(rowSize);
            row->setID("interactive-mode-row"_spr);

            auto buttonSprite = ButtonSprite::create("Interactive Mode", 130, 0, 0.55, true, "goldFont.fnt", "GJ_button_01.png", 24.0);
            auto button = CCMenuItemSpriteExtra::create(buttonSprite, this, menu_selector(ScriptSelectorPopup::onRunInteractive));
            button->setPosition({rowSize.width / 2, rowSize.height / 2});
            button->setID("interactive-move-button"_spr);

            row->addChild(button);
            rows.push_back(row);
        }
        for (int i = 0; i < scripts.size(); i++) {
            auto [name, path] = scripts[i];

            auto row = CCMenu::create();
            row->setContentSize(rowSize);
            row->setID("script-row"_spr);

            auto label = CCLabelBMFont::create(name.c_str(), "chatFont.fnt");
            label->setAnchorPoint({0, 0.5});
            label->setPosition({15, rowSize.height / 2});
            label->setScale(0.5);
            label->setID("script-label"_spr);

            auto buttonSprite = ButtonSprite::create("Run", 50, false, "bigFont.fnt", "GJ_button_01.png", 30, 1.0f);
            auto button = CCMenuItemSpriteExtra::create(buttonSprite, this, menu_selector(ScriptSelectorPopup::onClickRun));
            //button->setAnchorPoint({1, 0.5});
            button->setTag(i);
            button->setPosition({rowSize.width - 15 - button->getContentSize().width / 2, rowSize.height / 2});
            button->setID("script-run-button"_spr);

            row->addChild(label);
            row->addChild(button);

            rows.push_back(row);
        }

        list = ListView::create(rows.inner(), rowSize.height, listSize.width, listSize.height);
        list->setPosition(listPosition);
        list->ignoreAnchorPointForPosition(false);
        list->setID("script-list"_spr);

        listBorders = ListBorders::create();
        listBorders->setContentSize(listSize + CCSize(3, 5));
        listBorders->setPosition(listPosition);
        listBorders->setZOrder(1); //show above list
        listBorders->setID("script-list-border"_spr);

        if (Settings::interfaceTransparentScriptMenu()) {
            list->setPrimaryCellColor({0, 0, 0});
            list->setSecondaryCellColor({25, 25, 25});
            list->setCellOpacity(90);
            listBorders->setVisible(false);
        }
        scriptTab->addChild(list);
        scriptTab->addChild(listBorders);
    }
    {
        auto consoleSize = CCSize(300.0f, 200.0f);
        auto consolePosition = CCPoint(popupSize / 2);
        auto consoleOutSize = CCSize(consoleSize.width, consoleSize.height - 35);
        auto consoleOutOffset = (consoleSize - consoleOutSize) / 2;
        auto consoleInSize = CCSize(consoleSize.width, 30);
        auto consoleInOffset = -(consoleSize - consoleInSize) / 2;

        consoleBG = CCScale9Sprite::create("square02b_001.png");
        consoleBG->setScale(0.5f);
        consoleBG->setColor({0, 0, 0});
        consoleBG->setOpacity(90);
        consoleBG->setContentSize(consoleOutSize * 2);
        consoleBG->setZOrder(-1);
        consoleBG->setID("console-background"_spr);
        consoleTab->addChildAtPosition(consoleBG, Anchor::Center, consoleOutOffset);

        consoleWindow = ScrollingWindow::create(consoleOutSize - CCSize(20, 20));
        consoleWindow->ignoreAnchorPointForPosition(false);
        consoleWindow->setID("console-output-pane"_spr);

        consoleText = EnhancedTextPanel::create(consoleWindow,
            12 * Settings::consoleLineHeightMultiplier() * Settings::consoleFontSize(),
            FONT_NAME,
            FONT_SIZE_MULT * Settings::consoleFontSize());
        consoleText->setAnchorPoint({0, 0});
        consoleText->ignoreAnchorPointForPosition(false);
        consoleText->setContentSize(consoleOutSize - CCSize(20, 20));
        consoleText->setID("console-output-text"_spr);

        consoleWindow->addInnerChild(consoleText);
        consoleTab->addChildAtPosition(consoleWindow, Anchor::Center, consoleOutOffset);

        consoleInput = TextInput::create(consoleInSize.width - 40, "", FONT_NAME);
        consoleInput->setCommonFilter(CommonFilter::Any);
        consoleInput->setAlignment(Label::Alignment::Left);
        consoleInput->setID("console-input"_spr);

        consoleSendButton = CCMenuItemSpriteExtra::create(
            ButtonSprite::create(">", 30, false, "bigFont.fnt", "GJ_button_01.png", 30, 1.0f),
            this, menu_selector(ScriptSelectorPopup::sendConsoleInput));
        consoleSendButton->setID("console-send-button"_spr);

        consoleTab->addChildAtPosition(consoleSendButton, Anchor::Center, consoleInOffset + CCPoint((consoleInSize.width - 40) / 2.0f + 3, 0));
        consoleTab->addChildAtPosition(consoleInput, Anchor::Center, consoleInOffset - CCPoint(20, 0));
    }
    if (lastTabIndex == 1 && Settings::consoleAutoFocusInput()) {
        Loader::get()->queueInMainThread([consoleInput = Ref(consoleInput)]() {
            consoleInput->focus();
        });
    }

    loadingIndicator = LoadingCircleSprite::create(1.0f);
    loadingIndicator->setScale(0.45f);
    loadingIndicator->setZOrder(1);
    loadingIndicator->setID("script-loading-indicator");

    auto cancelButtonSprite = CCSprite::createWithSpriteFrameName("GJ_stopEditorBtn_001.png");
    if (!cancelButtonSprite) {
        log::error("Cancel button sprite not present");
        return false;
    }
    cancelButtonSprite->setScale(0.7f);
    cancelButton = CCMenuItemSpriteExtra::create(cancelButtonSprite, this, menu_selector(ScriptSelectorPopup::onClickCancel));
    cancelButton->setID("script-cancel-button");

    statusLabel = CCLabelBMFont::create("", "chatFont.fnt");
