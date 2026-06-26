//
// Created by nikolaj on 6/1/26.
//

#ifndef DUNGEONSKETCH_TITLEMENU_H
#define DUNGEONSKETCH_TITLEMENU_H
#include "../shared/AdventurerTemplate.h"
#include "../NumberRenderer.h"
#include "../scene.h"
#include "../soundWrap.h"
#include "../textureManager.h"
#include "../TexWrap.h"
#include "../MIGUI/contentTable.h"
#include "../MIGUI/dropdownMenu.h"
#include "../MIGUI/emptyControl.h"
#include "../MIGUI/guiManager.h"
#include "../MIGUI/numberControl.h"
#include "../MIGUI/numberInputControl.h"
#include "../MIGUI/slideControl.h"
#include "../MIGUI/tableControl.h"
#include "../MIGUI/textureControl.h"
#include "../shared/item.h"

#define MAX_PARTY_SIZE 8

class TitleMenu : public Scene {
public:
    TitleMenu(SDL_Renderer* renderer, int screenWidth, int screenHeight, TTF_Font* smallFont, TTF_Font* midFont, TTF_Font* largeFont, int smallFontSize, int midFontSize, int largeFontSize, bool startAtSettings=false);
    ~TitleMenu() override;

    void render(SDL_Renderer* renderer, int screenWidth, int screenHeight,const InputData& userInputs, unsigned int millis, unsigned int pmillis) const override;
    std::optional<std::pair<SceneInfo,SceneTransitionData>> update(SDL_Renderer* renderer, int screenWidth, int screenHeight,const InputData& userInputs,  unsigned int millis, unsigned int dmillis, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont) override;
private:
    void addWeaponStatPopup(int weapon,const AdventurerTemplate& adventurer,SDL_Renderer* renderer, int screenWidth, int screenHeight, TTF_Font* smallFont, TTF_Font* midFont, TTF_Font* largeFont);
    void updateSelectedAdventurerStats();


    //Assets
    std::shared_ptr<const SoundWrap> clickSound_;
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
    std::string splashText_;

    int transitionTimer_;
    bool transitionIn_;
    const int maxTransitionTimer_=10000;


    //main Menu
    std::shared_ptr<buttonControl> createPartyButton_;
    std::shared_ptr<buttonControl> creditsButton_;
    std::shared_ptr<buttonControl> settingsButton_;
    std::shared_ptr<buttonControl> quitButton_;

    //Credits page
    std::shared_ptr<buttonControl> goBackFromCreditsButton_;
    std::shared_ptr<emptyControl> creditsGoHere_;

    std::vector<std::string> creditsText_;
    std::vector<std::shared_ptr<TexWrap>> creditsTextures_;
    int creditsY=0;

    //Settings menu
    std::shared_ptr<buttonControl> goBackFromSettingsButton_;
    int smallFontSize_;
    int midFontSize_;
    int largeFontSize_;
    std::shared_ptr<NumberInputControl> smallFontSizeButton_;
    std::shared_ptr<NumberInputControl> largeFontSizeButton_;
    std::shared_ptr<NumberInputControl> midFontSizeButton_;
    std::shared_ptr<buttonControl> applySettingsButton_;

    void saveFontSize();

    //PartyCreation
    std::shared_ptr<buttonControl> goBackFromPartyCreationButton_;
    std::shared_ptr<buttonControl> addAdventurerButton_;
    std::shared_ptr<buttonControl> savePartyButton_;
    std::shared_ptr<buttonControl> loadPartyButton_;
    std::shared_ptr<buttonControl> playGameButton_;
    std::shared_ptr<contentTable> partyTable_;
    std::shared_ptr<SlideControl> selectAdventurerToShowSlide_;
    std::shared_ptr<numberControl> pointsLeftNR_;
    std::vector<std::shared_ptr<buttonControl>> weaponInfoButtonsMain_;
    std::vector<std::shared_ptr<buttonControl>> weaponInfoButtonsOff_;
    std::vector<std::shared_ptr<control>> weaponStatPanels;

    std::shared_ptr<textureControl> selectedNameTexture_;
    std::shared_ptr<buttonControl> editNameButton_;
    std::shared_ptr<NumberInputControl> strInputControl_;
    std::shared_ptr<NumberInputControl> dexInputControl_;
    std::shared_ptr<NumberInputControl> intInputControl_;
    std::shared_ptr<numberControl> speedInputControl_;
    std::shared_ptr<numberControl> carryInputControl_;
    std::shared_ptr<numberControl> dodgeInputControl_;
    std::shared_ptr<numberControl> aimInputControl_;
    std::shared_ptr<numberControl> attackInputControl_;
    std::shared_ptr<numberControl> blockInputControl_;
    std::shared_ptr<numberControl> magicInputControl_;
    std::shared_ptr<numberControl> magicDefenceInputControl_;
    std::shared_ptr<numberControl> selectedCostControl_;
    std::shared_ptr<DropdownMenu> mainHandWeaponSelection_;
    std::shared_ptr<DropdownMenu> offHandWeaponSelection_;
    std::shared_ptr<DropdownMenu> genderSelection_;
    std::vector<std::shared_ptr<textureControl>> adventurerNameControls_;
    std::vector<std::shared_ptr<numberControl>> adventurerCostControls_;




    std::shared_ptr<SlideControl> menuSlides_;


    void setupGui(SDL_Renderer *renderer, int screenWidth, int screenHeight, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont);

    std::unique_ptr<GUIManager> gui_;

    TextureManager textureManager_;
    NumberRenderer midNumberRenderer_;

    void updatePointsLeft();

    enum TypingTarget {
        NONE,
        NAME,
    } typingTarget=NONE;

    //Create team variables
    int maxPoints_ = 200;
    int currentPointsLeft_ = maxPoints_;
    std::vector<AdventurerTemplate> adventurers_;
    std::vector<std::shared_ptr<Item>> itemList_;
    int startingWeapons_ = 0;
    std::vector<std::shared_ptr<TexWrap>> adventurerNames_;
    size_t selectedAdventurerId_=-1;
};


#endif //DUNGEONSKETCH_TITLEMENU_H