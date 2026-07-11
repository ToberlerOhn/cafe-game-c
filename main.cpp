#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <ctime>
#include <functional>
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

enum class Category
{
    BASIC,
    TEA,
    DAIRY,
    FRUIT,
    ADDONS,
    TOPPINGS,
};
Category current_category = Category::BASIC;

enum class Ingredient
{
    // BASIC

    WATER,
    BEANS,

    // TEA

    MATCHA_POWDER,

    // DAIRY

    MILK,
    MILK_FOAM,
    STEAMED_MILK,

    // TOPPING

    ICE,
    WHIPPED_CREAM,
    ICE_CREAM,
};

std::vector<Ingredient> Toppings{
    Ingredient::ICE,
    Ingredient::WHIPPED_CREAM,
    Ingredient::ICE_CREAM,
};

const std::map<Ingredient, std::string> PRETTY_INGREDIENTS{
    {Ingredient::WATER, "Water"},
    {Ingredient::BEANS, "Coffee Beans"},
    {Ingredient::MATCHA_POWDER, "Matcha Powder"},
    {Ingredient::MILK, "Milk"},
    {Ingredient::MILK_FOAM, "Milk Foam"},
    {Ingredient::STEAMED_MILK, "Steamed Milk"},
    {Ingredient::ICE, "Ice"},
};

struct Palette
{
    std::vector<Color> colors;

    Color color(Ingredient ingredient) const
    {
        return colors.at((size_t)ingredient);
    };
};

const Palette palette = { // goes in Ingredient order
    {
        {120, 195, 240, 255}, // water
        {81, 50, 35, 255},    // coffee beans
        {140, 210, 150, 255}, // matcha
        {250, 245, 240, 255}, // milk
        {240, 238, 236, 255}, // milk foam
        {250, 245, 240, 255}, // steamed milk
        {41, 190, 189, 255},  // ice

    }};

/* --------------------------------- recipes -------------------------------- */

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
    Color frothed_colors[2]; // top color first, bottom color second
    bool always_hot;
};

const Recipe espresso{
    "Espresso",
    "Plain black coffee",
    3.00,
    {{Ingredient::BEANS, 0.3, "Espresso shot"}},
    {Color{125, 80, 55, 255}, Color{81, 50, 35, 255}}, true};

const Recipe flat_white{
    "Flat White",
    "A coffee with milk",
    4.00,
    {
        {Ingredient::BEANS, 0.3, "Espresso shot"},
        {Ingredient::MILK, 0.6, "Add milk"},
    },
    {Color{90, 70, 45, 255}, Color{55, 25, 15, 255}},
    false};

const Recipe latte{
    "Latte",
    "A coffee with milk and milk foam",
    4.25,
    {
        {Ingredient::BEANS, 0.3, "Espresso shot"},
        {Ingredient::MILK, 0.6, "Add milk"},
        {Ingredient::MILK_FOAM, 0.1, "Top with foam"},
    },
    {Color{100, 70, 45, 255}, Color{60, 30, 15, 255}},
    false};

const Recipe matcha{
    "Matcha",
    "Tea made from matcha powder",
    4.00,
    {{Ingredient::MATCHA_POWDER, 0.3, "Start with matcha powder"},
     {Ingredient::WATER, 0.6, "Whisk with hot water"}},
    {Color{120, 200, 150, 255}, Color{100, 170, 100, 255}},
    false,
};

const Recipe matcha_latte{
    "Matcha Latte",
    "Matcha with milk",
    4.75,
    {{Ingredient::MATCHA_POWDER, 0.2, "Start with matcha powder"},
     {Ingredient::WATER, 0.3, "Whisk with hot water"},
     {Ingredient::MILK, 0.5, "Add milk"}},
    {Color{210, 245, 215, 255}, Color{120, 200, 135, 255}},
    false};

struct RecipeState
{
    Recipe r;
    int ice;
};
RecipeState current_recipe;
const std::vector<Recipe> RECIPES{
    espresso,
    flat_white,
    latte,
    matcha,
    matcha_latte};


#pragma endregion

#pragma region functions
/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

/* ------------------------ general helper functions ------------------------ */

/// @brief
// "In mathematics, linear interpolation (sometimes lerp) is a method of curve
// fitting using linear polynomials to construct new data points within the
// range of a discrete set of known data points." - Wikipedia
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

