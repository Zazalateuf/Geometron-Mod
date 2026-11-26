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
                    console->consoleTab->getParent() != nullptr &&
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

// Si la police mono de Geode est introuvable, on retombe sur chatFont.fnt
// (une police absente provoquait un pointeur nul -> crash a l'ouverture).
static std::string const& consoleFontName() {
    static std::string name = [] {
        std::string preferred = "geode.loader/mdFontMono.fnt";
        if (CCBMFontConfiguration::create(preferred.c_str()) != nullptr) {
            return preferred;
        }
        log::warn("Geometron: {} not found, using chatFont.fnt", preferred);
        return std::string("chatFont.fnt");
    }();
    return name;
}

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

// ListView/TableView plante dans Geode 6 (macOS arm64) :
        // on utilise une zone de défilement simple à la place.
        list = ScrollingWindow::create(listSize);

        if (list == nullptr) {
            log::error(
                "Geometron: could not create scripts list"
            );
            return false;
        }

        list->setAnchorPoint({0.5f, 0.5f});
        list->ignoreAnchorPointForPosition(false);
        list->setPosition(listPosition);
        list->setID("script-list"_spr);

        float totalHeight = std::max(
            listSize.height,
            rowSize.height * static_cast<float>(rows.size())
        );
        list->resizeInner(totalHeight);

        for (size_t i = 0; i < rows.size(); i++) {
            auto row = rows[i];
            row->setAnchorPoint({0, 0});
            row->setPosition({
                0,
                totalHeight -
                    rowSize.height * static_cast<float>(i + 1)
            });

            if (i % 2 == 1) {
                auto stripe = CCLayerColor::create(
                    {255, 255, 255, 12},
                    rowSize.width,
                    rowSize.height
                );
                stripe->setZOrder(-1);
                row->addChild(stripe);
            }

            list->addInnerChild(row);
        }

        // Affiche le haut de la liste.
        list->scrollTo(totalHeight - listSize.height);

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
            listBorders->setVisible(false);
        } else {
            auto listBG = CCLayerColor::create(
                {0, 0, 0, 90},
                listSize.width,
                listSize.height
            );
            listBG->ignoreAnchorPointForPosition(false);
            listBG->setAnchorPoint({0.5f, 0.5f});
            listBG->setPosition(listPosition);
            listBG->setZOrder(-1);
            scriptTab->addChild(listBG);
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
            consoleFontName(),
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

// Rejoue l'historique de la console deja present dans le moteur.
        for (auto const& l : engine->getLines()) {
            consoleText->addLine(
                l.ends_with('\n') ? l.substr(0, l.size() - 1) : l
            );
        }
        consoleWindow->resizeInner(
            consoleText->lineCount() * consoleText->getLineHeight()
        );
        consoleTab->addChildAtPosition(
            consoleWindow,
            Anchor::Center,
            consoleOutOffset
        );

consoleInput = TextInput::create(
            consoleInSize.width - 40,
            "",
            consoleFontName()
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
    if (engine == nullptr || statusLabel == nullptr) {
        return;
    }

    if (
        engine->fastGetIsExecuting() &&
        engine->getStatus().type != STOPPED
    ) {
        auto const& status = engine->getStatus();

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
            if (loadingIndicator != nullptr) {
                loadingIndicator->setVisible(true);
            }

            if (cancelButton != nullptr) {
                cancelButton->setVisible(true);
            }

            std::string statusName = "(Unknown status)";

            switch (status.type) {
                case TERMINATING_TIMEOUT:
                    statusName = "Terminating (timed out)...";
                    break;
                case TERMINATING_CANCELED:
                    statusName = "Canceling...";
                    break;
                case YIELDING:
                    switch (status.yieldType) {
                        case SLEEPING:
                            statusName = "Sleeping...";
                            break;
                        case WAITING_FOR_CONSOLE_NEWLINE:
                        case WAITING_FOR_CONSOLE_CHARACTERS:
                            statusName = "Waiting for input...";
                            break;
                        case PREEMPTED:
                        case MANUAL:
                            statusName = "Running...";
                            break;
                    }
                    break;
                case RUNNING:
                default:
                    statusName = "Running...";
                    break;
            }

            statusLabel->setString(
                fmt::format(
                    "{}: {}",
                    status.scriptName,
                    statusName
                ).c_str()
            );
            statusLabel->limitLabelWidth(
                m_mainLayer->getContentWidth() - 100.0f,
                0.5f,
                0.1f
            );
        }
    } else {
        // Aucun script en cours
        lastScriptName.clear();
        lastExecutionType = static_cast<ScriptExecutionType>(-1);
        lastYieldType = static_cast<ScriptYieldType>(-1);

        if (loadingIndicator != nullptr) {
            loadingIndicator->setVisible(false);
        }

        if (cancelButton != nullptr) {
            cancelButton->setVisible(false);
        }

        statusLabel->setString("");
    }
}

