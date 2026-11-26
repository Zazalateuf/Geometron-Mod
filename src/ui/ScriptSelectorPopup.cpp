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

// Enter est géré par un hook du clavier.
// TextInputDelegate n'existe plus dans Geode v6.
#include <Geode/modify/CCKeyboardDispatcher.hpp>

class $modify(ConsoleKeyboardHook, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(
        enumKeyCodes key,
        bool isKeyDown,
        bool isKeyRepeat,
        double idk
    ) {
        if (key == KEY_Enter && isKeyDown) {
            auto luaEngine = LuaEngine::get();

if (luaEngine != nullptr) {
                auto console = luaEngine->getCurrentConsole();

if (
                    console != nullptr &&
                    console->consoleTab != nullptr &&
                    console->consoleTab->isVisible()
                ) {
                    console->sendConsoleInput(nullptr);
                    return true;
                }
            }
        }

return CCKeyboardDispatcher::dispatchKeyboardMSG(
            key,
            isKeyDown,
            isKeyRepeat,
            idk
        );
    }
};

inline static float FONT_SIZE_MULT = 0.6f;
inline static std::string FONT_NAME =
    "geode.loader/mdFontMono.fnt";

bool ScriptSelectorPopup::init(EditorUI* editor) {
    if (editor == nullptr) {
        log::error(
            "Geometron: ScriptSelectorPopup received a null EditorUI"
        );
        return false;
    }

if (!Popup::init(380.0f, 280.0f)) {
        return false;
    }

setUserObject(
        "is-script-selector-popup",
        CCBool::create(true)
    );

this->editor = editor;

auto popupSize = m_mainLayer->getContentSize();

if (Settings::interfaceTransparentScriptMenu()) {
        auto newBG = NineSlice::create("square02_001.png");

if (newBG == nullptr) {
            log::error(
                "Geometron: could not create popup background"
            );
            return false;
        }

newBG->setOpacity(125);
        newBG->setContentSize(m_bgSprite->getContentSize());
        newBG->setScaleMultiplier(
            m_bgSprite->getScaleMultiplier()
        );
        newBG->setPosition(m_bgSprite->getPosition());

m_mainLayer->addChild(newBG);
        m_bgSprite->removeFromParentAndCleanup(true);
        m_bgSprite = newBG;
    }

auto scriptLoader = ScriptLoader::get();

if (scriptLoader == nullptr) {
        log::error(
            "Geometron: ScriptLoader is null"
        );
        return false;
    }

auto scripts = scriptLoader->getScripts();

if (!scriptLoader->isLoaded()) {
        log::error(
            "Geometron: ScriptLoader is not loaded"
        );
        return false;
    }

engine = LuaEngine::get();

if (engine == nullptr) {
        log::error(
            "Geometron: LuaEngine is null"
        );
        return false;
    }

tabMenu = CCMenu::create();

if (tabMenu == nullptr) {
        log::error(
            "Geometron: could not create popup tab menu"
        );
        return false;
    }

tabMenu->setPosition({
        popupSize.width / 2,
        popupSize.height - 22
    });
    tabMenu->setContentSize({
        popupSize.width - 80,
        25
    });
    tabMenu->setZOrder(1);
    tabMenu->setTouchPriority(-503);
    tabMenu->setID("tab-menu"_spr);

scriptTabButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create(
            "Scripts",
            100,
            0,
            0.55,
            true,
            "bigFont.fnt",
            "GJ_button_04.png",
            24.0
        ),
        this,
        menu_selector(
            ScriptSelectorPopup::onClickTab
        )
    );

if (scriptTabButton == nullptr) {
        log::error(
            "Geometron: could not create scripts tab button"
        );
        return false;
    }

scriptTabButton->setTag(0);
    scriptTabButton->setID("tab-button-scripts"_spr);

consoleTabButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create(
            "Console",
            100,
            0,
            0.55,
            true,
            "bigFont.fnt",
            "GJ_button_04.png",
            24.0
        ),
        this,
        menu_selector(
            ScriptSelectorPopup::onClickTab
        )
    );

