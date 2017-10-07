#include "Ring.cpp"
#include "Neuron.cpp"

static Ring ring;

void onMasterReceive(Pack* pack) {
    test_nequ(
            "Checking: Sender and recipient can not be same in master device.", 
            pack->to, pack->from, 
            "Master given a self addressed message.");

    debug("(f) MASTER RECEIVE:", pack);
}


int teste;

void setup() {
    teste = 0;
    
    const int bus_pins[] = {3, 4, 5}; 
    const int bus_pins_analogs[] = {3, 5};
    const Bus bus = {3, bus_pins, 2, bus_pins_analogs};
    ring.init(RING_MASTER, 1, 0, 2, bus, onMasterReceive, 3);
    ring.start();
}



void loop() {
    //ring.loop(tick);
    if(teste == 0) {
        int buffer[] = {124};
        ring.send(ring.pack.make(0, 2, 1, buffer, RING_PACKTYPE_ANALOG));
        buffer[0] = 221;
        ring.send(ring.pack.make(0, 4, 1, buffer, RING_PACKTYPE_ANALOG));
        teste = 1;
    }
    
}

