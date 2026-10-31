#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <cmath>
#include <vector>
#include <functional>

using namespace geode::prelude;

static std::string currentStyle() {
    return Mod::get()->getSettingValue<std::string>("style");
}

static float rnd01(unsigned int& s) {
    s = s * 1664525u + 1013904223u;
    return ((s >> 16) & 0xFF) / 255.f;
}

static void addBlock(CCDrawNode* n, float x, float y, float s, ccColor4F c) {
    CCPoint v[4] = {
        CCPoint(x, y), CCPoint(x + s, y), CCPoint(x + s, y + s), CCPoint(x, y + s)
    };
    n->drawPolygon(v, 4, c, 0.f, c);
}

// Color in 0-255 with a small brightness variation
static ccColor4F blk(float r, float g, float b, float shade) {
    auto cl = [](float v) { return std::max(0.f, std::min(255.f, v)) / 255.f; };
    return ccc4f(cl(r + shade), cl(g + shade), cl(b + shade), 1.f);
}

// Periodic terrain height (so the strip loops without a seam)
static int heightAt(int i, int n, float base, float amp) {
    float t = 6.2831853f * (float)i / (float)n;
    float h = base + amp * (0.6f * std::sin(t) + 0.3f * std::sin(t * 3.f + 1.f) + 0.1f * std::sin(t * 5.f + 2.f));
    return std::max(1, (int)(h + 0.5f));
}

static CCNode* makeMountains(int n, float tile) {
    auto node = CCDrawNode::create();
    unsigned int seed = 777;
    for (int i = 0; i < n; i++) {
        int rows = heightAt(i, n, 11.f, 3.f);
        for (int r = 0; r < rows; r++) {
            float sh = (rnd01(seed) - 0.5f) * 14.f;
            ccColor4F c = (r == rows - 1) ? blk(130, 145, 170, sh) : blk(92, 106, 132, sh);
            addBlock(node, i * tile, r * tile, tile, c);
        }
    }
    return node;
}

static CCNode* makeTerrain(int n, float tile) {
    auto node = CCDrawNode::create();
    unsigned int seed = 4242;
    std::vector<int> tops(n);

    for (int i = 0; i < n; i++) {
        int rows = heightAt(i, n, 6.f, 2.f);
        tops[i] = rows;
        for (int r = 0; r < rows; r++) {
            float sh = (rnd01(seed) - 0.5f) * 20.f;
            ccColor4F c;
            if (r == rows - 1) c = blk(88, 160, 58, sh);        // grass
            else if (r >= rows - 4) c = blk(134, 96, 67, sh);   // dirt
            else c = blk(118, 118, 118, sh);                    // stone
            addBlock(node, i * tile, r * tile, tile, c);
        }
    }

    // Blocky trees
    int lastTree = -10;
    for (int i = 0; i < n; i++) {
        if (i - lastTree < 6) continue;
        if (rnd01(seed) > 0.18f) continue;
        lastTree = i;
        int base = tops[i];
        for (int k = 0; k < 3; k++) {
            addBlock(node, i * tile, (base + k) * tile, tile, blk(102, 76, 44, (rnd01(seed) - 0.5f) * 12.f));
        }
        for (int dx = -1; dx <= 1; dx++) {
            int col = ((i + dx) % n + n) % n;
            for (int k = 2; k <= 4; k++) {
                addBlock(node, col * tile, (base + k) * tile, tile, blk(52, 130, 44, (rnd01(seed) - 0.5f) * 16.f));
            }
        }
        addBlock(node, i * tile, (base + 5) * tile, tile, blk(52, 130, 44, 0.f));
    }
    return node;
}

static CCNode* makeClouds(int n, float tile, float winH) {
    auto node = CCDrawNode::create();
    unsigned int seed = 99;
    int i = 0;
    while (i < n) {
        if (rnd01(seed) < 0.25f) {
            int len = 3 + (int)(rnd01(seed) * 3.f);
            float y = winH - 60.f - rnd01(seed) * 90.f;
            y = std::floor(y / tile) * tile;
            for (int k = 0; k < len; k++) {
                int col = (i + k) % n;
                addBlock(node, col * tile, y, tile, ccc4f(1.f, 1.f, 1.f, 0.7f));
                if (k > 0 && k < len - 1) {
                    addBlock(node, col * tile, y + tile, tile, ccc4f(1.f, 1.f, 1.f, 0.7f));
                }
            }
            i += len + 3;
        } else {
            i++;
        }
    }
    return node;
}

// Two copies of a strip scrolling left forever (seamless loop)
static void addScrollingLayer(CCNode* parent, std::function<CCNode*()> make, float stripW, float duration, int z) {
    auto container = CCNode::create();
    auto a = make();
    auto b = make();
    b->setPosition(CCPoint(stripW, 0.f));
    container->addChild(a);
    container->addChild(b);
    container->runAction(CCRepeatForever::create(CCSequence::create(
        CCMoveTo::create(duration, CCPoint(-stripW, 0.f)),
        CCPlace::create(CCPoint(0.f, 0.f)),
        nullptr
    )));
    parent->addChild(container, z);
}

// Blocky world that keeps panning, like the Minecraft title screen
static void addPanorama(CCNode* parent, CCSize winSize) {
    const float tile = 16.f;
    int n = (int)std::ceil(winSize.width / tile) + 2;
    float stripW = n * tile;
    float winH = winSize.height;

    auto sky = CCLayerGradient::create(ccc4(100, 160, 255, 255), ccc4(195, 228, 255, 255));
    parent->addChild(sky, -100);

    addScrollingLayer(parent, [=]() { return makeClouds(n, tile, winH); }, stripW, 420.f, -99);
    addScrollingLayer(parent, [=]() { return makeMountains(n, tile); }, stripW, 260.f, -98);
    addScrollingLayer(parent, [=]() { return makeTerrain(n, tile); }, stripW, 140.f, -97);

    auto shade = CCLayerColor::create(ccc4(0, 0, 0, 45));
    parent->addChild(shade, -90);
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

// Puts a block tile behind every button of a menu
static void blockifyMenu(CCNode* menu, float pad) {
    if (!menu) return;
    for (auto child : CCArrayExt<CCNode*>(menu->getChildren())) {
        if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(child)) {
            CCSize s = item->getContentSize();
            if (s.width < 1.f || s.height < 1.f) continue;
            auto plate = makeBlockPlate(s.width + pad, s.height + pad);
            plate->setPosition({-pad / 2.f, -pad / 2.f});
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
            addPanorama(this, winSize);

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

            // Block tiles behind every other button of every visible menu
            for (auto child : CCArrayExt<CCNode*>(this->getChildren())) {
                auto menu = typeinfo_cast<CCMenu*>(child);
                if (!menu || !menu->isVisible()) continue;
                if (menu->getID() == "blocky-menu"_spr) continue;
                blockifyMenu(menu, 6.f);
            }

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

            // Corner texts
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
