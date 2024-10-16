#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "C-Collection-Vector/vector.h"

#define DRAW_HITBOX

#define ASTEROID_HEALTH_START 100
#define ASTEROID_HEALTH_END 0
#define ASTEROID_ASTEROID_COLLIDE_HEALTH_STEP 10

typedef enum
{
    ET_ERROR,
    ET_ASTEROID,
    ET_SHIP,
    ET_PROJECTILE,
} EntityType;

typedef struct
{
    Vector2 position;
    struct
    {
        Vector2 linear;
        float angular;
    } velocity;
    float rotation;
    int health;
    EntityType type;
    struct
    {
        Vector2 *points;
        int num_points;
        Vector2 center;
#ifdef DRAW_HITBOX
        Color color;
#endif
    } hitshape;
} EntityData;

void draw_poly_points(Vector2 points[], int num_points, Vector2 center, float rotation, float thickness, Color color)
{
    int i;
    for (i = 0; i < num_points; i++)
    {
        Vector2 point = Vector2Add(center, Vector2Rotate(points[i], DEG2RAD * rotation));
        Vector2 next_point = Vector2Add(center, Vector2Rotate(points[(i + 1) % num_points], DEG2RAD * rotation));
        DrawLineEx(point, next_point, thickness, color);
    }
}

int return_to_screen(EntityData *entity)
{
    int rc = 0;
    if (entity->position.x > GetScreenWidth())
    {
        entity->position.x = 0;
        rc++;
    }
    if (entity->position.x < 0)
    {
        entity->position.x = GetScreenWidth();
        rc++;
    }
    if (entity->position.y > GetScreenHeight())
    {
        entity->position.y = 0;
        rc++;
    }
    if (entity->position.y < 0)
    {
        entity->position.y = GetScreenHeight();
        rc++;
    }
    return rc;
}

#define ASTEROID_RADIUS_BIG 32
#define ASTEROID_RADIUS_MEDIUM 16
#define ASTEROID_RADIUS_SMALL 8
#define ASTEROID_POINTS 11
#define LINE_THICKNESS 2
typedef struct
{
    float radius;
    Vector2 points[ASTEROID_POINTS];
    EntityData entity;
} Asteroid;

Asteroid *asteroid_new(float asteroid_radius, Vector2 pos, Vector2 vel)
{
    Asteroid *asteroid = (Asteroid *)malloc(sizeof(Asteroid));
    if (!asteroid)
    {
        TraceLog(LOG_ERROR, "Failed to allocate memory for asteroid");
        exit(1);
    }
    *asteroid = (Asteroid){0};
    asteroid->radius = asteroid_radius;
    asteroid->entity.position = pos;
    asteroid->entity.velocity.linear = vel;
    asteroid->entity.rotation = 0;
    asteroid->entity.type = ET_ASTEROID;
    float max_y = -1, max_x = -1, min_y = 1000, min_x = 1000;
    int i;
    for (i = 0; i < ASTEROID_POINTS; i++)
    {
        float angle = (float)i / ASTEROID_POINTS * 2 * PI;                                                // even distribution of points around the circle
        float radius = asteroid_radius * 0.5 + ((float)GetRandomValue(0, 50) / 100.0f) * asteroid_radius; // random radius variation
        asteroid->points[i] = (Vector2){
            cosf(angle) * radius,
            sinf(angle) * radius};
        if (asteroid->points[i].x > max_x)
        {
            max_x = asteroid->points[i].x;
        }
        if (asteroid->points[i].y > max_y)
        {
            max_y = asteroid->points[i].y;
        }
        if (asteroid->points[i].x < min_x)
        {
            min_x = asteroid->points[i].x;
        }
        if (asteroid->points[i].y < min_y)
        {
            min_y = asteroid->points[i].y;
        }
    }
    // asteroid->entity.hitbox = (Rectangle){ asteroid->entity.position.x, asteroid->entity.position.y, asteroid->radius * 2, asteroid->radius * 2 };
    asteroid->entity.hitshape.points = (Vector2 *)calloc(4, sizeof(Vector2));
    if (!asteroid->entity.hitshape.points)
	{
		TraceLog(LOG_ERROR, "Failed to allocate memory for asteroid hitbox");
		exit(1);
	}
    asteroid->entity.hitshape.points[0] = (Vector2){min_x, min_y};
    asteroid->entity.hitshape.points[1] = (Vector2){max_x, min_y};
    asteroid->entity.hitshape.points[2] = (Vector2){max_x, max_y};
    asteroid->entity.hitshape.points[3] = (Vector2){min_x, max_y};
    asteroid->entity.hitshape.num_points = 4;
    asteroid->entity.hitshape.center = (Vector2){(max_x - min_x) / 2, (max_y - min_y) / 2};

    return asteroid;
}

