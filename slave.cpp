#include "Ring.cpp"

static Ring ring;

void setup() {
    Bus bus = {3, 4, 5}; 
    ring.init(RING_SLAVE, 1, 0, 2, bus);
}

void tick() {
    // ...
}

void loop() {
    ring.loop(tick);
}
