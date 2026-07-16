#pragma once

class IThreadedComponent {
public:
    virtual void run() = 0;
    virtual bool isThreadReady() const = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual ~IThreadedComponent() = default;
};
