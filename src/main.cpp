#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

static std::string g_currentCategory = "";

// ==========================================
// 1. HOOK IN LEVEL BROWSER LAYER (API FETCH)
// ==========================================
class $modify(MyLevelBrowserLayer, LevelBrowserLayer) {
    struct Fields {
        async::TaskHolder<web::WebResponse> m_webListener;
    };

    bool init(GJSearchObject* searchObj) {
        if (!LevelBrowserLayer::init(searchObj)) return false;

        if (!g_currentCategory.empty()) {
            std::string cat = g_currentCategory;
            g_currentCategory = "";
            this->fetchNonWorthyLevels(cat);
        }

        return true;
    }

    void fetchNonWorthyLevels(std::string const& category) {
        std::string url = "http://localhost:3000/api/levels?status=" + category;

        m_fields->m_webListener.spawn(
            web::WebRequest().get(url),
            [this](web::WebResponse const& res) {
                if (!res.ok()) {
                    log::error("Request error: {}", res.string().unwrapOrDefault());
                    return;
                }

                auto jsonRes = res.json();
                if (!jsonRes) {
                    log::error("Invalid JSON response");
                    return;
                }

                auto levels = jsonRes.unwrap();
                if (!levels.isArray()) return;

                std::string idList = "";
                for (auto const& item : levels.asArray().unwrap()) {
                    if (item.contains("level_id")) {
                        if (!idList.empty()) idList += ",";
                        idList += std::to_string(item["level_id"].asInt().unwrapOr(0));
                    }
                }

                if (idList.empty()) {
                    log::info("No levels returned for this category.");
                    return;
                }

                auto searchObj = GJSearchObject::create(SearchType::Type19, idList);
                GameLevelManager::sharedState()->getOnlineLevels(searchObj);
            }
        );
    }
};

// ==========================================
// 2. MAIN MOD MENU (FULLSCREEN)
// ==========================================
class NonWorthyLayer : public CCLayer {
public:
    static NonWorthyLayer* create() {
        auto ret = new NonWorthyLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto bg = CCSprite::create("GJ_gradientBG.png");
        auto bgSize = bg->getContentSize();
        bg->setScaleX(winSize.width / bgSize.width);
        bg->setScaleY(winSize.height / bgSize.height);
        bg->setPosition(winSize / 2);
        bg->setColor({ 0, 102, 255 });
        this->addChild(bg);

        auto title = CCLabelBMFont::create("NON-WORTHY RATES", "bigFont.fnt");
        title->setPosition({ winSize.width / 2, winSize.height - 40.f });
        title->setScale(0.9f);
        this->addChild(title);

        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });
        this->addChild(menu);

        auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow01Btn_001.png");
        auto backBtn = CCMenuItemSpriteExtra::create(
            backSpr,
            this,
            menu_selector(NonWorthyLayer::onBack)
        );
        backBtn->setPosition({ 30.f, winSize.height - 30.f });
        menu->addChild(backBtn);

        auto featSpr = ButtonSprite::create("Featured", 110, true, "goldFont.fnt", "GJ_button_01.png", 35, 0.8f);
        auto featBtn = CCMenuItemSpriteExtra::create(featSpr, this, menu_selector(NonWorthyLayer::onFeatured));
        featBtn->setPosition({ winSize.width / 2 - 130.f, winSize.height / 2 + 35.f });
        menu->addChild(featBtn);

        auto searchSpr = ButtonSprite::create("Search", 110, true, "goldFont.fnt", "GJ_button_01.png", 35, 0.8f);
        auto searchBtn = CCMenuItemSpriteExtra::create(searchSpr, this, menu_selector(NonWorthyLayer::onSearch));
        searchBtn->setPosition({ winSize.width / 2, winSize.height / 2 + 35.f });
        menu->addChild(searchBtn);

        auto sentSpr = ButtonSprite::create("Sent Levels", 110, true, "goldFont.fnt", "GJ_button_01.png", 35, 0.8f);
        auto sentBtn = CCMenuItemSpriteExtra::create(sentSpr, this, menu_selector(NonWorthyLayer::onSentLevels));
        sentBtn->setPosition({ winSize.width / 2 + 130.f, winSize.height / 2 + 35.f });
        menu->addChild(sentBtn);

        auto monthlySpr = ButtonSprite::create("Monthly", 110, true, "goldFont.fnt", "GJ_button_01.png", 35, 0.8f);
        auto monthlyBtn = CCMenuItemSpriteExtra::create(monthlySpr, this, menu_selector(NonWorthyLayer::onMonthly));
        monthlyBtn->setPosition({ winSize.width / 2 - 130.f, winSize.height / 2 - 35.f });
        menu->addChild(monthlyBtn);

        auto shameSpr = ButtonSprite::create("Hall of Shame", 110, true, "goldFont.fnt", "GJ_button_02.png", 35, 0.7f);
        auto shameBtn = CCMenuItemSpriteExtra::create(shameSpr, this, menu_selector(NonWorthyLayer::onHallOfShame));
        shameBtn->setPosition({ winSize.width / 2, winSize.height / 2 - 35.f });
        menu->addChild(shameBtn);

        auto randomSpr = ButtonSprite::create("Random", 110, true, "goldFont.fnt", "GJ_button_01.png", 35, 0.8f);
        auto randomBtn = CCMenuItemSpriteExtra::create(randomSpr, this, menu_selector(NonWorthyLayer::onRandom));
        randomBtn->setPosition({ winSize.width / 2 + 130.f, winSize.height / 2 - 35.f });
        menu->addChild(randomBtn);

        auto queueLabel = CCLabelBMFont::create("NON-WORTHY RATES SYSTEM", "bigFont.fnt");
        queueLabel->setScale(0.45f);
        queueLabel->setPosition({ winSize.width / 2, 35.f });
        this->addChild(queueLabel);

        this->setKeypadEnabled(true);
        return true;
    }

    void keyBackClicked() override { onBack(nullptr); }
    void onBack(CCObject*) { CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade); }

    void openCategory(std::string const& category) {
        g_currentCategory = category;
        auto searchObj = GJSearchObject::create(SearchType::Type19, "");
        auto browser = LevelBrowserLayer::create(searchObj);
        auto scene = CCScene::create();
        scene->addChild(browser);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
    }

    void onFeatured(CCObject*) { openCategory("featured"); }
    void onSearch(CCObject*) { openCategory(""); }
    void onSentLevels(CCObject*) { openCategory("pending"); }
    void onMonthly(CCObject*) { openCategory("monthly"); }
    void onHallOfShame(CCObject*) { openCategory("shame"); }
    void onRandom(CCObject*) { openCategory("random"); }
};

