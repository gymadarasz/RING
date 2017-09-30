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

void setup() {
    Bus bus = {3, 4, 5}; 
    int length = ring.init(RING_IS_MASTER, 1, 0, 2, bus, onMasterReceive, buffer);
    debug("address:", ring.address);
    debug("multiple:", ring.multiple);
    debug("length:", length);
    
    //delay(100);
    //simulation->stop();
}

//int cnt = 1;
//void tick() {
//    //debug("tick");
//    ring.send(-cnt);
//    //ring.send(-cnt);
//    cnt++;
//}

void tick() {
    Pack pack1;
    int buffer1[] = {100,200,300};
    pack1.from = ring.address;
    pack1.to = 1;
    pack1.buffer = buffer1;
    pack1.length = 3;
    
    Pack pack2;
    int buffer2[] = {-100,-200,-300};
    pack2.from = ring.address;
    pack2.to = 2;
    pack2.buffer = buffer2;
    pack2.length = 3;
    
    debug("(d) MASTER SEND:", pack1);
    ring.send(pack1);
    debug("(e) MASTER SEND:", pack2);
    ring.send(pack2);
}

void loop() {
    ring.loop(tick);
    
}

