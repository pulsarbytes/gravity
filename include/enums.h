#ifndef ENUMS_H
#define ENUMS_H

// Fonts
enum
{
    FONT_SIZE_14,
    FONT_SIZE_36,
    FONT_COUNT
};

// Colors
enum
{
    COLOR_WHITE_255,
    COLOR_WHITE_100,
    COLOR_ORANGE_32,
    COLOR_CYAN_70,
    COLOR_MAGENTA_40,
    COLOR_MAGENTA_70,
    COLOR_YELLOW_255,
    COLOR_SKY_BLUE_255,
    COLOR_GAINSBORO_255,
    COLOR_COUNT
};

// States
enum
{
    MENU,
    NAVIGATE,
    MAP,
    UNIVERSE,
    RESUME,
    NEW,
    QUIT,
};

// Menu buttons
enum
{
    MENU_BUTTON_START,
    MENU_BUTTON_RESUME,
    MENU_BUTTON_NEW,
    MENU_BUTTON_EXIT,
    MENU_BUTTON_COUNT
};

enum
{
    STAGE_OFF = -1, // Not landed
    STAGE_0         // Landed
};

enum
{
    FALSE,
    TRUE
};
enum
{
    OFF,
    ON
};

// Must be in sync with game_console_entries
enum
{
    FPS_INDEX,
    X_INDEX,
    Y_INDEX,
    V_INDEX,
    SCALE_INDEX,
    LOG_COUNT // number of elements in enumeration
};

enum
{
    GALAXY_CLASS_1 = 1,
    GALAXY_CLASS_2,
    GALAXY_CLASS_3,
    GALAXY_CLASS_4,
    GALAXY_CLASS_5,
    GALAXY_CLASS_6
};

enum
{
    STAR_CLASS_1 = 1,
    STAR_CLASS_2,
    STAR_CLASS_3,
    STAR_CLASS_4,
    STAR_CLASS_5,
    STAR_CLASS_6,
    PLANET_CLASS_1,
    PLANET_CLASS_2,
    PLANET_CLASS_3,
    PLANET_CLASS_4,
    PLANET_CLASS_5,
    PLANET_CLASS_6
};

enum
{
    LEVEL_STAR = 1,
    LEVEL_PLANET,
    LEVEL_MOON
};

enum
{
    ENTITY_SHIP,
    ENTITY_PLANET,
    ENTITY_STAR,
    ENTITY_GALAXY
};

#endif /* ENUMS_H */
