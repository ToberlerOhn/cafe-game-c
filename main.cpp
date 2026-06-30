#include <iostream>
#include <raylib.h>
#include <map>

/* -------------------------------------------------------------------------- */
/*                              Global Variables                              */
/* -------------------------------------------------------------------------- */

const int screenWidth = 900, screenHeight = 600;

int money;

enum class Ingredient {
    ESPRESSO,
    MILK,
    MILK_FOAM,
};

enum class Topping {
    ICE,
    WHIPPED_CREAM,
};

struct Palette {
    Color espresso;
    Color milk;
    Color milk_foam;
};

const Palette palette = {
    {81 , 50 , 35 , 255}, // espresso
    {250, 245, 240, 255}, // milk
    {240, 238, 236, 255}, // milk foam
};

struct RecipeStep {
    Ingredient ingredient;
    double amount;
    std::string description;
};

struct Recipe {
    std::string description;
    double cost;
    std::vector<RecipeStep> steps;
    Color frothed_color;
    bool always_hot;
};

const Recipe espresso {
    "Plain black coffee", 
    3.00, 
    { { Ingredient:: ESPRESSO, 0.3, "Espresso shot" } },
    palette.espresso, 
    true
};

const Recipe latte {
    "A coffee with milk",
    4.25,
    { 
        { Ingredient::ESPRESSO, 0.3, "Espresso shot" }, 
        { Ingredient::MILK, 0.6, "Add milk" },
        { Ingredient::MILK_FOAM, 0.1, "Top with foam" },
    },
    palette.espresso,
    false
};

const std::map<std::string, Recipe> RECIPES{
    {"Espresso", Recipe{
        "Plain black coffee",
        3.00,
        {
            {Ingredient::ESPRESSO, 0.3, "Espresso shot"}
        },
        palette.espresso,
        true}
    },
    {"Latte", Recipe{
        "A coffee with milk", 
        4.25, 
        {
            {Ingredient::ESPRESSO, 0.3, "Espresso shot"},
            {Ingredient::MILK, 0.6, "Add milk"},
            {Ingredient::MILK_FOAM, 0.1, "Top with foam"},
        },
        palette.espresso,
        false,}
    },
};

/* -------------------------------------------------------------------------- */
/*                                  Main Loop                                 */
/* -------------------------------------------------------------------------- */

int main(void) {

    /* --------------------------- initialization --------------------------- */

    InitWindow(screenWidth, screenHeight, "Café Game");
    SetTargetFPS(60);

    /* ------------------------------ main loop ----------------------------- */

    while (!WindowShouldClose()) {

        /* -------------------------- update window ------------------------- */
    
        /* -------------------------- drawing loop -------------------------- */
        
        BeginDrawing();
            ClearBackground(palette.espresso);
        EndDrawing();
    };
}