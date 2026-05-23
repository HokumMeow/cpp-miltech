#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

using namespace std;

int main(int argc, char* argv[])
{
    
    if (argc < 3) {
        cerr << "usage: telemetry <input_path> <output_path>\n";
        return 1;
    }

    const float g = 9.81f;
    float xd = 0.0f, yd = 0.0f, zd = 0.0f, targetX = 0.0f, targetY = 0.0f, attackSpeed = 0.0f, accelerationPath = 0.0f, ratio = 0.0f;
    char ammo_name[12];
    float m = 0.f, d = 0.f, l = 0.f;
    double a = 0.0, b = 0.0, c = 0.0, p = 0.0, q = 0.0, phi = 0.0, t = 0.0;
    float h = 0.f, D = 0.f;
    float fireX = 0.f, fireY = 0.f;

    ifstream input(argv[1]);
    if (!input.is_open()) {
        cout << "Error opening file" << endl;
        return 1;
    }

    input >> xd >> yd >> zd >> targetX >> targetY >> attackSpeed >> accelerationPath >> ammo_name;

    input.close();

    if (strcmp(ammo_name, "VOG-17") == 0) {
        m = 0.35f; d = 0.07f; l = 0.0f;
    } else if (strcmp(ammo_name, "M67") == 0) {
        m = 0.6f; d = 0.1f; l = 0.0f;
    } else if (strcmp(ammo_name, "RKG-3") == 0) {
        m = 1.2f; d = 0.1f; l = 0.0f;
    } else if (strcmp(ammo_name, "GLIDING-VOG") == 0) {
        m = 0.45f; d = 0.1f; l = 1.0f;
    } else if (strcmp(ammo_name, "GLIDING-RKG") == 0) {
        m = 1.4f; d = 0.1f; l = 1.0f;
    } else {
        cout << "Unknown ammo" << endl;
        return 1;
    }

    a = d*g*m - 2.f*powf(d,2.f) * l * attackSpeed;
    b = -3.f * g * powf(m,2.f) + 3.f * d * l * m * attackSpeed;
    c = 6.f * powf(m,2.f) * zd;
    p = (-powf(b,2.f)) / (3.f*powf(a,2.f));
    q = 2.f * powf(b,3.f) / (27.f * powf(a,3.f)) + c / a;
    double acosArg = 3.f * q / (2.f * p) * sqrtf(-3.f / p);
    if (acosArg < -1.0 || acosArg > 1.0){
       cout << "phi out of range" << endl;
       return 1;
    }
    phi = acosf(acosArg);

    t = 2.f*sqrtf(-p/3.f) * cosf( ( phi + 4.0 * M_PI ) / 3.f ) - b / (3.0*a);
    if (t <= 0){
       cout << "t out of range" << endl;
       return 1;
    }

    // h = V₀t − t²d·V₀/(2m) + t³(6d·g·l·m − 6d²(l²-1)·V₀)/(36m²) 
    // + t⁴ (−6d²g·l·(1+l²+l⁴)m + 3d³l²(1+l²)V₀ + 6d³l⁴(1+l²)V₀) 
    // / (36(1+l²)²m³) + t⁵(3d³g·l³m − 3d⁴l²(1+l²)V₀) / (36(1+l²)m⁴)
    h = attackSpeed * t - powf(t,2.f) * d * attackSpeed / (2.f*m) + powf(t,3.f) * (6.f * d * g * l * m - 6.f * powf(d,2.f) * (powf(l,2.f) - 1.f) * attackSpeed) / (36.f * powf(m,2.f))
        + powf(t,4.f) * (-6.f * powf(d,2.f) * g * l * (1.f + powf(l,2.f) + powf(l,4.f)) * m + 3.f * powf(d,3.f) * powf(l,2.f) * (1.f +  powf(l,2.f)) * attackSpeed + 6.f * powf(d,3.f)* powf(l,4.f) * (1.f + powf(l,2.f)) * attackSpeed)
        / (36.f * powf(1.f + powf(l,2.f),2.f) * powf(m,3.f) ) + powf(t,5.f) * (3.f * powf(d,3.f) * g * powf(l,3.f) * m - 3.f * powf(d,4.f) * powf(l,2.f) * (1.f + powf(l,2.f)) * attackSpeed ) / (36.f * (1.f + powf(l,2.f)) * powf(m,4.f));
    if (h <= 0.f){
       cout << "h out of range" << endl;
       return 1;
    }

    // D = √( (targetX − xd)² + (targetY − yd)² )
    D = sqrtf(powf(targetX-xd,2.f) + powf(targetY-yd,2.f));
    if (D <= 0){
       cout << "D out of range" << endl;
       return 1;
    }

    ratio = (D-h) / D;
    fireX = xd + (targetX - xd)*ratio;
    fireY = yd + (targetY - yd)*ratio;

    ofstream OutpuFile(argv[2]);
    if (!OutpuFile.is_open()) {
        cout << "Error opening output file" << endl;
        return 1;
    }

    if (h + accelerationPath > D) {
        // xd' = targetX − (targetX − xd) · (h + accelerationPath) / D
        // yd' = targetY − (targetY − yd) · (h + accelerationPath) / D
        xd = targetX - (targetX-xd) * (h + accelerationPath) / D;
        yd = targetY - (targetY-yd) * (h + accelerationPath) / D;
        OutpuFile << xd << " " << yd << " ";
    }
    OutpuFile << fireX << " " << fireY;
    OutpuFile.close();

    return 0;

}