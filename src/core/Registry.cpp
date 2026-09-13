#include "Registry.h"

#include <string.h>

namespace core {
namespace {

// Function-local static: the list exists before the first registrar runs,
// whatever order the translation units are initialised in.
EffectInfo*& listHead() {
    static EffectInfo* head = nullptr;
    return head;
}

}  // namespace

void Registry::add(EffectInfo& info) {
    // Append, so the catalogue keeps link order and the UI list is stable.
    EffectInfo** slot = &listHead();
    while (*slot != nullptr) slot = &(*slot)->next;
    *slot = &info;
    info.next = nullptr;
}

EffectInfo* Registry::head() { return listHead(); }

EffectInfo* Registry::find(const char* name) {
    for (EffectInfo* e = listHead(); e != nullptr; e = e->next)
        if (strcmp(e->name, name) == 0) return e;
    return nullptr;
}

EffectInfo* Registry::at(uint16_t index) {
    for (EffectInfo* e = listHead(); e != nullptr; e = e->next)
        if (index-- == 0) return e;
    return nullptr;
}

uint16_t Registry::count() {
    uint16_t n = 0;
    for (EffectInfo* e = listHead(); e != nullptr; e = e->next) ++n;
    return n;
}

}  // namespace core
