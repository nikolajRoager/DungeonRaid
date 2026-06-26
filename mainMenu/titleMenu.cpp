//
// Created by nikolaj on 6/1/26.
//

#include "titleMenu.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#include "../getAssets.h"
#include "../MIGUI/contentTable.h"
#include "../MIGUI/emptyControl.h"
#include "../MIGUI/mouseOverControl.h"
#include "../MIGUI/numberControl.h"
#include "../MIGUI/stackControl.h"
#include "../MIGUI/tableControl.h"
#include "../MIGUI/textureControl.h"

TitleMenu::~TitleMenu() = default;

TitleMenu::TitleMenu(SDL_Renderer *renderer, int screenWidth, int screenHeight, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont, int smallFontSize, int midFontSize, int largeFontSize, bool startAtSettings):
midNumberRenderer_(0,midFont,renderer)
{
    //First launch the threadpool and start loading everything
    //Using a threadpool is CRAZY overkill for this project, but I do it anyway
    std::vector<fs::path> textureRequests
    {
        fs::path("menu")/"editName.png",
        fs::path("menu")/"haircutButton.png",
        fs::path("menu")/"emptyButtonBackground.png",
        fs::path("menu")/"eyeButton.png",
        fs::path("menu")/"hand.png",
        fs::path("menu")/"genderButton.png",
        fs::path("menu")/"clothesButton.png",
        fs::path("menu")/"strengthButton.png",
        fs::path("menu")/"dexterityButton.png",
        fs::path("menu")/"intelligenceButton.png",
        fs::path("menu")/"speedButton.png",
        fs::path("menu")/"dodgeButton.png",
        fs::path("menu")/"aimButton.png",
        fs::path("menu")/"attackButton.png",
        fs::path("menu")/"blockButton.png",
        fs::path("menu")/"magicButton.png",
        fs::path("menu")/"magicDefenceButton.png",
        fs::path("menu")/"plus.png",
        fs::path("menu")/"minus.png",
        fs::path("menu")/"expand.png",
        fs::path("menu")/"infoButton.png",
        fs::path("menu")/"carryCapacityButton.png",
        fs::path("menu")/"baseButton.png",
        fs::path("menu")/"equalButton.png",
    };

    //Load items

    {
        std::ifstream itemsFile(assetsPath()/"items.json");

        if (!itemsFile.is_open())
            throw std::runtime_error("Could not open items.json");
        nlohmann::json itemsJson;
        itemsFile >> itemsJson;
        startingWeapons_= itemsJson["startingWeapons"];
        for (const auto& entry : itemsJson["items"]) {
            itemList_.push_back(std::make_shared<Item>(entry));
        }

        itemsFile.close();

    }

    ThreadPool loadingPool(std::thread::hardware_concurrency());
    textureManager_.launchTextureLoading(textureRequests, assetsPath(),loadingPool);
    smallFontSize_ = smallFontSize;
    midFontSize_ = midFontSize;
    largeFontSize_ = largeFontSize;

    clickSound_=std::make_shared<SoundWrap>(assetsPath()/"sounds"/"click.mp3");
    gongSound_=std::make_shared<SoundWrap>(assetsPath()/"sounds"/"gong.mp3");

    //Then display a loading bar
    while (textureManager_.getLoadedAssets()< textureManager_.getAssetsToLoad()&& !textureManager_.isCanceled()) {
        //Respond to window resize events;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_WINDOWEVENT) {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        screenWidth= event.window.data1;
                        screenHeight= event.window.data2;
                        break;
                    default:
                        break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        SDL_Rect loadingBarRect = {
            0, screenHeight/ 2, static_cast<int>((screenWidth* textureManager_.getLoadedAssets()) / textureManager_.getAssetsToLoad()), screenHeight/ 4
        };
        SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);

        SDL_RenderFillRect(renderer, &loadingBarRect);

        SDL_RenderPresent(renderer);
    }

    if (textureManager_.isCanceled()) {
        throw std::runtime_error("The loading cancelled with the following error: " + textureManager_.getErrorMessage());
    }


    textureManager_.uploadTexturesToGPU(renderer);

    editNameButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"editName.png");
    haircutButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"haircutButton.png");
    eyeButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"eyeButton.png");
    emptyButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"emptyButtonBackground.png");
    handTexture_=textureManager_.getTexWrap(fs::path("menu")/"hand.png");
    genderButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"genderButton.png");
    clothesButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"clothesButton.png");
    strengthButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"strengthButton.png");
    dexterityButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"dexterityButton.png");
    intelligenceButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"intelligenceButton.png");
    speedButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"speedButton.png");
    carryCapacityButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"carryCapacityButton.png");
    plusButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"plus.png");
    minusButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"minus.png");
    expandButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"expand.png");
    infoButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"infoButton.png");
    dodgeButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"dodgeButton.png");
    aimButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"aimButton.png");
    attackButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"attackButton.png");
    blockButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"blockButton.png");
    magicButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"magicButton.png");
    magicDefenceButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"magicDefenceButton.png");
    baseButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"baseButton.png");
    equalButtonTexture_=textureManager_.getTexWrap(fs::path("menu")/"equalButton.png");

    {
        std::ifstream splashStream(assetsPath()/"menu"/"splashscreen.txt");
        if (!splashStream.is_open()) {
            throw std::runtime_error("Could not open splashscreen.txt");
        }
        std::string line;
        while (std::getline(splashStream,line)) {
            splashText_.append(line+"\n");
        }
        splashStream.close();
    }

    {
        std::ifstream creditsStream(assetsPath()/"menu"/"credits.txt");
        if (!creditsStream.is_open()) {
            throw std::runtime_error("Could not open credits.txt");
        }
        std::string line;
        bool first = true;
        while (std::getline(creditsStream,line)) {
            creditsTextures_.emplace_back(std::make_shared<TexWrap>(line,renderer,first?largeFont:midFont,screenWidth));
            creditsText_.emplace_back(line);
            first = false;
        }
        creditsStream.close();
        creditsY=creditsTextures_.empty()? 0 : creditsTextures_.front()->getHeight();
    }

    transitionIn_=true;
    transitionTimer_=0;

    setupGui(renderer,screenWidth,screenHeight,smallFont,midFont,largeFont);

    if (startAtSettings) {
        transitionIn_=true;
        transitionTimer_=maxTransitionTimer_;
        menuSlides_->setActiveSlide(4);
    }
}

