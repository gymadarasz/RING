#include "Ring.cpp"

static Ring ring;

void onSlaveReceive(Pack* pack) {

    test_nequ(
            "Checking: Sender and recipient can not be same in slave device.", 
            pack->to, pack->from, 
            "Slave given a self addressed message.");
    
    debug("(a) SLAVE RECEIVE:", pack);
//    
//    if(pack.to != RING_ADDR_BROADCAST) {
//        pack.to++;
//        pack.buffer[0]++;
//        debug("(b) SLAVE FORWARDING:", pack);
//        ring.send(pack);
//    }
}

void setup() {
    const int bus[] = {3, 4, 5}; 
    ring.init(RING_SLAVE, 1, 0, 2, bus, onSlaveReceive, 3);
}

//void tick() {
//    // ...
//}

void loop() {
    //ring.loop(tick);
}
