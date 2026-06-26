//
// Created by nikolaj on 6/20/26.
//

#ifndef DUNGEONSKETCHTWO_GAME_H
#define DUNGEONSKETCHTWO_GAME_H
#include <random>
#include <utility>

#include "adventurer.h"
#include "corridor.h"
#include "foe.h"
#include "room.h"
#include "zone.h"
#include "../scene.h"
#include "../soundWrap.h"
#include "../textureManager.h"
#include "../MIGUI/contentTable.h"
#include "../MIGUI/dropdownMenu.h"
#include "../MIGUI/emptyControl.h"
#include "../MIGUI/guiManager.h"
#include "../MIGUI/numberControl.h"
#include "../MIGUI/numberInputControl.h"
#include "../MIGUI/slideControl.h"
#include "../MIGUI/textureControl.h"
#include "../shared/item.h"


class Game : public Scene
{
public:
    Game(const std::vector<AdventurerTemplate>& adventurerTemplates, uint32_t randomSeed, SDL_Renderer* renderer, int screenWidth, int screenHeight, TTF_Font* smallFont, TTF_Font* midFont, TTF_Font* largeFont);
    ~Game() override;

    void render(SDL_Renderer* renderer, int screenWidth, int screenHeight,const InputData& userInputs, unsigned int millis, unsigned int pmillis) const override;
    std::optional<std::pair<SceneInfo,SceneTransitionData>> update(SDL_Renderer* renderer, int screenWidth, int screenHeight,const InputData& userInputs,  unsigned int millis, unsigned int dmillis, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont) override;
private:
    void addWeaponStatPopup(int weapon,const Adventurer& adventurer,SDL_Renderer* renderer, int screenWidth, int screenHeight, TTF_Font* smallFont, TTF_Font* midFont, TTF_Font* largeFont);
    enum GameStatus {
        EXPLORATION=0,
        COMBAT=1,
        GAME_OVER=2,
    } gameStatus_=EXPLORATION;

    void endGame(SDL_Renderer *renderer, TTF_Font *midFont);
    std::string gameOverText_;
    int pointsInvested;
    int getPlayerValues();

    void checkPlayerStatus(SDL_Renderer *renderer, TTF_Font *midFont);

    void ensureValidSelection();

    struct AdventurerSideBarData {
        std::shared_ptr<textureControl> nameTC_;
        std::shared_ptr<numberControl> healthNC_;
        std::shared_ptr<numberControl> staminaNC_;
        std::shared_ptr<numberControl> xNC_;
        std::shared_ptr<numberControl> yNC_;
        std::shared_ptr<numberControl> zNC_;

        AdventurerSideBarData (const std::string& name, int h, int s,int x, int y, int z,SDL_Renderer* renderer, TTF_Font* font, const NumberRenderer& numberRenderer) {
            nameTC_=std::make_shared<textureControl>(std::make_shared<TexWrap>(name,renderer,font));
            healthNC_=std::make_shared<numberControl>(numberRenderer,h);
            staminaNC_=std::make_shared<numberControl>(numberRenderer,s);
            xNC_=std::make_shared<numberControl>(numberRenderer,x);
            yNC_=std::make_shared<numberControl>(numberRenderer,y);
            zNC_=std::make_shared<numberControl>(numberRenderer,z);
        }
    };

    void updateSidebarInfo();

    //Main view text
    std::string mainViewText_;
    size_t mainViewIndex_;
    uint32_t nextCharTime_;
    const uint32_t charTimer = 50;
    int prevWidth=0;
    size_t selectedPlayerIndex_=0;

    //Command bar commands
    //Exploration mode:
    //Navigation:
    std::shared_ptr<buttonControl> goUpButton_, goDownButton_, goWestButton_, goEastButton_, goNorthButton_, goSouthButton_;
    std::shared_ptr<buttonControl> pickUpItemButton_, dropItemButton_, castSpellButton_;

    //Generate the description text centered on this player
    void generateDescriptionText(const std::string& preText,size_t playerID,SDL_Renderer* renderer, TTF_Font* font);

    void updateSinceLocationChanged(SDL_Renderer* renderer, TTF_Font* smallFont, TTF_Font* midFont);

    void updateInventoryDisplay(SDL_Renderer* renderer, TTF_Font* midFont);

