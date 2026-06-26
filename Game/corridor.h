//
// Created by nikolaj on 6/13/26.
//

#ifndef DUNGEONSKETCH_CORRIDOR_H
#define DUNGEONSKETCH_CORRIDOR_H
#include "foe.h"

struct Corridor {
    enum Type {
        WALL = 0,
        OPEN = 1,
        DOOR = 2,
    } type;

    ///ID of the key to open this or the obstacle
    int id=-1;
    bool xDir=false;
    Corridor() {
        type=WALL;
    }

    std::vector<Foe> foes;
};

#endif //DUNGEONSKETCH_CORRIDOR_H