#include "Ring.cpp"

static Ring ring;

void onMasterReceive(Pack* pack) {
    test_nequ(
            "Checking: Sender and recipient can not be same in master device.", 
            pack->to, pack->from, 
            "Master given a self addressed message.");

    debug("(f) MASTER RECEIVE:", pack);
}

void setup() {
    const int bus[] = {3, 4, 5}; 
    ring.init(RING_MASTER, 1, 0, 2, bus, onMasterReceive, 3);
    
//    
//    Pack pack;
//
//    
//    pack.from = 0;
//    pack.to = RING_ADDR_BROADCAST;
//    pack.length = 1;
//    pack.buffer[0] = 1234;
//    debug("(d) MASTER SEND (*BROADCASK*):", &pack);
//    ring.send(&pack);
//    
//    pack.buffer[0] = 1000;
//    pack.to = 1;
//    debug("(e) MASTER SEND:", &pack);
//    ring.send(&pack);
//    
//    //delay(100);
//    //simulation->stop();
}

//void tick() {
//    // ...
//}

void loop() {
    //ring.loop(tick);
    
}

