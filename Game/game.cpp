//
// Created by nikolaj on 6/20/26.
//

#include "game.h"

#include <fstream>
#include <set>

#include "../getAssets.h"
#include "../MIGUI/contentTable.h"
#include "../MIGUI/mouseOverControl.h"
#include "../MIGUI/stackControl.h"
#include "../MIGUI/tableControl.h"
#include "../MIGUI/textureControl.h"

Game::~Game() = default;

Game::Game(const std::vector<AdventurerTemplate>& adventurerTemplates, uint32_t randomSeed, SDL_Renderer *renderer, int screenWidth, int screenHeight, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont):
midNumberRenderer_(0,midFont,renderer),
smallNumberRenderer_(0,smallFont,renderer),
roomIdOffset(dungeonWidth*dungeonHeight),
corridorIdOffset_(dungeonWidth*dungeonHeight*2),
maxFloorSnakeLength(dungeonWidth*dungeonHeight/2),
rng(randomSeed)
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
        fs::path("menu")/"carryCapacityButton.png",
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
        fs::path("menu")/"baseButton.png",
        fs::path("menu")/"equalButton.png",
    };

    //Load items

    itemsByValue_=std::vector<std::vector<int>>(4);//common, rare, epic, legendary (worthless items do not spawn, as it is only bare hands)
    {
        std::ifstream itemsFile(assetsPath()/"items.json");

        if (!itemsFile.is_open())
            throw std::runtime_error("Could not open items.json");
        nlohmann::json itemsJson;
        itemsFile >> itemsJson;
        for (const auto& entry : itemsJson["items"]) {
            itemList_.push_back(std::make_shared<Item>(entry));
            if (itemList_.back()->getValue()==Item::COMMON) {
                itemsByValue_[0].push_back(itemList_.size()-1);
            }
            else if (itemList_.back()->getValue()==Item::RARE) {
                itemsByValue_[1].push_back(itemList_.size()-1);
            }
            else if (itemList_.back()->getValue()==Item::EPIC) {
                itemsByValue_[2].push_back(itemList_.size()-1);
            }
            else if (itemList_.back()->getValue()==Item::LEGENDARY) {
                itemsByValue_[3].push_back(itemList_.size()-1);
                if (itemList_.back()->getWeight()==Item::LIGHT) {
                    legendaryRewards_.push_back(itemList_.size()-1);
                }
            }
        }

        itemsFile.close();

        for (const auto &itemList : itemsByValue_)
            if (itemList.empty())
                throw std::runtime_error("items.json is missing items of all value types");
        if (legendaryRewards_.empty())
            throw std::runtime_error("items.json is missing legendary rewards");
    }

    {
        std::ifstream foesFile(assetsPath()/"foes.json");

        if (!foesFile.is_open())
            throw std::runtime_error("Could not open foes.json");
        nlohmann::json foesJson;
        foesFile >> foesJson;

        for (const auto& entry : foesJson) {
            std::string name = entry["name"].get<std::string>();
            int intelligence = entry["intelligence"].get<int>();
            int strength = entry["strength"].get<int>();
            int dexterity = entry["dexterity"].get<int>();
            int health = entry["health"].get<int>();
            std::string mainItem = entry["mainItem"].get<std::string>();
            int mainItemId = -1;
            for (int i =0; i < itemList_.size(); i++) {
                if (itemList_[i]->getName()==mainItem) {
                    mainItemId = i;
                    break;
                }
            }
            if (mainItemId==-1)
                throw std::runtime_error("mainItem "+mainItem+" could not be found");
            std::string otherItem = entry["otherItem"].get<std::string>();
            int otherItemId = -1;
            for (int i =0; i < itemList_.size(); i++) {
                if (itemList_[i]->getName()==otherItem) {
                    otherItemId = i;
                    break;
                }
            }
            if (otherItemId==-1)
                throw std::runtime_error("otherItem "+otherItem+" could not be found");
            foeTemplates_.emplace_back(name,strength,dexterity,intelligence,health,mainItemId,otherItemId);

            /*
            const auto & foe = foeTemplates_.back();
            std::cout<<"==="<<name<<"==="<<std::endl;
            std::cout<<"Main weapon attack = "<<itemList_[foe.mainItem_]->getAttack(foe.strength_,foe.dexterity_,foe.intelligence_)<<std::endl;
            std::cout<<"Main weapon magic = "<<itemList_[foe.mainItem_]->getMagicAttack(foe.strength_,foe.dexterity_,foe.intelligence_)<<std::endl;
            std::cout<<"Main weapon aim = "<<itemList_[foe.mainItem_]->getAim(foe.strength_,foe.dexterity_,foe.intelligence_)<<std::endl;
            std::cout<<"offhand weapon attack = "<<itemList_[foe.otherItem_]->getAttack(foe.strength_,foe.dexterity_,foe.intelligence_)<<std::endl;
            std::cout<<"offhand weapon magic = "<<itemList_[foe.otherItem_]->getMagicAttack(foe.strength_,foe.dexterity_,foe.intelligence_)<<std::endl;
            std::cout<<"offhand weapon aim = "<<itemList_[foe.otherItem_]->getAim(foe.strength_,foe.dexterity_,foe.intelligence_)<<std::endl;
            std::cout<<"block = "<<foe.getBlock(itemList_)<<std::endl;
            std::cout<<"magic defence = "<<foe.getMagicDefence(itemList_)<<std::endl;
`           */
        }

        foesFile.close();
    }

    ThreadPool loadingPool(std::thread::hardware_concurrency());
    textureManager_.launchTextureLoading(textureRequests, assetsPath(),loadingPool);

    playerMapTexture_=std::make_shared<TexWrap>("P",renderer,smallFont);
    lockMapTexture_=std::make_shared<TexWrap>("L",renderer,smallFont);
    switchMapTexture_=std::make_shared<TexWrap>("S",renderer,smallFont);
    upMapTexture_=std::make_shared<TexWrap>("U",renderer,smallFont);
    downMapTexture_=std::make_shared<TexWrap>("D",renderer,smallFont);
    enemiesMapTexture_=std::make_shared<TexWrap>("E",renderer,smallFont);

    for (const auto& adventurer : adventurerTemplates)
        adventurers_.emplace_back(adventurer);

    zoneTemplates_=std::vector<Zone>{
        Zone("Red",255,0,0),
        Zone("Green",0,255,0),
        Zone("Blue",0,0,255),
        Zone("Magenta",255,0,255),
        Zone("Cyan",0,255,255),
        Zone("Yellow",255,255,0),
        Zone("Black",0,0,0),
        Zone("White",255,255,255),
        };
    generate(randomSeed);
    clickSound_=std::make_shared<SoundWrap>(assetsPath()/"sounds"/"click.mp3");
    walkSound_=std::make_shared<SoundWrap>(assetsPath()/"sounds"/"steps.mp3");
    bonkSound_=std::make_shared<SoundWrap>(assetsPath()/"sounds"/"bonk.mp3");
    pickupSound_=std::make_shared<SoundWrap>(assetsPath()/"sounds"/"pickup.wav");
    switchSound_=std::make_shared<SoundWrap>(assetsPath()/"sounds"/"switch.wav");
    gongSound_=std::make_shared<SoundWrap>(assetsPath()/"sounds"/"gong.mp3");

    //Immediately play the gong
    gongSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
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


    setupGui(renderer,screenWidth,screenHeight,smallFont,midFont,largeFont);

    playerTable_->setSelection(selectedPlayerIndex_);
    typingText=std::make_shared<TexWrap>(" ",renderer,smallFont);
    generateDescriptionText("Welcome to Dungeon Raid.\n",0,renderer,smallFont);
    mainViewIndex_=0;
    nextCharTime_=charTimer;

    pressEnterToFastForward_=std::make_shared<TexWrap>("Press enter to fast forward",renderer,midFont);

    updateInventoryDisplay(renderer,midFont);
    pointsInvested=getPlayerValues();
}

void Game::generateDescriptionText(const std::string& preText,size_t playerID,SDL_Renderer* renderer, TTF_Font* font) {
    if (playerID>adventurers_.size()) {
        nextCharTime_=charTimer;
        mainViewIndex_=0;
        mainViewText_=preText;
        typingText->reset(" ",renderer,font,prevWidth);
        return;
    }
    nextCharTime_=charTimer;
    mainViewIndex_=0;
    mainViewText_=preText;
    std::vector<size_t> playersInThisRoom={playerID};

    if (adventurers_[playerID].getFloor()<0) {
        mainViewText_+=adventurers_[playerID].getName()+" is outside the dungeon\n";
    }
    else {
        int roomId =adventurers_[playerID].getRoomX()+adventurers_[playerID].getRoomY()*dungeonWidth+adventurers_[playerID].getFloor()*roomIdOffset;

        for (int i = 0; i < adventurers_.size(); i++) {
            if (adventurers_[i].getFloor()==adventurers_[playerID].getFloor() &&
            adventurers_[i].getRoomX()==adventurers_[playerID].getRoomX() &&
            adventurers_[i].getRoomY()==adventurers_[playerID].getRoomY() && i!=playerID
            ) {
                playersInThisRoom.emplace_back(i);
            }
        }

        if (playersInThisRoom.size()==1) {
            mainViewText_+=adventurers_[playerID].getName()+" is ";
        }
        else if (playersInThisRoom.size()==2) {
            mainViewText_+=adventurers_[playersInThisRoom[0]].getName()+" and "+adventurers_[playersInThisRoom[1]].getName()+" are ";
        }
        else {
            for (int i = 0; i+1 < playersInThisRoom.size(); i++) {
                mainViewText_+=adventurers_[playersInThisRoom[i]].getName()+", ";
            }
            mainViewText_+=" and "+adventurers_[playersInThisRoom.back()].getName()+" are ";
        }
        auto &z =zoneTemplates_[rooms_[roomId].zone_];
        mainViewText_+="in a room with "+z.colourName_+" walls.\n\n";

        if (rooms_[roomId].hasSwitch_) {
            std::string colourName;
            if (rooms_[roomId].zone_==0) {
                colourName=zoneTemplates_[rooms_[roomId].zone_+1].colourName_;
            }
            else if ( rooms_[roomId].zone_==maxUsedZone_) {
                colourName=zoneTemplates_[rooms_[roomId].zone_].colourName_;
            }
            else
                colourName=zoneTemplates_[rooms_[roomId].zone_].colourName_+" and "+zoneTemplates_[rooms_[roomId].zone_+1].colourName_;
            mainViewText_+="There is a "+colourName+" pressure plate in this room, drop any HEAVY item inside this room, or have a player standing in this room to open all "+colourName+" doors!\n\n";
        }


        if (rooms_[roomId].ladder_==Room::UP) {
            if (adventurers_[playerID].getFloor()==0) {
                mainViewText_+="The exit ladder is here, go up to leave the dungeon (THIS CAN NOT BE UNDONE) when all players leave the dungeon (or die) the game ends\n\n";
            }
            else
                mainViewText_+="There is a ladder going up to floor "+std::to_string(1-adventurers_[playerID].getFloor())+"\n\n";
        }
        else if (rooms_[roomId].ladder_==Room::DOWN) {
            mainViewText_+="There is a ladder going down to floor "+std::to_string(-1-adventurers_[playerID].getFloor())+"\n\n";
        }

        //Describe corridors

        std::vector<std::pair<std::string,int>> possibleExitIds;

        if (adventurers_[playerID].getRoomX()>0) {
            possibleExitIds.emplace_back("west",adventurers_[playerID].getRoomX()-1 + adventurers_[playerID].getRoomY()*2*dungeonWidth + adventurers_[playerID].getFloor()*corridorIdOffset_);
        }
        if (adventurers_[playerID].getRoomX()+1<dungeonWidth) {
            possibleExitIds.emplace_back("east",adventurers_[playerID].getRoomX() + adventurers_[playerID].getRoomY()*2*dungeonWidth + adventurers_[playerID].getFloor()*corridorIdOffset_);
        }
        if (adventurers_[playerID].getRoomY()>0) {
            possibleExitIds.emplace_back("north",adventurers_[playerID].getRoomX() + (adventurers_[playerID].getRoomY()*2-1)*dungeonWidth + adventurers_[playerID].getFloor()*corridorIdOffset_);
        }
        if (adventurers_[playerID].getRoomY()+1<dungeonHeight) {
            possibleExitIds.emplace_back("south",adventurers_[playerID].getRoomX() + (adventurers_[playerID].getRoomY()*2+1)*dungeonWidth + adventurers_[playerID].getFloor()*corridorIdOffset_);
        }

        for (const auto& [dir,corridorId] : possibleExitIds) {
            if (corridors_[corridorId].type==Corridor::OPEN) {
                if (corridors_[corridorId].foes.empty())
                    mainViewText_+="There is an open corridor going "+dir+"\n\n";
                else {
                    mainViewText_+="The corridor "+dir+" is blocked by the following vile foes:\n";
                    for (const auto &foe : corridors_[corridorId].foes) {
                        mainViewText_+=foe.name_+" (level "+std::to_string(foe.getLevel())+")\n";
                    }
                    mainViewText_+="Go "+dir+" to initiate combat!\n\n";
                }
            }
            else if (corridors_[corridorId].type==Corridor::DOOR) {
                if (switchesActive_[corridors_[corridorId].id])
                    mainViewText_+="There is an open "+zoneTemplates_[corridors_[corridorId].id].colourName_+" metal door "+dir+"\n\n";
                else
                    mainViewText_+="There is a locked "+zoneTemplates_[corridors_[corridorId].id].colourName_+" metal door "+dir+", you need to find the matching *pressure plate* to open it\n\n";
            }
        }


        if (!rooms_[roomId].itemIds.empty()) {
            mainViewText_+="There is the following loot in this room:\n";
            for (int i : rooms_[roomId].itemIds) {
                mainViewText_+=itemList_[i]->getName()+": ("+itemList_[i]->getValueStr()+" "+itemList_[i]->getWeightStr()+"), ";
            }
        }
    }

    typingText->reset(" ",renderer,font,prevWidth);
}