inline float interpolate(float a, float b, float t, std::function<float(float)> f)
{
    float t1 = f(t);
    return lerp(a, b, t1);
};

/* ------------------------- raylib helper functions ------------------------ */
// These functions serve to replace the functions found in the raylib library
// and/or emulate those from pygame.

Color interpolate_color(Color a, Color b, float t);
Color interpolate_color(Color a, Color b, float t, std::function<float(float)> f);
Texture2D _LoadImage(const char *image_path);
Texture2D _LoadImage(const char *image_path, int width, int height);
void _DrawText(Font font, const char *text, int x, int y, int font_size, Color color);
void _DrawText(Font font, const char *text, int x, int y, int font_size, int anchor, Color color);

Color interpolate_color(Color a, Color b, float t)
{
    return interpolate_color(a, b, t, [](float x)
                             { return x; });
};

Color interpolate_color(Color a, Color b, float t, std::function<float(float)> f)
{
    unsigned char red = (unsigned char)interpolate((float)a.r, (float)b.r, t, f);
    unsigned char green = (unsigned char)interpolate((float)a.g, (float)b.g, t, f);
    unsigned char blue = (unsigned char)interpolate((float)a.b, (float)b.b, t, f);
    unsigned char alpha = (unsigned char)interpolate((float)a.a, (float)b.a, t, f);
    return Color{red, green, blue, alpha};
}

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

