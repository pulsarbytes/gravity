# TODO.md

A list of tasks to be completed.

### In Progress

-   Add more moons for class 5, class 6
-   `[`,`]`
    -   Zoom in/out global scale by step x
-   `Systems`
    -   Planet properties
        -   Colors
-   `Stars `
    -   Star properties
        -   image: create SDL circle with custom colors ??
    -   Show distance to star
    -   Fade opacity of distant stars projections
    -   Fast-travel to star
        -   Create method goto_star(x, y)
            -   Turns off projections of other stars
            -   Disables collisions and star generation
            -   Increases ship speed so that it arrives in 3 seconds
    -   Show current system and planet name and info on screen
-   `Speed`
    -   Create 2 layers of background stars. Front layer moves faster as speed increases beyond limit.
    -   `Space`:
        -   Find closest star in ship direction
        -   Rotate ship towards star
        -   Fast-travel to star
            -   Create method goto_star(x, y)
                -   Turns off projections of other stars
                -   Bgstars parallax
                -   Disables collisions and star generation
                -   Increases ship speed so that it arrives in 3 seconds
                -   On arrival, put ship in orbit
                -   Display planets
-   `Map` state
    -   Scroll camera with mouse
    -   Show star info on mouse hover
-   `Pause` state
    -   Show message

### Todo

-   `Menu` state
    -   Add save game
-   Rotate camera opposite of ship so that ship angle appears static
-   Separate physics from rendering
-   Add configuration file
-   Console can not display position coordinates with more than 16 characters
-   Show larger thrust (or halo when speed limit is off
-   Use font atlases for console text to keep memory consumption low (google it)
-   Destroy ship when crashing on planet with speed; explosion; game over

### Notes

// Mercury
mercury->radius = 60;
mercury->position.y = star.position.y - 1500;
mercury->color.r = 192;
mercury->color.g = 192;
mercury->color.b = 192;

// Venus
venus->radius = 100;
venus->position.y = star.position.y - 3000;
venus->color.r = 215;
venus->color.g = 140;
venus->color.b = 0;

// Earth
earth->radius = 100;
earth->position.y = star.position.y - 4500;
earth->color.r = 135;
earth->color.g = 206;
earth->color.b = 235;

// Moon
moon->position.y = earth->position.y - 1200;

// Mars
mars->radius = 70;
mars->position.y = star.position.y - 6000;
mars->color.r = 255;
mars->color.g = 69;
mars->color.b = 0;

// Jupiter
jupiter->radius = 160;
jupiter->position.y = star.position.y - 7800;
jupiter->color.r = 244;
jupiter->color.g = 164;
jupiter->color.b = 96;
