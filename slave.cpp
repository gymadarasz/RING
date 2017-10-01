#include "Ring.cpp"

static Ring ring;

#define BUFFER_SIZE_SLAVE 100

int buffer[BUFFER_SIZE_SLAVE];

Pack packQueue1;
Pack packQueue2;

int buffer1[] = {1001, -1001, 123};
int buffer2[] = {4001, -4001, 456};

void onSlaveReceive(Pack pack) {
    if(pack.from == ring.address) {
        error(3);
    }
    if(pack.to != RING_ADDR_BROADCAST && pack.to != ring.address) {
        error(4);
    }
    debug("(a) SLAVE RECEIVE:", pack);
    
    if(pack.to != RING_ADDR_BROADCAST) {
        pack.to++;
        pack.buffer[0]++;
        debug("(b) SLAVE FORWARDING:", pack);
        ring.send(pack);
    }
}

void setup() {
    Bus bus = {3, 4, 5}; 
    ring.init(RING_IS_SLAVE, 1, 0, 2, bus, onSlaveReceive, buffer);
    debug("address:", ring.address);
    debug("multiple:", ring.multiple);
}

void tick() {
    // ...
}

void loop() {
    ring.loop(tick);
}