if (consoleTabButton == nullptr) {
        log::error(
            "Geometron: could not create console tab button"
        );
        return false;
    }

consoleTabButton->setTag(1);
    consoleTabButton->setID("tab-button-console"_spr);

tabMenu->addChild(scriptTabButton);
    tabMenu->addChild(consoleTabButton);
    tabMenu->setLayout(RowLayout::create());
    m_mainLayer->addChild(tabMenu);

scriptTab = CCMenu::create();
    consoleTab = CCMenu::create();

if (
        scriptTab == nullptr ||
        consoleTab == nullptr
    ) {
        log::error(
            "Geometron: could not create popup tabs"
        );
        return false;
    }

scriptTab->setContentSize(popupSize);
    scriptTab->setPosition(popupSize / 2);
    scriptTab->ignoreAnchorPointForPosition(false);
    scriptTab->setID("tab-container-scripts"_spr);

consoleTab->setContentSize(popupSize);
    consoleTab->setPosition(popupSize / 2);
    consoleTab->ignoreAnchorPointForPosition(false);
    consoleTab->setID("tab-container-console"_spr);

// Les deux onglets utilisent des zones de défilement.
    // Ils sont conservés manuellement afin qu'un seul soit actif.
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

if (row == nullptr) {
                log::error(
                    "Geometron: could not create interactive row"
                );
                return false;
            }

row->setContentSize(rowSize);
            row->setID("interactive-mode-row"_spr);

auto buttonSprite = ButtonSprite::create(
                "Interactive Mode",
                130,
                0,
                0.55,
                true,
                "goldFont.fnt",
                "GJ_button_01.png",
                24.0
            );

auto button = CCMenuItemSpriteExtra::create(
                buttonSprite,
                this,
                menu_selector(
                    ScriptSelectorPopup::onRunInteractive
                )
            );

if (button == nullptr) {
                log::error(
                    "Geometron: could not create interactive button"
                );
                return false;
            }

button->setPosition({
                rowSize.width / 2,
                rowSize.height / 2
            });
            button->setID("interactive-move-button"_spr);

row->addChild(button);
            rows.push_back(row);
        }

for (int i = 0; i < scripts.size(); i++) {
            auto [name, path] = scripts[i];

auto row = CCMenu::create();

if (row == nullptr) {
                log::error(
                    "Geometron: could not create script row"
                );
                return false;
            }

row->setContentSize(rowSize);
            row->setID("script-row"_spr);

auto label = CCLabelBMFont::create(
                name.c_str(),
                "chatFont.fnt"
            );

if (label == nullptr) {
                log::error(
                    "Geometron: could not create script label"
                );
                return false;
            }

label->setAnchorPoint({0, 0.5});
            label->setPosition({
                15,
                rowSize.height / 2
            });
            label->setScale(0.5);
            label->setID("script-label"_spr);

auto buttonSprite = ButtonSprite::create(
                "Run",
                50,
                false,
                "bigFont.fnt",
                "GJ_button_01.png",
                30,
                1.0f
            );

auto button = CCMenuItemSpriteExtra::create(
                buttonSprite,
                this,
                menu_selector(
                    ScriptSelectorPopup::onClickRun
                )
            );

if (button == nullptr) {
                log::error(
                    "Geometron: could not create script button"
                );
                return false;
            }

button->setTag(i);
            button->setPosition({
                rowSize.width -
                    15 -
                    button->getContentSize().width / 2,
                rowSize.height / 2
            });
            button->setID("script-run-button"_spr);

row->addChild(label);
            row->addChild(button);

rows.push_back(row);
        }

list = ListView::create(
            rows.inner(),
            rowSize.height,
            listSize.width,
            listSize.height
        );

if (list == nullptr) {
            log::error(
                "Geometron: could not create scripts list"
            );
            return false;
        }

list->setPosition(listPosition);
        list->ignoreAnchorPointForPosition(false);
        list->setID("script-list"_spr);

listBorders = ListBorders::create();