std::array<int, 2> total_recipe_layers(RecipeState *recipe)
{
    Recipe r = recipe->r;
    int liquid_total = 0;
    std::vector<RecipeStep> steps = r.steps;
    for (auto step : steps)
        liquid_total += 100 * step.amount;
    return {liquid_total, recipe->ice};
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
    float x, y, width, height;
    Rectangle rect;
    RecipeState *current_recipe;
    std::vector<float> target_lines;

    std::vector<Ingredient> liquid_layers;
    std::vector<Ingredient> toppings;
    int num_liquid_layers;
    int num_toppings;
    bool frothed;
    Color frothed_color;

    float radius;

    Cup(float _width, float _height, RecipeState *_current_recipe)
    {
        width = _width;
        height = _height;
        x = (screenWidth - width) / 2;
        y = screenHeight - TABLE_HEIGHT - height;
        rect = Rectangle{x, y, width, height};
        current_recipe = _current_recipe;
        radius = 50.0f;
    };

    void reset()
    {
        liquid_layers = {};
        toppings = {};
        num_liquid_layers = 0;
        num_toppings = 0;
        frothed = false;
        frothed_color = BLANK;
    }

    void set_recipe(RecipeState *r)
    {
        current_recipe = r;
        target_lines.clear();
        float total = 0.0f;
        for (const auto &step : current_recipe->r.steps)
        {
            total += step.amount;
            target_lines.push_back(total);
        }
    }

    void add_ingredient(Ingredient ingredient)
    {
        if (num_liquid_layers < 100 || num_toppings < 4)
        {
            if (std::find(Toppings.begin(), Toppings.end(), ingredient) != Toppings.end())
            {
                if (num_toppings < 4)
                {
                    toppings.push_back(ingredient);
                    num_toppings++;
                }
            }
            else
            {
                if (num_liquid_layers < 100)
                {
                    liquid_layers.push_back(ingredient);
                    num_liquid_layers++;
                }
            }
        }
    };

private:
    void draw_cup(float r)
    {
        double height_percent = (float)total_recipe_layers(current_recipe)[0] / 100.0f;
        float drawn_height = height_percent * height;
        float h_diff = height - drawn_height;
        int gap = 15;

        // Draw edges
        DrawLineEx(Vector2{x, y + h_diff - gap}, Vector2{x + width, y + h_diff - gap}, 5, DARKGRAY);       // top
        DrawLineEx(Vector2{x, y + h_diff - gap}, Vector2{x, y + height - r}, 5, DARKGRAY);                 // left
        DrawLineEx(Vector2{x + width, y + h_diff - gap}, Vector2{x + width, y + height - r}, 5, DARKGRAY); // right
        DrawLineEx(Vector2{x + r, y + height}, Vector2{x + width - r, y + height}, 5, DARKGRAY);           // bottom

        // Draw rounded corners
        DrawRing(Vector2{x + r, y + height - r}, r - 2.5, r + 2.5, 90, 180, 16, DARKGRAY);       // left
        DrawRing(Vector2{x + width - r, y + height - r}, r - 2.5, r + 2.5, 0, 90, 16, DARKGRAY); // right
    };

    void draw_helper_lines()
    {
        if (frothed)
            return;
        std::vector<float> pos = target_lines;
        for (float _pos : pos)
        {
            float y_loc = y + (1 - _pos) * height;
            _DrawDashedLine(Vector2{x + 5, y_loc + 5}, Vector2{x + width - 5, y_loc + 5}, 3, 6, 2, BLACK);
        }
    };

    void draw_liquid_layers()
    {
        float layer_height = height / 100;
        int current_height = y + height;
        float r = radius;
        float cup_bottom = y + height;
        for (int i = 0; i < num_liquid_layers; i++)
        {
            Color layer_color;
            if (frothed)
            {
                Color top = current_recipe->r.frothed_colors[0];
                Color bottom = current_recipe->r.frothed_colors[1];
                float percent = (float)i / (float)(num_liquid_layers - 1);
                layer_color = interpolate_color(bottom, top, percent, [](float x)
                                                { return 3 * (1 - x) * x * x + x * x * x; }); // 3(1-x)x^2+x^3 (ease-in-out)
            }
            else
            {
                // search for nearest layer change
                int j = i;
                while (j < num_liquid_layers && liquid_layers[i] == liquid_layers[j])
                    j++;

                // If they are actually the same layer (i.e. this is the top layer) don't lerp
                // Or if they are more than 5 layers apart (too far to lerp, especially for smaller layers)
                // TODO: Variable lerping?
                if (j >= num_liquid_layers || j - i > 5 || liquid_layers[i] == liquid_layers[j])
                    layer_color = palette.color(liquid_layers[i]);
                else
                {
                    int diff = j - i;
                    layer_color = interpolate_color(palette.color(liquid_layers[i]), palette.color(liquid_layers[j]), 1 - diff / 5.0f);
                };
            }

            float current_y = current_height - layer_height;

            if (current_y <= cup_bottom - r)
                DrawRectangle(x, current_height - layer_height, width, layer_height, layer_color);
            else
            {
                float dy = cup_bottom - r - current_y;
                float dx = std::sqrt(r * r - dy * dy);
                float target_w = width - 2 * r + 2 * dx;
                float target_x = x + (width - target_w) / 2.0f;

                DrawRectangle(target_x, current_y, target_w, layer_height, layer_color);
            };
            current_height -= layer_height;
        };
    };

    void draw_toppings()
    {
        int num_ice = 0;
        for (int i = 0; i < num_toppings; i++)
        {
            if (toppings[i] == Ingredient::ICE)
            {
                num_ice++;
                draw_ice(num_ice);
            };
        }
    };

    void draw_ice(int num)
    {
        float liquid_surface_y = (y + height) - (num_liquid_layers * (height / 100.0f));
        float target_ice_y = (num_liquid_layers == 0) ? (y + height - 30) : (liquid_surface_y + 10);
        if (target_ice_y < y)
            target_ice_y = y + 10;
        float cube_x = x + 15 + (num % 4) * (width / 5.0f);

        DrawRectangleRounded(Rectangle{cube_x, target_ice_y, width / 6.0f, 25}, 0.3f, 4, Color{135, 206, 250, 160});
        DrawRectangleRoundedLinesEx(Rectangle{cube_x, target_ice_y, width / 6.0f, 25}, 0.3f, 4, 1.5f, Color{255, 255, 255, 200});
    }

public:
    void draw()
    {
        draw_cup(radius);
        draw_helper_lines();
        draw_liquid_layers();
        draw_toppings();
    }

    /// @return Returns -1.0f if the drink isn't complete.
    ///
    /// Otherwise, returns the grade as a decimal percent grade
    float grade()
    {
        // Not completed:
        std::array<int, 2> total_layers = total_recipe_layers(current_recipe);
        if (num_liquid_layers < total_layers[0] || num_toppings < total_layers[1])
            return -1.0f;

        int total_error = 0;
        /* -------------------------- liquid layers ------------------------- */
        std::unordered_map<Ingredient, int> target_liquids;
        for (const auto &step : current_recipe->r.steps)
        {
            target_liquids[step.ingredient] = (int)(step.amount * 100);
        }

        std::unordered_map<Ingredient, int> actual_liquids;
        for (auto ing : liquid_layers)
        {
            actual_liquids[ing]++;
        }

        for (const auto &pair : target_liquids)
        {
            const auto &ing = pair.first;
            const auto &target_layer_count = pair.second;
            int poured = actual_liquids[ing];
            total_error += std::abs(target_layer_count - poured);
        }

        for (const auto &pair : target_liquids)
        {
            const auto &ing = pair.first;
            const auto &poured_count = pair.second;
            if (target_liquids.find(ing) == target_liquids.end())
                total_error += poured_count;
        }

        /* ------------------------ toppings grading ------------------------ */

        int actual_ice = 0;
        for (auto t : toppings)
        {
            if (t == Ingredient::ICE)
                actual_ice++;
        };

        int target_ice = current_recipe->ice;
        int ice_error = std::abs(target_ice - actual_ice) * 10;
        total_error += ice_error;

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
    float x;
    float y;
    float width;
    float height;
    Rectangle rect;
    bool hovered;

    Button(float _x, float _y, float _width, float _height)
    {
        x = _x;
        y = _y;
        width = _width;
        height = _height;
        rect = Rectangle{x, y, width, height};
    };

    void update_hover()
    {
        Vector2 mouse_pos = GetMousePosition();
        hovered = CheckCollisionPointRec(mouse_pos, rect);
    };
};

class CategoryButton : public Button
{
public:
    Category category;
    std::string name;
    bool selected;

    CategoryButton(Category _category)
        : Button(25 + (int)_category * 100, 50, 75, 50)
    {
        category = _category;
        switch ((int)category)
        {
        case 0:
            name = "Basic";
            break;
        case 1:
            name = "Tea";
            break;
        case 2:
            name = "Dairy";
            break;
        case 3:
            name = "Fruit";
            break;
        case 4:
            name = "Addons";
            break;
        case 5:
            name = "Toppings";
            break;
        }
    };

    void _handle_event()
    {
        selected = (category == current_category);
        update_hover();
        if (hovered)
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                selected = true;
        }

        if (selected)
            current_category = category;
    };

    void draw()
    {
        Color color = hovered ? Color{250, 225, 195, 255} : selected ? Color{230, 220, 210, 255}
                                                                     : Color{237, 222, 202, 255};
        DrawRectangleRounded(rect, 0.5, 10, color);
        if (selected)
            DrawRectangleRoundedLinesEx(rect, 0.5, 10, 2, Color{170, 180, 230, 255});
        _DrawText(F_RALEWAY, name.c_str(), rect.x + rect.width / 2, rect.y + rect.height / 2 - 10, 20, Color{30, 20, 10, 255});
    }
};