void Game::generate(uint32_t randomSeed) {
    std::mt19937 randomGenerator(randomSeed);
    switchedRooms_.clear();
    switchesActive_=std::vector<bool>(zoneTemplates_.size(),false);

    //This vector is slightly too big, but it makes addressing corridors trivial
    //corridor id is x+y*dungeonWidth+z*dungeonWidth*(2*dungeonHeight)
    //Even y gets corridors in the x direction, there are really only dungeonWidth-1 of them for a fixed y
    //Odd y gets corridors in the y direction, there are dungeonWidth of them for a fixed y
    //This generates walls only
    corridors_=std::vector<Corridor>(dungeonWidth*(2*dungeonHeight)*dungeonDepth);

    rooms_=std::vector<Room>(dungeonWidth*dungeonHeight*dungeonDepth);

    //Update the ids of all rooms
    for (int x = 0; x < dungeonWidth; ++x) {
        for (int y = 0; y < dungeonHeight; ++y) {
            for (int z = 0; z < dungeonDepth; ++z) {
                rooms_[x+y*dungeonWidth+z*roomIdOffset].xId=x;
                rooms_[x+y*dungeonWidth+z*roomIdOffset].yId=y;
                rooms_[x+y*dungeonWidth+z*roomIdOffset].zId=z;
            }
        }
    }

    for (int x = 0; x < dungeonWidth; ++x) {
        for (int y = 0; y < dungeonHeight; ++y) {
            for (int z = 0; z < dungeonDepth; ++z) {
                corridors_[x+2*y*dungeonWidth+z*corridorIdOffset_].xDir=true;
            }
        }
    }

    //List of ids of all the rooms in all the branches
    std::vector<int>mainBranch;

    //First pick a starting room
    std::uniform_int_distribution<int> startRoomXDist(0, dungeonWidth-1);
    std::uniform_int_distribution<int> startRoomYDist(0, dungeonHeight-1);
    startRoomXId = startRoomXDist(randomGenerator);
    startRoomYId = startRoomYDist(randomGenerator);

    rooms_[startRoomXId + startRoomYId*dungeonWidth].ladder_=Room::UP;
    rooms_[startRoomXId + startRoomYId*dungeonWidth].exit=Room::MINUS_Z;
    rooms_[startRoomXId + startRoomYId*dungeonWidth].branchId=0;

    //Then snake the main branch through the dungeon
    {
        int headXId= startRoomXId;
        int headYId= startRoomYId;
        mainBranch.emplace_back(headXId + headYId*dungeonWidth+(0)*roomIdOffset);


        std::uniform_int_distribution<int> snakeLengthDist(maxFloorSnakeLength/2+1,maxFloorSnakeLength);
        for (int headZId=0; headZId<dungeonDepth; ++headZId) {
            int snakeLength = snakeLengthDist(randomGenerator);
            for (int i = 0; i < snakeLength; ++i) {
                //Check which neighbours are free
                std::vector<Room::Direction> directions;

                if (headXId > 0 && rooms_[headXId-1 + headYId*dungeonWidth+headZId*roomIdOffset].branchId==-1) {
                    directions.emplace_back(Room::Direction::MINUS_X);
                }
                if (headXId+1 < dungeonWidth && rooms_[headXId+1 + headYId*dungeonWidth+headZId*roomIdOffset].branchId==-1) {
                    directions.emplace_back(Room::Direction::PLUS_X);
                }
                if (headYId > 0 && rooms_[headXId + (headYId-1)*dungeonWidth+headZId*roomIdOffset].branchId==-1) {
                    directions.emplace_back(Room::Direction::MINUS_Y);
                }
                if (headYId+1 < dungeonHeight && rooms_[headXId + (headYId+1)*dungeonWidth+headZId*roomIdOffset].branchId==-1) {
                    directions.emplace_back(Room::Direction::PLUS_Y);
                }

                if (directions.empty() || i+1==snakeLength) {
                    if (headZId+1==dungeonDepth) {
                        rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].deeper=Room::HERE;
                    }
                    else {
                        rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].ladder_=Room::DOWN;
                        rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].deeper=Room::PLUS_Z;
                        rooms_[headXId + headYId*dungeonWidth+(headZId+1)*roomIdOffset].ladder_=Room::UP;
                        rooms_[headXId + headYId*dungeonWidth+(headZId+1)*roomIdOffset].exit=Room::MINUS_Z;
                        rooms_[headXId + headYId*dungeonWidth+(headZId+1)*roomIdOffset].branchId=0;
                        rooms_[headXId + headYId*dungeonWidth+(headZId+1)*roomIdOffset].depth=rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].depth+1;

                        mainBranch.emplace_back(headXId + headYId*dungeonWidth+(headZId+1)*roomIdOffset);
                    }
                    break;
                }
                else {
                    std::uniform_int_distribution<int> dirDist(0,directions.size()-1);
                    switch (directions[dirDist(randomGenerator)]) {
                        default:
                        case Room::Direction::MINUS_X:
                            rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].deeper=Room::MINUS_X;
                            rooms_[headXId-1 + headYId*dungeonWidth+(headZId)*roomIdOffset].exit=Room::PLUS_X;
                            rooms_[headXId-1 + headYId*dungeonWidth+(headZId)*roomIdOffset].branchId=0;
                            rooms_[headXId-1 + headYId*dungeonWidth+(headZId)*roomIdOffset].depth=rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].depth+1;

                            corridors_[headXId-1 + headYId*dungeonWidth*2+(headZId)*corridorIdOffset_].type=Corridor::OPEN;

                            headXId--;
                            mainBranch.emplace_back(headXId + headYId*dungeonWidth+(headZId)*roomIdOffset);
                            break;
                        case Room::Direction::PLUS_X:
                            rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].deeper=Room::PLUS_X;
                            rooms_[headXId+1 + headYId*dungeonWidth+(headZId)*roomIdOffset].exit=Room::MINUS_X;
                            rooms_[headXId+1 + headYId*dungeonWidth+(headZId)*roomIdOffset].branchId=0;
                            rooms_[headXId+1 + headYId*dungeonWidth+(headZId)*roomIdOffset].depth=rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].depth+1;

                            corridors_[headXId + headYId*dungeonWidth*2+(headZId)*corridorIdOffset_].type=Corridor::OPEN;

                            headXId++;
                            mainBranch.emplace_back(headXId + headYId*dungeonWidth+(headZId)*roomIdOffset);
                            break;
                        case Room::Direction::MINUS_Y:
                            rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].deeper=Room::MINUS_Y;
                            rooms_[headXId + (headYId-1)*dungeonWidth+(headZId)*roomIdOffset].exit=Room::PLUS_Y;
                            rooms_[headXId + (headYId-1)*dungeonWidth+(headZId)*roomIdOffset].branchId=0;
                            rooms_[headXId + (headYId-1)*dungeonWidth+(headZId)*roomIdOffset].depth=rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].depth+1;

                            corridors_[headXId + (headYId*2-1)*dungeonWidth+(headZId)*corridorIdOffset_].type=Corridor::OPEN;

                            headYId--;
                            mainBranch.emplace_back(headXId + headYId*dungeonWidth+(headZId)*roomIdOffset);
                            break;
                        case Room::Direction::PLUS_Y:
                            rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].deeper=Room::PLUS_Y;
                            rooms_[headXId + (headYId+1)*dungeonWidth+(headZId)*roomIdOffset].exit=Room::MINUS_Y;
                            rooms_[headXId + (headYId+1)*dungeonWidth+(headZId)*roomIdOffset].branchId=0;
                            rooms_[headXId + (headYId+1)*dungeonWidth+(headZId)*roomIdOffset].depth=rooms_[headXId + headYId*dungeonWidth+(headZId)*roomIdOffset].depth+1;

                            corridors_[headXId + (headYId*2+1)*dungeonWidth+(headZId)*corridorIdOffset_].type=Corridor::OPEN;

                            headYId++;
                            mainBranch.emplace_back(headXId + headYId*dungeonWidth+(headZId)*roomIdOffset);
                            break;
                    }
                }
            }
        }
    }

    //Now divide the main branch into zones with doors separating them

    int roomsPerZone=std::max(roomsPerZone_,static_cast<int>(mainBranch.size() / zoneTemplates_.size()));

    int zone=0;
    for (int i = 0; i < mainBranch.size();++i) {
        int r = mainBranch[i];
        rooms_[r].zone_=zone;
        if ((i+1)%roomsPerZone == 0 && zone+1<zoneTemplates_.size() && rooms_[r].deeper!=Room::MINUS_Z &&  rooms_[r].deeper!=Room::PLUS_Z) {
            ++zone;
            //Insert door
            switch (rooms_[r].deeper) {
                default:
                    break;
                case Room::Direction::MINUS_Y:
                    corridors_[rooms_[r].xId + (rooms_[r].yId*2-1)*dungeonWidth+(rooms_[r].zId)*corridorIdOffset_].type=Corridor::DOOR;
                    corridors_[rooms_[r].xId + (rooms_[r].yId*2-1)*dungeonWidth+(rooms_[r].zId)*corridorIdOffset_].id=zone;
                    break;
                case Room::Direction::PLUS_Y:
                    corridors_[rooms_[r].xId + (rooms_[r].yId*2+1)*dungeonWidth+(rooms_[r].zId)*corridorIdOffset_].type=Corridor::DOOR;
                    corridors_[rooms_[r].xId + (rooms_[r].yId*2+1)*dungeonWidth+(rooms_[r].zId)*corridorIdOffset_].id=zone;
                    break;
                case Room::Direction::MINUS_X:
                    corridors_[rooms_[r].xId-1 + (rooms_[r].yId*2)*dungeonWidth+(rooms_[r].zId)*corridorIdOffset_].type=Corridor::DOOR;
                    corridors_[rooms_[r].xId-1 + (rooms_[r].yId*2)*dungeonWidth+(rooms_[r].zId)*corridorIdOffset_].id=zone;
                    break;
                case Room::Direction::PLUS_X:
                    corridors_[rooms_[r].xId + (rooms_[r].yId*2)*dungeonWidth+(rooms_[r].zId)*corridorIdOffset_].type=Corridor::DOOR;
                    corridors_[rooms_[r].xId + (rooms_[r].yId*2)*dungeonWidth+(rooms_[r].zId)*corridorIdOffset_].id=zone;
                    break;
            }
        }
    }

    //Final step of room generation is to find and connect all unconnected rooms
    std::set<int> unconnectedRooms;

    for (int i =0; i < rooms_.size(); ++i) {
        if (rooms_[i].exit==Room::NOWHERE) {
            unconnectedRooms.insert(i);
        }
    }


    while (!unconnectedRooms.empty()) {
        int roomToRemove=-1;
        for (int rId : unconnectedRooms) {
            auto &r = rooms_[rId];
            //Check for neighbours in the xy direction which have an exit
            std::vector<Room::Direction> directions;

            if (r.xId > 0 && rooms_[r.xId-1 + r.yId*dungeonWidth+r.zId*roomIdOffset].exit!=Room::NOWHERE) {
                directions.emplace_back(Room::Direction::MINUS_X);
            }
            if (r.xId+1 < dungeonWidth && rooms_[r.xId+1 + r.yId*dungeonWidth+r.zId*roomIdOffset].exit!=Room::NOWHERE) {
                directions.emplace_back(Room::Direction::PLUS_X);
            }
            if (r.yId> 0 && rooms_[r.xId+ (r.yId-1)*dungeonWidth+r.zId*roomIdOffset].exit!=Room::NOWHERE) {
                directions.emplace_back(Room::Direction::MINUS_Y);
            }
            if (r.yId+1 < dungeonHeight && rooms_[r.xId + (r.yId+1)*dungeonWidth+r.zId*roomIdOffset].exit!=Room::NOWHERE) {
                directions.emplace_back(Room::Direction::PLUS_Y);
            }

            if (directions.empty()) {
                continue;
            }

            std::uniform_int_distribution<int> dirDist(0,directions.size()-1);
            switch (directions[dirDist(randomGenerator)]) {
                default:
                case Room::Direction::MINUS_X:
                    r.deeper=Room::MINUS_X;
                    r.exit=Room::MINUS_X;
                    r.depth=rooms_[r.xId-1+ r.yId*dungeonWidth+(r.zId)*roomIdOffset].depth+1;
                    r.zone_=rooms_[r.xId-1+ r.yId*dungeonWidth+(r.zId)*roomIdOffset].zone_;

                    corridors_[r.xId-1 + r.yId*dungeonWidth*2+(r.zId)*corridorIdOffset_].type=Corridor::OPEN;
                    break;
                case Room::Direction::PLUS_X:
                    r.deeper=Room::PLUS_X;
                    r.exit=Room::PLUS_X;
                    r.depth=rooms_[r.xId+1+ r.yId*dungeonWidth+(r.zId)*roomIdOffset].depth+1;
                    r.zone_=rooms_[r.xId+1+ r.yId*dungeonWidth+(r.zId)*roomIdOffset].zone_;

                    corridors_[r.xId + r.yId*dungeonWidth*2+(r.zId)*corridorIdOffset_].type=Corridor::OPEN;

                    break;
                case Room::Direction::MINUS_Y:

                    r.deeper=Room::MINUS_Y;
                    r.exit=Room::MINUS_Y;
                    r.depth=rooms_[r.xId+ (r.yId-1)*dungeonWidth+(r.zId)*roomIdOffset].depth+1;
                    r.zone_=rooms_[r.xId+ (r.yId-1)*dungeonWidth+(r.zId)*roomIdOffset].zone_;

                    corridors_[r.xId + (r.yId*2-1)*dungeonWidth+(r.zId)*corridorIdOffset_].type=Corridor::OPEN;
                    break;
                case Room::Direction::PLUS_Y:
                    r.deeper=Room::PLUS_Y;
                    r.exit=Room::PLUS_Y;
                    r.depth=rooms_[r.xId+ (r.yId+1)*dungeonWidth+(r.zId)*roomIdOffset].depth+1;
                    r.zone_=rooms_[r.xId+ (r.yId+1)*dungeonWidth+(r.zId)*roomIdOffset].zone_;

                    corridors_[r.xId + (r.yId*2+1)*dungeonWidth+(r.zId)*corridorIdOffset_].type=Corridor::OPEN;
                    break;
            }
            roomToRemove=rId;
            break;

        }
        if (roomToRemove==-1)
            throw std::runtime_error("World generation failed, could not remove unconnected rooms");
        else
            unconnectedRooms.erase(roomToRemove);
    }

    std::vector<std::vector<int>> zoneRooms(zoneTemplates_.size());
    maxUsedZone_=0;
    for (int i = 0; i < rooms_.size(); i++) {
        if (rooms_[i].zone_==-1)
            throw std::runtime_error("World generation failed, had room with no zone");
        zoneRooms[rooms_[i].zone_].emplace_back(i);
        maxUsedZone_=std::max(rooms_[i].zone_, maxUsedZone_);
    }

    for (int z = 0; z < zoneRooms.size(); z++) {
        if (zoneRooms[z].empty())
            continue;

        std::uniform_int_distribution<int> zoneSwitchDist(0, zoneRooms[z].size()-1);

        while (true) {
            int rId = zoneRooms[z][ zoneSwitchDist(randomGenerator)];
            if (rooms_[rId].ladder_==Room::DOWN || rooms_[rId].ladder_==Room::UP) {
                continue;
            }
            else {
                rooms_[rId].hasSwitch_ = true;
                rooms_[rId].switchDir=Room::HERE;
                switchedRooms_.emplace_back(rId);
                //Spread out knowledge about the switch
                std::deque<int> q{rId};
                while (!q.empty()) {
                    int thisRoom = q.front();
                    //check all neighbours, and add them to the queue
                    auto& r = rooms_[thisRoom];
                    if (r.xId > 0 && rooms_[r.xId-1 + r.yId*dungeonWidth+r.zId*roomIdOffset].zone_==z && rooms_[r.xId-1 + r.yId*dungeonWidth+r.zId*roomIdOffset].switchDir==Room::NOWHERE
                    && corridors_[r.xId-1+r.yId*2*dungeonWidth+r.zId*corridorIdOffset_].type!=Corridor::WALL
                    ) {
                        rooms_[r.xId-1 + r.yId*dungeonWidth+r.zId*roomIdOffset].switchDir=Room::PLUS_X;
                        q.emplace_back(r.xId-1 + r.yId*dungeonWidth+r.zId*roomIdOffset);
                    }
                    if (r.xId+1 < dungeonWidth && rooms_[r.xId+1 + r.yId*dungeonWidth+r.zId*roomIdOffset].zone_==z && rooms_[r.xId+1 + r.yId*dungeonWidth+r.zId*roomIdOffset].switchDir==Room::NOWHERE
                    && corridors_[r.xId+r.yId*2*dungeonWidth+r.zId*corridorIdOffset_].type!=Corridor::WALL
                    ) {
                        rooms_[r.xId+1 + r.yId*dungeonWidth+r.zId*roomIdOffset].switchDir=Room::MINUS_X;
                        q.emplace_back(r.xId+1 + r.yId*dungeonWidth+r.zId*roomIdOffset);
                    }
                    if (r.yId> 0 && rooms_[r.xId+ (r.yId-1)*dungeonWidth+r.zId*roomIdOffset].zone_==z && rooms_[r.xId+ (r.yId-1)*dungeonWidth+r.zId*roomIdOffset].switchDir==Room::NOWHERE
                    && corridors_[r.xId+(r.yId*2-1)*dungeonWidth+r.zId*corridorIdOffset_].type!=Corridor::WALL
                        ) {
                        rooms_[r.xId+ (r.yId-1)*dungeonWidth+r.zId*roomIdOffset].switchDir=Room::PLUS_Y;
                        q.emplace_back(r.xId+ (r.yId-1)*dungeonWidth+r.zId*roomIdOffset);
                    }
                    if (r.yId+1 < dungeonHeight && rooms_[r.xId+ (r.yId+1)*dungeonWidth+r.zId*roomIdOffset].zone_==z  && rooms_[r.xId + (r.yId+1)*dungeonWidth+r.zId*roomIdOffset].switchDir==Room::NOWHERE
                    && corridors_[r.xId+(r.yId*2+1)*dungeonWidth+r.zId*corridorIdOffset_].type!=Corridor::WALL
                        ) {
                        rooms_[r.xId+ (r.yId+1)*dungeonWidth+r.zId*roomIdOffset].switchDir=Room::MINUS_Y;
                        q.emplace_back(r.xId+ (r.yId+1)*dungeonWidth+r.zId*roomIdOffset);
                    }
                    if (r.zId> 0 && rooms_[r.xId+ (r.yId)*dungeonWidth+(r.zId-1)*roomIdOffset].zone_==z && rooms_[r.xId+ r.yId*dungeonWidth+(r.zId-1)*roomIdOffset].switchDir==Room::NOWHERE
                    && r.ladder_==Room::UP
                        ) {
                        rooms_[r.xId+ r.yId*dungeonWidth+(r.zId-1)*roomIdOffset].switchDir=Room::PLUS_Z;
                        q.emplace_back(r.xId+ r.yId*dungeonWidth+(r.zId-1)*roomIdOffset);
                        }
                    if (r.zId+1 < dungeonDepth&& rooms_[r.xId+ (r.yId)*dungeonWidth+(r.zId+1)*roomIdOffset].zone_==z  && rooms_[r.xId + (r.yId)*dungeonWidth+(r.zId+1)*roomIdOffset].switchDir==Room::NOWHERE
                    && r.ladder_==Room::DOWN
                        ) {
                        rooms_[r.xId+ (r.yId)*dungeonWidth+(r.zId+1)*roomIdOffset].switchDir=Room::MINUS_Z;
                        q.emplace_back(r.xId+ (r.yId)*dungeonWidth+(r.zId+1)*roomIdOffset);
                        }


                    q.pop_front();
                }
                break;
            }
            if (zoneRooms[z].size()==1)
                throw std::runtime_error("World generation failed, had only one room in zone "+std::to_string(z)+", and it has a ladder");
        }
    }

    //Sanity check, no orphaned ladders, no rooms without an exit, no room without a switch direction
    for (const auto &r : rooms_) {
        if (r.switchDir==Room::NOWHERE)
            throw std::runtime_error("World generation failed with room with no switch dir "+std::to_string(r.xId)+" "+std::to_string(r.yId)+" "+std::to_string(r.zId));
        if (r.exit==Room::NOWHERE)
            throw std::runtime_error("World generation failed with room with no exit dir");
        if (r.deeper==Room::NOWHERE)
            throw std::runtime_error("World generation failed with room with no deeper dir");
        if (r.ladder_==Room::DOWN && rooms_[r.xId+ (r.yId)*dungeonWidth+(r.zId+1)*roomIdOffset].ladder_!=Room::UP) {
            throw std::runtime_error("Orphaned down ladder");
        }
        if (r.ladder_==Room::UP &&  r.zId!=0 && rooms_[r.xId+ (r.yId)*dungeonWidth+(r.zId-1)*roomIdOffset].ladder_!=Room::DOWN) {
            throw std::runtime_error("Orphaned up ladder");
        }
    }

    //Technically, deeper champers may exists
    int maxDepth = rooms_[mainBranch.back()].depth;

    int rareTippingPoint = (maxDepth)/3;
    int epicTippingPoint = (2*maxDepth)/3;
    int legendaryTippingPoint = maxDepth;


    std::uniform_int_distribution<int> lootDist(-lootGenerationDepth_, lootGenerationDepth_);
    std::poisson_distribution<int> lootSpawnDist(lootChance_);

    //Now loop through every room and drop loot
    for (auto &room : rooms_) {
        //NO LOOT ON SWITCHES
        if (room.hasSwitch_)
            continue;
        int lootToSpawn = lootSpawnDist(randomGenerator);

        for (int i = 0; i < lootToSpawn; i++) {
            int lootValueWeight = lootDist(randomGenerator)+room.depth;
            int loot=0;
            if (lootValueWeight>legendaryTippingPoint) {
                std::uniform_int_distribution<int> legendaryDistribution(0, itemsByValue_[3].size()-1);
                loot = itemsByValue_[3][legendaryDistribution(randomGenerator)];
            }
            else if (lootValueWeight>epicTippingPoint) {
                std::uniform_int_distribution<int> epicDistribution(0, itemsByValue_[2].size()-1);
                loot = itemsByValue_[2][epicDistribution(randomGenerator)];
            }
            else if (lootValueWeight>rareTippingPoint) {
                std::uniform_int_distribution<int> rareDistribution(0, itemsByValue_[1].size()-1);
                loot = itemsByValue_[1][rareDistribution(randomGenerator)];
            }
            else {
                std::uniform_int_distribution<int> commonDistribution(0, itemsByValue_[0].size()-1);
                loot = itemsByValue_[0][commonDistribution(randomGenerator)];
            }

            if (loot==0)
                throw std::runtime_error("Loot generation generated item 0");

            room.itemIds.emplace_back(loot);
        }
    }

    //Place guaranteed light legendary loot in the end champer
    std::uniform_int_distribution<int> legendaryRewardDistribution(0, legendaryRewards_.size()-1);
    int finalReward = legendaryRewards_[legendaryRewardDistribution(randomGenerator)];
    rooms_[mainBranch.back()].itemIds.emplace_back(finalReward);

    //Now spawn enemies

    //Like loot value, enemy levels depend on the depth, but that depth is ever so slightly randomized
    std::uniform_int_distribution<int> enemyDist(-foeGenerationDepth_, foeGenerationDepth_);
    int highestEnemyLevel=0;
    for (const auto &foe : foeTemplates_) {
        highestEnemyLevel=std::max(highestEnemyLevel,foe.getLevel());
    }

    //The worst we are going to get is three of the highest level enemy
    int maxSpawnLevel = 3*highestEnemyLevel;
    std::uniform_int_distribution<int> enemySpawnDist(0,encounterSpawnDie );


    for (int zId = 0; zId<dungeonDepth; zId++)
        for (int xId = 0; xId<dungeonWidth; xId++) {
            for (int yId = 0; yId<dungeonHeight; yId++) {
                for (int offset = 0; offset<2; offset++)
                    {
                    if (xId+1==dungeonWidth && offset==0)
                        continue;
                    if (yId+1==dungeonHeight&& offset==1)
                        continue;

                    std::cout<<xId<<" "<<yId<<" "<<offset<<" "<<zId<<std::endl;
                    int corridorId = xId+(yId*2+offset)*dungeonWidth+zId*corridorIdOffset_;
                    if (corridors_[corridorId].type==Corridor::OPEN) {
                        //May spawn enemies
                        if (enemySpawnDist(randomGenerator)==0) {

                            //TEMP
                            corridors_[corridorId].foes.emplace_back(foeTemplates_[0]);
                        }
                    }

                }
            }
        }

    //FINALLY, place the adventurers in the starting room
    for (auto &adventurer: adventurers_) {
        adventurer.setFloor(0);
        adventurer.setRoomX(startRoomXId);
        adventurer.setRoomY(startRoomYId);
    }
    rooms_[startRoomXId+startRoomYId*dungeonWidth].discovered=true;
}

