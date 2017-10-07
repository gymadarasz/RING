#include "Ring.cpp"
#include "Neuron.cpp"

static Ring ring;

void onSlaveReceive(Pack* pack) {

    test_nequ(
            "Checking: Sender and recipient can not be same in slave device.", 
            pack->to, pack->from, 
            "Slave given a self addressed message.");
    
    debug("(a) SLAVE RECEIVE:", pack);
    
}

void setup() {    
    const int bus_pins[] = {3, 4, 5}; 
    const int bus_pins_analogs[] = {3, 5};
    const Bus bus = {3, bus_pins, 2, bus_pins_analogs};
    ring.init(RING_SLAVE, 1, 0, 2, bus, onSlaveReceive, 3);
    ring.start();
}

//void tick() {
//    // ...
//}

void loop() {
    //ring.loop(tick);
}