void ScriptSelectorPopup::addConsoleLine(const std::string& line) {
    if (consoleText == nullptr || consoleWindow == nullptr) {
        return;
    }

    bool atBottom =
        consoleWindow->scrollHeight() <
        consoleText->getLineHeight() / 2.0f;

    consoleText->addLine(line);
    consoleWindow->resizeInner(
        consoleText->lineCount() * consoleText->getLineHeight()
    );

    if (atBottom) {
        consoleWindow->scrollTo(0);
    }
}

void ScriptSelectorPopup::onConsoleLine(const std::string& line) {
    addConsoleLine(line);
}

void ScriptSelectorPopup::onConsoleText(const std::string& text) {
    if (consoleText == nullptr || consoleWindow == nullptr) {
        return;
    }

    int count = consoleText->lineCount();

    if (count <= 0) {
        addConsoleLine(text);
        return;
    }

    bool atBottom =
        consoleWindow->scrollHeight() <
        consoleText->getLineHeight() / 2.0f;

    consoleText->setLine(
        count - 1,
        consoleText->getLineFromBottom(0) + text
    );
    consoleWindow->resizeInner(
        consoleText->lineCount() * consoleText->getLineHeight()
    );

    if (atBottom) {
        consoleWindow->scrollTo(0);
    }
}

void ScriptSelectorPopup::sendConsoleInput(CCObject*) {
    if (engine == nullptr || consoleInput == nullptr) {
        return;
    }

    std::string text = consoleInput->getString();
    consoleInput->setString("");

    // sendInputString fait aussi l'écho dans la console.
    engine->sendInputString(text + "\n");

    consoleInput->focus();
}

void ScriptSelectorPopup::setTab(int tab) const {
    lastTabIndex = tab;

    if (
        m_mainLayer == nullptr ||
        scriptTab == nullptr ||
        consoleTab == nullptr
    ) {
        return;
    }

    CCNode* shown = tab == 0 ? scriptTab : consoleTab;
    CCNode* hidden = tab == 0 ? consoleTab : scriptTab;

    // Les onglets sont retenus (retain), on peut donc les détacher sans risque.
    if (hidden->getParent() != nullptr) {
        hidden->removeFromParent();
    }

    if (shown->getParent() == nullptr) {
        m_mainLayer->addChild(shown);
    }

    auto highlight = [](CCMenuItemSpriteExtra* button, bool active) {
        if (button == nullptr) {
            return;
        }

        if (
            auto sprite = typeinfo_cast<ButtonSprite*>(
                button->getNormalImage()
            )
        ) {
            sprite->updateBGImage(
                active ? "GJ_button_02.png" : "GJ_button_04.png"
            );
        }
    };

    highlight(scriptTabButton, tab == 0);
    highlight(consoleTabButton, tab == 1);
}

void ScriptSelectorPopup::onClickTab(CCObject* object) {
    auto node = typeinfo_cast<CCNode*>(object);

    if (node == nullptr) {
        return;
    }

    int tab = node->getTag();
    setTab(tab);

    if (tab == 1 && Settings::consoleAutoFocusInput()) {
        if (consoleInput != nullptr) {
            consoleInput->focus();
        }
    }
}

static void showScriptResult(
    const std::string& name,
    const ScriptResult& result
) {
    std::string message;

    switch (result.result) {
        case OK:
        case SCRIPT_CANCELED:
            return;
        case LUA_RUN_ERROR:
            message = fmt::format(
                "Runtime error in {}:\n{}",
                name,
                result.errorMessage
            );
            break;
        case LUA_COMPILE_ERROR:
            message = fmt::format(
                "Compile error in {}:\n{}",
                name,
                result.errorMessage
            );
            break;
        case ENGINE_LOCKED:
            message =
                "The Lua engine is busy, please try again.";
            break;
        case ENGINE_IN_USE:
            message =
                "Another script is already running.";
            break;
    }

    FLAlertLayer::create(
        "Geometron",
        message,
        "OK"
    )->show();
}

void ScriptSelectorPopup::startScript(
    const std::string& code,
    const std::string& name,
    int flag
) {
    if (engine == nullptr || editor == nullptr) {
        return;
    }

    auto task = engine->executeAsync(
        editor,
        code,
        name,
        flag
    );

    if (!task) {
        return;
    }

    // Passe par le moteur : il ne notifie que la console encore enregistrée,
    // donc aucun accès à un popup déjà fermé.
    task->onProgress = [](const ScriptExecutionStatus*) {
        if (LuaEngine::isInitialized()) {
            LuaEngine::get()->updateScriptStatusForConsole();
        }
    };

    task->setOnResult([name](ScriptResult result) {
        showScriptResult(name, result);

        if (LuaEngine::isInitialized()) {
            LuaEngine::get()->updateScriptStatusForConsole();
        }
    });

    updateScriptStatus(true);
}

