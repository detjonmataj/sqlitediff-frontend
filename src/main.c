#include "../raylib-5.0_linux_amd64/include/raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

void *libCore = NULL;
void *state = NULL;
void (*libcore_init)(void*);
void (*libcore_update)(void);
void (*libcore_clear)(void);

bool libcore_load(void);
bool libcore_reload(void);
bool libcore_unload(void);

int main(void) {
    const int screenWidth  = 800;
    const int screenHeight = 600;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);
    InitWindow(screenWidth, screenHeight, "SQLiteDiff Frontend");

    if(!libcore_load()) printf("Couldn't load libcore.\n DL_ERR: %s", dlerror());

    while(!WindowShouldClose()) {
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_R) && !libcore_reload()) {
            printf("Couldn't reload libcore.\n DL_ERR: %s", dlerror());
            return 1;
        }
        libcore_update();
    }

    libcore_clear();
    CloseWindow();

    return 0;
}

bool libcore_load(void) {
    libCore = dlopen("./bin/libcore.so", RTLD_NOW);
    libcore_init = dlsym(libCore, "init");
    libcore_update = dlsym(libCore, "update");
    libcore_clear = dlsym(libCore, "clear");
    libcore_init(&state);
    return libCore != NULL && libcore_init != NULL && libcore_update != NULL && libcore_clear != NULL;
}

bool libcore_reload(void) {
    if(libCore != NULL) dlclose(libCore);
    return libcore_load();
}
