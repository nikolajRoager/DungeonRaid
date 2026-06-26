//
// Created by nikolaj on 6/1/26.
//

#ifndef DUNGEONSKETCH_SCENE_H
#define DUNGEONSKETCH_SCENE_H
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>

#include "inputData.h"
#include "shared/AdventurerTemplate.h"

struct SceneTransitionData {
    std::string stringData_;
    std::vector<AdventurerTemplate> adventurers_;

    explicit SceneTransitionData(std::string  str): stringData_(std::move(str)) {}
    explicit SceneTransitionData(std::vector<AdventurerTemplate> adventurers): adventurers_(std::move(adventurers)) {}
};

class Scene {
public:
    enum SceneInfo {
        QUIT_GAME,
        START_GAME,
        RELOAD_FONTS,
        START_TYPING,
        STOP_TYPING,
        QUIT_TO_MENU,
    };

    Scene()=default;
    virtual ~Scene()=default;

    virtual void render(SDL_Renderer* renderer, int screenWidth, int screenHeight,const InputData& userInputs, unsigned int millis, unsigned int pmillis) const=0;
    ///Returns what scene we should transition to, and the arguments to that
    virtual std::optional<std::pair<SceneInfo,SceneTransitionData>> update(SDL_Renderer* renderer, int screenWidth, int screenHeight,const InputData& userInputs,  unsigned int millis, unsigned int dmillis, TTF_Font *smallFont, TTF_Font *midFont, TTF_Font *largeFont)=0;
};


#endif //DUNGEONSKETCH_SCENE_H