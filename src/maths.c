/*
 * maths.c - Definitions for math functions.
 */

#include <stdint.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/maths.h"

// Static function prototypes
static uint64_t maths_hash_double_to_uint64(double x);
static bool maths_points_equal(Point, Point);

/**
 * Generates a hash value for a given double number `x`.
 *
 * @param x The double number to be hashed
 *
 * @return A 64-bit unsigned integer representing the hash value of `x`
 */
static uint64_t maths_hash_double_to_uint64(double x)
{
    uint64_t x_bits = *(uint64_t *)&x;
    uint64_t x_hash = x_bits ^ (x_bits >> 33);
    x_hash *= 0xffffffff;
    x_hash ^= x_hash >> 33;
    x_hash *= 0xffffffff;
    x_hash ^= x_hash >> 33;

    return x_hash;
}

/*
 * Hash function that maps two double numbers to a unique 64-bit integer. Order sensitive.
 */
uint64_t maths_hash_position_to_uint64(Point position)
{
    uint64_t x_hash = maths_hash_double_to_uint64(position.x);
    uint64_t y_hash = maths_hash_double_to_uint64(position.y);
    uint64_t hash = x_hash ^ (y_hash + 0x9e3779b97f4a7c15ull + 1);

    return hash;
}

/*
 * Hash function that maps two double numbers to a unique 64-bit integer. Order sensitive.
 */
uint64_t maths_hash_position_to_uint64_2(Point position)
{
    uint64_t x_hash = maths_hash_double_to_uint64(position.x);
    uint64_t y_hash = maths_hash_double_to_uint64(position.y);
    uint64_t hash = (x_hash + 0x9e3779b97f4a7c15ull) ^ y_hash;

    return hash;
}

/*
 * Hash function that creates an index for an entry in a hash table.
 */
uint64_t maths_hash_position_to_index(Point position, int modulo, int entity_type)
{
    uint64_t index;

    if (entity_type == ENTITY_STAR)
        index = maths_hash_position_to_uint64(position);
    else if (entity_type == ENTITY_GALAXY)
        index = maths_hash_position_to_uint64_2(position);

    return index % modulo;
}

/*
 * Check whether point p is in rectangular rect.
 */
bool maths_point_in_rectanle(Point p, Point rect[])
{
    int i, j;
    int sign = 0;
    int n = 4;

    for (i = 0, j = n - 1; i < n; j = i++)
    {
        double dx1 = p.x - rect[i].x;
        double dy1 = p.y - rect[i].y;
        double dx2 = rect[j].x - rect[i].x;
        double dy2 = rect[j].y - rect[i].y;
        double cross_prod = dx1 * dy2 - dy1 * dx2;

        if (i == 0)
            sign = cross_prod > 0 ? 1 : -1;
        else if ((cross_prod > 0) != (sign > 0))
            return false; // point is outside the rectangle
    }

    return true; // point is inside the rectangle
}

/*
 * Transform a double to the nearest section point,
 * rounding up or down whichever is nearest.
 */
double maths_get_nearest_section_axis(double offset, int size)
{
    double round_down = floorf(offset / size) * size;
    double round_up = round_down + size;
    double diff_down = fabs(offset - round_down);
    double diff_up = fabs(offset - round_up);

    return diff_down < diff_up ? round_down : round_up;
}

/*
 * Find distance between two points.
 */
double maths_distance_between_points(double x1, double y1, double x2, double y2)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

/*
 * Compares two points.
 */
static bool maths_points_equal(Point a, Point b)
{
    return a.x == b.x && a.y == b.y;
}

/**
 * This function checks if the line segment specified by (x1,y1) and (x2,y2) intersects with the viewport defined by `camera`.
 *
 * @param camera Pointer to a camera structure that defines the viewport
 * @param x1 x-coordinate of the first end point of the line segment
 * @param y1 y-coordinate of the first end point of the line segment
 * @param x2 x-coordinate of the second end point of the line segment
 * @param y2 y-coordinate of the second end point of the line segment
 *
 * @return: Returns true if the line segment intersects with the viewport, and false otherwise
 */
bool maths_line_intersects_camera(const Camera *camera, double x1, double y1, double x2, double y2)
{
    double left = 0;
    double right = camera->w;
    double top = 0;
    double bottom = camera->h;

    // Check if both endpoints of the line are inside the viewport.
    if (gfx_relative_position_in_camera(camera, x1, y1) || gfx_relative_position_in_camera(camera, x2, y2))
        return true;

    // Check if the line intersects the left edge of the viewport.
    if (x1 < left && x2 >= left)
    {
        double y = y1 + (y2 - y1) * (left - x1) / (x2 - x1);
        if (y >= top && y <= bottom)
        {
            return true;
        }
    }

    // Check if the line intersects the right edge of the viewport.
    if (x1 > right && x2 <= right)
    {
        double y = y1 + (y2 - y1) * (right - x1) / (x2 - x1);
        if (y >= top && y <= bottom)
        {
            return true;
        }
    }

    // Check if the line intersects the top edge of the viewport.
    if (y1 < top && y2 >= top)
    {
        double x = x1 + (x2 - x1) * (top - y1) / (y2 - y1);
        if (x >= left && x <= right)
        {
            return true;
        }
    }

    // Check if the line intersects the bottom edge of the viewport.
    if (y1 > bottom && y2 <= bottom)
    {
        double x = x1 + (x2 - x1) * (bottom - y1) / (y2 - y1);
        if (x >= left && x <= right)
        {
            return true;
        }
    }

    return false;
}

/*
 * Checks whether a point exists in an array.
 */
bool maths_check_point_in_array(Point point, Point arr[], int len)
{
    for (int i = 0; i < len; ++i)
    {
        if (maths_points_equal(point, arr[i]))
            return true;
    }

    return false;
}