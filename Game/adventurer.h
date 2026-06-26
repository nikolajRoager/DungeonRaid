//
// Created by nikolaj on 6/20/26.
//

#ifndef DUNGEONSKETCHTWO_ADVENTURER_H
#define DUNGEONSKETCHTWO_ADVENTURER_H
#include <map>
#include <vector>

#include "../shared/AdventurerTemplate.h"


class Adventurer {
public:
    explicit Adventurer(const AdventurerTemplate& myTemplate);

    [[nodiscard]] int getLevelCost () const {
        return intelligence_+strength_+dexterity_+1;
    }

    [[nodiscard]] int getSpeed() const {
        return BASE_SPEED+dexterity_*SPEED_PER_DEX;
    }

    [[nodiscard]] int getDodge() const {
        return BASE_DODGE+dexterity_*DODGE_PER_DEX;
    }

    [[nodiscard]] int getCarryCap() const {
        return BASE_CARRY_CAP+strength_*CARRY_CAP_PER_STR;
    }

    ///Cost of creating this character
    [[nodiscard]] int getCost() const {
        int sumStats = intelligence_+strength_+dexterity_;
        //Sum of 1+2+3+...sumStats;
        return sumStats*(sumStats+1)/2;
    }

    int getRoomX() const {
        return roomX;
    }
    int getRoomY() const {return roomY;}
    int getFloor() const {return floor;}

    void setRoomX(int x) {roomX = x;}
    void setRoomY(int y) {roomY = y;}
    void setFloor(int floor) {this->floor = floor;}
    int getIntelligence() const {return intelligence_;}
    int getStrength() const {return strength_;}
    int getDexterity() const {return dexterity_;}
    int getMainHandWeaponIndex () const {
        if (mainHandWeaponLootTableIndex==-1)
            return 0;
        else
            return loot_[mainHandWeaponLootTableIndex];
    }
    int getOffHandWeaponIndex() const {
        if (offHandWeaponLootTableIndex==-1)
            return 0;
        else
            return loot_[offHandWeaponLootTableIndex];
    }
    int getStamina() const {return stamina;}
    int getHealth() const {return health;}
    const std::string& getName() const {return name_;}
    [[nodiscard]] AdventurerTemplate::Gender getGender () const {return gender_;}

    [[nodiscard]] std::string getTheir() const {
        switch (gender_) {
            case AdventurerTemplate::FEMALE:
                return "her";
            case AdventurerTemplate::MALE:
                return "his";
            default:
            case AdventurerTemplate::NEITHER:
                return "their";
        }
    }

    [[nodiscard]] const std::vector<int>& getLoot() const {return loot_;}
    void addLoot(int L) {
        loot_.emplace_back(L);
    }

    void toggleMainHand(int i) {
        if (mainHandWeaponLootTableIndex==i || i<0 || i>=loot_.size())
            mainHandWeaponLootTableIndex=-1;
        else {
            if (offHandWeaponLootTableIndex==i)
                offHandWeaponLootTableIndex=-1;
            mainHandWeaponLootTableIndex=i;
        }
    }
    void toggleOffHand(int i) {
        if (offHandWeaponLootTableIndex==i || i<0 || i>=loot_.size())
            offHandWeaponLootTableIndex=-1;
        else {
            if (mainHandWeaponLootTableIndex==i)
                mainHandWeaponLootTableIndex=-1;
            offHandWeaponLootTableIndex=i;

        }
    }

    void drop(int LId) {
        if (mainHandWeaponLootTableIndex ==LId)
            mainHandWeaponLootTableIndex =-1;
        else if (mainHandWeaponLootTableIndex>LId)
            mainHandWeaponLootTableIndex--;

        if (offHandWeaponLootTableIndex ==LId)
            offHandWeaponLootTableIndex =-1;
        else if (offHandWeaponLootTableIndex>LId)
            offHandWeaponLootTableIndex--;

        loot_.erase(loot_.begin()+LId);
    }
private:

    int roomX, roomY, floor;

    std::string name_;
    int intelligence_=1;
    int strength_=1;
    int dexterity_=1;
    int stamina = 0;
    int health = 4;
    AdventurerTemplate::Gender gender_;

    //0 is always bare hands
    int mainHandWeaponLootTableIndex = -1;//Index in my table of loot, -1 : bare hands
    int offHandWeaponLootTableIndex = -1;

    //Loot id
    std::vector<int> loot_;
};


#endif //DUNGEONSKETCHTWO_ADVENTURER_H