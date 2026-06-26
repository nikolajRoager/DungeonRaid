//
// Created by nikolaj on 6/13/26.
//

#ifndef DUNGEONSKETCH_ROOM_H
#define DUNGEONSKETCH_ROOM_H
#include <vector>

#include "../shared/item.h"

struct Room {
    bool discovered=true;
    enum Ladder {NONE, UP, DOWN} ladder_=NONE;
    enum Direction{NOWHERE, MINUS_Z, PLUS_Z, MINUS_X,PLUS_X,MINUS_Y,PLUS_Y,HERE};

    int xId=-1;
    int yId=-1;
    int zId=-1;

    Direction exit=NOWHERE;
    Direction deeper=NOWHERE;
    Direction switchDir=NOWHERE;

    bool hasSwitch_=false;
    bool switchPressed_=false;
    int depth=0;
    int zone_ = -1;
    //What branch is this, -1 = not generated, 0 = main branch
    int branchId=-1;

    std::vector<int> itemIds;
    Room() = default;
};

#endif //DUNGEONSKETCH_ROOM_H