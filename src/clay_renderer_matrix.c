#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../submodules/clay/clay.h"
#include "../submodules/rpi-rgb-led-matrix/include/led-matrix-c.h"
#include "matrix_helpers.c"

typedef struct
{
    uint8_t *imageData;
    size_t length;
} ImageCounted;
// TODO loader, maybe using MagickWand?

typedef struct
{
    struct LedFont *font;
    int width;
    int height;
} MonospacedFont;

MonospacedFont load_monospaced_font(const char *bdf_font_file, int width)
{
    MonospacedFont font;
    font.font = load_font(bdf_font_file);
    font.width = width;
    font.height = height_font(font.font);
    return font;
}

static inline Clay_Dimensions Matrix_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData)
{
    Clay_Dimensions textSize = {0};

    MonospacedFont *fonts = (MonospacedFont *)userData;
    MonospacedFont fontToUse = fonts[config->fontId];
    // TODO: to support non-monospaced fonts, we need to measure each character
    // Needs https://github.com/hzeller/rpi-rgb-led-matrix/issues/1775
    // and something like utf8codepoint from https://github.com/sheredom/utf8.h
    textSize.width = (float)(fontToUse.width + config->letterSpacing) * text.length;
    textSize.height = (float)fontToUse.height;
    return textSize;
}

// TODO provide something like CLAY_TEXT for scrolling text?

// A MALLOC'd buffer, that we keep modifying inorder to save from so many Malloc and Free Calls.
// Call Clay_Matrix_Close() to free
static char *temp_render_buffer = NULL;
static int temp_render_buffer_len = 0;

static struct RGBLedMatrix *matrix = NULL;
static struct LedCanvas *canvas = NULL;

void Clay_Matrix_Initialize(struct RGBLedMatrixOptions *options, int *argc, char ***argv)
{
    matrix = led_matrix_create_from_options(options, argc, argv);
    if (!matrix)
    {
        printf("Error: failed to initialize matrix.\n");
        exit(1);
    }
    canvas = led_matrix_create_offscreen_canvas(matrix);
}

void Clay_Matrix_Close()
{
    if (temp_render_buffer)
        free(temp_render_buffer);
    temp_render_buffer_len = 0;
    led_matrix_delete(matrix);
}

void Clay_Matrix_Render(Clay_RenderCommandArray renderCommands, MonospacedFont *fonts)
{
    led_canvas_clear(canvas);

    for (int j = 0; j < renderCommands.length; j++)
    {
        Clay_RenderCommand *renderCommand = Clay_RenderCommandArray_Get(&renderCommands, j);
        Clay_BoundingBox boundingBox = renderCommand->boundingBox;
        int bbWidth = (int)boundingBox.width;
        int bbHeight = (int)boundingBox.height;
        int bbX = (int)boundingBox.x;
        int bbY = (int)boundingBox.y;

        switch (renderCommand->commandType)
        {
        case CLAY_RENDER_COMMAND_TYPE_TEXT:
        {
            Clay_TextRenderData *textData = &renderCommand->renderData.text;
            MonospacedFont fontToUse = fonts[textData->fontId];

            int strlen = textData->stringContents.length + 1;

            if (strlen > temp_render_buffer_len)
            {
                // Grow the temp buffer if we need a larger string
                if (temp_render_buffer)
                    free(temp_render_buffer);
                temp_render_buffer = malloc(strlen);
                temp_render_buffer_len = strlen;
            }

            // Matrix uses standard C strings so isn't compatible with cheap slices, we need to clone the string to append null terminator
            memcpy(temp_render_buffer, textData->stringContents.chars, textData->stringContents.length);
            temp_render_buffer[textData->stringContents.length] = '\0';

            int y = bbY + baseline_font(fontToUse.font);
            draw_text(canvas, fontToUse.font, bbX, y,
                      (uint8_t)textData->textColor.r, (uint8_t)textData->textColor.g, (uint8_t)textData->textColor.b,
                      temp_render_buffer, textData->letterSpacing);

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_IMAGE:
        {

            Clay_ImageRenderData *imageData = &renderCommand->renderData.image;

            ImageCounted *image = (ImageCounted *)renderCommand->renderData.image.imageData;

            // TODO tint color?
            set_image(canvas, bbX, bbY,
                      image->imageData, image->length,
                      (int)imageData->sourceDimensions.width, (int)imageData->sourceDimensions.height,
                      /* is_bgr */ 0);

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
        {
            Clay_RectangleRenderData *config = &renderCommand->renderData.rectangle;

            uint8_t r = (uint8_t)config->backgroundColor.r;
            uint8_t g = (uint8_t)config->backgroundColor.g;
            uint8_t b = (uint8_t)config->backgroundColor.b;
            if (config->cornerRadius.topLeft > 0)
            {
                // TODO -- allow non-symmetric corner radius?
                draw_rounded_rectangle(canvas, bbX, bbY, bbWidth, bbHeight, (int)config->cornerRadius.topLeft,
                                       r, g, b);
            }
            else
            {
                draw_rectangle(canvas, bbX, bbY, bbWidth, bbHeight, r, g, b);
            }
            break;
        }

        case CLAY_RENDER_COMMAND_TYPE_BORDER:
        {
            Clay_BorderRenderData *config = &renderCommand->renderData.border;
            draw_border(canvas, bbX, bbY, bbWidth, bbHeight, config);
            break;
        }

        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
        default:
        {
            printf("Error: unhandled render command.");
            exit(1);
        }
        }
    }
    canvas = led_matrix_swap_on_vsync(matrix, canvas);
}