void asteroid_update(Asteroid *asteroid)
{
    asteroid->entity.position = Vector2Add(asteroid->entity.position, Vector2Scale(asteroid->entity.velocity.linear, 100.0f * GetFrameTime()));
    asteroid->entity.rotation += asteroid->entity.velocity.angular * GetFrameTime();
    int rc = return_to_screen(&asteroid->entity), i;
    if (rc)
    {
        asteroid->entity.rotation += GetRandomValue(0, 360);
        asteroid->entity.velocity.linear = (Vector2){GetRandomValue(-2, 2), GetRandomValue(-2, 2)};
        if (Vector2Equals(asteroid->entity.velocity.linear, Vector2Zero()))
        {
            asteroid->entity.velocity.linear = (Vector2){1, 1};
        }
    }
    asteroid->entity.velocity.linear = Vector2Clamp(asteroid->entity.velocity.linear, (Vector2) { -2, -2 }, (Vector2){2,2});
}

void asteroid_draw(Asteroid *asteroid)
{
    Vector2 center = Vector2Add(asteroid->entity.position, asteroid->entity.hitshape.center);
#ifdef DRAW_HITBOX
    draw_poly_points(asteroid->entity.hitshape.points, asteroid->entity.hitshape.num_points, center, asteroid->entity.rotation, LINE_THICKNESS, asteroid->entity.hitshape.color);
#endif
    for (int i = 0; i < ASTEROID_POINTS; i++)
    {
        // Rotate each point around the center of the asteroid
        Vector2 rotated_point = Vector2Rotate(asteroid->points[i], DEG2RAD * asteroid->entity.rotation);
        Vector2 point = Vector2Add(rotated_point, center);

        // Rotate the next point around the center of the asteroid
        Vector2 next_rotated_point = Vector2Rotate(asteroid->points[(i + 1) % ASTEROID_POINTS], DEG2RAD * asteroid->entity.rotation);
        Vector2 next_point = Vector2Add(next_rotated_point, center);

        // Draw the line between the current point and the next point
        DrawLineEx(point, next_point, LINE_THICKNESS, WHITE);
    }
}

void asteroid_free(Asteroid *asteroid)
{
    free(asteroid->entity.hitshape.points);
    free(asteroid);
}

typedef struct
{
    Vector2 body[4];
    Vector2 trail[3];
    EntityData entity;
    struct
    {
        bool is_immune;
        double last_hit_time;
        float immune_duration;
        bool draw_trail;
        double last_time_shot;
        float shot_cooldown;
    } state;
} Ship;