void TitleMenu::setupGui(SDL_Renderer *renderer, int screenWidth, int screenHeight, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont) {
    std::shared_ptr<control> splash0Control;
    {
        std::shared_ptr<control> nothingAbove=std::make_shared<emptyControl>();
        std::shared_ptr<control> nothingLeft0=std::make_shared<emptyControl>();
        std::shared_ptr<textureControl> splashTexture = std::make_shared<textureControl>(std::make_shared<TexWrap>(splashText_,renderer,smallFont,1024));
        std::shared_ptr<control> nothingRight0=std::make_shared<emptyControl>();
        std::shared_ptr<control> nothingBelow=std::make_shared<emptyControl>();

        auto leftRight =std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/3),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/3),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/3)},std::vector<std::shared_ptr<control> >{nothingLeft0,splashTexture,nothingRight0});
        splash0Control=std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight/3),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/3),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight/3)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth)},std::vector<std::shared_ptr<control> >{nothingAbove,leftRight,nothingBelow});
    }

    std::shared_ptr<control> splash1Control;
    {
        std::shared_ptr<control> nothingAbove=std::make_shared<emptyControl>();
        std::shared_ptr<control> nothingLeft0=std::make_shared<emptyControl>();
        std::shared_ptr<textureControl> splashTexture = std::make_shared<textureControl>(std::make_shared<TexWrap>("A game by Nikolaj R Christensen",renderer,midFont));
        std::shared_ptr<control> nothingRight0=std::make_shared<emptyControl>();
        std::shared_ptr<control> nothingBelow=std::make_shared<emptyControl>();

        auto leftRight =std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/3),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/3),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/3)},std::vector<std::shared_ptr<control> >{nothingLeft0,splashTexture,nothingRight0});
        splash1Control=std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight/3),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/3),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight/3)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth)},std::vector<std::shared_ptr<control> >{nothingAbove,leftRight,nothingBelow});
    }

    //---MAIN MENU PAGE----
    std::shared_ptr<control> mainTable ;
    {
        std::shared_ptr<control> gameTitle =std::make_shared<textureControl> (std::make_shared<TexWrap>("Dungeon Raid",renderer,largeFont));
        std::shared_ptr<control> nothingLeft0=std::make_shared<emptyControl>();
        std::shared_ptr<control> nothingRight0=std::make_shared<emptyControl>();
        std::shared_ptr<control> nothingDown0=std::make_shared<emptyControl>();
        std::shared_ptr<control> nothingDown1=std::make_shared<emptyControl>();
        std::shared_ptr<control> createPartyTextureControl=std::make_shared<textureControl> (std::make_shared<TexWrap>("Start Game",renderer,midFont));
        std::shared_ptr<control> settingsTextureControl=std::make_shared<textureControl> (std::make_shared<TexWrap>("Settings",renderer,midFont));
        std::shared_ptr<control> creditsTextureControl=std::make_shared<textureControl> (std::make_shared<TexWrap>("View Credits",renderer,midFont));
        std::shared_ptr<control> quitGameTextureControl=std::make_shared<textureControl> (std::make_shared<TexWrap>("Quit Game",renderer,midFont));
        createPartyButton_=std::make_shared<buttonControl>(createPartyTextureControl,128,128,128,false);
        settingsButton_=std::make_shared<buttonControl>(settingsTextureControl,128,128,128,false);
        creditsButton_=std::make_shared<buttonControl>(creditsTextureControl,128,128,128,false);
        quitButton_=std::make_shared<buttonControl>(quitGameTextureControl,128,128,128,false);
        std::shared_ptr<stackControl> buttonStack = std::make_shared<stackControl>(stackControl::VERTICAL,std::vector<std::shared_ptr<control> >{createPartyButton_,creditsButton_,settingsButton_,quitButton_});
        std::shared_ptr<control> middleMenu= std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/3),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/3),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight/3)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth)},std::vector<std::shared_ptr<control> >{gameTitle,buttonStack ,nothingDown1});
        mainTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/2)},std::vector<std::shared_ptr<control> >{middleMenu,nothingRight0});
    }
    //---CREATE PARTY PAGE----
    std::shared_ptr<control> characterCreationMenu;
    {
        //Empty space where character preview goes... lol I decided there won't be any preview
        std::shared_ptr<control> bigNothing=std::make_shared<emptyControl>();

        //The buttons at the top
        std::shared_ptr<control> goBackFromParyCreationButtonTC =std::make_shared<textureControl> (std::make_shared<TexWrap>("Back",renderer,midFont));
        goBackFromPartyCreationButton_ = std::make_shared<buttonControl>(goBackFromParyCreationButtonTC,128,128,128);
        std::shared_ptr<control> nothing0=std::make_shared<emptyControl>();
        std::shared_ptr<control> savePartyButtonTC =std::make_shared<textureControl> (std::make_shared<TexWrap>("Save Party",renderer,midFont));
        savePartyButton_ = std::make_shared<buttonControl>(savePartyButtonTC,128,128,128,false);
        std::shared_ptr<control> nothing1=std::make_shared<emptyControl>();
        std::shared_ptr<control> loadPartyButtonTC =std::make_shared<textureControl> (std::make_shared<TexWrap>("Load Party",renderer,midFont));
        loadPartyButton_ = std::make_shared<buttonControl>(loadPartyButtonTC,128,128,128,false);
        std::shared_ptr<control> nothing2=std::make_shared<emptyControl>();
        std::shared_ptr<control> playGameButtonTC =std::make_shared<textureControl> (std::make_shared<TexWrap>("Start Game!",renderer,midFont));
        playGameButton_ = std::make_shared<buttonControl>(playGameButtonTC,128,128,128,false);
        std::shared_ptr<control> buttonTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/7),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/7),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/7),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/7),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/7),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/7),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/7)},std::vector<std::shared_ptr<control> >{goBackFromPartyCreationButton_,nothing0,savePartyButton_,nothing1,loadPartyButton_,nothing2,playGameButton_},std::vector<tableControl::background>{tableControl::background(150,150,150),tableControl::background(150,150,150),tableControl::background(150,150,150),tableControl::background(150,150,150),tableControl::background(150,150,150),tableControl::background(150,150,150),tableControl::background(150,150,150)});

        //The table showing the party

        //Headers for the table
        std::shared_ptr<control> nameHeaderTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Name",renderer,midFont));
        std::shared_ptr<control> costHeaderTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Cost",renderer,midFont));

        partyTable_= std::make_shared<contentTable>(std::vector<std::shared_ptr<control>>{
            nameHeaderTC,costHeaderTC},std::vector<std::vector<std::shared_ptr<control>>>{},64);

        //std::shared_ptr<control> tempNothing =std::make_shared<emptyControl>();

        std::shared_ptr<control> pointsLeftCounterText = std::make_shared<textureControl>(std::make_shared<TexWrap>("Points left: ",renderer,midFont));
        pointsLeftNR_=std::make_shared<numberControl>(midNumberRenderer_,maxPoints_);
        std::shared_ptr<control> pointsLeftPair = std::make_shared<stackControl>(stackControl::HORIZONTAL,std::vector<std::shared_ptr<control>> {pointsLeftCounterText,pointsLeftNR_});
        std::shared_ptr<control> addNewTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Add adventurer",renderer,midFont));
        addAdventurerButton_=std::make_shared<buttonControl>(addNewTC,128,128,128,false);

        std::shared_ptr<control> barTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight/3),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/3),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/3)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth)},std::vector<std::shared_ptr<control> >{partyTable_,addAdventurerButton_,pointsLeftPair});

        std::shared_ptr<control> nothingLeft = std::make_shared<emptyControl>();
        std::shared_ptr<control> nothingRight = std::make_shared<emptyControl>();
        std::shared_ptr<control> addAdventurerToContinueText = std::make_shared<textureControl>(std::make_shared<TexWrap>("Add or select an adventurer to edit their stats...",renderer,midFont));


        std::shared_ptr<control> statHeader = std::make_shared<textureControl> (std::make_shared<TexWrap>("Stat",renderer,midFont));
        std::shared_ptr<control> valueHeader = std::make_shared<textureControl> (std::make_shared<TexWrap>("Value/edit",renderer,midFont));

        //The table with the things we can edit
        std::shared_ptr<control> nameTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Name:",renderer,midFont));

        selectedNameTexture_ = std::make_shared<textureControl> (std::make_shared<TexWrap>("null",renderer,midFont));
        std::shared_ptr<control> editNameButtonTC =  std::make_shared<textureControl>(editNameButtonTexture_);

        editNameButton_ = std::make_shared<buttonControl>(editNameButtonTC,128,128,128,false);
        std::shared_ptr<control> editNamePair = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2)},std::vector<std::shared_ptr<control>>{selectedNameTexture_,editNameButton_});

        std::shared_ptr<control> strTC=std::make_shared<textureControl> (strengthButtonTexture_);
        std::shared_ptr<control> dexTC=std::make_shared<textureControl> (dexterityButtonTexture_);
        std::shared_ptr<control> intTC=std::make_shared<textureControl> (intelligenceButtonTexture_);
        std::shared_ptr<control> speedTC=std::make_shared<textureControl> (speedButtonTexture_);
        std::shared_ptr<control> carryTC=std::make_shared<textureControl> (carryCapacityButtonTexture_);
        std::shared_ptr<control> dodgeTC=std::make_shared<textureControl> (dodgeButtonTexture_);
        std::shared_ptr<control> attackTC=std::make_shared<textureControl> (attackButtonTexture_);
        std::shared_ptr<control> blockTC=std::make_shared<textureControl> (blockButtonTexture_);
        std::shared_ptr<control> aimTC=std::make_shared<textureControl> (aimButtonTexture_);
        std::shared_ptr<control> magicTC=std::make_shared<textureControl> (magicButtonTexture_);
        std::shared_ptr<control> magicDefenceTC=std::make_shared<textureControl> (magicDefenceButtonTexture_);
        std::shared_ptr<control> costTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Cost:",renderer,midFont));
        std::shared_ptr<control> mainWeaponTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("main-hand:",renderer,midFont));
        std::shared_ptr<control> offWeaponTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("off-hand:",renderer,midFont));
        std::shared_ptr<control> genderTC=std::make_shared<textureControl> (genderButtonTexture_);

        std::shared_ptr<control> strMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Strength\nRequired to beat some traps, boost carry capacity, effects physical damage of most weapons",renderer,midFont,512));
        std::shared_ptr<control> dexMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Dexterity\nRequired to beat some traps, also effects combat speed, effects aim of most weapons",renderer,midFont,512));
        std::shared_ptr<control> intMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Intelligence\nRequired to beat some traps, effects magic damage of most wands",renderer,midFont,512));
        std::shared_ptr<control> speedMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Speed\nAttack more frequently, scales with dexterity",renderer,midFont,512));
        std::shared_ptr<control> carryMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Carry capacity\nEffects how much loot you can carry, scales with strength",renderer,midFont,512));
        std::shared_ptr<control> dodgeMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Dodge\nGives chance to avoid attacks, scales with dexterity",renderer,midFont,512));
        std::shared_ptr<control> attackMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Physical attack\nDeal physical type damage\nScales based on equipment",renderer,midFont,512));
        std::shared_ptr<control> blockMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Block\nGives chance to block physical attacks\nScales based on equipment, uses whichever hand has highest value",renderer,midFont,512));
        std::shared_ptr<control> aimMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Aim\nGives chance to hit enemies\nScales based on equipment",renderer,midFont,512));
        std::shared_ptr<control> magicMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Magic\nDeal magical type damage, and allows the use of spells\nScales based on equipment",renderer,midFont,512));
        std::shared_ptr<control> magicDefenceMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Magic defence\nGives chance to block magical attacks\nScales based on equipment, uses whichever hand has highest value",renderer,midFont,512));
        std::shared_ptr<control> costMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("The cost per level is Level*(Level+1)/2+weapon cost",renderer,midFont,512));
        std::shared_ptr<control> mainWeaponMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Weapon in main hand, click info box to view stat scaling",renderer,midFont,512));
        std::shared_ptr<control> offWeaponMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Weapon in off hand, click info box to view stat scaling",renderer,midFont,512));
        std::shared_ptr<control> genderMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Which pronouns shall the game use?",renderer,midFont,512));

        std::shared_ptr<control> strMO=std::make_shared<mouseOverControl> (strTC,strMTC);
        std::shared_ptr<control> dexMO=std::make_shared<mouseOverControl> (dexTC,dexMTC);
        std::shared_ptr<control> intMO=std::make_shared<mouseOverControl> (intTC,intMTC);
        std::shared_ptr<control> speedMO=std::make_shared<mouseOverControl> (speedTC,speedMTC);
        std::shared_ptr<control> carryMO=std::make_shared<mouseOverControl> (carryTC,carryMTC);
        std::shared_ptr<control> dodgeMO=std::make_shared<mouseOverControl> (dodgeTC,dodgeMTC);
        std::shared_ptr<control> aimMO=std::make_shared<mouseOverControl> (aimTC,aimMTC);
        std::shared_ptr<control> attackMO=std::make_shared<mouseOverControl> (attackTC,attackMTC);
        std::shared_ptr<control> blockMO=std::make_shared<mouseOverControl> (blockTC,blockMTC);
        std::shared_ptr<control> magicMO=std::make_shared<mouseOverControl> (magicTC,magicMTC);
        std::shared_ptr<control> magicDefenceMO=std::make_shared<mouseOverControl> (magicDefenceTC,magicDefenceMTC);
        std::shared_ptr<control> costMO=std::make_shared<mouseOverControl> (costTC,costMTC);
        std::shared_ptr<control> mainWeaponMO=std::make_shared<mouseOverControl> (mainWeaponTC,mainWeaponMTC);
        std::shared_ptr<control> offWeaponMO=std::make_shared<mouseOverControl> (offWeaponTC,offWeaponMTC);
        std::shared_ptr<control> genderMO=std::make_shared<mouseOverControl> (genderTC,genderMTC);



        strInputControl_ = std::make_shared<NumberInputControl>(midNumberRenderer_,1,1,32,plusButtonTexture_,minusButtonTexture_);
        dexInputControl_ = std::make_shared<NumberInputControl>(midNumberRenderer_,1,1,32,plusButtonTexture_,minusButtonTexture_);
        intInputControl_ = std::make_shared<NumberInputControl>(midNumberRenderer_,1,1,32,plusButtonTexture_,minusButtonTexture_);
        selectedCostControl_=std::make_shared<numberControl>(midNumberRenderer_,1);
        speedInputControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
        carryInputControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
        dodgeInputControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
        aimInputControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
        attackInputControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
        blockInputControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
        magicInputControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
        magicDefenceInputControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);




        //Weapons which go in the dropdown menu
        std::vector<std::shared_ptr<control>> mainHandWeaponControls;
        for (int i = 0; i < startingWeapons_; i++) {
            std::shared_ptr<control> TC=std::make_shared<textureControl> (std::make_shared<TexWrap>(itemList_[i]->getName(),renderer,midFont));
            std::shared_ptr<control> buttonTC = std::make_shared<textureControl>(infoButtonTexture_);
            std::shared_ptr<buttonControl> button = std::make_shared<buttonControl>(buttonTC);
            weaponInfoButtonsMain_.push_back(button);
            std::shared_ptr<control> pair = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, screenWidth/2)},std::vector<std::shared_ptr<control>>{TC,button});
            mainHandWeaponControls.emplace_back(pair);
        }
        mainHandWeaponSelection_ = std::make_shared<DropdownMenu>(mainHandWeaponControls,expandButtonTexture_);
        //Same for offhand
        std::vector<std::shared_ptr<control>> offHandWeaponControls;
        for (int i = 0; i < startingWeapons_; i++) {
            std::shared_ptr<control> TC=std::make_shared<textureControl> (std::make_shared<TexWrap>(itemList_[i]->getName(),renderer,midFont));
            std::shared_ptr<control> buttonTC = std::make_shared<textureControl>(infoButtonTexture_);
            std::shared_ptr<buttonControl> button = std::make_shared<buttonControl>(buttonTC);
            weaponInfoButtonsOff_.push_back(button);
            std::shared_ptr<control> pair = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, screenWidth/2)},std::vector<std::shared_ptr<control>>{TC,button});
            offHandWeaponControls.emplace_back(pair);
        }
        offHandWeaponSelection_ = std::make_shared<DropdownMenu>(offHandWeaponControls,expandButtonTexture_);

        {
            std::shared_ptr<control> femaleTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("She/Her",renderer,midFont));
            std::shared_ptr<control> maleTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("He/Him",renderer,midFont));
            std::shared_ptr<control> neitherTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("They/Them",renderer,midFont));
            genderSelection_= std::make_shared<DropdownMenu> (std::vector< std::shared_ptr<control>>{femaleTC,maleTC,neitherTC},expandButtonTexture_);
        }


        std::shared_ptr<control> editableTable = std::make_shared<contentTable>(std::vector<std::shared_ptr<control>>{statHeader,valueHeader},std::vector<std::vector<std::shared_ptr<control>>>{
            {nameTC,editNamePair},
            {strMO,strInputControl_},
            {dexMO,dexInputControl_},
            {intMO,intInputControl_},
            {speedMO,speedInputControl_},
            {carryMO,carryInputControl_},
            {dodgeMO,dodgeInputControl_},
            {aimMO,aimInputControl_},
            {attackMO,attackInputControl_},
            {blockMO,blockInputControl_},
            {magicMO,magicInputControl_},
            {magicDefenceMO,magicDefenceInputControl_},
            {costMO,selectedCostControl_},
            {mainWeaponMO,mainHandWeaponSelection_},
            {offWeaponMO,offHandWeaponSelection_},
            {genderMO,genderSelection_},
        },64,100,100,100,200,200,200,100,100,200,true);

        std::shared_ptr<tableControl> characterViewAndStatTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2)},std::vector<std::shared_ptr<control> >{bigNothing,editableTable},std::vector<tableControl::background>{tableControl::background(),tableControl::background(100,100,150)});
        selectAdventurerToShowSlide_  = std::make_shared<SlideControl>(std::vector<std::shared_ptr<control>>{addAdventurerToContinueText,characterViewAndStatTable});
        std::shared_ptr<tableControl> leftRightTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/2)},std::vector<std::shared_ptr<control> >{barTable,selectAdventurerToShowSlide_ },std::vector<tableControl::background>{tableControl::background(100,100,150),tableControl::background()});

        characterCreationMenu = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/2),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight/2)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth)},std::vector<std::shared_ptr<control> >{buttonTable,leftRightTable},std::vector<tableControl::background>{tableControl::background(100,100,150),tableControl::background()});

    }
    //---SETTINGS PAGE---
    std::shared_ptr<control> settingsMenu;
    {
        std::shared_ptr<control> nothingRight0=std::make_shared<emptyControl>();

        std::shared_ptr<control> settingsTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Settings",renderer,largeFont));
        std::shared_ptr<control> smallFontSampleTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("That was large font, this is small font btw.",renderer,smallFont));
        std::shared_ptr<control> smallFontSizeTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Small font size:",renderer,midFont));
        std::shared_ptr<control> midFontSizeTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Medium font size:",renderer,midFont));
        std::shared_ptr<control> largeFontSizeTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Large font size:",renderer,midFont));
        smallFontSizeButton_=std::make_shared<NumberInputControl>(midNumberRenderer_,smallFontSize_,8,128,plusButtonTexture_,minusButtonTexture_);
        midFontSizeButton_=std::make_shared<NumberInputControl>(midNumberRenderer_,midFontSize_,8,128,plusButtonTexture_,minusButtonTexture_);
        largeFontSizeButton_=std::make_shared<NumberInputControl>(midNumberRenderer_,largeFontSize_,8,128,plusButtonTexture_,minusButtonTexture_);

        std::shared_ptr<control> smallFontPair = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2)},std::vector<std::shared_ptr<control>>{smallFontSizeTC,smallFontSizeButton_});
        std::shared_ptr<control> midFontPair = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2)},std::vector<std::shared_ptr<control>>{midFontSizeTC,midFontSizeButton_});
        std::shared_ptr<control> largeFontPair = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2)},std::vector<std::shared_ptr<control>>{largeFontSizeTC,largeFontSizeButton_});

        std::shared_ptr<control> goBackTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Back",renderer,midFont));
        std::shared_ptr<control> applyTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Apply settings (forces reload)",renderer,midFont));
        goBackFromSettingsButton_=std::make_shared<buttonControl>(goBackTC,128,128,128,false);
        applySettingsButton_=std::make_shared<buttonControl>(applyTC,128,128,128,false);
        std::shared_ptr<stackControl> buttonStack = std::make_shared<stackControl>(stackControl::VERTICAL,std::vector<std::shared_ptr<control> >{settingsTC,smallFontSampleTC,smallFontPair,midFontPair,largeFontPair,applySettingsButton_,goBackFromSettingsButton_});
        settingsMenu = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth/2),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth/2)},std::vector<std::shared_ptr<control> >{buttonStack,nothingRight0});

    }
    std::shared_ptr<control> creditsPage;
    {
        std::shared_ptr<control> goBackTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Back",renderer,midFont));
        goBackFromCreditsButton_ = std::make_shared<buttonControl>(goBackTC,128,128,128,false);
        auto buttonStack = std::make_shared<stackControl>(stackControl::HORIZONTAL,std::vector<std::shared_ptr<control> >{goBackFromCreditsButton_});
        creditsGoHere_=std::make_shared<emptyControl>();
        creditsPage = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/2)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth)},std::vector<std::shared_ptr<control> >{creditsGoHere_,buttonStack});
    }
    menuSlides_ =std::make_shared<SlideControl>(std::vector<std::shared_ptr<control>>{splash0Control,splash1Control,mainTable,characterCreationMenu,settingsMenu,creditsPage});

    gui_ = std::make_unique<GUIManager>(menuSlides_ );
}

