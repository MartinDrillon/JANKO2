#include "midi_out.h"
#include <usb_midi.h>

namespace {
// Simple SPSC ring buffer
constexpr size_t kQueueSize = 128; // power of two for cheap masking
MidiOut::Event qbuf[kQueueSize];
volatile uint16_t qHead = 0; // write index
volatile uint16_t qTail = 0; // read index

inline uint16_t nextIndex(uint16_t idx) { return static_cast<uint16_t>((idx + 1) & (kQueueSize - 1)); }
inline bool isFull(uint16_t head, uint16_t tail) { return nextIndex(head) == tail; }
inline bool isEmpty(uint16_t head, uint16_t tail) { return head == tail; }

void sendEvent(const MidiOut::Event& ev) {
    switch (ev.kind) {
        case MidiOut::Kind::NoteOn:
            usbMIDI.sendNoteOn(ev.d1, ev.d2, ev.ch);
            break;
        case MidiOut::Kind::NoteOff:
            usbMIDI.sendNoteOff(ev.d1, ev.d2, ev.ch);
            break;
        case MidiOut::Kind::CC:
            usbMIDI.sendControlChange(ev.d1, ev.d2, ev.ch);
            break;
    }
}
} // namespace

namespace MidiOut {

void init() {
    // nothing yet
}

bool enqueue(const Event& ev) {
    uint16_t head = qHead;
    uint16_t tail = qTail; // snapshot
    if (isFull(head, tail)) {
        return false; // drop if full (non-blocking)
    }
    qbuf[head] = ev;
    qHead = nextIndex(head);
    return true;
}

void service(uint32_t budgetUs) {
    // Fast early-out: if queue is empty, do nothing
    {
        uint16_t head = qHead;
        uint16_t tail = qTail;
        if (isEmpty(head, tail)) return;
    }
    const uint32_t start = micros();
    bool didSend = false;
    // Drain until time budget or empty
    while (true) {
        uint16_t head = qHead; // snapshot
        uint16_t tail = qTail;
        if (isEmpty(head, tail)) break;
        const Event ev = qbuf[tail];
        qTail = nextIndex(tail);

        sendEvent(ev);
        didSend = true;

        // If we've spent the budget, flush and return
        if ((micros() - start) >= budgetUs) {
            if (didSend) usbMIDI.send_now();
            return;
        }
    }
    // Flush only if we actually pushed events this round
    if (didSend) usbMIDI.send_now();
}

} // namespace MidiOut
