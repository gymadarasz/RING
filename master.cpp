
#include "Neuron.cpp"

static Block block;

void onMasterReceive(Pack pack) {
    
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
    
    Bus busDigital = {3,  3, 4, 5};
    Bus busAnalog = {2,  4, 5};
    BusDA bus = {busDigital, busAnalog};
    block.init(RING_MASTER, 1, 0, 2, bus, 0, 3);
    block.start();
}



void loop() {
    //ring.loop(tick);
    if(teste == 0) {
        Platoon senders;
        senders.first = 0;
        senders.length = 1;
        
        block.pack.messageList.length = 1;
        block.pack.messageList.messages[0].make(124);
        block.pack.make(senders, block.pack.messageList, 2, RING_PACKTYPE_ANALOG);
        block.send(block.pack);
        
        block.pack.messageList.length = 1;
        block.pack.messageList.messages[0].make(221);
        block.pack.make(senders, block.pack.messageList, 4, RING_PACKTYPE_ANALOG);
        block.send(block.pack);
        teste = 1;
    }
    
}

