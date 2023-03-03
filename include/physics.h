#ifndef PHYSICS_H
#define PHYSICS_H

// Function prototypes
void phys_update_velocity(Vector *velocity, const Ship *);
void phys_calculate_orbital_velocity(float distance, float angle, float radius, float *vx, float *vy);
void phys_apply_gravity_to_ship(GameState *, int thrust, NavigationState *, CelestialBody *, Ship *, int star_class);

#endif