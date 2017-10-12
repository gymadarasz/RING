
#include "Neuron.cpp"

static Ring ring;

void onMasterReceive(Pack* pack) {
    
//    test_nequ(
//            "Checking: Sender and recipient can not be same in master device.", 
//            pack->to, pack->from.first, 
//            "Master given a self addressed message.");

    debug("(f) MASTER RECEIVE:", pack);
}


int teste;

void setup() {
    debug("Master start..");
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
        Platoon senders;
        senders.first = 0;
        senders.length = 1;
        
        ring.pack.message.make(124);
        ring.send(ring.pack.make(senders, ring.pack.message, 2, RING_PACKTYPE_ANALOG));
        
        ring.pack.message.make(221);
        ring.send(ring.pack.make(senders, ring.pack.message, 4, RING_PACKTYPE_ANALOG));
        teste = 1;
    }
    
}