Ship ship_new(Vector2 pos, Vector2 vel)
{
    Ship ship = {0};
    ship.entity.velocity.linear = vel;
    ship.entity.rotation = 0;
    ship.entity.health = 100;
    ship.entity.type = ET_SHIP;
    // centered at 0,0
    ship.body[0] = (Vector2){-10, -2};
    ship.body[1] = (Vector2){0, 2};
    ship.body[2] = (Vector2){10, -2};
    ship.body[3] = (Vector2){0, 18};
    ship.trail[0] = (Vector2){-5, 2};
    ship.trail[1] = (Vector2){5, 2};
    ship.trail[2] = (Vector2){0, -6};
    ship.state.draw_trail = false;
    ship.state.is_immune = false;
    ship.state.immune_duration = 2.0;
    // ship.entity.hitbox = (Rectangle){ ship.entity.position.x, ship.entity.position.y, 20, 20 };
    ship.entity.hitshape.points = (Vector2 *)calloc(3, sizeof(Vector2));
    ship.entity.hitshape.points[0] = (Vector2){-10, -2};
    ship.entity.hitshape.points[1] = (Vector2){10, -2};
    ship.entity.hitshape.points[2] = (Vector2){0, 18};
    ship.entity.hitshape.center = (Vector2){0, 0};
    ship.entity.hitshape.num_points = 3;
    ship.entity.position = pos; // Vector2Add(pos, ship.entity.hitshape.center);
#ifdef DRAW_HITBOX
    ship.entity.hitshape.color = BLUE;
#endif
    return ship;
}

void ship_update(Ship *ship)
{
    // Handling the rotation
    if (IsKeyDown(KEY_A))
    {
        ship->entity.rotation -= 180 * GetFrameTime(); // Rotate left
    }
    if (IsKeyDown(KEY_D))
    {
        ship->entity.rotation += 180 * GetFrameTime(); // Rotate right
    }

    // Handling the movement
    if (IsKeyDown(KEY_W))
    {
        Vector2 thrust = Vector2Rotate((Vector2){0, 1}, DEG2RAD * ship->entity.rotation);
        thrust = Vector2Scale(thrust, 100.0f * GetFrameTime());
        ship->entity.velocity.linear = Vector2Add(ship->entity.velocity.linear, thrust);
        ship->state.draw_trail = true;
    }
    else
    {
        ship->state.draw_trail = false;
    }
    if (IsKeyDown(KEY_S))
    {
        ship->entity.velocity.linear = Vector2Lerp(ship->entity.velocity.linear, Vector2Zero(), 2.0f * GetFrameTime());
    }

    // Updating the position
    ship->entity.position = Vector2Add(ship->entity.position, Vector2Scale(ship->entity.velocity.linear, GetFrameTime()));
    return_to_screen(&ship->entity);
    if (ship->state.is_immune)
    {
        if (GetTime() - ship->state.last_hit_time > ship->state.immune_duration)
        {
            ship->state.is_immune = false;
        }
    }
}

void draw_dotted_line(Vector2 a, Vector2 b, float thickness, Color color)
{
    Vector2 direction = Vector2Normalize(Vector2Subtract(b, a));
    float length = Vector2Distance(a, b);
    for (float i = 0; i < length; i += 2 * thickness)
    {
        DrawCircle(a.x + direction.x * i, a.y + direction.y * i, thickness, color);
    }
}

void ship_draw(Ship *ship)
{
    // Calculate the actual center of the ship based on its position
    Vector2 actualCenter = Vector2Add(ship->entity.position, ship->entity.hitshape.center);
    if (ship->state.draw_trail)
    {
        draw_poly_points(ship->trail, 3, actualCenter, ship->entity.rotation, LINE_THICKNESS, RED);
    }
    draw_poly_points(ship->body, 4, actualCenter, ship->entity.rotation, LINE_THICKNESS, WHITE);
#ifdef DRAW_HITBOX
    draw_poly_points(ship->entity.hitshape.points, ship->entity.hitshape.num_points, actualCenter, ship->entity.rotation, LINE_THICKNESS, ship->entity.hitshape.color);
#endif
}

void ship_on_hit(Ship *ship)
{
    if (ship->state.is_immune)
    {
        return;
    }
    ship->state.is_immune = true;
    ship->state.last_hit_time = GetTime();
    ship->entity.health -= 10;
    if (ship->entity.health <= 0)
    {
        // Game over
    }
}
void ship_free(Ship *ship)
{
    free(ship->entity.hitshape.points);
}

