#include "Ring.cpp"

#define BUFFER_SIZE_MASTER 100

static Ring ring;

int buffer[BUFFER_SIZE_MASTER];

void onMasterReceive(Pack pack) {
    debug("MASTER RECEIVE, from:", pack.address);
    debug("length:", pack.length);
    debug("data[0]:", pack.buffer[0]);
    debug("data[1]:", pack.buffer[1]);
    debug("data[2]:", pack.buffer[2]);
}

void setup() {
    Bus bus = {3, 4, 5}; 
    int length = ring.init(RING_MASTER, 1, 0, 2, bus, onMasterReceive, buffer);
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
    pack1.buffer = buffer1;
    pack1.length = 3;
    
    Pack pack2;
    int buffer2[] = {-100,-200,-300};
    pack2.buffer = buffer2;
    pack2.length = 3;
    
    ring.send(1, pack1);
    ring.send(2, pack2);
}

void loop() {
    ring.loop(tick);
}

