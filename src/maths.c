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
 * Calculates the closest point on the circumference of a circle to a given point,
 * and pushes it away from the circle by a specified distance (10% of radius) in the direction of
 * the line that connects the center of the circle and the point.
 *
 * @param cx The x-coordinate of the center of the circle.
 * @param cy The y-coordinate of the center of the circle.
 * @param radius The radius of the circle.
 * @param radius_ratio The radius ratio for the distance of (x,y) from the center of the circle.
 * @param px The x-coordinate of the given point.
 * @param py The y-coordinate of the given point.
 * @param x A pointer to the variable where the x-coordinate of the closest point will be stored.
 * @param y A pointer to the variable where the y-coordinate of the closest point will be stored.
 * @param degrees The degrees of rotation for the radius line where px,py lies.
 *
 * @return void
 */
void maths_closest_point_outside_circle(double cx, double cy, double radius, double radius_ratio, double px, double py, double *x, double *y, double degrees)
{
    // Convert the rotation angle from degrees to radians
    double rotation_angle = degrees * M_PI / 180.0;

    // Calculate the distance from the circle center to the given point
    double d = sqrt(pow(px - cx, 2) + pow(py - cy, 2));

    // Rotate the line connecting the two points by the specified angle
    double rotated_x = (px - cx) * cos(rotation_angle) - (py - cy) * sin(rotation_angle) + cx;
    double rotated_y = (px - cx) * sin(rotation_angle) + (py - cy) * cos(rotation_angle) + cy;

    // Calculate the coordinates of the closest point on the circumference
    double closest_cx = cx + radius * (rotated_x - cx) / d;
    double closest_cy = cy + radius * (rotated_y - cy) / d;

    // Calculate the coordinates of the pushed point using the fixed push factor
    double dx = closest_cx - cx;
    double dy = closest_cy - cy;
    double push_length = radius_ratio * radius;
    double nx = dx / radius;
    double ny = dy / radius;
    *x = cx + push_length * nx;
    *y = cy + push_length * ny;
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
 * Calculate the direction of rotation needed to turn from the line connecting (cx,cy) and (px,py)
 * to the line connecting (cx,cy) and (dest_x,dest_y).
 *
 * @param cx The x-coordinate of the center point.
 * @param cy The y-coordinate of the center point.
 * @param px The x-coordinate of the initial point.
 * @param py The y-coordinate of the initial point.
 * @param dest_x The x-coordinate of the final point.
 * @param dest_y The y-coordinate of the final point.
 *
 * @return Returns 1 if the rotation is clockwise, -1 if it is counterclockwise.
 */
int maths_get_rotation_direction(double cx, double cy, double px, double py, double dest_x, double dest_y)
{
    // Calculate the angle between the line connecting cx,cy and px,py and the line connecting cx,cy and dest_x, dest_y
    double angle1 = atan2(py - cy, px - cx);
    double angle2 = atan2(dest_y - cy, dest_x - cx);
    double angle_diff = angle2 - angle1;

    // Convert the angle difference to the range (-pi, pi]
    while (angle_diff <= -M_PI)
    {
        angle_diff += 2 * M_PI;
    }
    while (angle_diff > M_PI)
    {
        angle_diff -= 2 * M_PI;
    }

    // Determine the direction of rotation based on the sign of the angle difference
    if (angle_diff < 0)
    {
        return -1;
    }
    else
    {
        return 1;
    }
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
 * Checks if a point lies within a circle.
 *
 * @param point The point to check.
 * @param center The point representing the circle center.
 * @param radius The radius of the circle.
 * @return True if the point is inside the circle, false otherwise.
 */
bool maths_is_point_in_circle(Point point, Point center, double radius)
{
    // Calculate the distance between the point and the circle center using the Pythagorean theorem
    double dx = point.x - center.x;
    double dy = point.y - center.y;
    double distance_squared = dx * dx + dy * dy;

    // Compare the distance to the circle radius squared
    double radius_squared = radius * radius;

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
 * Checks if a point lies on a line.
 *
 * @param p1 The start point of the line.
 * @param p2 The end point of the line.
 * @param p3 The point to check for.
 *
 * @return True if the point is on the line, false otherwise.
 */
bool maths_is_point_on_line(Point p1, Point p2, Point p3)
{
    double tolerance = 3; // Distance from line
    double m = (p2.y - p1.y) / (p2.x - p1.x);
    double b = p1.y - (m * p1.x);
    double expected_y = m * p3.x + b;

    return fabs(p3.y - expected_y) < tolerance;
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
 * Checks if the line connecting the points (x1,y1) and (x2,y2)
 * intersects a circle (cx,cy).
 *
 * @param nav_state A pointer to the current NavigationState object.
 * @param x1 The x-coordinate of the first point.
 * @param y1 The y-coordinate of the first point.
 * @param x2 The x-coordinate of the second point.
 * @param y2 The y-coordinate of the second point.
 *
 * @return True if there is line of sight between the two points, false otherwise.
 */
bool maths_line_intersects_circle(double x1, double y1, double x2, double y2, double cx, double cy, double radius)
{
    // Calculate the vector between the two points
    double dx = x2 - x1;
    double dy = y2 - y1;

    // Calculate the squared length of the vector
    double length_squared = dx * dx + dy * dy;

    // If the line segment is degenerate (i.e., has zero length), return false
    if (length_squared == 0)
        return false;

    // Calculate the vector from the first point to the circle center
    double cx_minus_x1 = cx - x1;
    double cy_minus_y1 = cy - y1;

    // Calculate the projection of the vector from the circle center to the first point
    // onto the line segment
    double projection = (cx_minus_x1 * dx + cy_minus_y1 * dy) / length_squared;

    // If the projection is outside the bounds of the line segment, return false
    if (projection < 0 || projection > 1)
        return false;

    // Calculate the closest point on the line segment to the circle center
    double closest_x = x1 + projection * dx;
    double closest_y = y1 + projection * dy;

    // Calculate the distance between the closest point and the circle center
    double distance_squared = (closest_x - cx) * (closest_x - cx) + (closest_y - cy) * (closest_y - cy);

    // If the distance is less than or equal to the radius, the line segment intersects the circle
    if (distance_squared <= radius * radius)
        return true;

    // Otherwise, the line segment does not intersect the circle
    return false;
}

/**
 * Moves the point (x1,y1) closer to (x2,y2) by <step> units
 * and stores the new point at (x,y).
 *
 * @param x1 The x-coordinate of the starting point.
 * @param x2 The y-coordinate of the starting point.
 * @param x2 The x-coordinate of the ending point.
 * @param y2 The y-coordinate of the ending point.
 * @param step The distance of the movement.
 * @param x The x-coordinate of the new point.
 * @param y The y-coordinate of the new point.
 *
 * @return void
 */
void maths_move_point_along_line(double x1, double y1, double x2, double y2, double step, double *x, double *y)
{
    double distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
    double unit_x = (x2 - x1) / distance;
    double unit_y = (y2 - y1) / distance;

    *x = x1 + step * unit_x;
    *y = y1 + step * unit_y;
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