    cmake_minimum_required(VERSION 4.0)
    project(DungeonSketchTwo)


    add_definitions(-DSDL_MAIN_HANDLED)

    set(CMAKE_CXX_STANDARD 20)

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

    file (COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

    if (UNIX)
        find_package(PkgConfig REQUIRED)

        pkg_check_modules(SDL2 sdl2)
        pkg_check_modules(SDL2_image SDL2_image)
        pkg_check_modules(SDL2_ttf SDL2_ttf)
        pkg_check_modules(SDL2_mixer SDL2_mixer)
        link_libraries(${SDL2_LIBRARIES})
        link_libraries(${SDL2_image_LIBRARIES})
        link_libraries(${SDL2_mixer_LIBRARIES})
        link_libraries(${SDL2_ttf_LIBRARIES})
    endif (UNIX)
    if (WIN32)
        set(SDL2_PATH "C:/libs/SDL2")
        set(SDL2_INCLUDE_DIR "${SDL2_PATH}/include")
        set(SDL2_LIB_DIR "${SDL2_PATH}/lib")

        include_directories(${SDL2_INCLUDE_DIR})
        link_directories(${SDL2_LIB_DIR})

        link_libraries(mingw32)
        link_libraries(SDL2main)
        link_libraries(SDL2)
        link_libraries(SDL2_image)
        link_libraries(SDL2_ttf)
        link_libraries(SDL2_mixer)
    endif (WIN32)


    add_executable(DungeonSketchTwo main.cpp
            ThreadPool.cpp
            ThreadPool.cpp
            TexWrap.cpp
            textureManager.cpp
            scene.cpp
            NumberRenderer.cpp
            app.cpp
            mainMenu/titleMenu.cpp
            MIGUI/buttonControl.cpp
            MIGUI/contentTable.cpp
            MIGUI/control.cpp
            MIGUI/dialogue.cpp
            MIGUI/emptyControl.cpp
            MIGUI/guiManager.cpp
            MIGUI/mouseOverControl.cpp
            MIGUI/stackControl.cpp
            MIGUI/tableControl.cpp
            MIGUI/tableMenu.cpp
            MIGUI/textureControl.cpp
            MIGUI/slideControl.cpp
            MIGUI/slideControl.h
            MIGUI/numberControl.cpp
            MIGUI/numberControl.h
            MIGUI/numberInputControl.cpp
            MIGUI/numberInputControl.h
            shared/item.cpp
            shared/item.h
            MIGUI/dropdownMenu.cpp
            MIGUI/dropdownMenu.h
            musicwrap.cpp
            soundWrap.cpp
            Game/game.cpp
            Game/game.h
            Game/room.h
            Game/zone.h
            Game/corridor.h
            Game/adventurer.cpp
            Game/adventurer.h
            Game/foe.h
    )
