#include "../raylib-5.0_linux_amd64/include/raylib.h"
#include <assert.h>
#define RAYGUI_IMPLEMENTATION
#include "./raygui.h"
#undef RAYGUI_IMPLEMENTATION            // Avoid including raygui implementation again

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define da_append(dynamic_array, item)                                                                                              \
    do {                                                                                                                            \
        if ((dynamic_array)->count >= (dynamic_array)->capacity) {                                                                  \
            if ((dynamic_array)->capacity == 0) (dynamic_array)->capacity = 128;                                                    \
            else (dynamic_array)->capacity *= 2;                                                                                    \
            (dynamic_array)->items = realloc((dynamic_array)->items, (dynamic_array)->capacity*sizeof(*(dynamic_array)->items));    \
        }                                                                                                                           \
        (dynamic_array)->items[(dynamic_array)->count++] = (item);                                                                  \
    } while (0)

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} DynamicArray;

enum SqldiffOptions{
    SUMMARY = 0,
    TABLE,
    PRIMARY_KEY,
    TRANSACTION
};

typedef struct {
    char *sqldiff_output;
    char *source_db_path;
    char *destination_db_path;
    char **destinationBuffer;
    GuiWindowFileDialogState fileDialogState;
} State;

State *state = NULL;
void init(void **_state);
void update(void);
void clear(void);
char* get_sqldiff_output(void);
void show_sqldiff_output_to_screen(void);
void choose_databases(void);
void button(const char* button_text, int button_width, int button_height, int button_xPos, int button_yPos, Color button_color, Color text_color, void (*onClick)(void));
void init_file_dialog();
void file_dialog(void);
void set_source_db_path(void);
void set_destination_db_path(void);

void init(void **_state) {
    if(*_state == NULL) {
        state = calloc(1, sizeof(*state));
        *_state = state;
    }
    else {
        state = *_state;
    }
}

void update(void) {
    assert(state != NULL && "Init state");
    BeginDrawing();
        Color backgroundImage = GetColor(0x191919);
        ClearBackground(backgroundImage);
        choose_databases();
        // TODO: FIX RENDERING ISSUE
        // if(IsKeyPressed(KEY_F11)) ToggleFullscreen();
        if (IsKeyPressed(KEY_D) && state->source_db_path != NULL && state->destination_db_path != NULL) state->sqldiff_output = get_sqldiff_output(); 
        if(state->sqldiff_output != NULL) show_sqldiff_output_to_screen();
        const int height = (GetScreenHeight()*0.85)-(10*2);
        DrawLine(0, height+10, GetScreenWidth(), height+10, WHITE);
        file_dialog();
    EndDrawing();
}

void clear(void) {
    free(state);
}

char* get_sqldiff_output(void) {
    DynamicArray sqldiff_output = {0};
    // TODO: Fix: Command Injection
    const char *base_command = "sqldiff ";
    size_t total_length = strlen(base_command) + strlen(state->source_db_path) + strlen(state->destination_db_path) + 2 + 1; // +2 for spaces, +1 for null terminator
    char *command = (char *)malloc(total_length * sizeof(char));
    strcpy(command, base_command);
    strcat(command, state->source_db_path);
    strcat(command, " ");
    strcat(command, state->destination_db_path);
    FILE *stdout_stream = popen(command, "r");

    if(stdout_stream == NULL) {
        printf("Failed reading from sqldiff stdout stream\n");
        return NULL;
    }

    char ch;
    while ((ch = fgetc(stdout_stream)) != EOF)
    {
        da_append(&sqldiff_output, ch);
    }

    // Add string terminator
    da_append(&sqldiff_output, '\0');

    char *result = strdup(sqldiff_output.items);
    free(sqldiff_output.items);

    pclose(stdout_stream);
    return result;
}

