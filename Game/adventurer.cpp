//
// Created by nikolaj on 6/20/26.
//

#include "adventurer.h"

Adventurer::Adventurer(const AdventurerTemplate &myTemplate) {
    roomX=0;
    roomY=0;
    floor=0;
    name_=myTemplate.name_;
    intelligence_=myTemplate.intelligence_;
    strength_=myTemplate.strength_;
    dexterity_=myTemplate.dexterity_;
    gender_=myTemplate.gender_;

    if (myTemplate.mainHandWeaponIndex==0)//Bare hands
    {
        mainHandWeaponLootTableIndex = -1;
    }
    else {
        loot_.emplace_back(myTemplate.mainHandWeaponIndex);
        mainHandWeaponLootTableIndex = loot_.size()-1;
    }
    if (myTemplate.offHandWeaponIndex==0)//Bare hands
    {
        offHandWeaponLootTableIndex = -1;
    }
    else {
        loot_.emplace_back(myTemplate.offHandWeaponIndex);
        offHandWeaponLootTableIndex = loot_.size()-1;
    }
}