/* a onto b */
Vector2 Vector2Project(Vector2 a, Vector2 b)
{
    return Vector2Scale(b, Vector2DotProduct(a, b) / Vector2DotProduct(b, b));
}

/* Returns perpendicular vector to edge */
Vector2 Vector2EdgeNormal(Vector2 a, Vector2 b)
{
    Vector2 edge = Vector2Subtract(b, a);
    Vector2 normal = {-edge.y, edge.x};
    return Vector2Normalize(normal);
}

// Function to project a set of points onto an axis and find the min and max projections
void project_onto_vector(Vector2 *points, int pointCount, Vector2 position, float rotation, Vector2 axis, float *min, float *max)
{
    *min = *max = Vector2DotProduct(Vector2Add(Vector2Rotate(points[0], DEG2RAD * rotation), position), axis); // Initialize with the projection of the first point
    for (int i = 1; i < pointCount; i++)
    { // Start loop from the second point
        Vector2 rotatedPoint = Vector2Add(Vector2Rotate(points[i], DEG2RAD * rotation), position);
        float projection = Vector2DotProduct(rotatedPoint, axis); // Compute the projection
        if (projection < *min)
        {
            *min = projection; // Update min if the current projection is smaller
        }
        if (projection > *max)
        {
            *max = projection; // Update max if the current projection is larger
        }
    }
}

// Function to check if two projection intervals overlap
bool is_overlap(float minA, float maxA, float minB, float maxB, float *overlap)
{
    if (maxA < minB || maxB < minA)
    {
        return false; // No overlap
    }
    *overlap = (maxA < maxB ? maxA - minB : maxB - minA);
    return true;
}

bool point_in_polygon(Vector2 *polygon, int count, Vector2 position, float rotation, Vector2 point)
{
    bool result = false;
    for (int i = 0, j = count - 1; i < count; j = i++)
    {
        Vector2 polygon_i = Vector2Add(Vector2Rotate(polygon[i], DEG2RAD * rotation), position);
        Vector2 polygon_j = Vector2Add(Vector2Rotate(polygon[j], DEG2RAD * rotation), position);
        if (((polygon_i.y > point.y) != (polygon_j.y > point.y)) &&
            (point.x < (polygon_j.x - polygon_i.x) * (point.y - polygon_i.y) / (polygon_j.y - polygon_i.y) + polygon_i.x))
        {
            result = !result;
        }
    }
    return result;
}

bool sat_collision(Vector2 *shapeA, int countA, Vector2 positionA, float rotationA, Vector2 *shapeB, int countB, Vector2 positionB, float rotationB, Vector2 *mtv)
{
    float minOverlap = FLT_MAX;
    Vector2 smallestAxis = {0, 0};

    // Normals of shape A
    for (int i = 0; i < countA; i++)
    {
        Vector2 normal = Vector2EdgeNormal(shapeA[i], shapeA[(i + 1) % countA]);
        float minA, maxA, minB, maxB, overlap;
        project_onto_vector(shapeA, countA, positionA, rotationA, normal, &minA, &maxA);
        project_onto_vector(shapeB, countB, positionB, rotationB, normal, &minB, &maxB);
        if (!is_overlap(minA, maxA, minB, maxB, &overlap))
        {
            return false; // Separation found
        }
        if (overlap < minOverlap)
        {
            minOverlap = overlap;
            smallestAxis = normal;
        }
    }

    // Normals of shape B
    for (int i = 0; i < countB; i++)
    {
        Vector2 normal = Vector2EdgeNormal(shapeB[i], shapeB[(i + 1) % countB]);
        float minA, maxA, minB, maxB, overlap;
        project_onto_vector(shapeA, countA, positionA, rotationA, normal, &minA, &maxA);
        project_onto_vector(shapeB, countB, positionB, rotationB, normal, &minB, &maxB);
        if (!is_overlap(minA, maxA, minB, maxB, &overlap))
        {
            return false; // Separation found
        }
        if (overlap < minOverlap)
        {
            minOverlap = overlap;
            smallestAxis = normal;
        }
    }

    // Compute MTV (Minimum Translation Vector)
    if (mtv != NULL)
    {
        *mtv = Vector2Scale(Vector2Normalize(smallestAxis), minOverlap);
    }

    return true; // No separation found, collision detected
}

