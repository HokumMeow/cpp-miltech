#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;

int main(int argc, char* argv[]) {

/*     if (argc < 2) {
        cout << "Error: No input file specified!" << endl;
        return 1;
    } */

    // ifstream input(argv[1]);
    ifstream input("data/combined.txt");
    if (!input.is_open()) {
        cout << "Error opening input file" << endl;
        return 1;
    }

    ofstream OutpuFile("output.txt");
    if (!OutpuFile.is_open()) {
        cout << "Error opening output file" << endl;
        return 1;
    }

    const float wheel_radius_m = 0.3f;
    const int ticks_per_revolution = 1024;
    const float wheelbase_m = 1.0f;

    long timestamp_ms = 0, fl_ticks = 0, fr_ticks = 0, bl_ticks = 0, br_ticks = 0;
    long d_fl = 0, d_fr = 0, d_bl = 0, d_br = 0;
    long prev_fl = 0, prev_fr = 0, prev_bl = 0, prev_br = 0;
    float distance_per_tick = 0.0f;
    float x = 0.0f, y = 0.0f, theta = 0.0f;
    bool firstLine = true;

    distance_per_tick = static_cast<float>(2.0f * M_PI * wheel_radius_m / ticks_per_revolution);

    while (input >> timestamp_ms >> fl_ticks >> fr_ticks >> bl_ticks >> br_ticks) {
        if (firstLine) {
            firstLine = false;
        } else {

            //Крок 1. Delta iмпульсiв по кожному колесу:
            d_fl = fl_ticks - prev_fl;
            d_fr = fr_ticks - prev_fr;
            d_bl = bl_ticks - prev_bl;
            d_br = br_ticks - prev_br;

            //Крок 2. Усереднити борти (передне i заднє колесо одного боку обертаються синхронно):
            float d_left = (d_fl + d_bl) / 2.f;
            float d_right = (d_fr + d_br) / 2.0f;

            //Крок 3. Перевести iмпульси у метри:
            
            float dL = d_left * distance_per_tick;
            float dR = d_right * distance_per_tick;
            //Крок 4. Скiльки пройшов центр робота i на скiльки повернувся:
            float d = (dL + dR) / 2.f; // пройдена вiдстань центру
            float dtheta = (dR - dL) / wheelbase_m; // змiна орiєнтацiї
            //Крок 5. Оновити позицiю (midpoint integration - усереднений напрямок на кроцi):
            x += d * cos(theta + dtheta / 2.f);
            y += d * sin(theta + dtheta / 2.f);
            theta += dtheta;

            OutpuFile << timestamp_ms << " " << x << " " << y << " " << theta << endl;

        }
        prev_fl = fl_ticks;
        prev_fr = fr_ticks;
        prev_bl = bl_ticks;
        prev_br = br_ticks;


    }

    input.close();
    OutpuFile.close();

    return 0;

}    