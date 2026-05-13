#include "telemetry.hpp"

#include <iostream>

int main(int argc, char** argv) {
    // The executable expects exactly one telemetry log path.
    if (argc != 2) {
        std::cerr << "usage: telemetry_check <input_path>\n";
        return 1;
    }

    Frame frames[MAX_TELEMETRY_FRAMES];
    const int frame_count = read_frames(argv[1], frames, MAX_TELEMETRY_FRAMES);

    // вважаєм щось впало при читанні
    if (frame_count == -1) {
        std::cerr << "error parsing frames\n";
        return 1;
    }
    // вважаєм що немає даних для аналізу
    if (frame_count == 0) {
        std::cerr << "error: no frames found\n";
        return 1;
    }

    const Summary summary = summarize(frames, frame_count);
    print_summary(summary);

    return 0;
}
