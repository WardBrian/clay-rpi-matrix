#include <stdint.h>

#include "../submodules/rpi-rgb-led-matrix/include/led-matrix-c.h"

// pretty dumb, but it works
void draw_filled_circle(struct LedCanvas *c, int x, int y, int radius,
                        uint8_t r, uint8_t g, uint8_t b)
{
    int r2 = radius * radius;
    for (int i = -radius; i <= radius; i++)
    {
        for (int j = -radius; j <= radius; j++)
        {
            if (i * i + j * j <= r2)
            {
                led_canvas_set_pixel(c, x + i, y + j, r, g, b);
            }
        }
    }
}

void draw_rectangle_basic(struct LedCanvas *c, int x, int y, int width, int height,
                          uint8_t r, uint8_t g, uint8_t b)
{

    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            led_canvas_set_pixel(c, x + i, y + j, r, g, b);
        }
    }
}

int max(int a, int b)
{
    return a > b ? a : b;
}

void draw_rounded_rectangle(struct LedCanvas *c, int x, int y, int width, int height, Clay_RectangleRenderData *config)
{

    uint8_t r = (uint8_t)config->backgroundColor.r;
    uint8_t g = (uint8_t)config->backgroundColor.g;
    uint8_t b = (uint8_t)config->backgroundColor.b;

    int topLeftRadius = (int)config->cornerRadius.topLeft;
    int topRightRadius = (int)config->cornerRadius.topRight;
    int bottomLeftRadius = (int)config->cornerRadius.bottomLeft;
    int bottomRightRadius = (int)config->cornerRadius.bottomRight;

    int left = max(topLeftRadius, bottomLeftRadius);
    int right = max(topRightRadius, bottomRightRadius);
    int top = max(topLeftRadius, topRightRadius);
    int bottom = max(bottomLeftRadius, bottomRightRadius);

    // draw the four corners
    if (topLeftRadius > 0)
    {
        draw_filled_circle(c, x + topLeftRadius, y + topLeftRadius, topLeftRadius, r, g, b);
    }
    else
    {
        draw_rectangle_basic(c, x, y, left, top, r, g, b);
    }

    if (topRightRadius > 0)
    {
        draw_filled_circle(c, x + width - topRightRadius - 1, y + topRightRadius, topRightRadius, r, g, b);
    }
    else
    {
        draw_rectangle_basic(c, x + width - right, y, right, top, r, g, b);
    }

    if (bottomLeftRadius > 0)
    {
        draw_filled_circle(c, x + bottomLeftRadius, y + height - bottomLeftRadius - 1, bottomLeftRadius, r, g, b);
    }
    else
    {
        draw_rectangle_basic(c, x, y + height - bottom, left, bottom, r, g, b);
    }

    if (bottomRightRadius > 0)
    {
        draw_filled_circle(c, x + width - bottomRightRadius - 1, y + height - bottomRightRadius - 1, bottomRightRadius, r, g, b);
    }
    else
    {
        draw_rectangle_basic(c, x + width - right, y + height - bottom, right, bottom, r, g, b);
    }

    // draw a plus sign to fill in the gaps
    draw_rectangle_basic(c, x + left, y, width - left - right, height, r, g, b);
    draw_rectangle_basic(c, x, y + top, width, height - top - bottom, r, g, b);
}

typedef enum
{
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
} Corner;

// even dumber
void draw_arc(struct LedCanvas *c, int x, int y, int radius, int innerRadius, Corner corner,
              uint8_t r, uint8_t g, uint8_t b)
{
    int r2 = radius * radius;
    int ir2 = (innerRadius) * (innerRadius);

    int i_start, i_end, j_start, j_end;
    switch (corner)
    {
    case TOP_LEFT:
        i_start = -radius;
        i_end = 0;
        j_start = -radius;
        j_end = 0;
        break;
    case TOP_RIGHT:
        i_start = 0;
        i_end = radius;
        j_start = -radius;
        j_end = 0;
        break;
    case BOTTOM_LEFT:
        i_start = -radius;
        i_end = 0;
        j_start = 0;
        j_end = radius;
        break;
    case BOTTOM_RIGHT:
    default:
        i_start = 0;
        i_end = radius;
        j_start = 0;
        j_end = radius;
        break;
    }

    for (int i = i_start; i <= i_end; i++)
    {
        for (int j = j_start; j <= j_end; j++)
        {
            int ij2 = i * i + j * j;
            if (i * i + j * j <= r2 && ij2 > ir2)
            {
                led_canvas_set_pixel(c, x + i, y + j, r, g, b);
            }
        }
    }
}

void draw_border(struct LedCanvas *canvas, int bbX, int bbY, int bbWidth, int bbHeight,
                 Clay_BorderRenderData *config)
{
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
        draw_arc(canvas, bbX + topLeftRadius, bbY + topLeftRadius,
                 topLeftRadius, topLeftRadius - config->width.top, TOP_LEFT,
                 r, g, b);
    }
    if (topRightRadius > 0)
    {
        draw_arc(canvas, bbX + bbWidth - 1 - topRightRadius, bbY + topRightRadius,
                 topRightRadius, topRightRadius - config->width.top, TOP_RIGHT,
                 r, g, b);
    }
    if (bottomLeftRadius > 0)
    {
        draw_arc(canvas, bbX + bottomLeftRadius, bbY + bbHeight - 1 - bottomLeftRadius,
                 bottomLeftRadius, bottomLeftRadius - config->width.bottom, BOTTOM_LEFT,
                 r, g, b);
    }
    if (bottomRightRadius > 0)
    {
        draw_arc(canvas, bbX + bbWidth - 1 - bottomRightRadius, bbY + bbHeight - 1 - bottomRightRadius,
                 bottomRightRadius, bottomRightRadius - config->width.bottom, BOTTOM_RIGHT,
                 r, g, b);
    }
}
