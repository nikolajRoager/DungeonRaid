//
// Created by nikolaj on 6/2/26.
//

#ifndef DUNGEONSKETCH_ADVENTURERCREATION_H
#define DUNGEONSKETCH_ADVENTURERCREATION_H
#include <cstdint>
#include <string>
#include <utility>

#include "../TexWrap.h"

#define BASE_CARRY_CAP 4
#define CARRY_CAP_PER_STR 2

#define BASE_SPEED  3
#define SPEED_PER_DEX  0.5

#define BASE_DODGE  1
#define DODGE_PER_DEX  0.5


///A struct for storing the creation of an adventurer
struct AdventurerTemplate {
    std::string name_;
    int intelligence_=1;
    int strength_=1;
    int dexterity_=1;

    //0 is always bare hands
    int mainHandWeaponIndex = 0;
    int offHandWeaponIndex = 0;

    enum Gender {FEMALE=0, MALE=1, NEITHER=2} gender_=FEMALE;

    ///Cost of creating this character
    [[nodiscard]] int getCost() const {
        int sumStats = intelligence_+strength_+dexterity_;
        //Sum of 1+2+3+...sumStats;
        return sumStats*(sumStats+1)/2;
    }

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


    explicit AdventurerTemplate(std::string  name):name_(std::move(name)){}
    AdventurerTemplate(std::string  name, Gender gender, int intelligence, int strength, int dexterity):
    name_(std::move(name)),
    gender_(gender),
    intelligence_(intelligence),
    strength_(strength),
    dexterity_(dexterity)
    {}
};

#endif //DUNGEONSKETCH_ADVENTURERCREATION_H