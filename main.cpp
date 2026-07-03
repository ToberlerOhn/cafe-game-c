#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <map>
#include <unordered_map>

#define LEFT -1
#define CENTER 0
#define RIGHT 1

#define TABLE_HEIGHT 100

#pragma region global variables
/* -------------------------------------------------------------------------- */
/*                              Global Variables                              */
/* -------------------------------------------------------------------------- */
// This section also contains the structures for the palette, recipe, etc...

const int screenWidth = 1600, screenHeight = 900;

int money;

enum class Ingredient
{
    ESPRESSO,
    MILK,
    MILK_FOAM,
};

const std::map<Ingredient, std::string> PRETTY_INGREDIENTS{
    {Ingredient::ESPRESSO, "Espresso"},
    {Ingredient::MILK, "Milk"},
    {Ingredient::MILK_FOAM, "Milk Foam"},
};

const std::unordered_map<std::string, Ingredient> STR_TO_INGREDIENT{
    {"Espresso", Ingredient::ESPRESSO},
    {"Milk", Ingredient::MILK},
    {"Milk Foam", Ingredient::MILK_FOAM},
};

enum class Topping
{
    ICE,
    WHIPPED_CREAM,
};

struct Palette
{
    std::vector<Color> colors;

    Color color(Ingredient ingredient) const
    {
        return colors.at((size_t)ingredient);
    };
};

const Palette palette = {
    {
        {81, 50, 35, 255},    // espresso
        {250, 245, 240, 255}, // milk
        {240, 238, 236, 255}, // milk foam
    }
};

struct RecipeStep
{
    Ingredient ingredient;
    double amount;
    std::string description;
};

struct Recipe
{
    std::string description;
    double cost;
    std::vector<RecipeStep> steps;
    Color frothed_color;
    bool always_hot;
};

const Recipe espresso{
    "Plain black coffee",
    3.00,
    {{Ingredient::ESPRESSO, 0.3, "Espresso shot"}},
    palette.espresso,
    true};

const Recipe latte{
    "A coffee with milk",
    4.25,
    {
        {Ingredient::ESPRESSO, 0.3, "Espresso shot"},
        {Ingredient::MILK, 0.6, "Add milk"},
        {Ingredient::MILK_FOAM, 0.1, "Top with foam"},
    },
    palette.espresso,
    false};

Recipe current_recipe; 
const std::map<std::string, Recipe> RECIPES{
    {"Espresso", espresso},
    {"Latte", latte},
};

#pragma endregion

#pragma region functions
/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

/* ---------------------------- helper functions ---------------------------- */
// These functions serve to replace the functions found in the raylib library
// and emulate those from pygame.

Texture2D _LoadImage(const char *image_path);
Texture2D _LoadImage(const char *image_path, int width, int height);
void _DrawText(Font font, const char *text, int x, int y, int font_size, Color color);
void _DrawText(Font font, const char *text, int x, int y, int font_size, int anchor, Color color);

Texture2D _LoadImage(const char *image_path)
{
    Image image = LoadImage(image_path);
    return _LoadImage(image_path, image.width, image.height);
}

Texture2D _LoadImage(const char *image_path, int width, int height)
{
    Image loaded_image = LoadImage(image_path);
    ImageResize(&loaded_image, width, height);
    Texture2D texture = LoadTextureFromImage(loaded_image);
    UnloadImage(loaded_image);
    return texture;
}

void _DrawText(Font font, const char *text, int x, int y, int font_size, Color color)
{
    return _DrawText(font, text, x, y, font_size, 0, color);
}

void _DrawText(Font font, const char *text, int x, int y, int font_size, int anchor, Color color)
{
    int length = MeasureText(text, font_size);
    float mid_x = (float)x - length / 2;
    // -1: Left justified
    // 0: Center justified
    // 1: Right justified
    switch (anchor)
    {
    case -1:
        DrawTextEx(font, text, Vector2{(float)x, (float)y}, font_size, 1, color);
        break;
    case 0:
        DrawTextEx(font, text, Vector2{mid_x, (float)y}, font_size, 1, color);
        break;
    case 1:
        DrawTextEx(font, text, Vector2{(float)x - length, (float)y}, font_size, 1, color);
        break;
    }
}

