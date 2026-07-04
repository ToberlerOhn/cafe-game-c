#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <ctime>
#include <map>
#include <sstream>
#include <random>
#include <unordered_map>
#include <vector>

#define LEFT -1
#define CENTER 0
#define RIGHT 1

#define TABLE_HEIGHT 100

#pragma region global variables
/* -------------------------------------------------------------------------- */
/*                              Global Variables                              */
/* -------------------------------------------------------------------------- */
// This section also contains the structures for the palette, recipe, etc...

const int screenWidth = 1500, screenHeight = 900;

float money;
int number_of_orders = 1;

Font F_RALEWAY = {};
Font F_RALEWAY_I = {};
Font F_MONO = {};

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
    }};

struct RecipeStep
{
    Ingredient ingredient;
    double amount;
    std::string description;
};

struct Recipe
{
    std::string name;
    std::string description;
    double cost;
    std::vector<RecipeStep> steps;
    Color frothed_color;
    bool always_hot;
};

const Recipe espresso{
    "Espresso",
    "Plain black coffee",
    3.00,
    {{Ingredient::ESPRESSO, 0.3, "Espresso shot"}},
    palette.color(Ingredient::ESPRESSO),
    true};

const Recipe latte{
    "Latte",
    "A coffee with milk",
    4.25,
    {
        {Ingredient::ESPRESSO, 0.3, "Espresso shot"},
        {Ingredient::MILK, 0.6, "Add milk"},
        {Ingredient::MILK_FOAM, 0.1, "Top with foam"},
    },
    palette.color(Ingredient::ESPRESSO),
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
    Vector2 length = MeasureTextEx(font, text, (float)font_size, 1.0f);
    float mid_x = (float)x - length.x / 2;
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
        DrawTextEx(font, text, Vector2{(float)x - length.x, (float)y}, font_size, 1, color);
        break;
    }
}

std::vector<std::string> _SplitText(const std::string &text, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string token;

    while (std::getline(ss, token, delimiter))
        tokens.push_back(token);

    return tokens;
};

std::vector<std::string> _WrapText(Font font, std::string text, int font_size, int max_width)
{
    std::vector<std::string> words = _SplitText(text, ' ');
    std::vector<std::string> lines = {};
    std::string current_line = "";
    for (auto word : words)
    {
        std::string test = current_line + ' ' + word;
        float length = MeasureTextEx(font, test.c_str(), font_size, 1).x;
        if (length <= max_width)
            current_line = test;
        else
        {
            if (!current_line.empty())
                lines.push_back(current_line);
            current_line = word;
        };
    }
    if (!current_line.empty())
        lines.push_back(current_line);
    return lines;
};

void _DrawDashedLine(Vector2 startPos, Vector2 endPos, int dashSize, int gapSize, float thickness, Color color)
{
    float dx = endPos.x - startPos.x;
    float dy = endPos.y - startPos.y;
    float distance = std::sqrtf(dx * dx + dy * dy);
    if (distance == 0.0f)
        return;
    float dirX = dx / distance;
    float dirY = dy / distance;

    float currentDist = 0.0f;
    while (currentDist < distance)
    {
        float nextDist = currentDist + dashSize;
        if (nextDist > distance)
            nextDist = distance;

        Vector2 segStart = {startPos.x + dirX * currentDist, startPos.y + dirY * currentDist};
        Vector2 segEnd = {startPos.x + dirX * nextDist, startPos.y + dirY * nextDist};

        DrawLineEx(segStart, segEnd, thickness, color);

        currentDist += dashSize + gapSize;
    }
}

/// @brief
// Overload of raylib's DrawRectanglePro
//
// Takes in an origin and offsets points of the rectangle relative to the origin
void DrawRectanglePro(Vector2 origin, Rectangle rect, Color color)
{
    DrawRectangleRec(Rectangle{rect.x + origin.x, rect.y + origin.y, rect.width, rect.height}, color);
};

/* ----------------------------- game functions ----------------------------- */

