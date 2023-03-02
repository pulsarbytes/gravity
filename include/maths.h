#ifndef MATHS_H
#define MATHS_H

// Function prototypes
uint64_t double_hash(double x);
uint64_t pair_hash_order_sensitive(Point);
uint64_t pair_hash_order_sensitive_2(Point);
uint64_t unique_index(Point, int modulo, int entity_type);
int point_in_rect(Point, Point rect[]);
double find_nearest_section_axis(double offset, int size);
double find_distance(double x1, double y1, double x2, double y2);
bool points_equal(Point, Point);
bool line_intersects_viewport(const Camera *, double x1, double y1, double x2, double y2);
bool point_in_array(Point, Point arr[], int len);

// External function prototypes
int in_camera_relative(const Camera *, int x, int y);

#endif