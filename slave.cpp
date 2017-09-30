#include "Ring.cpp"

static Ring ring;

#define BUFFER_SIZE_SLAVE 100

int buffer[BUFFER_SIZE_SLAVE];

Pack packQueue1;
Pack packQueue2;

int buffer1[] = {1001, -1001, 123};
int buffer2[] = {4001, -4001, 456};

void onSlaveReceive(Pack pack) {
    debug("SLAVE RECEIVE, from:", pack.address);
    debug("length:", pack.length);
    debug("data[0]:", pack.buffer[0]);
    debug("data[1]:", pack.buffer[1]);
    debug("data[2]:", pack.buffer[2]);
    
    packQueue1.buffer = buffer1;
    packQueue1.length = 3;
    ring.sendQueue(pack.address, packQueue1);
    
    packQueue2.buffer = buffer2;
    packQueue2.length = 3;
    ring.sendQueue(4, packQueue2);
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
