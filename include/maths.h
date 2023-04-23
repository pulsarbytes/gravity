#ifndef MATHS_H
#define MATHS_H

// Function prototypes
bool maths_check_point_in_array(Point, Point arr[], int len);
void maths_closest_point_outside_circle(double cx, double cy, double radius, double radius_ratio, double px, double py, double *x, double *y, double degrees);
double maths_distance_between_points(double x1, double y1, double x2, double y2);
double maths_get_nearest_section_line(double offset, int size);
int maths_get_rotation_direction(double cx, double cy, double px, double py, double dest_x, double dest_y);
uint64_t maths_hash_position_to_index(Point, int modulo, int entity_type);
uint64_t maths_hash_position_to_uint64(Point);
uint64_t maths_hash_position_to_uint64_2(Point);
bool maths_is_point_in_circle(Point, Point, double radius);
bool maths_is_point_in_rectangle(Point, Point rect[]);
bool maths_is_point_on_line(Point, Point, Point);
bool maths_line_intersects_camera(const Camera *, double x1, double y1, double x2, double y2);
bool maths_line_intersects_circle(double x1, double y1, double x2, double y2, double cx, double cy, double radius);
void maths_move_point_along_line(double x1, double y1, double x2, double y2, double step, double *x, double *y);
bool maths_points_equal(Point, Point);
long double maths_rounded_double(long double number);

// External function prototypes
bool gfx_is_relative_position_in_camera(const Camera *, int x, int y);

#endif