void Game::updateSidebarInfo() {
    for (int i = 0; i < adventurers_.size(); ++i) {
        adventurerSideBarData_[i].xNC_->setValue(adventurers_[i].getRoomX());
        adventurerSideBarData_[i].yNC_->setValue(adventurers_[i].getRoomY());
        adventurerSideBarData_[i].zNC_->setValue(adventurers_[i].getFloor());
        adventurerSideBarData_[i].healthNC_->setValue(adventurers_[i].getHealth());
        adventurerSideBarData_[i].staminaNC_->setValue(adventurers_[i].getStamina());
    }
}


void Game::setupGui(SDL_Renderer *renderer, int screenWidth, int screenHeight, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont) {

    mapCanvas_=std::make_shared<emptyControl>();

    std::shared_ptr<textureControl> floorTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Floor:",renderer,midFont));
    selectedFloorNC_=std::make_shared<NumberInputControl>(midNumberRenderer_,0,1-dungeonDepth,0,plusButtonTexture_,minusButtonTexture_);

    std::shared_ptr<textureControl> playersTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Players:",renderer,midFont));
    std::shared_ptr<textureControl> playersLTC = std::make_shared<textureControl>(playerMapTexture_);

    std::shared_ptr<textureControl> lockTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Locked door:",renderer,midFont));
    std::shared_ptr<textureControl> lockLTC = std::make_shared<textureControl>(lockMapTexture_);

    std::shared_ptr<textureControl> switchTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Pressure plates:",renderer,midFont));
    std::shared_ptr<textureControl> switchLTC = std::make_shared<textureControl>(switchMapTexture_);

    std::shared_ptr<textureControl> upTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Ladder up:",renderer,midFont));
    std::shared_ptr<textureControl> upLTC = std::make_shared<textureControl>(upMapTexture_);

    std::shared_ptr<textureControl> downTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Ladder down:",renderer,midFont));
    std::shared_ptr<textureControl> downLTC = std::make_shared<textureControl>(downMapTexture_);

    std::shared_ptr<textureControl> enemiesTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Enemies:",renderer,midFont));
    std::shared_ptr<textureControl> enemiesLTC = std::make_shared<textureControl>(enemiesMapTexture_);

    std::shared_ptr<tableControl>mapLegend = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,48),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,48),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,48),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,48),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,48),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,48),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,48)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128)},std::vector<std::shared_ptr<control>>{floorTC,selectedFloorNC_,playersTC,playersLTC,lockTC,lockLTC,switchTC,switchLTC,upTC,upLTC,downTC,downLTC,enemiesTC,enemiesLTC});


    std::shared_ptr<tableControl> mapTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth-128)},std::vector<std::shared_ptr<control>>{mapCanvas_,mapLegend},std::vector<tableControl::background>{tableControl::background(),tableControl::background(50,50,50)});



    std::shared_ptr<textureControl> commandsTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Click any of these buttons to have the selected player(s) do stuff",renderer,midFont));

    auto goNorthTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Go North",renderer,midFont));
    auto goEastTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Go East",renderer,midFont));
    auto goWestTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Go West",renderer,midFont));
    auto goSouthTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Go South",renderer,midFont));
    auto goUpTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Go Up",renderer,midFont));
    auto goDownTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Go Down",renderer,midFont));
    goNorthButton_ = std::make_shared<buttonControl>(goNorthTC);
    goEastButton_ = std::make_shared<buttonControl>(goEastTC);
    goWestButton_ = std::make_shared<buttonControl>(goWestTC);
    goSouthButton_ = std::make_shared<buttonControl>(goSouthTC);
    goUpButton_ = std::make_shared<buttonControl>(goUpTC);
    goDownButton_ = std::make_shared<buttonControl>(goDownTC);

    std::shared_ptr<stackControl> movementCommandStack=std::make_shared<stackControl>(stackControl::HORIZONTAL,std::vector<std::shared_ptr<control>>{
        goUpButton_,goDownButton_,goEastButton_,goWestButton_,goNorthButton_,goSouthButton_,
    },64);


    auto pickUpItemTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Pick up item",renderer,midFont));
    auto dropItemTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Drop an item",renderer,midFont));
    auto castSpellTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Cast a spell",renderer,midFont));

    pickUpItemButton_= std::make_shared<buttonControl>(pickUpItemTC);
    dropItemButton_ = std::make_shared<buttonControl>(dropItemTC);
    castSpellButton_ = std::make_shared<buttonControl>(castSpellTC);

    std::shared_ptr<stackControl> actionCommandStack=std::make_shared<stackControl>(stackControl::HORIZONTAL,std::vector<std::shared_ptr<control>>{
        pickUpItemButton_,dropItemButton_,castSpellButton_,
    },64);

    std::shared_ptr<tableControl> commandBar=std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth)},std::vector<std::shared_ptr<control>>{commandsTC,movementCommandStack,actionCommandStack});


    mainCanvas_=std::make_shared<emptyControl>();

    //Make inventory/stats view
    std::shared_ptr<textureControl> selectAdventurerTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Select an adventurer to view stats/inventory",renderer,smallFont));
    std::shared_ptr<textureControl> selectAdventurer2TC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Select an adventurer to view stats/inventory",renderer,smallFont));


    auto statHeader = std::make_shared<textureControl>(std::make_shared<TexWrap>("stat",renderer,midFont));
    auto valueHeader = std::make_shared<textureControl>(std::make_shared<TexWrap>("value",renderer,midFont));

    auto itemNameHeader = std::make_shared<textureControl>(std::make_shared<TexWrap>("Item",renderer,midFont));
    auto itemValueHeader = std::make_shared<textureControl>(std::make_shared<TexWrap>("Value",renderer,midFont));
    auto itemWeightHeader = std::make_shared<textureControl>(std::make_shared<TexWrap>("Weight",renderer,midFont));


        std::shared_ptr<control> nameTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Name",renderer,midFont));
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
        std::shared_ptr<control> mainWeaponTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("main-hand:",renderer,midFont));
        std::shared_ptr<control> offWeaponTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("off-hand:",renderer,midFont));

        std::shared_ptr<control> strMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Strength\nRequired to beat some traps, boost carry capacity, effects physical damage of most weapons",renderer,midFont,512));
        std::shared_ptr<control> dexMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Dexterity\nRequired to beat some traps, also effects combat speed, effects aim of most weapons",renderer,midFont,512));
        std::shared_ptr<control> intMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Intelligence\nRequired to beat some traps, effects magic damage of most wands",renderer,midFont,512));
        std::shared_ptr<control> speedMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("speed\nAttack more frequently, scales with dexterity",renderer,midFont,512));
        std::shared_ptr<control> carryMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Carry capacity\nEffects how much loot you can carry, scales with strength",renderer,midFont,512));
        std::shared_ptr<control> dodgeMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("dodge\nGives chance to avoid attacks, scales with dexterity",renderer,midFont,512));
        std::shared_ptr<control> attackMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Physical attack\nDeal physical type damage\nScales based on equipment",renderer,midFont,512));
        std::shared_ptr<control> blockMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Block\nGives chance to block physical attacks\nScales based on equipment, uses whichever hand has highest value",renderer,midFont,512));
        std::shared_ptr<control> aimMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Aim\nGives chance to hit enemies\nScales based on equipment",renderer,midFont,512));
        std::shared_ptr<control> magicMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Magic\nDeal magical type damage, and allows the use of spells\nScales based on equipment",renderer,midFont,512));
        std::shared_ptr<control> magicDefenceMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Magic defence\nGives chance to block magical attacks\nScales based on equipment, uses whichever hand has highest value",renderer,midFont,512));
        std::shared_ptr<control> mainWeaponMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Weapon in main hand, click info box to view stat scaling",renderer,midFont,512));
        std::shared_ptr<control> offWeaponMTC=std::make_shared<textureControl> (std::make_shared<TexWrap>("Weapon in off hand, click info box to view stat scaling",renderer,midFont,512));

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
        std::shared_ptr<control> mainWeaponMO=std::make_shared<mouseOverControl> (mainWeaponTC,mainWeaponMTC);
        std::shared_ptr<control> offWeaponMO=std::make_shared<mouseOverControl> (offWeaponTC,offWeaponMTC);


    selectedNameTexture_= std::make_shared<textureControl>(std::make_shared<TexWrap>("null",renderer,midFont));
    strViewControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
    dexViewControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
    intViewControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
    speedViewControl_= std::make_shared<numberControl>(midNumberRenderer_,1);
    carryViewControl_= std::make_shared<numberControl>(midNumberRenderer_,1);
    dodgeViewControl_= std::make_shared<numberControl>(midNumberRenderer_,1);
    aimViewControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
    attackViewControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
    blockViewControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
    magicViewControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
    magicDefenceViewControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);
    mainHandWeaponSelection_= std::make_shared<textureControl>(std::make_shared<TexWrap>("null",renderer,midFont));
    offHandWeaponSelection_= std::make_shared<textureControl>(std::make_shared<TexWrap>("null",renderer,midFont));
    selectedCostControl_ = std::make_shared<numberControl>(midNumberRenderer_,1);

    std::shared_ptr<control> mainButtonTC = std::make_shared<textureControl>(infoButtonTexture_);
    std::shared_ptr<control> offButtonTC = std::make_shared<textureControl>(infoButtonTexture_);
    mainHandInfoButton_ = std::make_shared<buttonControl>(mainButtonTC);
    offHandInfoButton_ = std::make_shared<buttonControl>(offButtonTC);
    std::shared_ptr<control> mainPair = std::make_shared<tableControl>(128,64,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, 64)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, 64),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, 64)},std::vector<std::shared_ptr<control>>{mainHandWeaponSelection_,mainHandInfoButton_ });
    std::shared_ptr<control> offPair = std::make_shared<tableControl>(128,64,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, 64)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, 64),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, 64)},std::vector<std::shared_ptr<control>>{offHandWeaponSelection_,offHandInfoButton_ });

    std::shared_ptr<control> statTable = std::make_shared<contentTable>(std::vector<std::shared_ptr<control>>{statHeader,valueHeader},std::vector<std::vector<std::shared_ptr<control>>>{
        {nameTC,selectedNameTexture_},
        {strMO,strViewControl_},
        {dexMO,dexViewControl_},
        {intMO,intViewControl_},
        {speedMO,speedViewControl_},
        {carryMO,carryViewControl_},
        {dodgeMO,dodgeViewControl_},
        {aimMO,aimViewControl_},
        {attackMO,attackViewControl_},
        {blockMO,blockViewControl_},
        {magicMO,magicViewControl_},
        {magicDefenceMO,magicDefenceViewControl_},
        {mainWeaponMO,mainPair},
        {offWeaponMO,offPair},
    },64,100,100,100,200,200,200,100,100,200,true);

    auto setMainHandTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Equip/unequip in main-hand",renderer,midFont));
    auto setOffHandTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Equip/unequip in off-hand",renderer,midFont));
    setMainHandButton_ = std::make_shared<buttonControl>(setMainHandTC);
    setOffHandButton_ = std::make_shared<buttonControl>(setOffHandTC);


    inventoryTable_ = std::make_shared<contentTable>(std::vector<std::shared_ptr<control>>{itemNameHeader,itemValueHeader,itemWeightHeader},std::vector<std::vector<std::shared_ptr<control>>>{},64);

    std::shared_ptr<tableControl> upDownTable1 = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth)},std::vector<std::shared_ptr<control>>{inventoryTable_,setMainHandButton_,setOffHandButton_});

    inventorySlideControl_=std::make_shared<SlideControl>(std::vector<std::shared_ptr<control>>{selectAdventurerTC,upDownTable1});
    statSlideControl_=std::make_shared<SlideControl>(std::vector<std::shared_ptr<control>>{selectAdventurer2TC,statTable});
    mainWindow_=std::make_shared<SlideControl>(std::vector<std::shared_ptr<control>>{mainCanvas_,mapTable,inventorySlideControl_,statSlideControl_});

    mapButton_=std::make_shared<buttonControl>(std::make_shared<textureControl>(std::make_shared<TexWrap>("View Map",renderer,midFont)));
    mainButton_=std::make_shared<buttonControl>(std::make_shared<textureControl>(std::make_shared<TexWrap>("Main window",renderer,midFont)));
    inventoryButton_=std::make_shared<buttonControl>(std::make_shared<textureControl>(std::make_shared<TexWrap>("View inventory",renderer,midFont)));
    statsButton_=std::make_shared<buttonControl>(std::make_shared<textureControl>(std::make_shared<TexWrap>("View stats",renderer,midFont)));
    quitEarlyButton_=std::make_shared<buttonControl>(std::make_shared<textureControl>(std::make_shared<TexWrap>("Quit early",renderer,midFont)));
    reallyQuitButton_=std::make_shared<buttonControl>(std::make_shared<textureControl>(std::make_shared<TexWrap>("Quit anyway",renderer,midFont)));

    std::shared_ptr<stackControl> buttonStack=std::make_shared<stackControl>(stackControl::HORIZONTAL,std::vector<std::shared_ptr<control>>{mainButton_,mapButton_,inventoryButton_,statsButton_,quitEarlyButton_},64);

    //Player sidebar

    std::shared_ptr<textureControl> playerNameTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Player",renderer,midFont));
    std::shared_ptr<textureControl> playerHealthTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Health",renderer,midFont));
    std::shared_ptr<textureControl> playerStaminaTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Stamina",renderer,midFont));
    std::shared_ptr<textureControl> playerRoomTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Room",renderer,midFont));

    std::vector<std::vector<std::shared_ptr<control>>> playerRows;

    for (const auto &adventurer : adventurers_) {
        auto sbd = AdventurerSideBarData(adventurer.getName(),adventurer.getHealth(),adventurer.getStamina(),adventurer.getRoomX(),adventurer.getRoomY(),adventurer.getFloor(),renderer,midFont,midNumberRenderer_);
        auto loc = std::make_shared<stackControl>(stackControl::HORIZONTAL,std::vector<std::shared_ptr<control>>{sbd.zNC_,sbd.xNC_,sbd.yNC_},8);
        playerRows.push_back(std::vector<std::shared_ptr<control>>{
            sbd.nameTC_,
            sbd.healthNC_,
            sbd.staminaNC_,
            loc
        });
        adventurerSideBarData_.push_back(sbd);
    }
    playerTable_ = std::make_shared<contentTable>(std::vector<std::shared_ptr<control>>{playerNameTC,playerHealthTC,playerStaminaTC,playerRoomTC},playerRows,64);
    auto someMoreInfo = std::make_shared<textureControl>(std::make_shared<TexWrap>("CTRL+click to select multiple",renderer,midFont));

    std::shared_ptr<tableControl> infoTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight/2),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenHeight/2)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth)},std::vector<std::shared_ptr<control>>{playerTable_,someMoreInfo});

    std::shared_ptr<tableControl> leftRightTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth-512),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth)},std::vector<std::shared_ptr<control>>{mainWindow_,infoTable },std::vector<tableControl::background>{tableControl::background(),tableControl::background(100,100,150)});
    std::shared_ptr<tableControl> upDownTable = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight-256-128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,256)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth)},std::vector<std::shared_ptr<control>>{buttonStack,leftRightTable,commandBar},std::vector<tableControl::background>{tableControl::background(150,150,150),tableControl::background(),tableControl::background(150,150,150)});

    //Game over control


    std::shared_ptr<textureControl> sumUpStatusTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Raid over, score summary:",renderer,midFont));
    auto itemPlayerHeader = std::make_shared<textureControl>(std::make_shared<TexWrap>("Adventurer",renderer,midFont));
    auto itemNameHeader1 = std::make_shared<textureControl>(std::make_shared<TexWrap>("Item",renderer,midFont));
    auto itemValueHeaderPts = std::make_shared<textureControl>(std::make_shared<TexWrap>("points",renderer,midFont));

    std::shared_ptr<textureControl> quitGameTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Quit game",renderer,midFont));
    quitGameButton_ = std::make_shared<buttonControl>(quitGameTC);
    scoreTable_ = std::make_shared<contentTable>(std::vector<std::shared_ptr<control>>{itemPlayerHeader,itemNameHeader1,itemValueHeaderPts},std::vector<std::vector<std::shared_ptr<control>>>{},64,100,100,100,200,200,200,100,100,200,true);

    std::shared_ptr<tableControl> upDownTable2 = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenHeight-256-128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,256)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::EXPAND,screenWidth)},std::vector<std::shared_ptr<control>>{sumUpStatusTC,scoreTable_,quitGameButton_});

    gameOverSlide_ = std::make_shared<SlideControl>(std::vector<std::shared_ptr<control>>{upDownTable,upDownTable2});

    gui_=std::make_unique<GUIManager>(gameOverSlide_);
}


