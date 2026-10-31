#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

static std::string currentStyle() {
    return Mod::get()->getSettingValue<std::string>("style");
}

// Earth-colored pixel tile background, drawn entirely in code
static CCNode* makeBlockyBackground(CCSize size) {
    auto node = CCDrawNode::create();
    const float tile = 16.f;
    unsigned int seed = 12345;
    for (float y = 0.f; y < size.height; y += tile) {
        for (float x = 0.f; x < size.width; x += tile) {
            seed = seed * 1664525u + 1013904223u;
            float r = ((seed >> 16) & 0xFF) / 255.f;
            float shade = 0.22f + r * 0.14f;
            ccColor4F c = ccc4f(shade * 1.4f, shade * 0.95f, shade * 0.6f, 1.f);
            CCPoint v[4] = {
                CCPoint(x, y),
                CCPoint(x + tile - 1.f, y),
                CCPoint(x + tile - 1.f, y + tile - 1.f),
                CCPoint(x, y + tile - 1.f)
            };
            node->drawPolygon(v, 4, c, 0.f, c);
        }
    }
    return node;
}

// Tints every button in a menu with one color
static void tintMenu(CCNode* menu, ccColor3B color) {
    if (!menu) return;
    for (auto child : CCArrayExt<CCNode*>(menu->getChildren())) {
        if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(child)) {
            if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(item->getNormalImage())) {
                rgba->setColor(color);
            }
        }
    }
}

// Stone block plate: black border, light top/left bevel, dark bottom/right bevel
static CCNode* makeBlockPlate(float w, float h) {
    auto node = CCNode::create();
    node->setContentSize({w, h});

    auto border = CCLayerColor::create(ccc4(0, 0, 0, 255), w, h);
    node->addChild(border);

    auto body = CCLayerColor::create(ccc4(112, 112, 112, 255), w - 4.f, h - 4.f);
    body->setPosition({2.f, 2.f});
    node->addChild(body);

    auto top = CCLayerColor::create(ccc4(170, 170, 170, 255), w - 4.f, 2.f);
    top->setPosition({2.f, h - 4.f});
    node->addChild(top);

    auto left = CCLayerColor::create(ccc4(170, 170, 170, 255), 2.f, h - 4.f);
    left->setPosition({2.f, 2.f});
    node->addChild(left);

    auto bottom = CCLayerColor::create(ccc4(70, 70, 70, 255), w - 4.f, 2.f);
    bottom->setPosition({2.f, 2.f});
    node->addChild(bottom);

    auto right = CCLayerColor::create(ccc4(70, 70, 70, 255), 2.f, h - 4.f);
    right->setPosition({w - 4.f, 2.f});
    node->addChild(right);

    return node;
}

// Block button with a white label and a dark text shadow
static CCNode* makeBlockButton(char const* text, float w, float h) {
    auto node = makeBlockPlate(w, h);

    auto shadow = CCLabelBMFont::create(text, "bigFont.fnt");
    shadow->setColor(ccc3(40, 40, 40));
    shadow->setScale(0.55f);
    shadow->setPosition({w / 2.f + 1.f, h / 2.f - 1.f});
    node->addChild(shadow);

    auto label = CCLabelBMFont::create(text, "bigFont.fnt");
    label->setScale(0.55f);
    label->setPosition({w / 2.f, h / 2.f});
    node->addChild(label);

    return node;
}

// Puts a block tile behind every icon button of a menu
static void blockifyMenu(CCNode* menu, float pad) {
    if (!menu) return;
    for (auto child : CCArrayExt<CCNode*>(menu->getChildren())) {
        if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(child)) {
            CCSize s = item->getContentSize();
            float side = std::max(s.width, s.height) + pad;
            auto plate = makeBlockPlate(side, side);
            plate->setPosition({(s.width - side) / 2.f, (s.height - side) / 2.f});
            item->addChild(plate, -1);
        }
    }
}

// Corner text like the Minecraft title screen (white with a shadow)
static void addCornerText(CCNode* parent, char const* text, CCPoint pos, CCPoint anchor) {
    auto shadow = CCLabelBMFont::create(text, "chatFont.fnt");
    shadow->setColor(ccc3(60, 60, 60));
    shadow->setScale(0.6f);
    shadow->setAnchorPoint(anchor);
    shadow->setPosition(pos + CCPoint(0.8f, -0.8f));
    parent->addChild(shadow, 20);

    auto label = CCLabelBMFont::create(text, "chatFont.fnt");
    label->setScale(0.6f);
    label->setAnchorPoint(anchor);
    label->setPosition(pos);
    parent->addChild(label, 21);
}