typedef struct
{
    float damage;
    float radius;
    EntityData entity;
} Projectile;

Projectile projectile_new(float damage, float radius, Vector2 pos, Vector2 vel)
{
    Projectile projectile;
    projectile.damage = damage;
    projectile.radius = radius;
    projectile.entity.position = pos;
    projectile.entity.velocity.linear = vel;
    projectile.entity.rotation = 0;
    projectile.entity.type = ET_PROJECTILE;
    projectile.entity.hitshape.points = (Vector2 *)calloc(4, sizeof(Vector2));
    projectile.entity.hitshape.points[0] = (Vector2){-radius, -radius};
    projectile.entity.hitshape.points[1] = (Vector2){radius, -radius};
    projectile.entity.hitshape.points[2] = (Vector2){radius, radius};
    projectile.entity.hitshape.points[3] = (Vector2){-radius, radius};
    projectile.entity.hitshape.center = (Vector2){0, 0};
    projectile.entity.hitshape.num_points = 4;
    return projectile;
}

void projectile_update(Projectile *projectile)
{
    projectile->entity.position = Vector2Add(projectile->entity.position, Vector2Scale(projectile->entity.velocity.linear, 100.0f * GetFrameTime()));
}

void projectile_draw(Projectile *projectile)
{
    draw_poly_points(projectile->entity.hitshape.points, projectile->entity.hitshape.num_points, Vector2Add(projectile->entity.position, projectile->entity.hitshape.center), projectile->entity.rotation, LINE_THICKNESS, WHITE);
}

void vec_free_asteroid(const void *asteroid)
{
    asteroid_free(*(Asteroid **)asteroid);
}

void asteroid_split(Asteroid *asteroid, Vec *asteroid_ptr_vec)
{
    asteroid_ptr_vec->free_entry = vec_free_asteroid;
    int i;
    if (asteroid->radius == ASTEROID_RADIUS_BIG)
    {
        for (i = 0; i < 4; i++)
        {
            Asteroid *medium = asteroid_new(ASTEROID_RADIUS_MEDIUM, asteroid->entity.position, (Vector2){GetRandomValue(-2, 2), GetRandomValue(-2, 2)});
            if (Vector2Equals(asteroid->entity.velocity.linear, Vector2Zero()))
            {
                medium->entity.velocity.linear = (Vector2){1, 1};
            }
            vec_push_back(asteroid_ptr_vec, &medium);
        }
    }
    else if (asteroid->radius == ASTEROID_RADIUS_MEDIUM)
    {
        Asteroid *asteroid1 = asteroid_new(ASTEROID_RADIUS_SMALL, asteroid->entity.position, (Vector2){GetRandomValue(-2, 2), GetRandomValue(-2, 2)});
        Asteroid *asteroid2 = asteroid_new(ASTEROID_RADIUS_SMALL, asteroid->entity.position, (Vector2){GetRandomValue(-2, 2), GetRandomValue(-2, 2)});
        vec_push_back(asteroid_ptr_vec, &asteroid1);
        vec_push_back(asteroid_ptr_vec, &asteroid2);
    }
}

float Vector2CrossProduct(Vector2 a, Vector2 b)
{
    return a.x * b.y - a.y * b.x;
}

