#include <iostream>
#include <fstream>

#include "ballistics.hpp"

using namespace std;

struct AmmoParams
{
    float mass;
    float drag;
    float lift;
};

int main(int argc, char* argv[])
{
    
    if (argc < 3) {
        cerr << "usage: ballistic <input_path> <output_path>" << endl;
        return 1;
    }

    BallisticsInput input;

    char ammo_name[12];

    ifstream file_input(argv[1]);
    if (!file_input.is_open()) {
        cerr << "Error opening file" << endl;
        return 1;
    }

    file_input >> input.drone_x >> input.drone_y >> input.drone_z >> input.target_x >> input.target_y >> input.attack_speed >> input.acceleration_path >> ammo_name;

    file_input.close();

    auto solution_opt = compute_drop_solution(input);
    if (!solution_opt) {    
        cerr << "No drop solution found" << endl;
        return 1;
    }
    DropSolution solution = *solution_opt;

    ofstream OutputFile(argv[2]);
    if (!OutputFile.is_open()) {
        cerr << "Error opening output file" << endl;
        return 1;
    }

    if (solution.intermediate_x != 0.f && solution.intermediate_y != 0.f) {
        OutputFile << solution.intermediate_x << " " << solution.intermediate_y << " ";
    }

    OutputFile << solution.fire_x << " " << solution.fire_y << endl;
    OutputFile.close();

    return 0;

}