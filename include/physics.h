#ifndef PHYSICS_H
#define PHYSICS_H

// Function prototypes
void update_velocity(Vector *velocity, const Ship *);
void calc_orbital_velocity(float distance, float angle, float radius, float *vx, float *vy);
void apply_gravity_to_ship(GameState *, int thrust, NavigationState *, CelestialBody *, Ship *, const Camera *);

#endif