if (listBorders == nullptr) {
            log::error(
                "Geometron: could not create list borders"
            );
            return false;
        }

listBorders->setContentSize(
            listSize + CCSize(3, 5)
        );
        listBorders->setPosition(listPosition);
        listBorders->setZOrder(1);
        listBorders->setID("script-list-border"_spr);

if (Settings::interfaceTransparentScriptMenu()) {
            list->setPrimaryCellColor({0, 0, 0});
            list->setSecondaryCellColor({
                25,
                25,
                25
            });
            list->setCellOpacity(90);
            listBorders->setVisible(false);
        }

scriptTab->addChild(list);
        scriptTab->addChild(listBorders);
    }

{
        auto consoleSize = CCSize(300.0f, 200.0f);
        auto consolePosition = CCPoint(popupSize / 2);
        auto consoleOutSize = CCSize(
            consoleSize.width,
            consoleSize.height - 35
        );
        auto consoleOutOffset =
            (consoleSize - consoleOutSize) / 2;
        auto consoleInSize = CCSize(
            consoleSize.width,
            30
        );
        auto consoleInOffset =
            -(consoleSize - consoleInSize) / 2;

consoleBG = CCScale9Sprite::create(
            "square02b_001.png"
        );

if (consoleBG == nullptr) {
            log::error(
                "Geometron: could not create console background"
            );
            return false;
        }

consoleBG->setScale(0.5f);
        consoleBG->setColor({0, 0, 0});
        consoleBG->setOpacity(90);
        consoleBG->setContentSize(consoleOutSize * 2);
        consoleBG->setZOrder(-1);
        consoleBG->setID("console-background"_spr);

consoleTab->addChildAtPosition(
            consoleBG,
            Anchor::Center,
            consoleOutOffset
        );

consoleWindow = ScrollingWindow::create(
            consoleOutSize - CCSize(20, 20)
        );

if (consoleWindow == nullptr) {
            log::error(
                "Geometron: could not create console window"
            );
            return false;
        }

consoleWindow->ignoreAnchorPointForPosition(false);
        consoleWindow->setID("console-output-pane"_spr);

consoleText = EnhancedTextPanel::create(
            consoleWindow,
            12 *
                Settings::consoleLineHeightMultiplier() *
                Settings::consoleFontSize(),
            FONT_NAME,
            FONT_SIZE_MULT *
                Settings::consoleFontSize()
        );

if (consoleText == nullptr) {
            log::error(
                "Geometron: could not create console text panel"
            );
            return false;
        }

consoleText->setAnchorPoint({0, 0});
        consoleText->ignoreAnchorPointForPosition(false);
        consoleText->setContentSize(
            consoleOutSize - CCSize(20, 20)
        );
        consoleText->setID("console-output-text"_spr);

consoleWindow->addInnerChild(consoleText);
        consoleTab->addChildAtPosition(
            consoleWindow,
            Anchor::Center,
            consoleOutOffset
        );

consoleInput = TextInput::create(
            consoleInSize.width - 40,
            "",
            FONT_NAME
        );

if (consoleInput == nullptr) {
            log::error(
                "Geometron: could not create console input"
            );
            return false;
        }

consoleInput->setCommonFilter(CommonFilter::Any);
        consoleInput->setAlignment(Label::Alignment::Left);
        consoleInput->setID("console-input"_spr);

consoleSendButton = CCMenuItemSpriteExtra::create(
            ButtonSprite::create(
                ">",
                30,
                false,
                "bigFont.fnt",
                "GJ_button_01.png",
                30,
                1.0f
            ),
            this,
            menu_selector(
                ScriptSelectorPopup::sendConsoleInput
            )
        );

if (consoleSendButton == nullptr) {
            log::error(
                "Geometron: could not create console send button"
            );
            return false;
        }

consoleSendButton->setID("console-send-button"_spr);

consoleTab->addChildAtPosition(
            consoleSendButton,
            Anchor::Center,
            consoleInOffset +
                CCPoint(
                    (consoleInSize.width - 40) / 2.0f + 3,
                    0
                )
        );

consoleTab->addChildAtPosition(
            consoleInput,
            Anchor::Center,
            consoleInOffset - CCPoint(20, 0)
        );
    }

