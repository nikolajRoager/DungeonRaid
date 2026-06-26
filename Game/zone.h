//
// Created by nikolaj on 6/13/26.
//

#ifndef DUNGEONSKETCH_ZONE_H
#define DUNGEONSKETCH_ZONE_H
#include <string>
#include <utility>
#include <SDL2/SDL_stdinc.h>

struct Zone {
    std::string colourName_;
    Uint8 R_,G_,B_;
    Zone(std::string colourName, Uint8 R, Uint8 G, Uint8 B) {
        colourName_ = std::move(colourName);
        R_ = R;
        G_ = G;
        B_ = B;
    }
};

#endif //DUNGEONSKETCH_ZONE_H