void handle_asteroid_collision1(Asteroid *asteroid0, Asteroid *asteroid1)
{
    // Calculate the point of impact (this is a simplified example, normally you'd need to calculate this)
    Vector2 pointOfImpact = Vector2Add(asteroid0->entity.position, Vector2Scale(asteroid0->entity.velocity.linear, GetFrameTime()));

    // Calculate the relative velocity at the point of impact
    Vector2 relativeVelocity = Vector2Subtract(asteroid0->entity.velocity.linear, asteroid1->entity.velocity.linear);

    // Calculate the normal of the collision
    Vector2 collisionNormal = Vector2Normalize(Vector2Subtract(asteroid0->entity.position, asteroid1->entity.position));

    // Calculate the impulse
    float relativeVelocityAlongNormal = Vector2DotProduct(relativeVelocity, collisionNormal);
    float impulseMagnitude = (-(1 + 0.5f) * relativeVelocityAlongNormal) /
                             (1 / asteroid0->radius + 1 / asteroid1->radius); // assuming uniform density for simplicity

    Vector2 impulse = Vector2Scale(collisionNormal, impulseMagnitude);

    // Apply impulse to the linear velocities
    asteroid0->entity.velocity.linear = Vector2Add(asteroid0->entity.velocity.linear, Vector2Scale(impulse, 1 / asteroid0->radius));
    asteroid1->entity.velocity.linear = Vector2Subtract(asteroid1->entity.velocity.linear, Vector2Scale(impulse, 1 / asteroid1->radius));

    // Calculate the torque (cross product of radius vector and impulse)
    Vector2 radiusVector0 = Vector2Subtract(pointOfImpact, asteroid0->entity.position);
    Vector2 radiusVector1 = Vector2Subtract(pointOfImpact, asteroid1->entity.position);
    float torque0 = Vector2CrossProduct(radiusVector0, impulse);
    float torque1 = Vector2CrossProduct(radiusVector1, impulse);

    // Update angular velocities (assuming moment of inertia is proportional to radius squared)
    asteroid0->entity.velocity.angular += torque0 / (asteroid0->radius * asteroid0->radius);
    asteroid1->entity.velocity.angular -= torque1 / (asteroid1->radius * asteroid1->radius);
}

