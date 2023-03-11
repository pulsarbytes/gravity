#ifndef ENUMS_H
#define ENUMS_H

enum
{
    FONT_SIZE_14,
    FONT_SIZE_15,
    FONT_SIZE_22,
    FONT_SIZE_36,
    FONT_COUNT
};

enum
{
    COLOR_CYAN_40,
    COLOR_CYAN_70,
    COLOR_GAINSBORO_255,
    COLOR_LAVENDER_255,
    COLOR_LIGHT_BLUE_255,
    COLOR_LIGHT_GREEN_255,
    COLOR_LIGHT_ORANGE_255,
    COLOR_LIGHT_RED_255,
    COLOR_MAGENTA_40,
    COLOR_MAGENTA_70,
    COLOR_ORANGE_32,
    COLOR_PALE_YELLOW_255,
    COLOR_SKY_BLUE_255,
    COLOR_WHITE_100,
    COLOR_WHITE_180,
    COLOR_WHITE_255,
    COLOR_YELLOW_255,
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
    CONSOLE_FPS,
    CONSOLE_X,
    CONSOLE_Y,
    CONSOLE_V,
    CONSOLE_SCALE,
    CONSOLE_COUNT
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
    ENTITY_CELESTIALBODY,
    ENTITY_STAR,
    ENTITY_GALAXY
};

enum
{
    GALAXY_INFO_NAME,
    GALAXY_INFO_TYPE,
    GALAXY_INFO_X,
    GALAXY_INFO_Y,
    GALAXY_INFO_CLASS,
    GALAXY_INFO_RADIUS,
    GALAXY_INFO_STARS,
    GALAXY_INFO_COUNT
};

enum
{
    STAR_INFO_NAME,
    STAR_INFO_X,
    STAR_INFO_Y,
    STAR_INFO_CLASS,
    STAR_INFO_RADIUS,
    STAR_INFO_PLANETS,
    STAR_INFO_COUNT
};

#endif /* ENUMS_H */
