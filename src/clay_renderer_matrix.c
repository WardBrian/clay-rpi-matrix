#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../submodules/clay/clay.h"
#include "../submodules/utf8.h/utf8.h"
#include "../submodules/rpi-rgb-led-matrix/include/led-matrix-c.h"
#include "matrix_helpers.c"

typedef struct
{
    uint8_t *imageData;
    size_t length;
    int32_t height, width;
} ImageCounted;
// TODO loader, maybe using MagickWand?

static inline Clay_Dimensions Matrix_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData)
{
    Clay_Dimensions textSize = {0};

    struct LedFont **fonts = (struct LedFont **)userData;
    struct LedFont *fontToUse = fonts[config->fontId];

    const char *current = text.chars;
    while (current < text.chars + text.length)
    {
        utf8_int32_t codepoint;
        current = utf8codepoint(current, &codepoint);
        textSize.width += (float)(character_width_font(fontToUse, (uint32_t)codepoint) + config->letterSpacing);
    }

    textSize.height = (float)height_font(fontToUse);
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

void Clay_Matrix_Render(Clay_RenderCommandArray renderCommands, struct LedFont **fonts)
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
            struct LedFont *fontToUse = fonts[textData->fontId];
            
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

            int y = bbY + baseline_font(fontToUse);
            draw_text(canvas, fontToUse, bbX, y,
                      (uint8_t)textData->textColor.r, (uint8_t)textData->textColor.g, (uint8_t)textData->textColor.b,
                      temp_render_buffer, textData->letterSpacing);

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_IMAGE:
        {

            Clay_ImageRenderData *imageData = &renderCommand->renderData.image;
            ImageCounted *image = (ImageCounted *)imageData->imageData;

            // TODO tint color? rounded corners?
            set_image(canvas, bbX, bbY,
                      image->imageData, image->length,
                      image->width, image->height,
                      /* is_bgr */ 0);

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
        {
            Clay_RectangleRenderData *config = &renderCommand->renderData.rectangle;
            draw_rounded_rectangle(canvas, bbX, bbY, bbWidth, bbHeight, config);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_BORDER:
        {
            Clay_BorderRenderData *config = &renderCommand->renderData.border;
            draw_border(canvas, bbX, bbY, bbWidth, bbHeight, config);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_NONE:
        {
            // Do nothing
            break;
        }

        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
        // TODO
        default:
        {
            printf("Error: unhandled render command.");
            exit(1);
        }
        }
    }
    canvas = led_matrix_swap_on_vsync(matrix, canvas);
}
