//
// Created by nikolaj on 6/17/26.
//

#ifndef DUNGEONSKETCHTWO_ITEM_H
#define DUNGEONSKETCHTWO_ITEM_H
#include <array>
#include <iostream>
#include <string>

#include "../nlohmann/json.hpp"


class Item {
public:
    enum Value {WORTHLESS=0,COMMON=1, RARE=2, EPIC=3, LEGENDARY=4};
    enum Weight {WEIGHTLESS=0,LIGHT=1,MEDIUM=2,HEAVY=3,VERY_HEAVY=4};

    explicit Item(const nlohmann::json& itemsJson);


    [[nodiscard]] const std::string& getName() const {return name_;}
    [[nodiscard]] const std::string& getDescription() const {return description_;}
    [[nodiscard]] bool isSharp () const {return sharp_;}
    [[nodiscard]] Value getValue() const {return value_;}
    [[nodiscard]] Weight getWeight() const {return weight_;}

    int getAttack(int strength, int dexterity, int intelligence) const {
        return physicAttackFactors_[0].first/physicAttackFactors_[0].second//Base
        +(physicAttackFactors_[1].first*strength)/physicAttackFactors_[1].second//Strength scaling
        +(physicAttackFactors_[2].first*dexterity)/physicAttackFactors_[2].second//Dexterity scaling
        +(physicAttackFactors_[3].first*intelligence)/physicAttackFactors_[3].second;//Intelligence scaling
    }
    int getAim(int strength, int dexterity, int intelligence) const {
        return aimFactors_[0].first/aimFactors_[0].second//Base
        +(aimFactors_[1].first*strength)/aimFactors_[1].second//Strength scaling
        +(aimFactors_[2].first*dexterity)/aimFactors_[2].second//Dexterity scaling
        +(aimFactors_[3].first*intelligence)/aimFactors_[3].second;//Intelligence scaling
    }
    int getMagicAttack(int strength, int dexterity, int intelligence) const {
        return magicAttackFactors_[0].first/magicAttackFactors_[0].second//Base
        +(magicAttackFactors_[1].first*strength)/magicAttackFactors_[1].second//Strength scaling
        +(magicAttackFactors_[2].first*dexterity)/magicAttackFactors_[2].second//Dexterity scaling
        +(magicAttackFactors_[3].first*intelligence)/magicAttackFactors_[3].second;//Intelligence scaling
    }
    int getMagicDefence(int strength, int dexterity, int intelligence) const {
        return magicDefenceFactors_[0].first/magicDefenceFactors_[0].second//Base
        +(magicDefenceFactors_[1].first*strength)/magicDefenceFactors_[1].second//Strength scaling
        +(magicDefenceFactors_[2].first*dexterity)/magicDefenceFactors_[2].second//Dexterity scaling
        +(magicDefenceFactors_[3].first*intelligence)/magicDefenceFactors_[3].second;//Intelligence scaling
    }
    int getBlock(int strength, int dexterity, int intelligence) const {
        return blockFactors_[0].first/blockFactors_[0].second//Base
        +(blockFactors_[1].first*strength)/blockFactors_[1].second//Strength scaling
        +(blockFactors_[2].first*dexterity)/blockFactors_[2].second//Dexterity scaling
        +(blockFactors_[3].first*intelligence)/blockFactors_[3].second;//Intelligence scaling
    }

    int getValueCost () const;
    int getWeightPenalty() const;

    [[nodiscard]] std::string getValueStr() const;
    [[nodiscard]] std::string getWeightStr() const;

    const std::array<std::pair<int,int>,4>& getPhysicAttackFactors()const {
        return physicAttackFactors_;
    }
    const std::array<std::pair<int,int>,4>& getAimFactors()const {
        return aimFactors_;
    }
    const std::array<std::pair<int,int>,4>& getBlockFactors()const {
        return blockFactors_;
    }
    const std::array<std::pair<int,int>,4>& getMagicFactors()const {
        return magicAttackFactors_;
    }
    const std::array<std::pair<int,int>,4>& getMagicDefenceFactors()const {
        return magicDefenceFactors_;
    }

private:
    std::string name_;
    std::string description_;
    //May cut parts of players
    bool sharp_ = false;

    Value value_=COMMON;
    Weight weight_=LIGHT;

    //Base, Str, Dex, and Int factors as fractions
    std::array<std::pair<int,int>,4> physicAttackFactors_{ std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),};
    std::array<std::pair<int,int>,4> magicAttackFactors_{ std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),};
    std::array<std::pair<int,int>,4> aimFactors_{ std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),};
    std::array<std::pair<int,int>,4> blockFactors_{ std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),};
    std::array<std::pair<int,int>,4> magicDefenceFactors_{ std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),std::make_pair(0,1),};
};


#endif //DUNGEONSKETCHTWO_ITEM_H