if (
        lastTabIndex == 1 &&
        Settings::consoleAutoFocusInput()
    ) {
        Loader::get()->queueInMainThread(
            [consoleInput = Ref(consoleInput)]() {
                if (consoleInput != nullptr) {
                    consoleInput->focus();
                }
            }
        );
    }

loadingIndicator = LoadingCircleSprite::create(1.0f);

if (loadingIndicator == nullptr) {
        log::error(
            "Geometron: could not create loading indicator"
        );
        return false;
    }

loadingIndicator->setScale(0.45f);
    loadingIndicator->setZOrder(1);
    loadingIndicator->setID(
        "script-loading-indicator"
    );

auto cancelButtonSprite =
        CCSprite::createWithSpriteFrameName(
            "GJ_stopEditorBtn_001.png"
        );

if (cancelButtonSprite == nullptr) {
        log::error(
            "Geometron: cancel button sprite not present"
        );
        return false;
    }

cancelButtonSprite->setScale(0.7f);

cancelButton = CCMenuItemSpriteExtra::create(
        cancelButtonSprite,
        this,
        menu_selector(
            ScriptSelectorPopup::onClickCancel
        )
    );

if (cancelButton == nullptr) {
        log::error(
            "Geometron: could not create cancel button"
        );
        return false;
    }

cancelButton->setID("script-cancel-button");

statusLabel = CCLabelBMFont::create(
        "",
        "chatFont.fnt"
    );

if (statusLabel == nullptr) {
        log::error(
            "Geometron: could not create status label"
        );
        return false;
    }

statusLabel->setAnchorPoint({0, 0.5});
    statusLabel->setID("script-status-label");

updateScriptStatus();

m_mainLayer->addChildAtPosition(
        loadingIndicator,
        Anchor::BottomLeft,
        {55, 22.5}
    );

m_buttonMenu->addChildAtPosition(
        cancelButton,
        Anchor::BottomLeft,
        {55, 22.5}
    );

m_mainLayer->addChildAtPosition(
        statusLabel,
        Anchor::BottomLeft,
        {85, 22.5}
    );

#ifdef GEODE_IS_MOBILE
    scheduleUpdate();
#endif

return true;
}

ScriptSelectorPopup::~ScriptSelectorPopup() {
    if (scriptTab != nullptr) {
        scriptTab->release();
        scriptTab = nullptr;
    }

if (consoleTab != nullptr) {
        consoleTab->release();
        consoleTab = nullptr;
    }
}

void ScriptSelectorPopup::update(float delta) {
#ifdef GEODE_IS_MOBILE
    if (consoleInput != nullptr) {
        bool focused = consoleInput->isFocused();

if (focused != consoleInputShifted) {
            consoleInputShifted = focused;
            m_mainLayer->setPositionY(
                m_mainLayer->getPositionY() +
                (focused ? 140 : -140)
            );
        }
    }
#endif
}

void ScriptSelectorPopup::updateScriptStatus(bool force) {
    if (engine == nullptr) {
        return;
    }

if (
        engine->fastGetIsExecuting() &&
        engine->getStatus().type != STOPPED
    ) {
        auto status = engine->getStatus();

bool changed = force;

if (status.scriptName != lastScriptName) {
            lastScriptName = status.scriptName;
            changed = true;
        }

if (status.type != lastExecutionType) {
            lastExecutionType = status.type;
            changed = true;
        }

if (status.yieldType != lastYieldType) {
            lastYieldType = status.yieldType;
            changed = true;
        }

if (changed) {
            lastScriptName = status.scriptName;

if (loadingIndicator != nullptr) {
                loadingIndicator->setVisible(true);
            }

if (cancelButton != nullptr) {
                cancelButton->setVisible(true);
            }

std::string statusName = "(Unknown status)";

switch (status.yieldType) {
                case PREEMPTED:
                case MANUAL:
                    statusName = "Running...";
                    break;

case SLEEPING:

