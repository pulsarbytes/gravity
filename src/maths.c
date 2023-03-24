/*
 * maths.c
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

/**
 * Check if a point exists in an array of points.
 *
 * @param point The point to check.
 * @param arr The array of points to search.
 * @param len The length of the array.
 * @return True if the point exists in the array, false otherwise.
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

/**
 * Calculates the Euclidean distance between two points in a 2D space.
 *
 * @param x1 The x-coordinate of the first point.
 * @param y1 The y-coordinate of the first point.
 * @param x2 The x-coordinate of the second point.
 * @param y2 The y-coordinate of the second point.
 *
 * @return The distance between the two points.
 */
double maths_distance_between_points(double x1, double y1, double x2, double y2)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

/**
 * Returns the nearest section line value to the given offset, based on the section size.
 *
 * @param offset The offset value to get the nearest section line for.
 * @param size The size of the sections to get the nearest line for.
 *
 * @return The nearest section line value to the given offset.
 */
double maths_get_nearest_section_line(double offset, int size)
{
    double round_down = floorf(offset / size) * size;
    double round_up = round_down + size;
    double diff_down = fabs(offset - round_down);
    double diff_up = fabs(offset - round_up);

    return diff_down < diff_up ? round_down : round_up;
}

/**
 * This function checks if the line segment specified by (x1,y1) and (x2,y2) intersects with the viewport defined by `camera`.
 *
 * @param camera A pointer to the current Camera object.
 * @param x1 The x-coordinate of the first end point of the line segment.
 * @param y1 The y-coordinate of the first end point of the line segment.
 * @param x2 The x-coordinate of the second end point of the line segment.
 * @param y2 The y-coordinate of the second end point of the line segment.
 *
 * @return True if the line segment intersects with the viewport, and false otherwise.
 */
bool maths_line_intersects_camera(const Camera *camera, double x1, double y1, double x2, double y2)
{
    double left = 0;
    double right = camera->w;
    double top = 0;
    double bottom = camera->h;

    // Check if both endpoints of the line are inside the camera.
    if (gfx_is_relative_position_in_camera(camera, x1, y1) || gfx_is_relative_position_in_camera(camera, x2, y2))
        return true;

    // Check if the line intersects the left edge of the camera.
    if (x1 < left && x2 >= left)
    {
        double y = y1 + (y2 - y1) * (left - x1) / (x2 - x1);
        if (y >= top && y <= bottom)
        {
            return true;
        }
    }

    // Check if the line intersects the right edge of the camera.
    if (x1 > right && x2 <= right)
    {
        double y = y1 + (y2 - y1) * (right - x1) / (x2 - x1);
        if (y >= top && y <= bottom)
        {
            return true;
        }
    }

    // Check if the line intersects the top edge of the camera.
    if (y1 < top && y2 >= top)
    {
        double x = x1 + (x2 - x1) * (top - y1) / (y2 - y1);
        if (x >= left && x <= right)
        {
            return true;
        }
    }

    // Check if the line intersects the bottom edge of the camera.
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

/**
 * Generates a hash value for a given double number `x`.
 *
 * @param x The double number to be hashed.
 *
 * @return A 64-bit unsigned integer representing the hash value of `x`.
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

/**
 * Computes the hash index of a given 2D position for a given entity type.
 * The index will be used in a hash table.
 *
 * @param position The position of the entity.
 * @param modulo The modulo value used to wrap the hash index.
 * @param entity_type The type of the entity (star or galaxy).
 *
 * @return The hash index of the given position for the given entity type, modulo the provided value.
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

/**
 * Hashes a Point position into a 64-bit unsigned integer.
 *
 * @param position A Point structure containing the x and y coordinates of the position to be hashed.
 *
 * @return A 64-bit unsigned integer that represents the hash of the input position.
 */
uint64_t maths_hash_position_to_uint64(Point position)
{
    uint64_t x_hash = maths_hash_double_to_uint64(position.x);
    uint64_t y_hash = maths_hash_double_to_uint64(position.y);
    uint64_t hash = x_hash ^ (y_hash + 0x9e3779b97f4a7c15ull + 1);

    return hash;
}

/**
 * Hashes a Point position into a 64-bit unsigned integer.
 *
 * @param position A Point structure containing the x and y coordinates of the position to be hashed.
 *
 * @return A 64-bit unsigned integer that represents the hash of the input position.
 */
uint64_t maths_hash_position_to_uint64_2(Point position)
{
    uint64_t x_hash = maths_hash_double_to_uint64(position.x);
    uint64_t y_hash = maths_hash_double_to_uint64(position.y);
    uint64_t hash = (x_hash + 0x9e3779b97f4a7c15ull) ^ y_hash;

    return hash;
}

/**
 * Determines if two points are equal.
 *
 * @param a The first point to compare.
 * @param b The second point to compare.
 *
 * @return True if the points are equal, false otherwise.
 */
bool maths_points_equal(Point a, Point b)
{
    return a.x == b.x && a.y == b.y;
}

/**
 * Checks if a point lies within a circle.
 *
 * @param point The point to check.
 * @param center The point representing the circle center.
 * @param radius The radius of the circle.
 * @return True if the point is inside the circle, false otherwise.
 */
bool maths_is_point_in_circle(Point point, Point center, int radius)
{
    // Calculate the distance between the point and the circle center using the Pythagorean theorem
    int dx = point.x - center.x;
    int dy = point.y - center.y;
    int distance_squared = dx * dx + dy * dy;

    // Compare the distance to the circle radius squared
    int radius_squared = radius * radius;

    if (distance_squared <= radius_squared)
        return true;
    else
        return false;
}

/**
 * Checks if a point lies within a given rectangle.
 *
 * @param p The point to check.
 * @param rect An array of four points representing the rectangle's vertices.
 * @return True if the point is inside the rectangle, false otherwise.
 */
bool maths_is_point_in_rectangle(Point p, Point rect[])
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
            return false;
    }

    return true;
}

/**
 * Rounds the given number to the nearest multiple of 10 to the power of its order of magnitude.
 *
 * @param number The number to be rounded.
 *
 * @return The rounded number.
 */
long double maths_rounded_double(long double number)
{
    long double rounded_number;

    if (number == 0.0)
        rounded_number = 0.0;
    else
    {
        double abs_num = fabs(number);
        int exponent = floor(log10(abs_num));             // Get the exponent of the absolute value of the number
        double factor = pow(10, exponent);                // Calculate the scaling factor
        rounded_number = round(number / factor) * factor; // Round the number using the scaling factor
    }

    return rounded_number;
}