class IngredientButton : public Button
{
public:
    Ingredient ingredient;
    Category category;
    Texture2D image;
    std::string name;

    IngredientButton(Ingredient _ingredient, Category _category, Texture2D _image, float _width, float _height)
        : Button(0, 0, _width, _height),
          ingredient(_ingredient),
          category(_category),
          image(_image),
          name(PRETTY_INGREDIENTS.at(_ingredient)) {};

    void set_texture(Texture2D _image)
    {
        image = _image;
    };

    void position(int number)
    {
        x = 35 + (number % 2) * (width + 25);
        y = 125 + (number / 2) * (height + 25);
        rect = Rectangle{x, y, width, height};
    };

    void _handle_event()
    {
        update_hover();
        if (hovered)
        {
            bool is_topping = (category == Category::TOPPINGS);
            bool input_triggered;

            input_triggered = is_topping ? IsMouseButtonPressed(MOUSE_BUTTON_LEFT) : IsMouseButtonDown(MOUSE_BUTTON_LEFT);

            if (input_triggered)
                cup.add_ingredient(ingredient);
            DrawRectangleRoundedLinesEx(Rectangle{x - 5, y - 5, width + 10, height + 10}, 0.5, 5, 2, palette.color(ingredient));
        }
    }

    void draw()
    {
        if (current_category == category)
        {
            DrawTexture(image, x, y, WHITE);
            _DrawText(F_RALEWAY, name.c_str(), x + width / 2, y + height + 5, 15, DARKGRAY);
        }
    };
};

class Receipt
{
public:
    RecipeState *current_recipe;
    int *order_number;
    float x, y, width, height;
    Rectangle rect;
    Vector2 origin;
    Vector2 mid;