// ==========================================
// 3. HOOK IN LEVEL SEARCH LAYER
// ==========================================
class $modify(MyLevelSearchLayer, LevelSearchLayer) {
    bool init(int p0) {
        if (!LevelSearchLayer::init(p0)) return false;

        auto spr = CCSprite::createWithSpriteFrameName("GJ_everyplayBtn_001.png");
        if (!spr) spr = CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png");

        auto btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            menu_selector(MyLevelSearchLayer::onNonWorthyRates)
        );

        auto menu = CCMenu::create();
        menu->setPosition({ CCDirector::sharedDirector()->getWinSize().width - 40.f, 40.f });
        menu->addChild(btn);

        this->addChild(menu);
        return true;
    }

    void onNonWorthyRates(CCObject*) {
        auto layer = NonWorthyLayer::create();
        auto scene = CCScene::create();
        scene->addChild(layer);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
    }
};

// ==========================================
// 4. HOOK IN LEVEL INFO LAYER (SEND & RATE)
// ==========================================
class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        async::TaskHolder<web::WebResponse> m_sendListener;
        async::TaskHolder<web::WebResponse> m_rateListener;
    };

    bool init(GJGameLevel* p0, bool p1) {
        if (!LevelInfoLayer::init(p0, p1)) return false;

        auto playMenu = this->getChildByID("play-menu");
        if (playMenu) {
            auto sendSpr = CCSprite::createWithSpriteFrameName("GJ_sendBtn_001.png");
            if (!sendSpr) sendSpr = CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png");
            
            auto sendBtn = CCMenuItemSpriteExtra::create(
                sendSpr,
                this,
                menu_selector(MyLevelInfoLayer::onSendNonWorthy)
            );
            sendBtn->setPosition({ 75.f, 0.f });
            sendBtn->setID("non-worthy-send-btn"_spr);
            playMenu->addChild(sendBtn);

            auto rateSpr = CCSprite::createWithSpriteFrameName("GJ_starBtn_001.png");
            if (!rateSpr) rateSpr = CCSprite::createWithSpriteFrameName("GJ_crownBtn_001.png");

            auto rateBtn = CCMenuItemSpriteExtra::create(
                rateSpr,
                this,
                menu_selector(MyLevelInfoLayer::onGiveNonWorthyRate)
            );
            rateBtn->setPosition({ -75.f, 0.f });
            rateBtn->setID("non-worthy-rate-btn"_spr);
            playMenu->addChild(rateBtn);

            playMenu->updateLayout();
        }

        return true;
    }

    void onSendNonWorthy(CCObject*) {
        int levelID = m_level->m_levelID.value();
        std::string levelName = m_level->m_levelName;
        std::string authorName = m_level->m_creatorName;

        matjson::Value body = matjson::Value::object();
        body["level_id"] = levelID;
        body["level_name"] = levelName;
        body["author_name"] = authorName;

        m_fields->m_sendListener.spawn(
            web::WebRequest().bodyJSON(body).post("http://localhost:3000/api/send"),
            [](web::WebResponse const& res) {
                if (res.ok()) {
                    FLAlertLayer::create("Success", "Level sent to the queue!", "OK")->show();
                } else {
                    FLAlertLayer::create("Error", "Failed to send level.", "OK")->show();
                }
            }
        );
    }

    void onGiveNonWorthyRate(CCObject*) {
        int levelID = m_level->m_levelID.value();

        matjson::Value body = matjson::Value::object();
        body["level_id"] = levelID;
        body["status"] = "featured";
        body["rate_stars"] = 10;

        m_fields->m_rateListener.spawn(
            web::WebRequest().bodyJSON(body).post("http://localhost:3000/api/rate"),
            [](web::WebResponse const& res) {
                if (res.ok()) {
                    FLAlertLayer::create("Success", "Level rated as Featured!", "OK")->show();
                } else {
                    FLAlertLayer::create("Error", "Failed to apply rate.", "OK")->show();
                }
            }
        );
    }
};