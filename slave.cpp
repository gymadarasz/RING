#include "Ring.cpp"

static Ring ring;

#define BUFFER_SIZE_SLAVE 100

int buffer[BUFFER_SIZE_SLAVE];

void onSlaveReceive(Pack pack) {
    debug("SLAVE RECEIVE, from:", pack.from);
    debug("length:", pack.length);
    debug("data[0]:", pack.buffer[0]);
    debug("data[1]:", pack.buffer[1]);
    debug("data[2]:", pack.buffer[2]);
}

void setup() {
    Bus bus = {3, 4, 5}; 
    ring.init(RING_SLAVE, 1, 0, 2, bus, onSlaveReceive, buffer);
}

void tick() {
    // ...
}

void loop() {
    ring.loop(tick);
}