void TitleMenu::updateSelectedAdventurerStats() {
    if (selectedAdventurerId_<adventurers_.size()) {
        //Lock level up if too expensive
        if (currentPointsLeft_<adventurers_[selectedAdventurerId_].getLevelCost()) {
            strInputControl_->setAllowIncrement(false);
            dexInputControl_->setAllowIncrement(false);
            intInputControl_->setAllowIncrement(false);
        }
        else {
            strInputControl_->setAllowIncrement(true);
            dexInputControl_->setAllowIncrement(true);
            intInputControl_->setAllowIncrement(true);
        }
        genderSelection_->setSelection((int)adventurers_[selectedAdventurerId_].gender_);

        mainHandWeaponSelection_->setSelection(adventurers_[selectedAdventurerId_].mainHandWeaponIndex);
        offHandWeaponSelection_->setSelection(adventurers_[selectedAdventurerId_].offHandWeaponIndex);
        adventurerCostControls_[selectedAdventurerId_]->setValue(adventurers_[selectedAdventurerId_].getCost()+(itemList_[adventurers_[selectedAdventurerId_].offHandWeaponIndex]->getValueCost())+(itemList_[adventurers_[selectedAdventurerId_].mainHandWeaponIndex]->getValueCost()));
        selectedNameTexture_->setTexture(adventurerNames_[selectedAdventurerId_]);
        selectedNameTexture_->updateSize();

        strInputControl_->setValue(adventurers_[selectedAdventurerId_].strength_);
        dexInputControl_->setValue(adventurers_[selectedAdventurerId_].dexterity_);
        intInputControl_->setValue(adventurers_[selectedAdventurerId_].intelligence_);

        speedInputControl_->setValue(adventurers_[selectedAdventurerId_].getSpeed());
        carryInputControl_->setValue(adventurers_[selectedAdventurerId_].getCarryCap());
        dodgeInputControl_->setValue(adventurers_[selectedAdventurerId_].getDodge());
        aimInputControl_->setValue(itemList_[ adventurers_[selectedAdventurerId_].mainHandWeaponIndex]->getAim(adventurers_[selectedAdventurerId_].strength_,adventurers_[selectedAdventurerId_].dexterity_,adventurers_[selectedAdventurerId_].intelligence_));
        attackInputControl_->setValue(itemList_[ adventurers_[selectedAdventurerId_].mainHandWeaponIndex]->getAttack(adventurers_[selectedAdventurerId_].strength_,adventurers_[selectedAdventurerId_].dexterity_,adventurers_[selectedAdventurerId_].intelligence_));
        blockInputControl_->setValue(
            std::max(
            itemList_[ adventurers_[selectedAdventurerId_].mainHandWeaponIndex]->getBlock(adventurers_[selectedAdventurerId_].strength_,adventurers_[selectedAdventurerId_].dexterity_,adventurers_[selectedAdventurerId_].intelligence_),
            itemList_[ adventurers_[selectedAdventurerId_].offHandWeaponIndex]->getBlock(adventurers_[selectedAdventurerId_].strength_,adventurers_[selectedAdventurerId_].dexterity_,adventurers_[selectedAdventurerId_].intelligence_)
            )
            );
        magicInputControl_->setValue(itemList_[ adventurers_[selectedAdventurerId_].mainHandWeaponIndex]->getMagicAttack(adventurers_[selectedAdventurerId_].strength_,adventurers_[selectedAdventurerId_].dexterity_,adventurers_[selectedAdventurerId_].intelligence_));
        magicDefenceInputControl_->setValue(
            std::max(
            itemList_[ adventurers_[selectedAdventurerId_].mainHandWeaponIndex]->getMagicDefence(adventurers_[selectedAdventurerId_].strength_,adventurers_[selectedAdventurerId_].dexterity_,adventurers_[selectedAdventurerId_].intelligence_),
            itemList_[ adventurers_[selectedAdventurerId_].offHandWeaponIndex]->getMagicDefence(adventurers_[selectedAdventurerId_].strength_,adventurers_[selectedAdventurerId_].dexterity_,adventurers_[selectedAdventurerId_].intelligence_)
            )
            );

        selectedCostControl_->setValue(adventurers_[selectedAdventurerId_].getCost()+(itemList_[adventurers_[selectedAdventurerId_].mainHandWeaponIndex]->getValueCost())+(itemList_[adventurers_[selectedAdventurerId_].offHandWeaponIndex]->getValueCost()));
    }
}



