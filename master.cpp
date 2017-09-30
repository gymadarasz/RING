#include "Ring.cpp"

static Ring ring;

void setup() {
    Bus bus = {3, 4, 5}; 
    int length = ring.init(RING_MASTER, 1, 0, 2, bus);
    debug("length:", length);
    
    //delay(100);
    //simulation->stop();
}

unsigned int cnt = 1000;
void tick() {
    //debug("tick");
    ring.send(cnt++);
}

void loop() {
    ring.loop(tick);
}