    Receipt(RecipeState *_recipe, int *_order_num)
    {
        current_recipe = _recipe;
        order_number = _order_num;
        width = 300;
        height = 500;
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

    std::array<int, 2> draw_text_content()
    {
        int offset_d = 0;
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
        _DrawText(F_RALEWAY, current_recipe->r.name.c_str(), mid.x, y + 70, 24, Color{50, 30, 25, 255});

        // Draw toppings if necessary
        // ice
        int ice = current_recipe->ice;
        std::string ice_txt = "";
        switch (ice)
        {
        case 0:
            ice_txt = "";
            break;
        case 1:
            ice_txt = "light ice";
            break;
        case 2:
            ice_txt = "regular ice";
            break;
        case 3:
            ice_txt = "extra ice";
            break;
        case 4:
            ice_txt = "heavy ice";
            break;
        };
        if (ice_txt != "")
        {
            _DrawText(F_RALEWAY, ("With " + ice_txt).c_str(), mid.x, y + 90, 14, Color{70, 45, 45, 255});
            offset_d += 15;
        };

        // Draw description
        std::vector<std::string> description = _WrapText(F_RALEWAY_I, current_recipe->r.description, 16, width - 10);
        for (int i = 0; i < (int)description.size(); i++)
        {
            _DrawText(F_RALEWAY_I, description[i].c_str(), mid.x, y + 90 + offset_d + 18 * i, 16, Color{60, 40, 40, 255});
        }
        offset_d += 18 * (description.size() - 1);

        // Draw steps + amounts
        int step_idx = 0;
        int offset_i = 0;
        for (auto step : current_recipe->r.steps)
        {
            std::string description = step.description;
            std::string ingredient = PRETTY_INGREDIENTS.at(step.ingredient);
            int amount = (int)std::round(step.amount * 100);
            std::string str_amount = TextFormat("%i", amount);
            std::string txt = description + "  (" + str_amount + "% " + ingredient + ')';

            _DrawText(F_RALEWAY, txt.c_str(), x + 5, y + 120 + offset_d + 20 * step_idx, 14, -1, Color{30, 30, 30, 255});
            step_idx++;
        }
        if (ice_txt != "")
        {
            _DrawText(F_RALEWAY, ("Add " + std::to_string(ice) + " ice cubes").c_str(), x + 5, y + 120 + offset_d + 20 * step_idx, 14, -1, Color{30, 30, 30, 255});
            step_idx++;
        }
        offset_i += (step_idx - 1) * 20;

        // Draw price
        std::string price_str = TextFormat("Total: $%01.02f", (float)current_recipe->r.cost);
        _DrawText(F_RALEWAY, price_str.c_str(), x + 5, y + height - 40, 24, -1, Color{20, 15, 10, 255});

        return {offset_d, offset_i};
    };

    void draw_dashed_lines(int offset1, int offset2)
    {
        std::vector<float> y_levels = {y + 70.0f, y + 110.0f + offset1, y + 165.0f + offset2};
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
        std::array<int, 2> offsets = draw_text_content();
        draw_dashed_lines(offsets[0], offsets[1]);
    };
};

class Customizer
{
public:
    float *ptr;
    float x;
    float y;
    float min;
    float max;
    float width;
    float height;
    bool dragging;

    Customizer(float *_ptr, float _x, float _y, float _min, float _max, float _width)
    {
        ptr = _ptr;
        x = _x;
        y = _y;
        min = _min;
        max = _max;
        width = _width;
        height = 5;
        dragging = false;
    };

    float get_px()
    {
        if (max == min)
            return x;
        return x + width * ((*ptr - min) / (max - min));
    };

    void update()
    {
        Vector2 mouse_pos = GetMousePosition();
        Rectangle hitbox = Rectangle{x, y - 5, width, height + 10};
        bool hovered = CheckCollisionPointRec(mouse_pos, hitbox);
        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            dragging = true;

        if (dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            float clamped_x = Clamp(mouse_pos.x, x, x + width);
            *ptr = min + ((clamped_x - x) / width) * (max - min);
            *ptr = Clamp(*ptr, min, max);

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                dragging = false;
        }
    };