class $modify(ThemeMenuLayer, MenuLayer) {
    CCMenuItemSpriteExtra* makeWideButton(char const* text, SEL_MenuHandler cb) {
        return CCMenuItemSpriteExtra::create(makeBlockButton(text, 220.f, 28.f), this, cb);
    }

    bool init() {
        if (!MenuLayer::init()) return false;

        std::string style = currentStyle();
        if (style == "Default") return true;

        auto winSize = CCDirector::get()->getWinSize();

        // Hide the original menu background
        if (auto children = this->getChildren()) {
            for (auto child : CCArrayExt<CCNode*>(children)) {
                if (typeinfo_cast<MenuGameLayer*>(child)) {
                    child->setVisible(false);
                } else if (typeinfo_cast<CCSprite*>(child)
                           && child->getID().empty()
                           && child->getZOrder() < 0
                           && child->getContentSize().width * child->getScaleX() > winSize.width * 0.9f) {
                    child->setVisible(false);
                }
            }
        }

        if (style == "Blocky") {
            this->addChild(makeBlockyBackground(winSize), -100);

            // Custom logo (resources/mc-logo.png). Falls back to the original title if missing.
            CCPoint splashPos(winSize.width / 2.f + 110.f, winSize.height - 80.f);
            auto logo = CCSprite::create("mc-logo.png"_spr);
            if (logo) {
                if (auto title = this->getChildByID("main-title")) {
                    title->setVisible(false);
                }
                float targetW = 340.f;
                logo->setScale(targetW / logo->getContentSize().width);
                CCPoint logoPos(winSize.width / 2.f, winSize.height - 40.f);
                logo->setPosition(logoPos);
                this->addChild(logo, 6);
                splashPos = CCPoint(logoPos.x + targetW / 2.f - 30.f, logoPos.y - 20.f);
            }

            // Hide the original button row and build a block button column
            if (auto oldMenu = this->getChildByID("main-menu")) {
                oldMenu->setVisible(false);
            }

            auto column = CCMenu::create();
            column->setID("blocky-menu"_spr);
            column->setPosition({0.f, 0.f});

            float cx = winSize.width / 2.f;
            float y = winSize.height - 95.f;
            float gap = 34.f;

            auto play = makeWideButton("Play", menu_selector(MenuLayer::onPlay));
            play->setPosition({cx, y});
            column->addChild(play);

            auto garage = makeWideButton("Garage", menu_selector(MenuLayer::onGarage));
            garage->setPosition({cx, y - gap});
            column->addChild(garage);

            auto create = makeWideButton("Create", menu_selector(MenuLayer::onCreator));
            create->setPosition({cx, y - gap * 2.f});
            column->addChild(create);

            auto options = makeWideButton("Settings", menu_selector(MenuLayer::onOptions));
            options->setPosition({cx, y - gap * 3.f});
            column->addChild(options);

            this->addChild(column, 5);

            // Block tiles behind the round icon buttons
            blockifyMenu(this->getChildByID("bottom-menu"), 6.f);
            blockifyMenu(this->getChildByID("social-media-menu"), 4.f);

            // Pulsing tilted splash text
            auto splash = CCLabelBMFont::create("Maybe Not", "goldFont.fnt");
            splash->setRotation(-20.f);
            splash->setScale(0.55f);
            splash->setPosition(splashPos);
            splash->runAction(CCRepeatForever::create(CCSequence::create(
                CCScaleTo::create(0.5f, 0.62f),
                CCScaleTo::create(0.5f, 0.55f),
                nullptr
            )));
            this->addChild(splash, 10);

            // Corner texts (top corners, the bottom ones are used by GD buttons)
            addCornerText(this, "Geometry Dash 2.2081", CCPoint(34.f, winSize.height - 8.f), CCPoint(0.f, 1.f));
            addCornerText(this, "Theme UI by SillyCat", CCPoint(winSize.width - 10.f, winSize.height - 8.f), CCPoint(1.f, 1.f));
        }
        else if (style == "Dark") {
            auto bg = CCLayerGradient::create(ccc4(20, 20, 24, 255), ccc4(50, 50, 60, 255));
            this->addChild(bg, -100);
        }
        else if (style == "Neon") {
            auto bg = CCLayerGradient::create(ccc4(25, 0, 60, 255), ccc4(0, 40, 80, 255));
            this->addChild(bg, -100);
            tintMenu(this->getChildByID("main-menu"), ccc3(0, 255, 255));
        }

        return true;
    }
};
