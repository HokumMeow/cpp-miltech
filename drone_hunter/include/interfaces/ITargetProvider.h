#pragma once
#include "dto/Target.h"
#include "interfaces/IThreadedComponent.h"

// Назовні цілі видно лише як знімок поточних значень під мьютексом.
// Жодних запитів "де ціль була/буде" у довільний момент часу — це
// узгоджено з тим, що джерело даних може бути не лише файлом траєкторій,
// а й зовнішньою телеметрією, яка знає тільки "зараз".
class ITargetProvider : public IThreadedComponent {
public:
    virtual int getTargetCount() const = 0;
    virtual Target getTarget(int idx) const = 0;
    virtual ~ITargetProvider() override {}
};
