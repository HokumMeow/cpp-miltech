#include "interfaces/ITargetProvider.h"

enum class SourceType { JSON, SERIAL, TEST };
 
ITargetProvider* createProvider(
    SourceType type, const char* param) {
    switch (type) {
    case SourceType::JSON:
        return new JsonTargetProvider(param);
    case SourceType::SERIAL:
        return new SerialTargetProvider(param);
    case SourceType::TEST:
        return new TestTargetProvider();
    default: return nullptr;
    }
}

