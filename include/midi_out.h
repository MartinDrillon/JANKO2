#pragma once
#include <Arduino.h>

namespace MidiOut {
    enum class Kind : uint8_t { NoteOn = 0, NoteOff = 1, CC = 2 };

    struct Event {
        Kind kind;
        uint8_t ch;   // MIDI channel 1..16
        uint8_t d1;   // note or CC number
        uint8_t d2;   // velocity or CC value
    };

    // Initialize (placeholder for future backends)
    void init();

    // Non-blocking enqueue (returns false if queue full)
    bool enqueue(const Event& ev);

    // Drain queue for up to budgetUs microseconds (non-blocking overall)
    void service(uint32_t budgetUs);
}