void Game::render(SDL_Renderer *renderer, int screenWidth, int screenHeight, const InputData &userInputs, unsigned int millis, unsigned int pmillis) const {
    if (gameStatus_!=GAME_OVER)
    {

        if (selectedView_ ==MAIN_WINDOW) {
            SDL_Rect canvasRect {mainCanvas_->getX0(),mainCanvas_->getY0(),mainCanvas_->getWidth(),mainCanvas_->getHeight()};
            typingText->render(mainCanvas_->getX0(),mainCanvas_->getY0(),renderer,canvasRect);

            if (mainViewIndex_<mainViewText_.size()) {
                pressEnterToFastForward_->render(mainCanvas_->getX0()+mainCanvas_->getWidth()-pressEnterToFastForward_->getWidth(),mainCanvas_->getY0()+mainCanvas_->getHeight()-pressEnterToFastForward_->getHeight(),255,255,255,255*(0.5+0.5*sin(millis*0.01)),renderer,canvasRect);
            }
        }
        else if (selectedView_==MAP_WINDOW) {
            //Render the map on the canvas
            SDL_Rect background {mapCanvas_->getX0(),mapCanvas_->getY0(),mapCanvas_->getWidth(),mapCanvas_->getHeight()};
            SDL_SetRenderDrawColor(renderer,128,128,128,255);
            const int corridorLength = 32;
            const int corridorWidth = 32;


            SDL_RenderFillRect(renderer,&background);


            int roomHeight = (background.h-corridorLength*(dungeonHeight-1))/dungeonHeight;
            int roomWidth = (background.w-corridorLength*(dungeonWidth-1))/dungeonWidth;
            int selectedFloor = -selectedFloorNC_->getValue();

            //Draw all rooms
            SDL_Rect roomRect {0,0,roomWidth,roomHeight};
            SDL_Rect corridorXRect {0,0,corridorLength,corridorWidth};
            SDL_Rect corridorYRect {0,0,corridorWidth,corridorLength};
            for (int xId = 0; xId < dungeonWidth; ++xId) {
                for (int yId = 0; yId < dungeonHeight; ++yId) {
                    int roomId = xId+dungeonWidth*yId+selectedFloor*roomIdOffset;
                    roomRect.x=xId*(roomWidth+corridorLength)+background.x;
                    roomRect.y=yId*(roomHeight+corridorLength)+background.y;

                    const auto &z = zoneTemplates_[rooms_[roomId].zone_];
                    if (rooms_[roomId].discovered) {

                        SDL_SetRenderDrawColor(renderer,z.R_/4+191,z.G_/4+191,z.B_/4+191,255);
                        SDL_RenderFillRect(renderer,&roomRect);
                        SDL_SetRenderDrawColor(renderer,0,0,0,255);
                        SDL_RenderDrawRect(renderer,&roomRect);

                        int xTexture = roomRect.x+roomRect.w/2;
                        int yTexture = roomRect.y+roomRect.h/2;
                        if (rooms_[roomId].ladder_==Room::UP) {
                            upMapTexture_->render(xTexture,yTexture,0,0,0,renderer);
                            xTexture+=upMapTexture_->getWidth();
                        }
                        else if (rooms_[roomId].ladder_==Room::DOWN) {
                            downMapTexture_->render(xTexture,yTexture,0,0,0,renderer);
                            xTexture+=downMapTexture_->getWidth();
                        }
                        if (rooms_[roomId].hasSwitch_) {
                            switchMapTexture_->render(xTexture,yTexture,0,0,0,renderer);
                            xTexture+=switchMapTexture_->getWidth();
                        }
                        xTexture=roomRect.x;
                        yTexture=roomRect.y+roomRect.h-smallNumberRenderer_.getHeight();
                        xTexture+=smallNumberRenderer_.render(selectedFloor,xTexture,yTexture,0,0,0,128,renderer,roomRect)+4;
                        xTexture+=smallNumberRenderer_.render(xId,xTexture,yTexture,0,0,0,128,renderer,roomRect)+4;
                        smallNumberRenderer_.render(yId,xTexture,yTexture,0,0,0,128,renderer,roomRect);
                    }
                            //check if this room, or the next one over is discovered
                    if (xId+1<dungeonWidth)
                        if (rooms_[roomId].discovered || rooms_[roomId+1].discovered)
                        {
                            int corridorId = xId+dungeonWidth*yId*2+selectedFloor*corridorIdOffset_;
                            if (corridors_[corridorId].type!=Corridor::WALL) {
                                corridorXRect.x=roomRect.x+roomWidth;
                                corridorXRect.y=roomRect.y+roomHeight/2-corridorWidth/2;
                                if (corridors_[corridorId].type==Corridor::DOOR) {
                                    const auto &zC = zoneTemplates_[corridors_[corridorId].id];
                                    SDL_SetRenderDrawColor(renderer,zC.R_,zC.G_,zC.B_,255);
                                }
                                else {
                                    SDL_SetRenderDrawColor(renderer,z.R_/4+191,z.G_/4+191,z.B_/4+191,255);

                                }

                                SDL_RenderFillRect(renderer,&corridorXRect);
                                SDL_SetRenderDrawColor(renderer,0,0,0,255);
                                SDL_RenderDrawRect(renderer,&corridorXRect);

                                int xTexture = corridorXRect.x+corridorXRect.w/2;
                                int yTexture = corridorXRect.y;
                                if (corridors_[corridorId].type==Corridor::DOOR) {
                                    lockMapTexture_->render(xTexture,yTexture,0,0,0,renderer);
                                    xTexture+=lockMapTexture_->getWidth();
                                }
                                if (corridors_[corridorId].type==Corridor::OPEN && !corridors_[corridorId].foes.empty()) {
                                    enemiesMapTexture_->render(xTexture,yTexture,0,0,0,renderer);
                                    xTexture+=enemiesMapTexture_->getWidth();
                                }

                            }
                        }
                    if (yId+1<dungeonHeight)
                        if (rooms_[roomId].discovered || (rooms_[roomId+dungeonWidth].discovered)) {
                        int corridorId = xId+dungeonWidth*(yId*2+1)+selectedFloor*corridorIdOffset_;
                        if (corridors_[corridorId].type!=Corridor::WALL) {
                            corridorYRect.x=roomRect.x+roomWidth/2-corridorWidth/2;
                            corridorYRect.y=roomRect.y+roomHeight;
                            if (corridors_[corridorId].type==Corridor::DOOR) {
                                const auto &zC = zoneTemplates_[corridors_[corridorId].id];
                                SDL_SetRenderDrawColor(renderer,zC.R_,zC.G_,zC.B_,255);
                            }
                            else
                                SDL_SetRenderDrawColor(renderer,z.R_/4+191,z.G_/4+191,z.B_/4+191,255);

                            SDL_RenderFillRect(renderer,&corridorYRect);
                            SDL_SetRenderDrawColor(renderer,0,0,0,255);
                            SDL_RenderDrawRect(renderer,&corridorYRect);

                            int xTexture = corridorYRect.x+corridorYRect.w/2;
                            int yTexture = corridorYRect.y;
                            if (corridors_[corridorId].type==Corridor::DOOR) {
                                if (!switchesActive_[corridors_[corridorId].id])
                                    lockMapTexture_->render(xTexture,yTexture,0,0,0,renderer);
                                xTexture+=lockMapTexture_->getWidth();
                            }
                            if (corridors_[corridorId].type==Corridor::OPEN && !corridors_[corridorId].foes.empty()) {
                                enemiesMapTexture_->render(xTexture,yTexture,0,0,0,renderer);
                                xTexture+=enemiesMapTexture_->getWidth();
                            }
                        }
                    }
                }
            }

            //Loop through and display the adventurers, yes this draws some adventurers on top of each other, I DON'T CARE
            for (const auto &adventurer : adventurers_) {
                if (adventurer.getFloor()==selectedFloor) {
                    int x=adventurer.getRoomX()*(roomWidth+corridorLength)+background.x;
                    int y=adventurer.getRoomY()*(roomHeight+corridorLength)+background.y;
                    playerMapTexture_->render(x,y,0,0,0,renderer);
                }
            }
        }
    }


    gui_->render(renderer,screenWidth,screenHeight);

}