std::optional<std::pair<Scene::SceneInfo,SceneTransitionData>> TitleMenu::update(SDL_Renderer *renderer, int screenWidth, int screenHeight, const InputData &userInputs, unsigned int millis, unsigned int dmillis, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont) {

    //For debugging, instant qui
    //if (userInputs.rightMouseDown)
    //    return std::make_pair(QUIT_GAME,SceneTransitionData(""));


    gui_->update(userInputs,screenWidth,screenHeight);

    if (transitionIn_ && transitionTimer_ < maxTransitionTimer_) {
        transitionTimer_+=static_cast<int>(std::max(dmillis,static_cast<uint32_t>(100)));
        transitionTimer_ = std::min(transitionTimer_,maxTransitionTimer_);
    }
    if (!transitionIn_ && transitionTimer_ > 0) {
        transitionTimer_-=static_cast<int>(std::max(dmillis,static_cast<uint32_t>(100)));
        transitionTimer_ = std::max(transitionTimer_,0);
    }

    //Handle typing also hides all other inputs
    if (userInputs.typingIsActive) {
        if (typingTarget==NAME && selectedAdventurerId_<adventurers_.size()) {
            adventurers_[selectedAdventurerId_].name_=userInputs.typingText;
            adventurerNames_[selectedAdventurerId_]->reset(adventurers_[selectedAdventurerId_].name_,renderer,midFont);
            adventurerNameControls_[selectedAdventurerId_]->updateSize();
            selectedNameTexture_ ->updateSize();
        }
        else//No supported text input is active, tell the app to stop
        {
            typingTarget=NONE;
            selectedNameTexture_->setBackground(false);
            return std::make_pair(STOP_TYPING, SceneTransitionData(""));
        }

        if ((userInputs.leftMouseDown && !userInputs.prevLeftMouseDown) || userInputs.rightMouseDown && !userInputs.prevRightMouseDown) {
            typingTarget=NONE;
            selectedNameTexture_->setBackground(false);
            return std::make_pair(STOP_TYPING, SceneTransitionData(""));
        }

    }
    else {
        if (typingTarget!=NONE) {
            selectedNameTexture_->setBackground(false);
            typingTarget=NONE;
        }
        if (menuSlides_->getActiveSlide()==0) {
            //Start smooth transition out
            if (userInputs.enterPressed && !userInputs.prevEnterPressed) {
                transitionIn_=false;
                gongSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
            if (!transitionIn_ && transitionTimer_==0) {
                transitionIn_=true;
                menuSlides_->setActiveSlide(1);
            }
        }
        else if (menuSlides_->getActiveSlide()==1) {
            if ( (userInputs.enterPressed && !userInputs.prevEnterPressed) || transitionTimer_==maxTransitionTimer_) {
                transitionIn_=false;
            }
            if (!transitionIn_ && transitionTimer_==0) {
                transitionIn_=true;
                menuSlides_->setActiveSlide(2);
            }
        }
        else if (menuSlides_->getActiveSlide()==2) {
            if (quitButton_->isClicked()) {
                return std::make_pair(QUIT_GAME,SceneTransitionData(""));
            }
            if (createPartyButton_->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                menuSlides_->setActiveSlide(3);
            }
            if (settingsButton_->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                smallFontSizeButton_->setValue(smallFontSize_);
                midFontSizeButton_->setValue(midFontSize_);
                largeFontSizeButton_->setValue(largeFontSize_);
                menuSlides_->setActiveSlide(4);
            }
            if (creditsButton_->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                menuSlides_->setActiveSlide(5);
                creditsY=creditsTextures_.empty()? 0 : creditsTextures_.front()->getHeight();
            }
        }
        else if (menuSlides_->getActiveSlide()==4) {
            if (goBackFromSettingsButton_->isClicked()) {
                smallFontSizeButton_->setValue(smallFontSize_);
                midFontSizeButton_->setValue(midFontSize_);
                largeFontSizeButton_->setValue(largeFontSize_);
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                menuSlides_->setActiveSlide(2);
            }
            if (applySettingsButton_->isClicked()) {
                saveFontSize();
                return std::make_pair(RELOAD_FONTS,SceneTransitionData(""));
            }
            if (smallFontSizeButton_->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
            if (midFontSizeButton_->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
            if (largeFontSizeButton_->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
        }
        else if (menuSlides_->getActiveSlide()==5) {
            //Reload credits textures
            if (userInputs.sizeChanged) {
                for (int i = 0; i < creditsText_.size();++i) {
                    creditsTextures_[i]->reset(creditsText_[i],renderer,i==0?largeFont:midFont,screenWidth);
                }
            }
            if (goBackFromCreditsButton_->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                menuSlides_->setActiveSlide(2);
            }
            creditsY+=dmillis/16;
            //Check if credits are out of bounds
            int totalCreditsY=creditsGoHere_->getY0()+creditsGoHere_->getHeight()-creditsY;
            for (const auto &text : creditsTextures_) {
                totalCreditsY+=std::max(text->getHeight(),12);
            }
            //Loop around
            if (totalCreditsY<0) {
                creditsY=0;
            }
        }
        else if (menuSlides_->getActiveSlide()==3) {
            if (goBackFromPartyCreationButton_->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                menuSlides_->setActiveSlide(2);
            }

            size_t thisSelectedAdventurer = partyTable_->getMainSelectedRow();
            if (thisSelectedAdventurer!=selectedAdventurerId_) {
                selectedAdventurerId_=thisSelectedAdventurer  ;
                //Signal that adventurer has been deselected
                if (thisSelectedAdventurer>adventurers_.size()) {
                    selectAdventurerToShowSlide_->setActiveSlide(0);
                }
                else {
                    selectAdventurerToShowSlide_->setActiveSlide(1);
                    //Update shown information
                    updateSelectedAdventurerStats();
                }
            }

            if (playGameButton_->isClicked()) {
                if (!adventurers_.empty()) {
                    return std::make_pair(START_GAME,SceneTransitionData(adventurers_));
                }
                else {
                    clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                    auto TC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Not enough players",renderer,midFont));
                    gui_->addDialogue("Invalid",TC,screenHeight,renderer,smallFont);
                }
            }

            if (selectedAdventurerId_<adventurers_.size()) {
                if (editNameButton_->isClicked() ) {
                    clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                    typingTarget=NAME;
                    selectedNameTexture_->setBackground(true);
                    return std::make_pair(START_TYPING,SceneTransitionData(adventurers_[selectedAdventurerId_].name_));
                }
                if (strInputControl_->isClicked()) {
                    clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                    adventurers_[selectedAdventurerId_].strength_=strInputControl_->getValue();
                    updatePointsLeft();
                    updateSelectedAdventurerStats();
                }
                if (dexInputControl_->isClicked()) {
                    clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                    adventurers_[selectedAdventurerId_].dexterity_=dexInputControl_->getValue();
                    updatePointsLeft();
                    updateSelectedAdventurerStats();
                }
                if (intInputControl_->isClicked()) {
                    clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                    adventurers_[selectedAdventurerId_].intelligence_=intInputControl_->getValue();
                    updatePointsLeft();
                    updateSelectedAdventurerStats();
                }
                //Loop through all weapon infoboxes and check if something changed
                int clickedMainHandInfoBox=-1;
                int clickedOffHandInfoBox=-1;
                for (int i = 0; i < weaponInfoButtonsMain_.size(); ++i) {
                    if (weaponInfoButtonsMain_[i]->isClicked()) {
                        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                        clickedMainHandInfoBox=i;
                    }
                    if (weaponInfoButtonsOff_[i]->isClicked()) {
                        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                        clickedOffHandInfoBox=i;
                    }
                }
                if (mainHandWeaponSelection_->getChangedSelection()) {
                    if (currentPointsLeft_> (itemList_[mainHandWeaponSelection_->getSelection()]->getValueCost())-(itemList_[adventurers_[selectedAdventurerId_].mainHandWeaponIndex]->getValueCost()) && clickedMainHandInfoBox==-1) {
                        adventurers_[selectedAdventurerId_].mainHandWeaponIndex=mainHandWeaponSelection_->getSelection();
                        updatePointsLeft();
                        updateSelectedAdventurerStats();
                    }
                    else {
                        mainHandWeaponSelection_->setSelection(adventurers_[selectedAdventurerId_].mainHandWeaponIndex);
                    }
                }
                if (offHandWeaponSelection_->getChangedSelection()) {
                    if (currentPointsLeft_> (itemList_[offHandWeaponSelection_->getSelection()]->getValueCost())-(itemList_[adventurers_[selectedAdventurerId_].offHandWeaponIndex]->getValueCost()) && clickedOffHandInfoBox==-1) {
                        adventurers_[selectedAdventurerId_].offHandWeaponIndex=offHandWeaponSelection_->getSelection();
                        updatePointsLeft();
                        updateSelectedAdventurerStats();
                    }
                    else {
                        offHandWeaponSelection_->setSelection(adventurers_[selectedAdventurerId_].offHandWeaponIndex);
                    }
                }
                if (clickedMainHandInfoBox!=-1) {
                    if (selectedAdventurerId_<adventurers_.size())
                        addWeaponStatPopup(clickedMainHandInfoBox,adventurers_[selectedAdventurerId_],renderer,screenWidth,screenHeight,smallFont,midFont,largeFont);
                }
                if (clickedOffHandInfoBox!=-1) {
                    if (selectedAdventurerId_<adventurers_.size())
                        addWeaponStatPopup(clickedOffHandInfoBox,adventurers_[selectedAdventurerId_],renderer,screenWidth,screenHeight,smallFont,midFont,largeFont);
                }
                if (genderSelection_->getChangedSelection()) {

                    adventurers_[selectedAdventurerId_].gender_=static_cast<AdventurerTemplate::Gender>(genderSelection_->getSelection());
                }
            }

            if (addAdventurerButton_->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                if (currentPointsLeft_>6) {
                    adventurers_.emplace_back("No name");
                    adventurerNames_.emplace_back(std::make_shared<TexWrap>("No name",renderer,midFont));
                    std::shared_ptr<textureControl> nameTC = std::make_shared<textureControl>(adventurerNames_.back());
                    std::shared_ptr<numberControl> costNR = std::make_shared<numberControl>(midNumberRenderer_,adventurers_.back().getCost());
                    adventurerNameControls_.push_back(nameTC);
                    adventurerCostControls_.push_back(costNR);
                    partyTable_->addRow(std::vector<std::shared_ptr<control>>{nameTC,costNR});
                    updatePointsLeft();
                    //Update shown information
                    updateSelectedAdventurerStats();
                }
            }
        }
    }

    return std::nullopt;
}

void TitleMenu::addWeaponStatPopup(int weapon,const AdventurerTemplate& adventurer,SDL_Renderer* renderer, int screenWidth, int screenHeight, TTF_Font* smallFont, TTF_Font* midFont, TTF_Font* largeFont) {

    if (weapon>=0 && weapon<itemList_.size()) {

        auto descriptionTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(itemList_[weapon]->getDescription(),renderer,smallFont,512));

        auto nothingTopLeft = std::make_shared<emptyControl>();
        auto baseTC= std::make_shared<textureControl>(baseButtonTexture_);
        auto strengthTC= std::make_shared<textureControl>(strengthButtonTexture_);
        auto dexterityTC= std::make_shared<textureControl>(dexterityButtonTexture_);
        auto intelligenceTC= std::make_shared<textureControl>(intelligenceButtonTexture_);
        auto equalTC= std::make_shared<textureControl>(equalButtonTexture_);

        auto baseMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Base value",renderer,midFont,512));
        auto strengthMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Strength scaling",renderer,midFont,512));
        auto dexterityMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Dexterity scaling",renderer,midFont,512));
        auto intelligenceMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Intelligence scaling",renderer,midFont,512));
        auto equalMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Result, for this adventurer",renderer,midFont,512));

        auto baseMO = std::make_shared<mouseOverControl>(baseTC,baseMTC);
        auto strengthMO = std::make_shared<mouseOverControl>(strengthTC,strengthMTC);
        auto dexterityMO = std::make_shared<mouseOverControl>(dexterityTC,dexterityMTC);
        auto intelligenceMO = std::make_shared<mouseOverControl>(intelligenceTC,intelligenceMTC);
        auto equalMO = std::make_shared<mouseOverControl>(equalTC,equalMTC);



        auto attackTC= std::make_shared<textureControl>(attackButtonTexture_);
        auto aimTC= std::make_shared<textureControl>(aimButtonTexture_);
        auto blockTC= std::make_shared<textureControl>(blockButtonTexture_);
        auto magicTC= std::make_shared<textureControl>(magicButtonTexture_);
        auto magicDefenceTC= std::make_shared<textureControl>(magicDefenceButtonTexture_);

        auto attackMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Physical Attack",renderer,midFont,512));
        auto aimMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Aim",renderer,midFont,512));
        auto blockMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Block",renderer,midFont,512));
        auto magicMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Magic",renderer,midFont,512));
        auto magicDefenceMTC= std::make_shared<textureControl> (std::make_shared<TexWrap>("Magic Defence",renderer,midFont,512));

        auto attackMO = std::make_shared<mouseOverControl>(attackTC,attackMTC);
        auto aimMO = std::make_shared<mouseOverControl>(aimTC,aimMTC);
        auto blockMO = std::make_shared<mouseOverControl>(blockTC,blockMTC);
        auto magicMO = std::make_shared<mouseOverControl>(magicTC,magicMTC);
        auto magicDefenceMO = std::make_shared<mouseOverControl>(magicDefenceTC,magicDefenceMTC);



        std::vector<std::shared_ptr<control> > content={nothingTopLeft,baseMO,strengthMO,dexterityMO,intelligenceMO,equalMO};
        //Add content
        content.emplace_back(attackMO);
        for (const auto & i : itemList_[weapon]->getPhysicAttackFactors()) {
            std::string valueStr = std::to_string(i.first)+"/"+std::to_string(i.second)+" ";
            auto valueTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(valueStr,renderer,midFont,512));
            content.emplace_back(valueTC);
        }
        {
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getAttack(adventurer.strength_,adventurer.dexterity_,adventurer.intelligence_));
            auto resultTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(resultStr,renderer,midFont,512));
            content.emplace_back(resultTC);
        }
        content.emplace_back(aimMO);
        for (const auto & i : itemList_[weapon]->getAimFactors()) {
            std::string valueStr = std::to_string(i.first)+"/"+std::to_string(i.second)+"  ";
            auto valueTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(valueStr,renderer,midFont,512));
            content.emplace_back(valueTC);
        }
        {
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getAim(adventurer.strength_,adventurer.dexterity_,adventurer.intelligence_));
            auto resultTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(resultStr,renderer,midFont,512));
            content.emplace_back(resultTC);
        }
        content.emplace_back(blockMO);
        for (const auto & i : itemList_[weapon]->getBlockFactors()) {
            std::string valueStr = std::to_string(i.first)+"/"+std::to_string(i.second)+"  ";
            auto valueTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(valueStr,renderer,midFont,512));
            content.emplace_back(valueTC);
        }
        {
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getBlock(adventurer.strength_,adventurer.dexterity_,adventurer.intelligence_));
            auto resultTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(resultStr,renderer,midFont,512));
            content.emplace_back(resultTC);
        }
        content.emplace_back(magicMO);
        for (const auto & i : itemList_[weapon]->getMagicFactors()) {
            std::string valueStr = std::to_string(i.first)+"/"+std::to_string(i.second)+"  ";
            auto valueTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(valueStr,renderer,midFont,512));
            content.emplace_back(valueTC);
        }
        {
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getMagicAttack(adventurer.strength_,adventurer.dexterity_,adventurer.intelligence_));
            auto resultTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(resultStr,renderer,midFont,512));
            content.emplace_back(resultTC);
        }
        content.emplace_back(magicDefenceMO);
        for (const auto & i : itemList_[weapon]->getMagicDefenceFactors()) {
            std::string valueStr = std::to_string(i.first)+"/"+std::to_string(i.second)+"  ";
            auto valueTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(valueStr,renderer,midFont,512));
            content.emplace_back(valueTC);
        }
        {
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getMagicDefence(adventurer.strength_,adventurer.dexterity_,adventurer.intelligence_));
            auto resultTC= std::make_shared<textureControl> (std::make_shared<TexWrap>(resultStr,renderer,midFont,512));
            content.emplace_back(resultTC);
        }

        auto table = std::make_shared<tableControl>(screenWidth,screenHeight,
            std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128)},
            std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128)},
            content
            );
        auto upDownTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/2)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth)},std::vector<std::shared_ptr<control> >{descriptionTC,table});

        gui_->addDialogue(itemList_[weapon]->getName(),upDownTable ,screenHeight,renderer,smallFont);
    }
}