void show_sqldiff_output_to_screen(void) {
    const int x = 10;
    const int y = 10;
    const int width = GetScreenWidth()-x*2;
    const int height = GetScreenHeight()*0.85-y*2;
    const int font_size = 12;
    // TODO: Add horizontal scrolling
    // TODO: Scroll using mouse drag
    // TODO: Add offest to global state 
    static float vertical_scroll_offset = 0;
    static float horizontal_scroll_offset = 0;
    static float velocity = 0;
    if(!state->fileDialogState.windowActive) {
        velocity *= 0.9;
        velocity += GetMouseWheelMove()*88;
        vertical_scroll_offset += velocity*GetFrameTime();
        // TODO: Clamp the scrollable area for down and right
        if(vertical_scroll_offset > 0) vertical_scroll_offset = 0; 
        if(horizontal_scroll_offset > 0) horizontal_scroll_offset = 0;
    }

    const int text_x_position = x+horizontal_scroll_offset;
    const int text_y_position = y+vertical_scroll_offset;

    char *str_copy = strdup(state->sqldiff_output);
    char *rest;
    char *token = strtok_r(str_copy, "\n", &rest);
    int line_number = 0;

    BeginScissorMode(x, y, width, height);
        while (token != NULL)
        { 
            DrawText(token, text_x_position, text_y_position + ((font_size+5) * line_number++), font_size, WHITE);
            token = strtok_r(NULL, "\n", &rest);
        }
        free(str_copy);
    EndScissorMode();
}

void choose_databases(void) {
    const int upper_boundary = GetScreenHeight()*0.85;
    // const int bottom_boundary = GetScreenHeight();
    // const int button_padding = 20;
    const int button_margin = 15;
    // int source_buttonm_width;
    const int button_text_font_size = 15;
    const int button_xPos = 20;
    const int button_yPos = upper_boundary + button_margin;
    button("Choose Sorce DB", 150, button_text_font_size, button_xPos, button_yPos, SKYBLUE, WHITE, set_source_db_path);
    button("Choose Destination DB", 175, button_text_font_size, button_xPos + 150 + button_margin, button_yPos, SKYBLUE, WHITE, set_destination_db_path);
}

// TODO: Collect button props in a struct
// TODO: We might want to pass args to onclick
void button(const char* button_text, int button_width, int button_height, int button_xPos, int button_yPos, Color button_color, Color text_color, void (*onClick)(void)) {
    const int button_padding = 14;
    // const int button_margin = 15
    int text_width = MeasureText(button_text, button_height);
    DrawRectangle(button_xPos, button_yPos, button_width, button_height + button_padding, button_color);
    DrawText(button_text, button_xPos + ((button_width - text_width)/2), button_yPos + ((button_padding)/2), button_height, text_color);
    bool clicked = (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), (Rectangle){ button_xPos, button_yPos, button_width, button_height + button_padding}));
    if(clicked) if(onClick != NULL) onClick();
}

void init_file_dialog() {
    state->fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());
    state->fileDialogState.windowActive = true;
}

void set_source_db_path(void) {
    state->destinationBuffer = &state->source_db_path;
    init_file_dialog();
}

void set_destination_db_path(void) {
    state->destinationBuffer = &state->destination_db_path;
    init_file_dialog();
}

void file_dialog() {
    if(!state->fileDialogState.windowActive && !state->fileDialogState.SelectFilePressed) {
        state->destinationBuffer = NULL;
        state->fileDialogState.windowActive = false;
    }
    if(state->fileDialogState.SelectFilePressed && state->destinationBuffer != NULL) {
        *state->destinationBuffer = strdup(TextFormat("%s" PATH_SEPERATOR "%s", state->fileDialogState.dirPathText, state->fileDialogState.fileNameText));
        state->destinationBuffer = NULL;
        state->fileDialogState.windowActive = false;
        state->fileDialogState.SelectFilePressed = false;
    }
    if(state->fileDialogState.windowActive) GuiWindowFileDialog(&state->fileDialogState); 
}