std::optional<std::pair<Scene::SceneInfo, SceneTransitionData> > Game::update(SDL_Renderer *renderer, int screenWidth, int screenHeight, const InputData &userInputs, unsigned int millis, unsigned int dmillis, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont) {
    //Check loot dialogue
    if (activePickupDialog_!=nullptr) {

        if (activePickupDialog_->pickupButton_->isClicked()) {
            //I do a lot of un-necessary checks
            int toPickUpID=activePickupDialog_->selectItemMenu_->getSelection();
            if (activePickupDialog_->targetPlayerId_<adventurers_.size()) {
                auto &adventurer = adventurers_[activePickupDialog_->targetPlayerId_];
                if (adventurer.getFloor()>=0) {
                    int roomId =adventurer.getRoomX()+adventurer.getRoomY()*dungeonWidth+adventurer.getFloor()*roomIdOffset;
                    if (rooms_[roomId].itemIds.size()>toPickUpID) {
                        //Get total carried weight
                        int total = 0;
                        for (int i : adventurer.getLoot()) {
                            total+=itemList_[i]->getWeightPenalty();
                        }

                        int loot = rooms_[roomId].itemIds[toPickUpID];
                        total+=itemList_[loot]->getWeightPenalty();

                        if (total>adventurer.getCarryCap()) {
                            std::shared_ptr<control> noLootControl = std::make_shared<textureControl>(std::make_shared<TexWrap>(adventurer.getName()+" can't carry that",renderer,midFont,512));
                            gui_->addDialogue("Loot",noLootControl,screenHeight,renderer,smallFont);
                        }
                        else {
                            rooms_[roomId].itemIds.erase(rooms_[roomId].itemIds.begin()+toPickUpID);
                            adventurer.addLoot(loot);
                            pickupSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                            generateDescriptionText(adventurer.getName()+" picks up "+itemList_[loot]->getName()+"\n\n",selectedPlayerIndex_, renderer, smallFont);
                            updateSwitches();
                            updateInventoryDisplay(renderer,midFont);
                        }
                    }
                }
            }
            activePickupDialog_->dialogue_->close();
        }

        if (activePickupDialog_->dialogue_->getShouldClose()) {
            activePickupDialog_=nullptr;
        }
    }
    if (activeDropDialog_!=nullptr) {

        if (activeDropDialog_->pickupButton_->isClicked()) {
            //I do a lot of un-necessary checks
            int toDropId=activeDropDialog_->selectItemMenu_->getSelection();
            if (activeDropDialog_->targetPlayerId_<adventurers_.size()) {
                auto &adventurer = adventurers_[activeDropDialog_->targetPlayerId_];
                if (adventurer.getFloor()>=0) {
                    int roomId =adventurer.getRoomX()+adventurer.getRoomY()*dungeonWidth+adventurer.getFloor()*roomIdOffset;
                    if (adventurer.getLoot().size()>toDropId) {
                        int loot = adventurer.getLoot()[toDropId];
                        adventurer.drop(toDropId);
                        rooms_[roomId].itemIds.emplace_back(loot);
                        bonkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                        generateDescriptionText(adventurer.getName()+" drops "+itemList_[loot]->getName()+"\n\n",selectedPlayerIndex_, renderer, smallFont);
                        updateSwitches();
                        updateInventoryDisplay(renderer,midFont);
                    }
                }
            }
            activeDropDialog_->dialogue_->close();
        }

        if (activeDropDialog_->dialogue_->getShouldClose()) {
            activeDropDialog_=nullptr;
        }
    }
    gui_->update(userInputs,screenWidth,screenHeight);


    if (gameStatus_==GAME_OVER) {
        if (quitGameButton_->isClicked()) {
            return std::make_pair(QUIT_TO_MENU,SceneTransitionData(""));
        }
        return std::nullopt;
    }

    if (playerTable_->getMainSelectedRow()!=selectedPlayerIndex_) {
        selectedPlayerIndex_=playerTable_->getMainSelectedRow();

        closeAllDialogues();
        ensureValidSelection();
        updateInventoryDisplay(renderer,midFont);
        if (playerTable_->getMainSelectedRow()<adventurers_.size()) {
            generateDescriptionText("",selectedPlayerIndex_, renderer, smallFont);
        }
    }
    if (setMainHandButton_->isClicked()) {
        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        if (selectedPlayerIndex_<adventurers_.size()) {
            adventurers_[selectedPlayerIndex_].toggleMainHand(inventoryTable_->getMainSelectedRow());
        }
        updateInventoryDisplay(renderer,midFont);
    }
    if (setOffHandButton_->isClicked()) {
        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        if (selectedPlayerIndex_<adventurers_.size()) {
            adventurers_[selectedPlayerIndex_].toggleOffHand(inventoryTable_->getMainSelectedRow());
        }
        updateInventoryDisplay(renderer,midFont);
    }

    if (selectedView_ ==MAIN_WINDOW ) {
        if (userInputs.enterPressed && !userInputs.prevEnterPressed) {
            clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            nextCharTime_=charTimer;
            mainViewIndex_=mainViewText_.size();
            typingText->reset(mainViewText_.substr(0,mainViewIndex_),renderer,smallFont,prevWidth);
        }
        //Type text
        if (prevWidth!=mainCanvas_->getWidth()) {
            prevWidth=mainCanvas_->getWidth();
            typingText->reset(mainViewText_.substr(0,mainViewIndex_),renderer,smallFont,prevWidth);
        }
        if (mainViewIndex_<mainViewText_.size()) {
            if (nextCharTime_<dmillis) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
                nextCharTime_=charTimer;
                mainViewIndex_=std::min(mainViewIndex_+1,mainViewText_.size());
                typingText->reset(mainViewText_.substr(0,mainViewIndex_),renderer,smallFont,prevWidth);
            }
            else
                nextCharTime_-=dmillis;
        }
    }

    if (mainButton_->isClicked()) {
        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        selectedView_=MAIN_WINDOW;
        mainWindow_->setActiveSlide(0);
    }
    if (mapButton_->isClicked()) {
        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        selectedView_=MAP_WINDOW;
        mainWindow_->setActiveSlide(1);
    }
    if (inventoryButton_->isClicked()) {
        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        selectedView_=INVENTORY;
        mainWindow_->setActiveSlide(2);
    }
    if (statsButton_->isClicked()) {
        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        selectedView_=INVENTORY;
        mainWindow_->setActiveSlide(3);
    }
    if (selectedFloorNC_->isClicked()) {
        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
    }
    if (quitEarlyButton_->isClicked()) {
        clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        auto TC = std::make_shared<textureControl>(std::make_shared<TexWrap>("If you quit now, all characters still in the dungeon will die!",renderer,midFont,512));
        auto table = std::make_shared<tableControl>(screenWidth,screenHeight,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,128),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,256)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,screenWidth)},std::vector<std::shared_ptr<control>>{TC,reallyQuitButton_});
        gui_->addDialogue("Really quit",table ,screenHeight,renderer,smallFont);
    }
    if (reallyQuitButton_->isClicked()) {
        endGame(renderer,midFont);
        closeAllDialogues();
    }

    if (selectedView_==INVENTORY && selectedPlayerIndex_<adventurers_.size()) {
        //Check if any info-buttons have been pressed

        for (int i = 0; i < inventoryInfoButtons_.size(); ++i) {
            if (inventoryInfoButtons_[i]->isClicked()) {
                clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);

                addWeaponStatPopup(adventurers_[selectedPlayerIndex_].getLoot()[i],adventurers_[selectedPlayerIndex_],renderer,screenWidth,screenHeight,smallFont,midFont,largeFont);
            }
        }
        if (mainHandInfoButton_->isClicked()) {
            clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            addWeaponStatPopup(adventurers_[selectedPlayerIndex_].getMainHandWeaponIndex(),adventurers_[selectedPlayerIndex_],renderer,screenWidth,screenHeight,smallFont,midFont,largeFont);
        }
        if (offHandInfoButton_->isClicked()) {
            clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            addWeaponStatPopup(adventurers_[selectedPlayerIndex_].getOffHandWeaponIndex(),adventurers_[selectedPlayerIndex_],renderer,screenWidth,screenHeight,smallFont,midFont,largeFont);
        }
    }

    if (gameStatus_==EXPLORATION) {
        //Go around the map
        if (goUpButton_->isClicked()) {
            bool valid = false;
            std::string text;
            ensureValidSelection();
            for (size_t i : playerTable_->getSelectedRows()) {
                auto &adventurer = adventurers_[i];
                //Check if going up is legal
                int roomId =adventurer.getRoomX()+adventurer.getRoomY()*dungeonWidth+adventurer.getFloor()*roomIdOffset;
                if (rooms_[roomId].ladder_==Room::UP) {
                    //OK
                    text+=adventurer.getName()+ " climbs up"+(adventurer.getFloor()==0?" and leaves the dungeon":"")+"\n";
                    adventurer.setFloor(adventurer.getFloor()-1);
                    valid=true;
                }
                else {
                    //Not ok
                    text+=adventurer.getName()+ " jumps on the spot\n";
                }
            }
            updateSinceLocationChanged(renderer,smallFont,midFont);
            generateDescriptionText(text,selectedPlayerIndex_,renderer,smallFont);
            if (valid) {
                walkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
            else
                bonkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        }
        if (goDownButton_->isClicked()) {
            bool valid = false;
            std::string text;
            ensureValidSelection();
            for (size_t i : playerTable_->getSelectedRows()) {
                auto &adventurer = adventurers_[i];
                //Check if going up is legal
                int roomId =adventurer.getRoomX()+adventurer.getRoomY()*dungeonWidth+adventurer.getFloor()*roomIdOffset;
                if (rooms_[roomId].ladder_==Room::DOWN) {
                    //OK
                    text+=adventurer.getName()+ " climbs down\n";
                    adventurer.setFloor(adventurer.getFloor()+1);
                    valid=true;
                }
                else {
                    //Not ok
                    text+=adventurer.getName()+ " bangs their head against the floor\n";
                }
            }
            updateSinceLocationChanged(renderer,smallFont,midFont);
            generateDescriptionText(text,selectedPlayerIndex_,renderer,smallFont);
            if (valid) {
                walkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
            else
                bonkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        }
        if (goEastButton_->isClicked()) {
            bool valid = false;
            std::string text;
            ensureValidSelection();
            for (size_t i : playerTable_->getSelectedRows()) {
                auto &adventurer = adventurers_[i];
                //Check if going up is legal
                if (adventurer.getRoomX()+1>=dungeonWidth) {
                    text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the wall, it doesn't budge\n";
                }
                else {
                    int corridorId =adventurer.getRoomX()+adventurer.getRoomY()*2*dungeonWidth+adventurer.getFloor()*corridorIdOffset_;
                    if (corridors_[corridorId].type==Corridor::OPEN) {
                        //OK
                        text+=adventurer.getName()+ " walks east\n";
                        adventurer.setRoomX(adventurer.getRoomX()+1);
                        valid=true;
                    }
                    else if (corridors_[corridorId].type==Corridor::DOOR) {

                        if (!switchesActive_[corridors_[corridorId].id])
                            text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the door, it doesn't budge\n";
                        else
                        {
                            text+=adventurer.getName()+ " walks east\n";
                            adventurer.setRoomX(adventurer.getRoomX()+1);
                            valid=true;
                        }
                    }
                    //TODO start combat
                    else {
                        //Not ok
                        text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the wall, it doesn't budge\n";
                    }
                }
            }
            updateSinceLocationChanged(renderer,smallFont,midFont);
            generateDescriptionText(text,selectedPlayerIndex_,renderer,smallFont);
            if (valid) {
                walkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
            else
                bonkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        }
        if (goWestButton_->isClicked()) {
            bool valid = false;
            std::string text;
            ensureValidSelection();
            for (size_t i : playerTable_->getSelectedRows()) {
                auto &adventurer = adventurers_[i];
                //Check if going up is legal
                if (adventurer.getRoomX()<=0) {
                    text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the wall, it doesn't budge\n";
                }
                else {
                    int corridorId =(adventurer.getRoomX()-1)+adventurer.getRoomY()*2*dungeonWidth+adventurer.getFloor()*corridorIdOffset_;
                    if (corridors_[corridorId].type==Corridor::OPEN) {
                        //OK
                        text+=adventurer.getName()+ " walks west\n";
                        valid=true;
                        adventurer.setRoomX(adventurer.getRoomX()-1);
                    }
                    else if (corridors_[corridorId].type==Corridor::DOOR) {
                        if (!switchesActive_[corridors_[corridorId].id])
                            text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the door, it doesn't budge\n";
                        else
                        {
                            text+=adventurer.getName()+ " walks west\n";
                            adventurer.setRoomX(adventurer.getRoomX()-1);
                            valid=true;
                        }
                    }
                    //TODO start combat
                    else {
                        //Not ok
                        text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the wall, it doesn't budge\n";
                    }
                }
            }
            updateSinceLocationChanged(renderer,smallFont,midFont);
            generateDescriptionText(text,selectedPlayerIndex_,renderer,smallFont);
            if (valid) {
                walkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
            else
                bonkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        }
        if (goNorthButton_->isClicked()) {
            bool valid = false;
            std::string text;
            ensureValidSelection();
            for (size_t i : playerTable_->getSelectedRows()) {
                auto &adventurer = adventurers_[i];
                //Check if going up is legal
                if (adventurer.getRoomY()<=0) {
                    text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the wall, it doesn't budge\n";
                }
                else {
                    int corridorId =(adventurer.getRoomX())+(adventurer.getRoomY()*2-1)*dungeonWidth+adventurer.getFloor()*corridorIdOffset_;
                    if (corridors_[corridorId].type==Corridor::OPEN) {
                        //OK
                        text+=adventurer.getName()+ " walks north\n";
                        valid = true;
                        adventurer.setRoomY(adventurer.getRoomY()-1);
                    }
                    else if (corridors_[corridorId].type==Corridor::DOOR) {
                        if (!switchesActive_[corridors_[corridorId].id])
                            text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the door, it doesn't budge\n";
                        else
                        {
                            text+=adventurer.getName()+ " walks north\n";
                            adventurer.setRoomY(adventurer.getRoomY()-1);
                            valid=true;
                        }
                    }
                    //TODO start combat
                    else {
                        //Not ok
                        text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the wall, it doesn't budge\n";
                    }
                }
            }
            updateSinceLocationChanged(renderer,smallFont,midFont);
            generateDescriptionText(text,selectedPlayerIndex_,renderer,smallFont);
            if (valid) {
                walkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
            else
                bonkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        }
        if (goSouthButton_->isClicked()) {
            bool valid = false;
            std::string text;
            ensureValidSelection();
            for (size_t i : playerTable_->getSelectedRows()) {
                auto &adventurer = adventurers_[i];
                //Check if going up is legal
                if (adventurer.getRoomY()+1>=dungeonHeight) {
                    text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the wall, it doesn't budge\n";
                }
                else {
                    int corridorId =(adventurer.getRoomX())+(adventurer.getRoomY()*2+1)*dungeonWidth+adventurer.getFloor()*corridorIdOffset_;
                    if (corridors_[corridorId].type==Corridor::OPEN) {
                        //OK
                        text+=adventurer.getName()+ " walks south\n";
                        valid=true;
                        adventurer.setRoomY(adventurer.getRoomY()+1);
                    }
                    else if (corridors_[corridorId].type==Corridor::DOOR) {
                        if (!switchesActive_[corridors_[corridorId].id])
                            text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the door, it doesn't budge\n";
                        else
                        {
                            text+=adventurer.getName()+ " walks south\n";
                            adventurer.setRoomY(adventurer.getRoomY()+1);
                            valid=true;
                        }
                    }
                    //TODO start combat
                    else {
                        //Not ok
                        text+=adventurer.getName()+ " bangs "+adventurer.getTheir()+" head against the wall, it doesn't budge\n";
                    }
                }
            }
            updateSinceLocationChanged(renderer,smallFont,midFont);
            generateDescriptionText(text,selectedPlayerIndex_,renderer,smallFont);
            if (valid) {
                walkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            }
            else
                bonkSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
        }
        if (dropItemButton_->isClicked()) {
            clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            if (selectedPlayerIndex_<adventurers_.size() && adventurers_[selectedPlayerIndex_].getFloor()>=0) {
                //Check if there is items on the floor in this room to pick up
                int roomId = adventurers_[selectedPlayerIndex_].getRoomX()+adventurers_[selectedPlayerIndex_].getRoomY()*dungeonWidth+adventurers_[selectedPlayerIndex_].getFloor()*roomIdOffset;
                if (adventurers_[selectedPlayerIndex_].getLoot().empty()) {
                    //Create sorry not possible dialogue
                    std::shared_ptr<control> noLootControl = std::make_shared<textureControl>(std::make_shared<TexWrap>("Nothing to drop",renderer,midFont,512));
                    gui_->addDialogue("Loot",noLootControl,screenHeight,renderer,smallFont);
                }
                else {
                    if (activeDropDialog_!=nullptr) {
                        activeDropDialog_->dialogue_->close();
                        activeDropDialog_=nullptr;
                    }
                    std::shared_ptr<textureControl> buttonTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Confirm",renderer,midFont));
                    std::shared_ptr<buttonControl> pickUpButton = std::make_shared<buttonControl>(buttonTC);
                    std::shared_ptr<textureControl> selectItemTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Select item for "+adventurers_[selectedPlayerIndex_].getName()+"to drop:",renderer,midFont));
                    std::vector<std::shared_ptr<control>> itemsToPickUpTCs;
                    for (int i : adventurers_[selectedPlayerIndex_].getLoot()) {
                        std::shared_ptr<textureControl> itemTC= std::make_shared<textureControl>(std::make_shared<TexWrap>(itemList_[i]->getName()+" ("+itemList_[i]->getValueStr()+" "+itemList_[i]->getWeightStr()+")",renderer,midFont));
                        itemsToPickUpTCs.push_back(itemTC);
                    }
                    std::shared_ptr<DropdownMenu> menu = std::make_shared<DropdownMenu>(itemsToPickUpTCs,expandButtonTexture_);

                    std::shared_ptr<tableControl> dialogueTable = std::make_shared<tableControl>(512,600,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,200),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,200),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,200)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,512)},std::vector<std::shared_ptr<control>>{selectItemTC,menu,pickUpButton});
                    std::shared_ptr<dialogue> newDropDialogue = std::make_shared<dialogue>(screenWidth/3,screenHeight/3,"Loot",200,512,dialogueTable,renderer,smallFont,true);
                    activeDropDialog_=std::make_shared<pickupDialogue>(selectedPlayerIndex_,newDropDialogue,pickUpButton,menu);
                    gui_->addDialogue(newDropDialogue);
                }
            }
            else
            {
                //Create sorry not possible dialogue
                std::shared_ptr<control> noLootControl = std::make_shared<textureControl>(std::make_shared<TexWrap>("Not a valid player",renderer,midFont,512));
                gui_->addDialogue("Loot",noLootControl,screenHeight,renderer,smallFont);
            }
        }
        if (pickUpItemButton_->isClicked()) {
            clickSound_->play(screenWidth/2,screenHeight/2,0,0,screenWidth,screenHeight);
            if (selectedPlayerIndex_<adventurers_.size() && adventurers_[selectedPlayerIndex_].getFloor()>=0) {
                //Check if there is items on the floor in this room to pick up
                int roomId = adventurers_[selectedPlayerIndex_].getRoomX()+adventurers_[selectedPlayerIndex_].getRoomY()*dungeonWidth+adventurers_[selectedPlayerIndex_].getFloor()*roomIdOffset;
                if (rooms_[roomId].itemIds.empty()) {
                    //Create sorry not possible dialogue
                    std::shared_ptr<control> noLootControl = std::make_shared<textureControl>(std::make_shared<TexWrap>("There is no loot in this room",renderer,midFont,512));
                    gui_->addDialogue("Loot",noLootControl,screenHeight,renderer,smallFont);
                }
                else {
                    if (activePickupDialog_!=nullptr) {
                        activePickupDialog_->dialogue_->close();
                        activePickupDialog_=nullptr;
                    }
                    std::shared_ptr<textureControl> buttonTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Confirm",renderer,midFont));
                    std::shared_ptr<buttonControl> pickUpButton = std::make_shared<buttonControl>(buttonTC);
                    std::shared_ptr<textureControl> selectItemTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Select item for "+adventurers_[selectedPlayerIndex_].getName()+"to pick up:",renderer,midFont));
                    std::vector<std::shared_ptr<control>> itemsToPickUpTCs;
                    for (int i : rooms_[roomId].itemIds) {
                        std::shared_ptr<textureControl> itemTC= std::make_shared<textureControl>(std::make_shared<TexWrap>(itemList_[i]->getName()+" ("+itemList_[i]->getValueStr()+" "+itemList_[i]->getWeightStr()+")",renderer,midFont));
                        itemsToPickUpTCs.push_back(itemTC);
                    }
                    std::shared_ptr<DropdownMenu> menu = std::make_shared<DropdownMenu>(itemsToPickUpTCs,expandButtonTexture_);

                    std::shared_ptr<tableControl> dialogueTable = std::make_shared<tableControl>(512,600,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,200),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,200),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,200)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK,512)},std::vector<std::shared_ptr<control>>{selectItemTC,menu,pickUpButton});
                    std::shared_ptr<dialogue> newPickupDialogue = std::make_shared<dialogue>(screenWidth/3,screenHeight/3,"Loot",200,512,dialogueTable,renderer,smallFont,true);
                    activePickupDialog_=std::make_shared<pickupDialogue>(selectedPlayerIndex_,newPickupDialogue,pickUpButton,menu);
                    gui_->addDialogue(newPickupDialogue);
                }
            }
            else
            {
                //Create sorry not possible dialogue
                std::shared_ptr<control> noLootControl = std::make_shared<textureControl>(std::make_shared<TexWrap>("Not a valid player",renderer,midFont,512));
                gui_->addDialogue("Loot",noLootControl,screenHeight,renderer,smallFont);
            }
        }
    }

    return std::nullopt;
}

