
#include "Neuron.cpp"

static Block block;

void onSlaveReceive(Pack pack) {

//    test_nequ(
//            "Checking: Sender and recipient can not be same in slave device.", 
//            pack->to, pack->from, 
//            "Slave given a self addressed message.");
    
    debug("(a) SLAVE RECEIVE:", pack);
    
}

void setup() {
    debug("Slave start..");
    
    Bus busDigital = {3,  3, 4, 5};
    Bus busAnalog = {2,  4, 5};
    BusDA bus = {busDigital, busAnalog};
    block.init(RING_SLAVE, 1, 0, 2, bus, 0, 3);
    block.start();
}

//void tick() {
//    // ...
//}

void loop() {
    //ring.loop(tick);
}
