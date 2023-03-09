#ifndef MATHS_H
#define MATHS_H

// Function prototypes
bool maths_check_point_in_array(Point, Point arr[], int len);
double maths_distance_between_points(double x1, double y1, double x2, double y2);
double maths_get_nearest_section_line(double offset, int size);
bool maths_line_intersects_camera(const Camera *, double x1, double y1, double x2, double y2);
uint64_t maths_hash_position_to_index(Point, int modulo, int entity_type);
uint64_t maths_hash_position_to_uint64(Point);
uint64_t maths_hash_position_to_uint64_2(Point);
bool maths_is_point_in_circle(Point, Point, int radius);
bool maths_is_point_in_rectangle(Point, Point rect[]);
bool maths_points_equal(Point, Point);

// External function prototypes
bool gfx_is_relative_position_in_camera(const Camera *, int x, int y);

#endif