void Game::ensureValidSelection() {
    auto selection = playerTable_->getSelectedRows();

    //If the primary selected player is outside, deselect everyone
    if (selectedPlayerIndex_>adventurers_.size() || adventurers_[selectedPlayerIndex_].getFloor()<0) {
        playerTable_->dropSelection(selection);
        selectedPlayerIndex_=-1;
    }
    else {
        //Remove people in different floors, or outside the dungeon
        std::set<size_t> toRemove;
        for (size_t i : selection) {
            if (adventurers_[i].getFloor()<0)
                toRemove.insert(i);
            if (adventurers_[i].getHealth()<=0)
                toRemove.insert(i);
            if (!(adventurers_[i].getFloor()==adventurers_[selectedPlayerIndex_].getFloor() &&
            adventurers_[i].getRoomY()==adventurers_[selectedPlayerIndex_].getRoomY() &&
            adventurers_[i].getRoomX()==adventurers_[selectedPlayerIndex_].getRoomX())
                )
                toRemove.insert(i);
        }
        playerTable_->dropSelection(toRemove);
    }
}

void Game::updateSinceLocationChanged(SDL_Renderer *renderer, TTF_Font *smallFont, TTF_Font *midFont) {
    //Discover all rooms
    for (const auto &adventurer : adventurers_) {
        if (adventurer.getFloor()<0)
            continue;
        int roomId =adventurer.getRoomX()+adventurer.getRoomY()*dungeonWidth+adventurer.getFloor()*roomIdOffset;
        rooms_[roomId].discovered=true;
    }

    updateSidebarInfo();
    ensureValidSelection();
    updateSwitches();
    closeAllDialogues();
    checkPlayerStatus(renderer,midFont);
}

