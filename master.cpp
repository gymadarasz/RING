#include "Ring.cpp"

#define BUFFER_SIZE_MASTER 100

static Ring ring;

int buffer[BUFFER_SIZE_MASTER];

void onMasterReceive(Pack pack) {
    if(pack.from == ring.address) {
        error(1);
    }
    if(pack.to != ring.address) {
        error(2);
    }
    debug("(f) MASTER RECEIVE:", pack);
}

Pack pack;
int obuffer[10];

void setup() {
    Bus bus = {3, 4, 5}; 
    int length = ring.init(RING_IS_MASTER, 1, 0, 2, bus, onMasterReceive, buffer);
    debug("address:", ring.address);
    debug("multiple:", ring.multiple);
    debug("length:", length);
    
    
    pack.from = ring.address;
    pack.to = RING_ADDR_BROADCAST;
    pack.buffer = obuffer;
    pack.length = 1;
    obuffer[0] = 1234;
    debug("(d) MASTER SEND (*BROADCASK*):", pack);
    ring.send(pack);
    
    obuffer[0] = 1000;
    pack.to = 1;
    debug("(e) MASTER SEND:", pack);
    ring.send(pack);
    
    //delay(100);
    //simulation->stop();
}

void tick() {
    // ...
}

void loop() {
    ring.loop(tick);
    
}