void _DrawDashedLine(Vector2 startPos, Vector2 endPos, int dashSize, int gapSize, float thickness, Color color)
{
    float dx = endPos.x - startPos.x;
    float dy = endPos.y - startPos.y;
    float distance = std::sqrtf(dx * dx + dy * dy);
    if (distance == 0.0f) return;
    float dirX = dx / distance;
    float dirY = dy / distance;

    float currentDist = 0.0f;
    while (currentDist < distance) {
        float nextDist = currentDist + dashSize;
        if (nextDist > distance) nextDist = distance;

        Vector2 segStart = {startPos.x + dirX * currentDist, startPos.y + dirY * currentDist};
        Vector2 segEnd   = {startPos.x + dirX * nextDist   , startPos.y + dirY * nextDist   };

        DrawLineEx(segStart, segEnd, thickness, color);

        currentDist += dashSize + gapSize;
    }
}

#pragma endregion

#pragma region classes
/* -------------------------------------------------------------------------- */
/*                                   Classes                                  */
/* -------------------------------------------------------------------------- */

class Cup;
extern Cup cup;

class Cup
{
public:
    int x, y, width, height;
    Rectangle rect;
    Recipe* current_recipe;
    std::vector<float> target_lines;

    std::vector<Ingredient> layers;
    int num_layers;
    bool frothed;
    Color frothed_color;

    Cup(int _width, int _height, Recipe* _current_recipe)
    {
        width = _width;
        height = _height;
        x = (screenWidth - width) / 2;
        y = screenHeight - TABLE_HEIGHT - height;
        rect = Rectangle{(float)x, (float)y, (float)width, (float)height};
        current_recipe = _current_recipe;
    };

    void reset()
    {
        layers = {};
        num_layers = 0;
        frothed = false;
        frothed_color = BLANK;

        target_lines.clear();
        float total = 0.0f;
        for (const auto& step : current_recipe->steps) {
            total += step.amount;
            target_lines.push_back(total);
        }
    };

    void add_ingredient(Ingredient ingredient)
    {
        if (num_layers < 100)
        {
            layers.push_back(ingredient);
            num_layers++;
        }
    };

private:

    
    void draw_cup()
    {
        DrawRectangleLinesEx(rect, 5, DARKGRAY);
    };
    
    void draw_helper_lines() {
        std::vector<float> pos = target_lines;
        for (float _pos : pos) {
            float y_loc = y + (1 - _pos) * height;
            _DrawDashedLine(Vector2{(float)x, y_loc}, Vector2{(float)x + width, y_loc}, 3, 6, 2, BLACK);
        }
    };

    void draw_layers()
    {
        float layer_height = height / 100;
        int current_height = y + height;
        for (auto layer : layers)
        {
            DrawRectangle(x + 5, current_height - layer_height, width - 10, layer_height, palette.color(layer));
            current_height -= layer_height;
        };
    };

public:
    void draw()
    {
        draw_cup();
        draw_helper_lines();
        draw_layers();
    }

    float grade() 
    {
        if (num_layers < target_lines.back()) return -1.0f;
        return 0.1f;
    };
};

// Base Button class
class Button
{
protected:
    int x;
    int y;
    int width;
    int height;
    Rectangle rect;
    bool hovered;

public:
    Button(int _x, int _y, int _width, int _height)
    {
        x = _x;
        y = _y;
        width = _width;
        height = _height;
        rect = Rectangle{(float)x, (float)y, (float)width, (float)height};
    };

    void update_hover()
    {
        Vector2 mouse_pos = GetMousePosition();
        hovered = CheckCollisionPointRec(mouse_pos, rect);
    };
};

