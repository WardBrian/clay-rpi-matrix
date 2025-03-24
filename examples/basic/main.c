#define CLAY_IMPLEMENTATION
#include "../../src/clay_renderer_matrix.c"
#include <signal.h>
#include <time.h>

#define FONT_ID_4X6 0
#define FONT_ID_5X7 1

static volatile int keepRunning = 1;
void exitCtrlC(int)
{
    keepRunning = 0;
}

Clay_RenderCommandArray CreateLayout(void)
{
    Clay_BeginLayout();
    CLAY({.id = CLAY_ID("Background"),
          .layout = {.childGap = 2,
                     .childAlignment = {CLAY_ALIGN_X_CENTER},
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = {.width = CLAY_SIZING_GROW(0),
                                .height = CLAY_SIZING_GROW(0)},
                     .padding = CLAY_PADDING_ALL(2)},
          .backgroundColor = {25, 25, 75, 255}})
    {
        CLAY({.id = CLAY_ID("Box1"),
              .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                         .padding = {1, 0, 1, 0},
                         .sizing = {.width = CLAY_SIZING_FIT(32),
                                    .height = CLAY_SIZING_FIXED(8)}},
              .backgroundColor = {150, 100, 255, 255}})
        {
            CLAY_TEXT(CLAY_STRING("Hi there"), CLAY_TEXT_CONFIG({.fontId = FONT_ID_5X7,
                                                                 .textColor = {255, 0, 0, 255},
                                                                 .textAlignment = CLAY_TEXT_ALIGN_RIGHT,
                                                                 .wrapMode = CLAY_TEXT_WRAP_NONE}));
        }

        CLAY({.id = CLAY_ID("Box2"),
              .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                         .sizing = {.width = CLAY_SIZING_FIT(32),
                                    .height = CLAY_SIZING_FIXED(12)},
                         .padding = {2, 2, 1, 1}},
              .border = {.width = CLAY_BORDER_ALL(1),
                         .color = {240, 240, 10, 255}},
              .cornerRadius = CLAY_CORNER_RADIUS(3),
              .backgroundColor = {0, 240, 100, 255}})
        {
            CLAY_TEXT(CLAY_STRING("Bye now!"), CLAY_TEXT_CONFIG({.fontId = FONT_ID_4X6,
                                                                 .textColor = {0, 0, 0, 255},
                                                                 .letterSpacing = 2,
                                                                 .textAlignment = CLAY_TEXT_ALIGN_RIGHT,
                                                                 .wrapMode = CLAY_TEXT_WRAP_NONE}));
        }

        CLAY({.id = CLAY_ID("Error"),
              .backgroundColor = {168, 66, 28, 255},
              .layout = {
                  .padding = CLAY_PADDING_ALL(2),
                  .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
              },
              .floating = {.attachTo = CLAY_ATTACH_TO_PARENT, .zIndex = 1, .attachPoints = {CLAY_ATTACH_POINT_RIGHT_BOTTOM, CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .offset = {0, 0}}})
        {
            CLAY_TEXT(CLAY_STRING("?"), CLAY_TEXT_CONFIG({.fontId = FONT_ID_5X7,
                                                          .textColor = {255, 255, 255, 255},
                                                          .textAlignment = CLAY_TEXT_ALIGN_RIGHT,
                                                          .wrapMode = CLAY_TEXT_WRAP_NONE}));
        }
    }
    return Clay_EndLayout();
}

bool reinitializeClay = false;

void HandleClayErrors(Clay_ErrorData errorData)
{
    printf("%s", errorData.errorText.chars);
    if (errorData.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED)
    {
        reinitializeClay = true;
        Clay_SetMaxElementCount(Clay_GetMaxElementCount() * 2);
    }
    else if (errorData.errorType == CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED)
    {
        reinitializeClay = true;
        Clay_SetMaxMeasureTextCacheWordCount(Clay_GetMaxMeasureTextCacheWordCount() * 2);
    }
}

int main(int argc, char **argv)
{
    struct RGBLedMatrixOptions options = {0};

    Clay_Matrix_Initialize(&options, &argc, &argv);
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(clayMemory, (Clay_Dimensions){(float)options.cols, (float)options.rows}, (Clay_ErrorHandler){HandleClayErrors, 0});

    MonospacedFont fonts[2];
    fonts[FONT_ID_4X6] = load_monospaced_font("../rpi-rgb-led-matrix/fonts/4x6.bdf", 4);
    fonts[FONT_ID_5X7] = load_monospaced_font("../rpi-rgb-led-matrix/fonts/5x7.bdf", 5);
    Clay_SetMeasureTextFunction(Matrix_MeasureText, fonts);

    signal(SIGINT, exitCtrlC);
    while (keepRunning)
    {

        if (reinitializeClay)
        {
            Clay_SetMaxElementCount(8192);
            totalMemorySize = Clay_MinMemorySize();
            clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
            Clay_Initialize(clayMemory, (Clay_Dimensions){(float)options.cols, (float)options.rows}, (Clay_ErrorHandler){HandleClayErrors, 0});
            reinitializeClay = false;
        }

        // struct timespec gettime_now;
        // clock_gettime(CLOCK_MONOTONIC, &gettime_now);
        // long int startTime = gettime_now.tv_nsec;

        Clay_RenderCommandArray renderCommands = CreateLayout();
        // clock_gettime(CLOCK_MONOTONIC, &gettime_now);
        // printf("layout time: %ld ns\n", (gettime_now.tv_nsec - startTime));
        // startTime = gettime_now.tv_nsec;

        Clay_Matrix_Render(renderCommands, fonts);
        // clock_gettime(CLOCK_MONOTONIC, &gettime_now);
        // printf("render time: %ld ns\n", (gettime_now.tv_nsec - startTime));
    }

    Clay_Matrix_Close();

    return 0;
}