    std::shared_ptr<TexWrap> typingText;
    std::shared_ptr<TexWrap> pressEnterToFastForward_;
    std::shared_ptr<emptyControl> mainCanvas_;

    //Dungeon data

    std::vector<Corridor> corridors_;
    std::vector<Room> rooms_;

    ///Size of the dungeon
    const int dungeonWidth = 6, dungeonHeight = 8, dungeonDepth = 3;
    //How far does the main branch snake before being done
    const int maxFloorSnakeLength;
    const int roomIdOffset;
    const int corridorIdOffset_;
    const int roomsPerZone_ = 4;
    const int lootGenerationDepth_ =5;//How many rooms ahead or behind do we look when generating loot
    const double lootChance_ = 1.0;//chance each champer spawns loot

    std::vector<Zone> zoneTemplates_;
    int maxUsedZone_=0;
    std::vector<int> switchedRooms_;
    std::vector<bool> switchesActive_;

    void updateSwitches();

    int startRoomXId=0;
    int startRoomYId=0;

    void generate(uint32_t randomSeed);

    void closeAllDialogues();


    struct pickupDialogue {
        size_t targetPlayerId_;
        std::shared_ptr<dialogue> dialogue_;
        std::shared_ptr<buttonControl> pickupButton_;
        std::shared_ptr<DropdownMenu> selectItemMenu_;

        pickupDialogue(size_t targetPlayerId,
            std::shared_ptr<dialogue> dialogue,
            std::shared_ptr<buttonControl> pickupButton,
            std::shared_ptr<DropdownMenu> selectItemMenu) {
            targetPlayerId_=targetPlayerId;
            dialogue_=std::move(dialogue);
            pickupButton_=std::move(pickupButton);
            selectItemMenu_ = std::move(selectItemMenu);
        }
    };

    std::shared_ptr<pickupDialogue> activePickupDialog_=nullptr;


    struct dropDialogue {
        size_t targetPlayerId_;
        std::shared_ptr<dialogue> dialogue_;
        std::shared_ptr<buttonControl> pickupButton_;
        std::shared_ptr<DropdownMenu> selectItemMenu_;

        dropDialogue(size_t targetPlayerId,
            std::shared_ptr<dialogue> dialogue,
            std::shared_ptr<buttonControl> pickupButton,
            std::shared_ptr<DropdownMenu> selectItemMenu) {
            targetPlayerId_=targetPlayerId;
            dialogue_=std::move(dialogue);
            pickupButton_=std::move(pickupButton);
            selectItemMenu_ = std::move(selectItemMenu);
        }
    };

    std::shared_ptr<pickupDialogue> activeDropDialog_=nullptr;




    //Assets
    std::shared_ptr<const SoundWrap> clickSound_;
    std::shared_ptr<const SoundWrap> walkSound_;
    std::shared_ptr<const SoundWrap> bonkSound_;
    std::shared_ptr<const SoundWrap> pickupSound_;
    std::shared_ptr<const SoundWrap> switchSound_;
    std::shared_ptr<const SoundWrap> gongSound_;

    std::shared_ptr<const TexWrap> haircutButtonTexture_;
    std::shared_ptr<const TexWrap> eyeButtonTexture_;
    std::shared_ptr<const TexWrap> emptyButtonTexture_;
    std::shared_ptr<const TexWrap> handTexture_;
    std::shared_ptr<const TexWrap> clothesButtonTexture_;
    std::shared_ptr<const TexWrap> genderButtonTexture_;
    std::shared_ptr<const TexWrap> strengthButtonTexture_;
    std::shared_ptr<const TexWrap> dexterityButtonTexture_;
    std::shared_ptr<const TexWrap> intelligenceButtonTexture_;
    std::shared_ptr<const TexWrap> speedButtonTexture_;
    std::shared_ptr<const TexWrap> dodgeButtonTexture_;
    std::shared_ptr<const TexWrap> attackButtonTexture_;
    std::shared_ptr<const TexWrap> aimButtonTexture_;
    std::shared_ptr<const TexWrap> blockButtonTexture_;
    std::shared_ptr<const TexWrap> magicButtonTexture_;
    std::shared_ptr<const TexWrap> magicDefenceButtonTexture_;
    std::shared_ptr<const TexWrap> carryCapacityButtonTexture_;
    std::shared_ptr<const TexWrap> editNameButtonTexture_;
    std::shared_ptr<const TexWrap> plusButtonTexture_;
    std::shared_ptr<const TexWrap> minusButtonTexture_;
    std::shared_ptr<const TexWrap> expandButtonTexture_;
    std::shared_ptr<const TexWrap> infoButtonTexture_;
    std::shared_ptr<const TexWrap> baseButtonTexture_;
    std::shared_ptr<const TexWrap> equalButtonTexture_;