void Game::endGame(SDL_Renderer *renderer, TTF_Font *midFont) {
    gongSound_->play(512,512,0,0,1024,1024);
    gameStatus_=GAME_OVER;
    gameOverSlide_->setActiveSlide(1);
    //Sum up cost

    int totalPoints=0;
    {
        auto allTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Everyone",renderer,midFont));
        auto itemTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Character creation",renderer,midFont));
        auto costTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(std::to_string(-pointsInvested),renderer,midFont));
        auto row =std::vector<std::shared_ptr<control>>{allTC,itemTC,costTC};
        scoreTable_->addRow(row);
        totalPoints-=pointsInvested;
    }
    for (const auto &adventurer : adventurers_) {
        auto nameTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(adventurer.getName(),renderer,midFont));
        auto itemTC = std::make_shared<textureControl>(std::make_shared<TexWrap>((adventurer.getHealth()>0 && adventurer.getFloor()<0) ? "Returned alive" : "Did not return",renderer,midFont));
        auto costTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(std::to_string((adventurer.getHealth()>0 && adventurer.getFloor()<0) ? adventurer.getCost() : 0),renderer,midFont));

        totalPoints+=(adventurer.getHealth()>0 && adventurer.getFloor()<0) ? adventurer.getCost() : 0;
        auto row =std::vector<std::shared_ptr<control>>{nameTC,itemTC,costTC};
        scoreTable_->addRow(row);
    }
    //Sum up loot of living adventurers
    for (const auto &adventurer : adventurers_) {
        if ((adventurer.getHealth()>0 && adventurer.getFloor()<0)) {
            for (int i : adventurer.getLoot()) {
                auto nameTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(adventurer.getName(),renderer,midFont));
                auto itemTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(itemList_[i]->getName(),renderer,midFont));
                auto costTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(std::to_string(itemList_[i]->getValueCost()),renderer,midFont));

                totalPoints+=itemList_[i]->getValueCost();
                auto row =std::vector<std::shared_ptr<control>>{nameTC,itemTC,costTC};
                scoreTable_->addRow(row);
            }
        }
    }
    //TODO sum up spells

    {
        auto allTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Total",renderer,midFont));
        auto itemTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(" ",renderer,midFont));
        auto costTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(std::to_string(totalPoints),renderer,midFont));
        auto row =std::vector<std::shared_ptr<control>>{allTC,itemTC,costTC};
        scoreTable_->addRow(row);
    }

}