class IngredientButton : public Button
{
    Ingredient ingredient;
    Texture2D image;
    std::string name;

public:
    IngredientButton(Ingredient _ingredient, Texture2D _image, int _x, int _y, int _width, int _height)
        : Button(_x, _y, _width, _height),
          ingredient(_ingredient),
          image(_image),
          name(PRETTY_INGREDIENTS.at(_ingredient)) {};

    void set_texture(Texture2D _image)
    {
        image = _image;
    };

    void position(int number)
    {
        x = 20 + (number % 2) * 80;
        y = 90 + (number / 2) * 80;
        rect = Rectangle{(float)x, (float)y, (float)width, (float)height};
    };

    void _handle_event()
    {
        update_hover();
        if (hovered)
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) cup.add_ingredient(ingredient);
            DrawRectangle(x, y, width, height, Color{245, 240, 235, 128});
        }
    }

    void draw()
    {
        DrawRectangleRec(rect, BLANK);
        DrawTexture(image, x, y, WHITE);
        _DrawText(GetFontDefault(), name.c_str(), x + width / 2, y + height + 5, 15, DARKGRAY);
    };

};

#pragma endregion

#pragma region objects

/* -------------------------------------------------------------------------- */
/*                                   Objects                                  */
/* -------------------------------------------------------------------------- */

Cup cup(150, 300, &current_recipe);
IngredientButton EspressoBtn(Ingredient::ESPRESSO , Texture2D{}, 0, 0, 50, 50);
IngredientButton MilkBtn    (Ingredient::MILK     , Texture2D{}, 0, 0, 50, 50);
IngredientButton MilkFoamBtn(Ingredient::MILK_FOAM, Texture2D{}, 0, 0, 50, 50);
IngredientButton IngBtns[3] = {EspressoBtn, MilkBtn, MilkFoamBtn};
int NumIngBtns = sizeof(IngBtns) / sizeof(IngBtns[0]);

#pragma endregion
#pragma region main loop
/* -------------------------------------------------------------------------- */
/*                                  Main Loop                                 */
/* -------------------------------------------------------------------------- */

int main(void)
{
    ChangeDirectory(GetApplicationDirectory());

    /* --------------------------- initialization --------------------------- */

    InitWindow(screenWidth, screenHeight, "Café Game");
    ToggleBorderlessWindowed();
    SetTargetFPS(60);

    IngBtns[0].set_texture(_LoadImage("images/espresso.png"  , 50, 50));
    IngBtns[1].set_texture(_LoadImage("images/milk.png"      , 50, 50));
    IngBtns[2].set_texture(_LoadImage("images/milk\ foam.png", 50, 50));
    int index = 0;
    for (auto &btn : IngBtns)
    {
        btn.position(index);
        index++;
    };

    current_recipe = latte;

    /* ------------------------------ main loop ----------------------------- */

    while (!WindowShouldClose())
    {

        /* -------------------------- update window ------------------------- */

        /* -------------------------- drawing loop -------------------------- */
        BeginDrawing();
        ClearBackground(Color{245, 240, 235, 255}); // offwhite (similar to RAYWHITE but orange tinted and slightly darker)

        // draw table:
        DrawRectangle(0, screenHeight - TABLE_HEIGHT, screenWidth, TABLE_HEIGHT, Color{125, 105, 85, 250});

        // std::cout << "========================================" << std::endl;
        cup.draw();
        // for (auto layer: cup.layers) {
            // std::cout << (PRETTY_INGREDIENTS.at(layer)) << std::endl;
        // };
        // std::cout << "========================================" << std::endl;

        for (auto &btn : IngBtns)
        {
            btn.draw();
            btn._handle_event();
        };

        DrawText(TextFormat("Money: $%02.02f", money), 10, 30, 24, LIME);

        EndDrawing();
    };

    CloseWindow();
    return 0;
}
#pragma endregion