    std::shared_ptr<const TexWrap> playerMapTexture_;
    std::shared_ptr<const TexWrap> lockMapTexture_;
    std::shared_ptr<const TexWrap> switchMapTexture_;
    std::shared_ptr<const TexWrap> upMapTexture_;
    std::shared_ptr<const TexWrap> downMapTexture_;
    std::shared_ptr<const TexWrap> enemiesMapTexture_;



    //The place we will be drawing the map to
    std::shared_ptr<buttonControl> mapButton_;
    std::shared_ptr<buttonControl> mainButton_;
    std::shared_ptr<buttonControl> inventoryButton_;
    std::shared_ptr<buttonControl> statsButton_;
    std::shared_ptr<NumberInputControl> selectedFloorNC_;

    enum selectedView {
        MAIN_WINDOW=0,
        MAP_WINDOW=1,
        INVENTORY=2,
    } selectedView_=MAIN_WINDOW;

    std::vector<std::shared_ptr<buttonControl>> inventoryInfoButtons_;
    std::shared_ptr<buttonControl> mainHandInfoButton_=nullptr;
    std::shared_ptr<buttonControl> offHandInfoButton_=nullptr;


    std::shared_ptr<contentTable> scoreTable_;
    std::shared_ptr<buttonControl> quitGameButton_;

    std::shared_ptr<buttonControl> quitEarlyButton_;
    std::shared_ptr<buttonControl> reallyQuitButton_;

    std::shared_ptr<SlideControl> gameOverSlide_;

    std::shared_ptr<SlideControl> mainWindow_;
    std::shared_ptr<emptyControl> mapCanvas_;
    std::shared_ptr<contentTable> playerTable_;

    std::shared_ptr<contentTable> inventoryTable_;
    std::shared_ptr<SlideControl> inventorySlideControl_;
    std::shared_ptr<SlideControl> statSlideControl_;

    //Stuff in the inventory/stats view:

    std::shared_ptr<textureControl>selectedNameTexture_;
    std::shared_ptr<numberControl> strViewControl_;
    std::shared_ptr<numberControl> dexViewControl_;
    std::shared_ptr<numberControl> intViewControl_;
    std::shared_ptr<numberControl> speedViewControl_;
    std::shared_ptr<numberControl> carryViewControl_;
    std::shared_ptr<numberControl> dodgeViewControl_;
    std::shared_ptr<numberControl> aimViewControl_;
    std::shared_ptr<numberControl> attackViewControl_;
    std::shared_ptr<numberControl> blockViewControl_;
    std::shared_ptr<numberControl> magicViewControl_;
    std::shared_ptr<numberControl> magicDefenceViewControl_;
    std::shared_ptr<numberControl> selectedCostControl_;
    std::shared_ptr<textureControl>mainHandWeaponSelection_;
    std::shared_ptr<textureControl>offHandWeaponSelection_;

    std::shared_ptr<buttonControl> setMainHandButton_;
    std::shared_ptr<buttonControl> setOffHandButton_;



    std::unique_ptr<GUIManager> gui_;

    TextureManager textureManager_;
    NumberRenderer midNumberRenderer_;
    NumberRenderer smallNumberRenderer_;
    std::mt19937 rng;

    std::vector<std::shared_ptr<Item>> itemList_;
    std::vector<Foe> foeTemplates_;
    std::vector<std::vector<int>> itemsByValue_;
    std::vector<int> legendaryRewards_;//The deepest champer always contains a Legendary light item

    std::vector<Adventurer> adventurers_;
    std::vector<AdventurerSideBarData > adventurerSideBarData_;

    void setupGui(SDL_Renderer *renderer, int screenWidth, int screenHeight, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont);
};


#endif //DUNGEONSKETCHTWO_GAME_H