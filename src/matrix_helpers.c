#include <stdint.h>

#include "../rpi-rgb-led-matrix/include/led-matrix-c.h"

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

void draw_rectangle(struct LedCanvas *c, int x, int y, int width, int height,
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

typedef enum
{
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
} Corner;

// even dumber
void draw_arc(struct LedCanvas *c, int x, int y, int radius, Corner corner,
              uint8_t r, uint8_t g, uint8_t b)
{
    int r2 = radius * radius;
    int rm12 = (radius - 1) * (radius - 1);

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
            if (i * i + j * j <= r2 && ij2 > rm12)
            {
                led_canvas_set_pixel(c, x + i, y + j, r, g, b);
            }
        }
    }
}

void draw_rounded_rectangle(struct LedCanvas *c, int x, int y, int width, int height, int radius,
                            uint8_t r, uint8_t g, uint8_t b)
{
    // draw the four corners
    draw_filled_circle(c, x + radius, y + radius, radius, r, g, b);
    draw_filled_circle(c, x + width - radius - 1, y + radius, radius, r, g, b);
    draw_filled_circle(c, x + radius, y + height - radius - 1, radius, r, g, b);
    draw_filled_circle(c, x + width - radius - 1, y + height - radius - 1, radius, r, g, b);
    // draw a plus sign to fill in the gaps
    draw_rectangle(c, x + radius, y, width - (radius * 2), height, r, g, b);
    draw_rectangle(c, x, y + radius, width, height - (radius * 2), r, g, b);
}
