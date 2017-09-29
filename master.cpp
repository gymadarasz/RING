#include "Ring.cpp"

static Ring ring;

void setup() {
    RING(RING_MASTER, 1, 0, 2, 3, 4, 5);
    debug("length:", length);
    
    delay(10);
    simulation->stop();
}

void loop() {
}
