#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../clay/clay.h"
#include "../rpi-rgb-led-matrix/include/led-matrix-c.h"
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

    textSize.width = (float)(fontToUse.width + config->letterSpacing) * text.length;
    textSize.height = (float)fontToUse.height;
    return textSize;
}

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
            uint8_t r = (uint8_t)config->color.r;
            uint8_t g = (uint8_t)config->color.g;
            uint8_t b = (uint8_t)config->color.b;

            int topLeftRadius = (int)config->cornerRadius.topLeft;
            int bottomLeftRadius = (int)config->cornerRadius.bottomLeft;
            int topRightRadius = (int)config->cornerRadius.topRight;
            int bottomRightRadius = (int)config->cornerRadius.bottomRight;

            // Left border
            for (int x = bbX; x < bbX + config->width.left; x++)
            {
                draw_line(canvas, x, bbY + topLeftRadius, x, bbY + bbHeight - 1 - bottomLeftRadius, r, g, b);
            }
            // Right border
            for (int x = bbX + bbWidth - config->width.right; x < bbX + bbWidth; x++)
            {
                draw_line(canvas, x, bbY + topRightRadius, x, bbY + bbHeight - 1 - bottomRightRadius, r, g, b);
            }
            // Top border
            for (int y = bbY; y < bbY + config->width.top; y++)
            {
                draw_line(canvas, bbX + topLeftRadius, y, bbX + bbWidth - 1 - topRightRadius, y, r, g, b);
            }
            // Bottom border
            for (int y = bbY + bbHeight - config->width.bottom; y < bbY + bbHeight; y++)
            {
                draw_line(canvas, bbX + bottomLeftRadius, y, bbX + bbWidth - 1 - bottomRightRadius, y, r, g, b);
            }

            // corners
            if (topLeftRadius > 0)
            {
                draw_arc(canvas, bbX + topLeftRadius, bbY + topLeftRadius, topLeftRadius, TOP_LEFT,
                         r, g, b);
            }
            if (topRightRadius > 0)
            {
                draw_arc(canvas, bbX + bbWidth - 1 - topRightRadius, bbY + topRightRadius, topRightRadius, TOP_RIGHT,
                         r, g, b);
            }
            if (bottomLeftRadius > 0)
            {
                draw_arc(canvas, bbX + bottomLeftRadius, bbY + bbHeight - 1 - bottomLeftRadius, bottomLeftRadius, BOTTOM_LEFT,
                         r, g, b);
            }
            if (bottomRightRadius > 0)
            {
                draw_arc(canvas, bbX + bbWidth - 1 - bottomRightRadius, bbY + bbHeight - 1 - bottomRightRadius, bottomRightRadius, BOTTOM_RIGHT,
                         r, g, b);
            }

            break;
        }

        // TODO: could be used for something scrolling-text-like?
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
