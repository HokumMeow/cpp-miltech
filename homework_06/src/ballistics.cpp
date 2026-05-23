#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <cstring>
#include "ballistics.hpp"

using namespace std;

const float g = 9.81f;

struct AmmoParams
{
    float mass;
    float drag;
    float lift;
};

float calc_t(const AmmoParams& a, float attackSpeed, float zd)
{
    float d = a.drag;
    float m = a.mass;
    float l = a.lift;
    float a_val = d * g * m - 2.f * d * d * l * attackSpeed;
    float b = -3.f * g * m * m + 3.f * d * l * m * attackSpeed;
    float c = 6.f * m * m * zd;
    float p = (-(b * b)) / (3.f * a_val * a_val);
    float q = 2.f * b * b * b / (27.f * a_val * a_val * a_val) + c / a_val;
    double acosArg = 3.f * q / (2.f * p) * sqrtf(-3.f / p);
    if (acosArg < -1.0 || acosArg > 1.0)
    {
        cerr << "phi out of range" << endl;
        return -1;
    }
    float phi = acosf(acosArg);
    float t = 2.f * sqrtf(-p / 3.f) * cosf((phi + 4.0 * M_PI) / 3.f) - b / (3.0 * a_val);
    if (t <= 0)
    {
        return -1;
    }
    return t;
}

float calc_h(const AmmoParams& a, float attackSpeed, float t)
{
    float t2 = t * t, t3 = t2 * t, t4 = t3 * t, t5 = t4 * t;
    float d2 = a.drag * a.drag, d3 = d2 * a.drag, d4 = d3 * a.drag;
    float l2 = a.lift * a.lift, l3 = l2 * a.lift, l4 = l3 * a.lift;
    float m2 = a.mass * a.mass, m3 = m2 * a.mass, m4 = m3 * a.mass;
    float result = attackSpeed * t - t2 * a.drag * attackSpeed 
    / (2.f * a.mass) + t3 * (6.f * a.drag * g * a.lift * a.mass - 6.f * d2 * (l2 - 1.f) * attackSpeed) 
    / (36.f * m2) + t4 * (-6.f * d2 * g * a.lift * (1.f + l2 + l4) * a.mass + 3.f * d3 * l2 * (1.f + l2) * attackSpeed + 6.f * d2 * a.drag * l4 * (1.f + l2) * attackSpeed) 
    / (36.f * (1.f + l2) * (1.f + l2) * m3) + t5 * (3.f * d3 * g * l3 * a.mass - 3.f * d4 * l2 * (1.f + l2) * attackSpeed) 
    / (36.f * (1.f + l2) * m4);
    return result;
}

static AmmoParams lookup_ammo(const char* name)
{
    AmmoParams ammo;
    if (strcmp(name, "VOG-17") == 0) {
        ammo.mass = 0.35f; ammo.drag = 0.07f; ammo.lift = 0.0f;
    } else if (strcmp(name, "M67") == 0) {
        ammo.mass = 0.6f; ammo.drag = 0.1f; ammo.lift = 0.0f;
    } else if (strcmp(name, "RKG-3") == 0) {
        ammo.mass = 1.2f; ammo.drag = 0.1f; ammo.lift = 0.0f;
    } else if (strcmp(name, "GLIDING-VOG") == 0) {
        ammo.mass = 0.45f; ammo.drag = 0.1f; ammo.lift = 1.0f;
    } else if (strcmp(name, "GLIDING-RKG") == 0) {
        ammo.mass = 1.4f; ammo.drag = 0.1f; ammo.lift = 1.0f;
    } else {
        cerr << "Unknown ammo" << endl;
        exit(1);
    }
    return ammo;
}

DropSolution compute_drop_solution(const BallisticsInput& input) {
    AmmoParams ammo = lookup_ammo(input.ammo_name);
   
    
    float t = calc_t(ammo, input.attack_speed, input.drone_z);
    
    if (t <= 0){
       cerr << "t out of range" << endl;
       return {0.f, 0.f};
    }

    float h = calc_h(ammo, input.attack_speed, t);

    float D = sqrtf(powf(input.target_x-input.drone_x,2.f) + powf(input.target_y-input.drone_y,2.f));
    if (D <= 0){
       cerr << "D out of range" << endl;
       return {0.f, 0.f};
    }

    float ratio = (D-h) / D;
    float fireX = input.drone_x + (input.target_x - input.drone_x)*ratio;
    float fireY = input.drone_y + (input.target_y - input.drone_y)*ratio;


    DropSolution solution;
    solution.fire_x = fireX;
    solution.fire_y = fireY;
    return solution;
}