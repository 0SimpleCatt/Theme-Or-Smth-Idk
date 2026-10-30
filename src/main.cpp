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

class $modify(ThemeMenuLayer, MenuLayer) {
    // One wide button of the vertical column
    CCMenuItemSpriteExtra* makeWideButton(char const* text, SEL_MenuHandler cb) {
        auto spr = ButtonSprite::create(text, 220, true, "bigFont.fnt", "GJ_button_04.png", 30.f, 0.7f);
        return CCMenuItemSpriteExtra::create(spr, this, cb);
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

            // Replace the original button row with a centered vertical column
            if (auto oldMenu = this->getChildByID("main-menu")) {
                oldMenu->setVisible(false);
            }

            auto column = CCMenu::create();
            column->setID("blocky-menu"_spr);
            column->setPosition({0.f, 0.f});

            float cx = winSize.width / 2.f;
            float y = winSize.height / 2.f + 10.f;
            float gap = 36.f;

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

            // Pulsing tilted splash text
            auto splash = CCLabelBMFont::create("Jump!", "goldFont.fnt");
            splash->setRotation(-18.f);
            splash->setScale(0.7f);
            CCPoint pos(winSize.width / 2.f + 110.f, winSize.height - 80.f);
            if (auto title = this->getChildByID("main-title")) {
                pos = title->getPosition() + CCPoint(110.f, 20.f);
            }
            splash->setPosition(pos);
            splash->runAction(CCRepeatForever::create(CCSequence::create(
                CCScaleTo::create(0.5f, 0.8f),
                CCScaleTo::create(0.5f, 0.7f),
                nullptr
            )));
            this->addChild(splash, 10);
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
