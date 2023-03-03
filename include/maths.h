#ifndef MATHS_H
#define MATHS_H

// Function prototypes
uint64_t maths_hash_position_to_uint64(Point);
uint64_t maths_hash_position_to_uint64_2(Point);
uint64_t maths_hash_position_to_index(Point, int modulo, int entity_type);
bool maths_point_in_rectanle(Point, Point rect[]);
double maths_get_nearest_section_axis(double offset, int size);
double maths_distance_between_points(double x1, double y1, double x2, double y2);
bool maths_line_intersects_camera(const Camera *, double x1, double y1, double x2, double y2);
bool maths_check_point_in_array(Point, Point arr[], int len);

// External function prototypes
bool gfx_relative_position_in_camera(const Camera *, int x, int y);

#endif