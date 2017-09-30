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
    if(pack.to != ring.address) {
        error(4);
    }
    debug("SLAVE RECEIVE:", pack);
    
    packQueue1.from = pack.to;
    packQueue1.to = pack.from;
    packQueue1.buffer = buffer1;
    packQueue1.length = 3;
    debug("SLAVE SEND:", packQueue1);   
    ring.send(packQueue1);
    
    packQueue2.from = pack.to;
    packQueue2.to = 4;
    packQueue2.buffer = buffer2;
    packQueue2.length = 3;
    debug("SLAVE SEND:", packQueue1); 
    ring.send(packQueue2);
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
