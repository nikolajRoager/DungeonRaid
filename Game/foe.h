//
// Created by nikolaj on 6/22/26.
//

#ifndef DUNGEONSKETCHTWO_FOE_H
#define DUNGEONSKETCHTWO_FOE_H
#include <string>
#include <vector>

#include "../shared/item.h"

struct Foe {
    std::string name_;

    int getBlock(const std::vector<std::shared_ptr<Item>>& itemList) const {
        return std::max( itemList[mainItem_]->getBlock(strength_,dexterity_,intelligence_),itemList[otherItem_]->getBlock(strength_,dexterity_,intelligence_));
    }
    int getMagicDefence(const std::vector<std::shared_ptr<Item>>& itemList) const {
        return std::max( itemList[mainItem_]->getMagicDefence(strength_,dexterity_,intelligence_),itemList[otherItem_]->getMagicDefence(strength_,dexterity_,intelligence_));
    }

    ///Level of the enemy, controls when and where they will spawn
    int getLevel() const {
        return intelligence_+strength_+dexterity_+maxHealth_;
    }

    int intelligence_=1;
    int strength_=1;
    int dexterity_=1;
    int stamina_ = 0;
    int health_ = 4;
    int maxHealth_ = 4;

    int mainItem_ = 0;//default to bare hands
    int otherItem_ = 0;

    Foe(const std::string &name, int strength, int dexterity, int intelligence, int health, int mainItem, int otherItem) {
        name_ = name;
        strength_ = strength;
        dexterity_ = dexterity;
        intelligence_ = intelligence;
        stamina_ = 0;
        health_ = health;
        maxHealth_ = health;
        mainItem_ = mainItem;
        otherItem_ = otherItem;
    }
};

#endif //DUNGEONSKETCHTWO_FOE_H