int total_recipe_layers(Recipe *r)
{
    int total = 0;
    std::vector<RecipeStep> steps = r->steps;
    for (auto step : steps)
        total += 100 * step.amount;
    return total;
};

class Cup;
extern Cup cup;
class Receipt;
extern Receipt receipt;
void randomize_recipe();

#pragma endregion

#pragma region classes
/* -------------------------------------------------------------------------- */
/*                                   Classes                                  */
/* -------------------------------------------------------------------------- */

class Cup
{
public:
    int x, y, width, height;
    Rectangle rect;
    Recipe *current_recipe;
    std::vector<float> target_lines;

    std::vector<Ingredient> layers;
    int num_layers;
    bool frothed;
    Color frothed_color;

    Cup(int _width, int _height, Recipe *_current_recipe)
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
    }

    void set_recipe(Recipe *r)
    {
        current_recipe = r;
        target_lines.clear();
        float total = 0.0f;
        for (const auto &step : current_recipe->steps)
        {
            total += step.amount;
            target_lines.push_back(total);
        }
    }

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

    void draw_helper_lines()
    {
        if (frothed)
            return;
        std::vector<float> pos = target_lines;
        for (float _pos : pos)
        {
            float y_loc = y + (1 - _pos) * height;
            _DrawDashedLine(Vector2{(float)x + 5, y_loc + 5}, Vector2{(float)x + width - 5, y_loc + 5}, 3, 6, 2, BLACK);
        }
    };

    void draw_layers()
    {
        float layer_height = height / 100;
        int current_height = y + height;
        for (auto layer : layers)
        {
            Color layer_color;
            if (frothed)
                layer_color = current_recipe->frothed_color;
            else
                layer_color = palette.color(layer);
            DrawRectangle(x + 5, current_height - layer_height, width - 10, layer_height, layer_color);
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

    /* @return Returns -1.0f if the drink isn't complete,

    Otherwise, returns the grade as a decimal percent grade*/
    float grade()
    {
        // Not completed:
        if (num_layers < total_recipe_layers(current_recipe))
            return -1.0f;

        // Actually grade:
        std::unordered_map<Ingredient, int> target_counts;
        for (const auto &step : current_recipe->steps)
        {
            target_counts[step.ingredient] = (int)(step.amount * 100);
        }

        std::unordered_map<Ingredient, int> actual_counts;
        for (auto ing : layers)
        {
            actual_counts[ing]++;
        }

        int total_error = 0;

        for (const auto &pair : target_counts)
        {
            const auto &ing = pair.first;
            const auto &target_layer_count = pair.second;
            int poured = actual_counts[ing];
            total_error += std::abs(target_layer_count - poured);
        }

        for (const auto &pair : target_counts)
        {
            const auto &ing = pair.first;
            const auto &poured_count = pair.second;
            if (target_counts.find(ing) == target_counts.end())
                total_error += poured_count;
        }

        float score = 1.0f - (float)total_error / 100.0f;
        if (score < 0.0f)
            score = 0.0f;

        return score;
    };

    void serve(float grade);
};

// Base Button class
class Button
{
public:
    int x;
    int y;
    int width;
    int height;
    Rectangle rect;
    bool hovered;

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
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                cup.add_ingredient(ingredient);
            DrawRectangle(x, y, width, height, Color{245, 240, 235, 128});
        }
    }

    void draw()
    {
        DrawRectangleRec(rect, BLANK);
        DrawTexture(image, x, y, WHITE);
        _DrawText(F_RALEWAY, name.c_str(), x + width / 2, y + height + 5, 15, DARKGRAY);
    };
};

class Receipt
{
public:
    Recipe *current_recipe;
    int *order_number;
    float x, y, width, height;
    Rectangle rect;
    Vector2 origin;
    Vector2 mid;

