#pragma once
// The catalogue of effects.
//
// Effects register themselves: REGISTER_EFFECT at the bottom of an effect's
// .cpp links it into the list before main() runs. There is no enum to extend,
// no name string to keep in sync and no switch to edit - adding an effect
// touches exactly one file.
//
// Construction uses placement new into a static arena, so switching effects
// never touches the heap. An effect too large for the arena fails to compile
// instead of failing in the field.

#include <stdint.h>

#include "Effect.h"

namespace core
{
    enum class Tag : uint8_t
    {
        Ambient, // slow, meant to be lived with
        Dynamic, // fast, meant to be watched
        Reactive, // driven by the microphone
        System, // notifications, diagnostics
    };

    struct EffectInfo
    {
        const char* name;
        Tag tag;
        uint16_t size;

        Effect* (*construct)(void* storage);

        EffectInfo* next;
    };

    class Registry
    {
    public:
        static void add(EffectInfo& info) noexcept;

        static EffectInfo* head();

        static EffectInfo* find(const char* name);

        static EffectInfo* at(uint16_t index);

        // Position of a registered effect; 0 for one that is not.
        static uint16_t indexOf(const EffectInfo* info);

        static uint16_t count();
    };

    struct Registrar
    {
        explicit Registrar(EffectInfo& info) noexcept { Registry::add(info); }
    };
}

#define REGISTER_EFFECT(Type, DisplayName, TagValue)                        \
    static_assert(sizeof(Type) <= EFFECT_ARENA_SIZE,                        \
                  #Type " does not fit into EFFECT_ARENA_SIZE");            \
    static ::core::Effect* construct_##Type(void* storage) {                \
        return new (storage) Type();                                        \
    }                                                                       \
    static ::core::EffectInfo info_##Type{                                  \
        DisplayName, TagValue, sizeof(Type), &construct_##Type, nullptr};   \
    static const ::core::Registrar registrar_##Type{info_##Type};