void handle_asteroid_collision(Asteroid* asteroid0, Asteroid* asteroid1, Vector2 mtv)
{
    // Approximate the point of collision using the MTV
    Vector2 pointOfImpact = Vector2Add(asteroid0->entity.position, Vector2Scale(mtv, 0.5f));

    // Calculate the relative velocity at the point of impact
    Vector2 relativeVelocity = Vector2Subtract(asteroid0->entity.velocity.linear, asteroid1->entity.velocity.linear);

    // Calculate the normal of the collision using MTV
    Vector2 collisionNormal = Vector2Normalize(mtv);

    // Calculate the impulse
    float relativeVelocityAlongNormal = Vector2DotProduct(relativeVelocity, collisionNormal);
    float impulseMagnitude = (-(1 + 0.5f) * relativeVelocityAlongNormal) /
        (1 / asteroid0->radius + 1 / asteroid1->radius); // assuming uniform density for simplicity

    Vector2 impulse = Vector2Scale(collisionNormal, impulseMagnitude);

    // Apply impulse to the linear velocities
    asteroid0->entity.velocity.linear = Vector2Add(asteroid0->entity.velocity.linear, Vector2Scale(impulse, 1 / asteroid0->radius));
    asteroid1->entity.velocity.linear = Vector2Subtract(asteroid1->entity.velocity.linear, Vector2Scale(impulse, 1 / asteroid1->radius));

    // Calculate the torque (cross product of radius vector and impulse)
    Vector2 radiusVector0 = Vector2Subtract(pointOfImpact, asteroid0->entity.position);
    Vector2 radiusVector1 = Vector2Subtract(pointOfImpact, asteroid1->entity.position);
    float torque0 = Vector2CrossProduct(radiusVector0, impulse);
    float torque1 = Vector2CrossProduct(radiusVector1, impulse);

    // Update angular velocities (assuming moment of inertia is proportional to radius squared)
    asteroid0->entity.velocity.angular += torque0 / (asteroid0->radius * asteroid0->radius);
    asteroid1->entity.velocity.angular -= torque1 / (asteroid1->radius * asteroid1->radius);

    // Move the asteroids apart using the MTV to prevent overlap
    asteroid0->entity.position = Vector2Add(asteroid0->entity.position, Vector2Scale(mtv, 0.5f));
    asteroid1->entity.position = Vector2Subtract(asteroid1->entity.position, Vector2Scale(mtv, 0.5f));
}   

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, "Asteroids");
    Ship ship = ship_new((Vector2){500, 225}, (Vector2){500, 225});
    ship.entity.velocity.linear = Vector2Zero();
    Vec *asteroid_ptr_vec = VEC(Asteroid *);
    asteroid_ptr_vec->free_entry = vec_free_asteroid;
    int i;
    for (i = 0; i < 10; i++)
    {
        Asteroid *asteroid = asteroid_new(ASTEROID_RADIUS_BIG, (Vector2){GetRandomValue(0, GetScreenWidth()), GetRandomValue(0, GetScreenHeight())}, (Vector2){1, 1});
        vec_push_back(asteroid_ptr_vec, &asteroid);
    }
    /* Testing collision */
    Asteroid *right_move_asteroid = asteroid_new(ASTEROID_RADIUS_BIG, (Vector2){GetScreenWidth(), GetScreenHeight() / 2}, (Vector2){-1, 0});
    Asteroid *left_move_asteroid = asteroid_new(ASTEROID_RADIUS_BIG, (Vector2){0, GetScreenHeight() / 2}, (Vector2){1, 0});
    vec_push_back(asteroid_ptr_vec, &right_move_asteroid);
    vec_push_back(asteroid_ptr_vec, &left_move_asteroid);
    Vec *projectile_vec = VEC(Projectile);
    ship.state.shot_cooldown = 1.0f / 15.0f;
    bool sim = true;
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        if (IsKeyPressed(KEY_P))
        {
            sim = !sim;
        }
        if (IsKeyPressed(KEY_SPACE) && (GetTime() - ship.state.last_time_shot > ship.state.shot_cooldown))
        {
            ship.state.last_time_shot = GetTime();
            Projectile projectile = projectile_new(10, 2, ship.entity.position, Vector2Rotate((Vector2){0, 3}, DEG2RAD * ship.entity.rotation));
            vec_push_back(projectile_vec, &projectile);
        }
        ship.entity.hitshape.color = BLUE;
        for (i = 0; i < vec_size(asteroid_ptr_vec); i++)
        {
            Asteroid *asteroid = *(Asteroid **)vec_at(asteroid_ptr_vec, i);
            /* Check ship collision with asteroid */
            if (sat_collision(ship.entity.hitshape.points, ship.entity.hitshape.num_points, Vector2Add(ship.entity.position, ship.entity.hitshape.center), ship.entity.rotation, asteroid->entity.hitshape.points, asteroid->entity.hitshape.num_points, Vector2Add(asteroid->entity.position, asteroid->entity.hitshape.center), asteroid->entity.rotation, NULL))
            {
                asteroid->entity.hitshape.color = RED;
                ship.entity.hitshape.color = RED;
            }
            else
            {
                asteroid->entity.hitshape.color = BLUE;
            }
            /* Check asteroid collision with projectile */
            int j = 0;
        continue_projectile_loop:
            for (; j < vec_size(projectile_vec); j++)
            {
                Projectile *projectile = (Projectile *)vec_at(projectile_vec, j);
                if (return_to_screen(&projectile->entity))
                {
                    vec_remove_fast(projectile_vec, j);
                    j--;
                }
                else
                {
                    if (sat_collision(projectile->entity.hitshape.points, projectile->entity.hitshape.num_points, Vector2Add(projectile->entity.position, projectile->entity.hitshape.center), projectile->entity.rotation, asteroid->entity.hitshape.points, asteroid->entity.hitshape.num_points, Vector2Add(asteroid->entity.position, asteroid->entity.hitshape.center), asteroid->entity.rotation, NULL))
                    {
                        vec_remove_fast(projectile_vec, j);
                        j--;
                        asteroid->entity.health -= projectile->damage;
                        if (asteroid->entity.health <= 0)
                        {
                            asteroid_split(asteroid, asteroid_ptr_vec);
                            vec_remove_fast(asteroid_ptr_vec, i);
                            i--;
                            break;
                        }
                        goto continue_projectile_loop;
                        /* Need to do this because continues and breaks apply to the i for loop not the j for loop */
                    }
                }
            }
        }
        /* Check asteroid collision with asteroid */
        for (i = 0; i < vec_size(asteroid_ptr_vec); i++)
        {
            Asteroid *asteroid0 = *(Asteroid **)vec_at(asteroid_ptr_vec, i);
            int j;
            for (j = 0; j < vec_size(asteroid_ptr_vec); j++)
            {
                if (i == j)
                {
                    continue;
                }
                Asteroid *asteroid1 = *(Asteroid **)vec_at(asteroid_ptr_vec, j);
                Vector2 mtv;
                if (sat_collision(asteroid0->entity.hitshape.points, asteroid0->entity.hitshape.num_points, Vector2Add(asteroid0->entity.position, asteroid0->entity.hitshape.center), asteroid0->entity.rotation, asteroid1->entity.hitshape.points, asteroid1->entity.hitshape.num_points, Vector2Add(asteroid1->entity.position, asteroid1->entity.hitshape.center), asteroid1->entity.rotation, &mtv))
                {
                    handle_asteroid_collision(asteroid0, asteroid1, mtv);
                    /*
                    asteroid0->entity.position = Vector2Add(asteroid0->entity.position, Vector2Scale(mtv, 0.25));
                    asteroid1->entity.position = Vector2Subtract(asteroid1->entity.position, Vector2Scale(mtv, 0.25));
                    */
                }
            }
            /* Drag asteroid with mouse */
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse_pos = GetMousePosition();

                if (point_in_polygon(asteroid0->points, ASTEROID_POINTS, Vector2Add(asteroid0->entity.position, asteroid0->entity.hitshape.center), asteroid0->entity.rotation, mouse_pos))
                {
                    Vector2 x = Vector2Subtract(mouse_pos, asteroid0->entity.hitshape.center);
                    Vector2 old_pos = asteroid0->entity.position;
                    asteroid0->entity.position = x;
                    Vector2 dx = Vector2Subtract(asteroid0->entity.position, old_pos);
                    asteroid0->entity.velocity.linear = Vector2Add(asteroid0->entity.velocity.linear, dx);
                }
            }
            if (sim)
                asteroid_update(asteroid0);
            asteroid_draw(asteroid0);
        }
        for (i = 0; i < vec_size(projectile_vec); i++)
        {
            Projectile *projectile = (Projectile *)vec_at(projectile_vec, i);
            if (sim)
                projectile_update(projectile);
            projectile_draw(projectile);
        }
        if (sim)
            ship_update(&ship);
        ship_draw(&ship);
        DrawFPS(0, 0);
        DrawText(TextFormat("Velocity: %f,%f", ship.entity.velocity.linear.x, ship.entity.velocity.linear.y), 0, 20, 20, WHITE);
        DrawText(TextFormat("Position: %f,%f", ship.entity.position.x, ship.entity.position.y), 0, 40, 20, WHITE);
        DrawText(TextFormat("Rotation: %f", ship.entity.rotation), 0, 60, 20, WHITE);
        DrawText(TextFormat("Health: %d", ship.entity.health), 0, 80, 20, WHITE);
        DrawText(TextFormat("Asteroids: %d", vec_size(asteroid_ptr_vec)), 0, 100, 20, WHITE);
        EndDrawing();
    }
    vec_free(asteroid_ptr_vec);
    vec_free(projectile_vec);
    ship_free(&ship);
}