    Receipt(Recipe *_recipe, int *_order_num)
    {
        current_recipe = _recipe;
        order_number = _order_num;
        width = 200;
        height = 320;
        x = screenWidth - width - 50;
        y = 50;
        rect = {x, y, width, height};
        origin = {x, y};
        mid = {x + width / 2, y + height / 2};
    };

private:
    void draw_base()
    {
        DrawRectangle(x, y, width, height, Color{230, 225, 205, 255});
        // DrawRectanglePro(origin, Rectangle{0, 0, width, height}, Color{230, 225, 205, 255});
        DrawRectanglePro(origin, Rectangle{0, 0, width, 36}, Color{175, 145, 110, 255});
    };

    int draw_text_content()
    {
        int offset = 0;
        _DrawText(F_RALEWAY, "Toby's Cafe", mid.x, y + 5, 30, Color{40, 20, 20, 255});

        // date/time + order number
        time_t timestamp = time(NULL);
        struct tm datetime = *localtime(&timestamp);
        char date_text[35];
        strftime(date_text, 35, "%I:%M %p\n%a %b %d, %Y", &datetime);
        const char *order_text = TextFormat("#%02i", *order_number);
        _DrawText(F_RALEWAY, date_text, x + 5, y + 35, 16, -1, Color{40, 20, 20, 255});
        _DrawText(F_RALEWAY, order_text, x + width - 5, y + 35, 16, 1, Color{40, 20, 20, 255});

        // Draw title
        _DrawText(F_RALEWAY, current_recipe->name.c_str(), mid.x, y + 70, 24, Color{50, 30, 25, 255});

        // Draw description
        std::vector<std::string> description = _WrapText(F_RALEWAY_I, current_recipe->description, 16, width - 10);
        for (int i = 0; i < (int)description.size(); i++)
        {
            _DrawText(F_RALEWAY_I, description[i].c_str(), mid.x, y + 90 + 18*i, 16, Color{60, 40, 40, 255});
        }
        offset += 18 * (description.size() - 1);

        // Draw steps + amounts
        int step_idx = 0;
        for (auto step : current_recipe->steps) {
            std::string description = step.description;
            std::string ingredient  = PRETTY_INGREDIENTS.at(step.ingredient);
            int amount = (int)std::round(step.amount * 100);
            std::string str_amount = TextFormat("%i", amount);
            std::string txt = description + "   (" + str_amount + "% " + ingredient + ')';
            
            _DrawText(F_RALEWAY, txt.c_str(), x + 5, y + 120 + offset + 20*step_idx, 14, -1, Color{30, 30, 30, 255});
            step_idx++;
        }



        return offset;
    };

    void draw_dashed_lines(int offset) 
    {
        std::vector<float> y_levels = {y + 70.0f, y + 110.0f + offset};
        for (float y_ : y_levels)
        {
            _DrawDashedLine(Vector2{x + 5.0f, y_}, Vector2{x + width - 5.0f, y_}, 4, 8, 1, Color{30, 20, 20, 200});
            _DrawDashedLine(Vector2{x + 5.0f, y_}, Vector2{x + width - 5.0f, y_}, 4, 8, 1, Color{30, 20, 20, 200});
        };
    };

public:
    void draw()
    {
        draw_base();
        int offset = draw_text_content();
        draw_dashed_lines(offset);
    };
};

// define ranomize_recipe() after completing cup but before Cup::serve
void randomize_recipe()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, RECIPES.size() - 1);

    int random_index = distr(gen);
    auto it = std::next(RECIPES.begin(), random_index);

    current_recipe = it->second;
    cup.set_recipe(&current_recipe);
    receipt.current_recipe = &current_recipe;
};

