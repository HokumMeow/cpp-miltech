#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;

int main(int argc, char** argv) {

    if (argc != 2) {
        cerr << "usage: ugv_odometry <input_path>\n";
        return 1;
    }

    ifstream inputFile(argv[1]);

    if (!inputFile.is_open()) {
        cout << "Error opening input file" << endl;
        return 1;
    }

    ofstream outputFile("output.txt");
    if (!outputFile.is_open()) {
        cout << "Error opening output file" << endl;
        return 1;
    }

    const float wheel_radius_m = 0.3f;
    const int ticks_per_revolution = 1024;
    const float wheelbase_m = 1.0f;

    long timestamp_ms = 0, fl_ticks = 0, fr_ticks = 0, bl_ticks = 0, br_ticks = 0;
    
    while (inputFile >> timestamp_ms >> fl_ticks >> fr_ticks >> bl_ticks >> br_ticks) {
        

    }

    inputFile.close();
    outputFile.close();

    return 0;
}