#include <stdio.h>
#include <raylib.h>

/* -------------------------------------------------------------------------- */
/*                              Global Variables                              */
/* -------------------------------------------------------------------------- */

const int screenWidth = 900, screenHeight = 600;

int main(void) {

    /* --------------------------- initialization --------------------------- */

    InitWindow(screenWidth, screenHeight, "Café Game");
    SetTargetFPS(60);

    /* ------------------------------ main loop ----------------------------- */

    while (!WindowShouldClose()) {

        /* -------------------------- update window ------------------------- */
    
        /* -------------------------- drawing loop -------------------------- */
        
        BeginDrawing();
            ClearBackground(RAYWHITE);
        EndDrawing();
    };
}