    void draw()
    {
        DrawRectangle(x, y, width, height, DARKGRAY);
        DrawCircle(get_px(), y, height * 2, ORANGE);
        DrawText(TextFormat("%.2f", *ptr), x, y + 10, 20, DARKGRAY);
    };
};

// define ranomize_recipe() after completing cup but before Cup::serve
void randomize_recipe()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, RECIPES.size() - 1);

    int random_index = distr(gen);
    int ice = rand() % 5; // random number between 0 & 4
    if (RECIPES[random_index].always_hot)
        ice = 0;
    current_recipe = {RECIPES[random_index], ice};

    cup.set_recipe(&current_recipe);
    receipt.current_recipe = &current_recipe;
};

// define the method outside of the class in order to have access to buttons
void Cup::serve(float grade)
{
    std::array<int, 2> total_layers = total_recipe_layers(current_recipe);
    if (num_liquid_layers < total_layers[0] || num_toppings < total_layers[1])
        return;
    Color arrow_color;

    frothed = true;
    Button ArrowHitbox(screenWidth - 200, screenHeight - 200, 100, 50);
    ArrowHitbox.update_hover();
    arrow_color = GOLD;
    arrow_color = ArrowHitbox.hovered ? ORANGE : GOLD;
    if (ArrowHitbox.hovered)
        arrow_color = ORANGE;
    if ((ArrowHitbox.hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ENTER))
    {
        reset();
        money += grade * current_recipe->r.cost;
        number_of_orders += 1;
        randomize_recipe();
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

/* ---------------------------- category buttons ---------------------------- */

CategoryButton BasicBtn(Category::BASIC);
CategoryButton TeaBtn(Category::TEA);
CategoryButton DairyBtn(Category::DAIRY);
CategoryButton FruitBtn(Category::FRUIT);
CategoryButton AddonsBtn(Category::ADDONS);
CategoryButton ToppingsBtn(Category::TOPPINGS);
CategoryButton CatBtns[6] = {BasicBtn, TeaBtn, DairyBtn, FruitBtn, AddonsBtn, ToppingsBtn};
int NumCatBtns = sizeof(CatBtns) / sizeof(CategoryButton);

/* --------------------------- ingredient buttons --------------------------- */

IngredientButton WaterBtn(Ingredient::WATER, Category::BASIC, Texture2D{}, 64, 64);
IngredientButton BeansBtn(Ingredient::BEANS, Category::BASIC, Texture2D{}, 64, 64);
IngredientButton MatchaBtn(Ingredient::MATCHA_POWDER, Category::TEA, Texture2D{}, 64, 64);
IngredientButton MilkBtn(Ingredient::MILK, Category::DAIRY, Texture2D{}, 64, 64);
IngredientButton MilkFoamBtn(Ingredient::MILK_FOAM, Category::DAIRY, Texture2D{}, 64, 64);
IngredientButton IceBtn(Ingredient::ICE, Category::TOPPINGS, Texture2D{}, 64, 64);
IngredientButton IngBtns[6] = {WaterBtn, BeansBtn, MatchaBtn, MilkBtn, MilkFoamBtn, IceBtn};
int NumIngBtns = sizeof(IngBtns) / sizeof(IngredientButton);

/* -------------------------------------------------------------------------- */

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

    IngBtns[0].set_texture(_LoadImage("images/water_sprite_1.png", 64, 64));
    IngBtns[1].set_texture(_LoadImage("images/beans_sprite_1.png", 64, 64));
    IngBtns[2].set_texture(_LoadImage("images/match_sprite_1a.png", 64, 64));
    IngBtns[3].set_texture(_LoadImage("images/milk_sprite_1.png", 64, 64));
    IngBtns[4].set_texture(_LoadImage("images/milk foam_sprite_1.png", 64, 64));
    IngBtns[5].set_texture(_LoadImage("images/ice_sprite_1.png", 64, 64));
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

        if (IsKeyPressed(KEY_R)) randomize_recipe();

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

        for (auto &btn : CatBtns)
        {
            btn._handle_event();
            btn.draw();
        };

        std::vector<IngredientButton> VisIngBtns = {};
        for (auto &btn : IngBtns)
        {
            if (btn.category == current_category)
                VisIngBtns.push_back(btn);
        };
        int VIB_idx = 0;
        for (auto &btn : VisIngBtns)
        {
            btn.position(VIB_idx);
            btn.draw();
            btn._handle_event();
            VIB_idx++;
        }

        receipt.draw();

        _DrawText(F_RALEWAY, TextFormat("Money: $%02.02f", money), 25, 10, 36, -1, LIME);

        EndDrawing();
    };

    CloseWindow();
    return 0;
}
#pragma endregion