void TitleMenu::updatePointsLeft() {
    int pts = maxPoints_;
    for (const auto& adventurer : adventurers_) {
        pts-=adventurer.getCost()+( itemList_[adventurer.mainHandWeaponIndex]->getValueCost())+( itemList_[adventurer.offHandWeaponIndex]->getValueCost());
        //TODO get cost of spells
    }
    if (pts!=currentPointsLeft_) {
        currentPointsLeft_=pts;
        pointsLeftNR_->setValue(pts);
    }
}

//Ai generated
void TitleMenu::saveFontSize() {
    // 1. Open and parse the JSON file
    std::ifstream inputFile(assetsPath()/"usersettings.json");
    if (!inputFile.is_open()) {
        throw std::runtime_error("Could not open file usersettings.json");
    }

    nlohmann::json settings;
    inputFile >> settings;
    inputFile.close();

    // 2. Modify the values
    settings["smallFontSize"] = std::min(std::max(smallFontSizeButton_->getValue(),8),128);
    settings["midFontSize"] = std::min(std::max(midFontSizeButton_->getValue(),8),128);
    settings["largeFontSize"] = std::min(std::max(largeFontSizeButton_->getValue(),8),128);

    // 3. Save back to file
    std::ofstream outputFile(assetsPath()/"usersettings.json");
    if (!outputFile.is_open()) {
        throw std::runtime_error("Could not open file usersettings.json for writing");
    }

    outputFile << settings.dump(4) << std::endl;
    outputFile.close();

}


void TitleMenu::render(SDL_Renderer *renderer, int screenWidth, int screenHeight, const InputData &userInputs, unsigned int millis, unsigned int pmillis) const {

    gui_->render(renderer,screenWidth,screenHeight,255,255,255,transitionTimer_<maxTransitionTimer_?(255*transitionTimer_)/maxTransitionTimer_:255);
    if (menuSlides_->getActiveSlide()==5) {//Roll credits
        int y = creditsGoHere_->getY0()+creditsGoHere_->getHeight()-creditsY;
        int x = creditsGoHere_->getX0();
        for (const auto &text: creditsTextures_) {
            int diff =creditsGoHere_->getY0()+creditsGoHere_->getHeight()-(y+text->getHeight());
            if (diff>0) {
                text->render(x,y,255,255,255,diff<255?diff:255, renderer);
            }
            y+=std::max(text->getHeight(),12);
        }

    }
}