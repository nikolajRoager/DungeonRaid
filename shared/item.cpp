//
// Created by nikolaj on 6/17/26.
//

#include "item.h"



std::string Item::getValueStr() const {
    switch (value_) {
        default:
        case WORTHLESS:
            return "worthless";
        case COMMON:
            return "common";
        case RARE:
            return "rare";
        case EPIC:
            return "epic";
        case LEGENDARY:
            return "legendary";
    }
}

std::string Item::getWeightStr() const {
    switch (weight_) {
        default:
        case WEIGHTLESS:
            return "weightless";
        case LIGHT:
            return "light";
        case MEDIUM:
            return "medium";
        case HEAVY:
            return "heavy";
        case VERY_HEAVY:
            return "very heavy";
    }
}

int Item::getValueCost() const {
    switch (value_) {
        default:
        case WORTHLESS:
            return 0;
        case COMMON:
            return 1;
        case RARE:
            return 4;
        case EPIC:
            return 16;
        case LEGENDARY:
            return 64;
    }
}

int Item::getWeightPenalty() const {
    switch (weight_) {
        default:
        case WEIGHTLESS:
            return 0;
        case LIGHT:
            return 1;
        case MEDIUM:
            return 2;
        case HEAVY:
            return 4;
        case VERY_HEAVY:
            return 8;
    }

}

Item::Item(const nlohmann::json& itemsJson) {
    name_ = itemsJson["name"].get<std::string>();
    description_ = itemsJson["description"].get<std::string>();
    sharp_ = itemsJson.value("sharp", false);

    std::string valueStr = itemsJson["value"].get<std::string>();
    if (valueStr=="WORTHLESS")
        value_ = WORTHLESS;
    else if (valueStr=="COMMON")
        value_ = COMMON;
    else if (valueStr=="RARE")
        value_ = RARE;
    else if (valueStr=="EPIC")
        value_ = EPIC;
    else if (valueStr=="LEGENDARY")
        value_ = LEGENDARY;
    else
        throw std::invalid_argument("Invalid item value for "+name_+" : "+valueStr);

    std::string weightStr = itemsJson["weight"].get<std::string>();
    if (weightStr=="WEIGHTLESS")
        weight_ = WEIGHTLESS;
    else if (weightStr=="LIGHT")
        weight_ = LIGHT;
    else if (weightStr=="MEDIUM")
        weight_ = MEDIUM;
    else if (weightStr=="HEAVY")
        weight_ = HEAVY;
    else if (weightStr=="VERY_HEAVY")
        weight_ = VERY_HEAVY;
    else
        throw std::invalid_argument("Invalid item value for "+name_+" : "+weightStr);

    for (int i = 0; i < physicAttackFactors_.size(); ++i) {
        if (itemsJson.contains("physicAttackFactors")) {
            physicAttackFactors_[i].first = itemsJson["physicAttackFactors"][i*2].get<int>();
            physicAttackFactors_[i].second= std::max(itemsJson["physicAttackFactors"][i*2+1].get<int>(),1);
        }

        if (itemsJson.contains("magicAttackFactors")) {
            magicAttackFactors_[i].first = itemsJson["magicAttackFactors"][i*2].get<int>();
            magicAttackFactors_[i].second= std::max(itemsJson["magicAttackFactors"][i*2+1].get<int>(),1);
        }

        if (itemsJson.contains("aimFactors")) {
            aimFactors_[i].first = itemsJson["aimFactors"][i*2].get<int>();
            aimFactors_[i].second= std::max(itemsJson["aimFactors"][i*2+1].get<int>(),1);
        }

        if (itemsJson.contains("blockFactors")) {
            blockFactors_[i].first = itemsJson["blockFactors"][i*2].get<int>();
            blockFactors_[i].second= std::max(itemsJson["blockFactors"][i*2+1].get<int>(),1);
        }

        if (itemsJson.contains("magicDefenceFactors")) {
            magicDefenceFactors_[i].first = itemsJson["magicDefenceFactors"][i*2].get<int>();
            magicDefenceFactors_[i].second= std::max(itemsJson["magicDefenceFactors"][i*2+1].get<int>(),1);
        }
    }
}