void ScriptSelectorPopup::onClickRun(CCObject* object) {
    auto node = typeinfo_cast<CCNode*>(object);
    auto scriptLoader = ScriptLoader::get();

    if (
        node == nullptr ||
        scriptLoader == nullptr ||
        engine == nullptr
    ) {
        return;
    }

    auto scripts = scriptLoader->getScripts();
    int index = node->getTag();

    if (index < 0 || index >= static_cast<int>(scripts.size())) {
        return;
    }

    auto const& entry = scripts[index];
    auto code = scriptLoader->loadScript(entry.name);

    if (!code) {
        FLAlertLayer::create(
            "Geometron",
            fmt::format("Could not read script {}", entry.name),
            "OK"
        )->show();
        return;
    }

    startScript(*code, entry.name, 0);

    // L'éditeur doit rester utilisable : on ferme le menu après le lancement.
    this->onClose(nullptr);
}

static const char* REPL_SCRIPT = R"LUA(local running = true
function exit()
    running = false
    error("exited", 0)
end

local function compile(input)
    --try expression first
    local func, err = load("return " .. input, "=interactive")
    if not func then
        func, err = load(input, "=interactive")
    end
    return func, err
end
local function run(func)
    local ret = table.pack(pcall(func))
    if ret.n >= 2 then
        print(table.unpack(ret, 2, ret.n))
    end
end
local function isIncompleteStatement(err)
    local i, j = string.find(err, "<eof>", 1, true)
    if i and j == #err then
        return true
    end
    return false
end
local function multiline(firstInput)
    local str = firstInput
    while true do
        io.write(" .. > ")

        local input = io.read()
        if input == "" then return end

        str = str .. "\n" .. input
        local func, err = compile(str)

        if func then
            run(func)
            return
        else
            if not isIncompleteStatement(err) then
                print(err)
                return
            end
        end
    end
end

local specialNames = {
    O = editor.getSelectedObject;
    OA = function() return smart(editor.getSelectedObjects()) end;
}
setmetatable(_ENV, {
    __index = function(_, name)
        local specialFunc = specialNames[name]
        if specialFunc then
            return specialFunc()
        end
        return nil
    end
})
CO = editor.createObject
function CCO(id, ...)
    local obj = editor.createObject(id, ...)
    if obj then
        obj.pos = editor.getViewGridCenter()
        if id == 747 then
            obj.orangePortal.y = obj.y + 100
        end
    end
    return obj
end
P = point.new

while running do
    io.write("> ")

    local input = io.read()
    local func, err = compile(input)

    if not func then
        if isIncompleteStatement(err) then
            multiline(input)
        else
            print(err)
        end
    else
        run(func)
    end

    if not running then break end
end
)LUA";

void ScriptSelectorPopup::onRunInteractive(CCObject*) {
    if (engine == nullptr) {
        return;
    }

    std::string code = REPL_SCRIPT;

    // Un repl.lua placé dans le dossier des scripts est prioritaire.
    if (auto loader = ScriptLoader::get()) {
        if (auto custom = loader->loadScript("repl.lua")) {
            code = *custom;
        }
    }

    startScript(code, "<interactive>", 1);
    setTab(1);

    if (consoleInput != nullptr) {
        consoleInput->focus();
    }
}

void ScriptSelectorPopup::onClickCancel(CCObject*) {
    if (engine == nullptr) {
        return;
    }

    auto const& status = engine->getStatus();

    if (
        status.asyncData.isAsync &&
        status.asyncData.task != nullptr
    ) {
        status.asyncData.task->cancel();
    }
}

void ScriptSelectorPopup::onKeybindRelease() {
    // Rien à faire : le menu est ouvert à l'appui de la touche.
}

ScriptSelectorPopup* ScriptSelectorPopup::create(EditorUI* editor) {
    auto ret = new ScriptSelectorPopup();

    if (ret->init(editor)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

void ScriptSelectorPopup::show() {
    Popup::show();

    if (engine != nullptr) {
        engine->registerConsole(this);
        updateScriptStatus(true);
    }
}

void ScriptSelectorPopup::onClose(CCObject* sender) {
    if (engine != nullptr) {
        engine->unregisterConsole(this);
    }

    Popup::onClose(sender);
}