// define the method outside of the class in order to have access to buttons
void Cup::serve(float grade)
{
    int total_layers = total_recipe_layers(current_recipe);
    if (num_layers < total_layers)
        return;
    Color arrow_color;

    frothed = true;
    Button ArrowHitbox(screenWidth - 200, screenHeight - 200, 100, 50);
    ArrowHitbox.update_hover();
    arrow_color = GOLD;
    if (ArrowHitbox.hovered)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            reset();
            money += grade * current_recipe->cost;
            number_of_orders += 1;
            randomize_recipe();
        };
        arrow_color = ORANGE;
    };

    DrawRectangle((int)(ArrowHitbox.x),
                  (int)(ArrowHitbox.y + ArrowHitbox.height / 3),
                  (int)(ArrowHitbox.width * 2 / 3),
                  (int)(ArrowHitbox.height / 3),
                  arrow_color);
    DrawTriangle(Vector2{(float)(ArrowHitbox.x + ArrowHitbox.width * 2 / 3), (float)(ArrowHitbox.y)},
                 Vector2{(float)(ArrowHitbox.x + ArrowHitbox.width * 2 / 3), (float)(ArrowHitbox.y + ArrowHitbox.height)},
                 Vector2{(float)(ArrowHitbox.x + ArrowHitbox.width), (float)(ArrowHitbox.y + ArrowHitbox.height / 2)},
                 arrow_color);
    _DrawText(F_RALEWAY, "Serve", ArrowHitbox.x + 20, ArrowHitbox.y + ArrowHitbox.height / 2 - 8, 16, -1, DARKGRAY);
};

#pragma endregion

#pragma region objects

/* -------------------------------------------------------------------------- */
/*                                   Objects                                  */
/* -------------------------------------------------------------------------- */

Cup cup(150, 300, &current_recipe);
IngredientButton EspressoBtn(Ingredient::ESPRESSO, Texture2D{}, 0, 0, 50, 50);
IngredientButton MilkBtn(Ingredient::MILK, Texture2D{}, 0, 0, 50, 50);
IngredientButton MilkFoamBtn(Ingredient::MILK_FOAM, Texture2D{}, 0, 0, 50, 50);
IngredientButton IngBtns[3] = {EspressoBtn, MilkBtn, MilkFoamBtn};
int NumIngBtns = sizeof(IngBtns) / sizeof(IngBtns[0]);

Receipt receipt(&current_recipe, &number_of_orders);

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
    // ToggleBorderlessWindowed();
    SetTargetFPS(60);

    F_RALEWAY = LoadFont("resources/Raleway-Regular.ttf");
    F_RALEWAY_I = LoadFont("resources/Raleway-Italic.ttf");
    F_MONO = LoadFont("/resources/Andale Mono.ttf");

    IngBtns[0].set_texture(_LoadImage("images/espresso.png", 50, 50));
    IngBtns[1].set_texture(_LoadImage("images/milk.png", 50, 50));
    IngBtns[2].set_texture(_LoadImage("images/milk foam.png", 50, 50));
    int index = 0;
    for (auto &btn : IngBtns)
    {
        btn.position(index);
        index++;
    };

    randomize_recipe();
    cup.reset();
    money = 0.0f;

    /* ------------------------------ main loop ----------------------------- */

    while (!WindowShouldClose())
    {

        /* -------------------------- update window ------------------------- */

        /* -------------------------- drawing loop -------------------------- */
        BeginDrawing();
        ClearBackground(Color{245, 240, 235, 255}); // offwhite (similar to RAYWHITE but orange tinted and slightly darker)

        // draw table:
        DrawRectangle(0, screenHeight - TABLE_HEIGHT, screenWidth, TABLE_HEIGHT, Color{125, 105, 85, 250});

        cup.draw();
        float grade = cup.grade();
        if (grade != -1.0f)
        {
            float percent_grade = grade * 100.0f;
            _DrawText(F_RALEWAY, TextFormat("%i%s", (int)percent_grade, "%"), screenWidth / 2, screenHeight - 75, 24, DARKGRAY);
        };
        cup.serve(grade);

        for (auto &btn : IngBtns)
        {
            btn.draw();
            btn._handle_event();
        };

        receipt.draw();

        _DrawText(F_RALEWAY, TextFormat("Money: $%02.02f", money), 10, 30, 36, -1, LIME);

        EndDrawing();
    };

    CloseWindow();
    return 0;
}
#pragma endregion