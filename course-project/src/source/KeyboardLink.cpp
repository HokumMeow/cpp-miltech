#include "source/KeyboardLink.h"

#include <poll.h>
#include <unistd.h>

void KeyboardLink::poll() {
    pollfd pfd{STDIN_FILENO, POLLIN, 0};

    while (::poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) <= 0) return;

        if (c == 'l' || c == 'L') linkOk_ = !linkOk_;
        if (c == 'q' || c == 'Q') quit_ = true;
    }
}
