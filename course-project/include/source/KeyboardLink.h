#pragma once

// l вмикає/вимикає втрату зв'язку
// q вихід з програми
class KeyboardLink {
public:
    void poll();

    bool linkOk() const { return linkOk_; }
    bool quitRequested() const { return quit_; }

private:
    bool linkOk_ = true;
    bool quit_ = false;
};