int Game::getPlayerValues() {
    int total = 0;
    for (const auto &adventurer : adventurers_) {
        total += adventurer.getCost();
        for (int i : adventurer.getLoot()) {
            total += itemList_[i]->getValueCost();
        }
    }
    //TODO, count spells
    return total;
}


void Game::checkPlayerStatus(SDL_Renderer *renderer, TTF_Font *midFont) {
    bool allQuit = true;
    for (const auto& adventurer : adventurers_) {
        if (adventurer.getHealth()>0 && adventurer.getFloor()>=0) {
            allQuit =false;
            break;
        }
    }

    if (allQuit)
        endGame(renderer,midFont);
}



void Game::updateSwitches() {
    std::vector<bool> newSwitchesActive_=std::vector<bool>(zoneTemplates_.size(),false);
    for ( int r : switchedRooms_) {
        int zone = rooms_[r].zone_;
        //Switch is pressed if there is a heavy item in the room
        bool pressed=false;
        for (int i : rooms_[r].itemIds) {
            if (itemList_[i]->getWeight()==Item::HEAVY || itemList_[i]->getWeight()==Item::VERY_HEAVY) {
                pressed=true;
                break;
            }
        }
        //Check if there are any players in this room
        if (!pressed)
            for (const auto &adventurer: adventurers_) {
                int roomId =adventurer.getRoomX()+adventurer.getRoomY()*dungeonWidth+adventurer.getFloor()*roomIdOffset;
                if (roomId==r) {
                    pressed=true;
                    break;
                }
            }

        if (pressed) {
            newSwitchesActive_[zone]=true;
            if (zone+1<zoneTemplates_.size()) {
                newSwitchesActive_[zone+1]=true;
            }
        }
    }
    for (int i = 0; i < zoneTemplates_.size(); i++) {
        if (switchesActive_[i]!=newSwitchesActive_[i]) {
            switchesActive_[i]=newSwitchesActive_[i];
            switchSound_->play(512,512,0,0,1024,1024);
        }
    }
}

void Game::closeAllDialogues() {
    gui_->closeAllDialogues();
}

void Game::updateInventoryDisplay(SDL_Renderer* renderer, TTF_Font* midFont) {
   if (selectedPlayerIndex_<adventurers_.size()) {
       auto& adventurer = adventurers_[selectedPlayerIndex_];
       inventorySlideControl_->setActiveSlide(1);
       statSlideControl_->setActiveSlide(1);
       selectedNameTexture_->setTexture(std::make_shared<TexWrap>(adventurer .getName(),renderer,midFont));
       strViewControl_->setValue(adventurer.getStrength());
       dexViewControl_->setValue(adventurer.getDexterity());
       intViewControl_->setValue(adventurer.getIntelligence());
       speedViewControl_->setValue(adventurer.getSpeed());
       carryViewControl_->setValue(adventurer.getCarryCap());
       dodgeViewControl_->setValue(adventurer.getDodge());
       aimViewControl_->setValue(itemList_[adventurer.getMainHandWeaponIndex()]->getAim(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()));
       attackViewControl_->setValue(itemList_[adventurer.getMainHandWeaponIndex()]->getAttack(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()));
       blockViewControl_->setValue(
           std::max(
           itemList_[adventurer.getMainHandWeaponIndex()]->getBlock(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()),
           itemList_[adventurer.getOffHandWeaponIndex()]->getBlock(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence())
           )
       );
       magicViewControl_->setValue(itemList_[adventurer.getMainHandWeaponIndex()]->getMagicAttack(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()));
       magicDefenceViewControl_->setValue(
           std::max(
           itemList_[adventurer.getMainHandWeaponIndex()]->getMagicDefence(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()),
           itemList_[adventurer.getOffHandWeaponIndex()]->getMagicDefence(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence())
           )
           );
       mainHandWeaponSelection_->setTexture(std::make_shared<TexWrap>(itemList_[adventurer.getMainHandWeaponIndex()]->getName(),renderer,midFont));
       offHandWeaponSelection_->setTexture(std::make_shared<TexWrap>(itemList_[adventurer.getOffHandWeaponIndex()]->getName(),renderer,midFont));


       int totalCarry=0;
       int totalValue=0;
        //selectedCostControl_.;
       //clear the inventory table, and create it anew
       inventoryTable_ ->deleteRows();
       inventoryInfoButtons_.clear();
       for (int i : adventurer.getLoot()) {
           std::shared_ptr<textureControl> TC = std::make_shared<textureControl>(std::make_shared<TexWrap>(itemList_[i]->getName(),renderer,midFont));
           std::shared_ptr<textureControl> MTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(itemList_[i]->getDescription(),renderer,midFont));
           std::shared_ptr<mouseOverControl> MO = std::make_shared<mouseOverControl>(TC,MTC);

           std::shared_ptr<control> buttonTC = std::make_shared<textureControl>(infoButtonTexture_);
           std::shared_ptr<buttonControl> button = std::make_shared<buttonControl>(buttonTC);
           inventoryInfoButtons_.push_back(button);
           std::shared_ptr<control> pair = std::make_shared<tableControl>(128,64,std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, 64)},std::vector<tableControl::rowOrCol>{tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, 64),tableControl::rowOrCol(tableControl::rowOrCol::SHRINK, 64)},std::vector<std::shared_ptr<control>>{MO,button});

           std::shared_ptr<textureControl> valueTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(itemList_[i]->getValueStr(),renderer,midFont));
           std::shared_ptr<textureControl> valueMTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(std::to_string(itemList_[i]->getValueCost())+" points",renderer,midFont));
           std::shared_ptr<mouseOverControl> valueMO = std::make_shared<mouseOverControl>(valueTC,valueMTC);
           std::shared_ptr<textureControl> weightTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(itemList_[i]->getWeightStr(),renderer,midFont));
           std::shared_ptr<textureControl> weightMTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(std::to_string(itemList_[i]->getWeightPenalty())+" carry cap",renderer,midFont));
           std::shared_ptr<mouseOverControl> weightMO= std::make_shared<mouseOverControl>(weightTC,weightMTC);

           totalValue+=itemList_[i]->getValueCost();
           totalCarry+=itemList_[i]->getWeightPenalty();
           auto row =std::vector<std::shared_ptr<control>>{pair,valueMO,weightMO};
           inventoryTable_->addRow(row);
       }

       std::shared_ptr<textureControl> TC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Total:",renderer,midFont));

       std::shared_ptr<textureControl> valueTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(std::to_string(totalValue+adventurer.getCost()),renderer,midFont));
       std::shared_ptr<textureControl> valueMTC = std::make_shared<textureControl>(std::make_shared<TexWrap>("Value of all loot, stats, and spells",renderer,midFont));
       std::shared_ptr<mouseOverControl> valueMO = std::make_shared<mouseOverControl>(valueTC,valueMTC);
       std::shared_ptr<textureControl> weightTC = std::make_shared<textureControl>(std::make_shared<TexWrap>(std::to_string(totalCarry)+"/"+std::to_string(adventurer.getCarryCap()),renderer,midFont));

       auto row =std::vector<std::shared_ptr<control>>{TC,valueMO,weightTC};
       inventoryTable_->addRow(row);

   }
   else {
       inventorySlideControl_->setActiveSlide(0);
       statSlideControl_->setActiveSlide(0);
   }
}

void Game::addWeaponStatPopup(int weapon, const Adventurer&adventurer, SDL_Renderer *renderer, int screenWidth, int screenHeight, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont) {


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
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getAttack(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()));
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
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getAim(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()));
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
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getBlock(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()));
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
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getMagicAttack(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()));
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
            std::string resultStr = "= "+std::to_string(itemList_[weapon]->getMagicDefence(adventurer.getStrength(),adventurer.getDexterity(),adventurer.getIntelligence()));
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
