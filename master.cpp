
#include "Neuron.cpp"

static IOBlock ioBlock;

int teste;

void setup() {
    debug("Master start..");
    teste = 0;
    
    Bus busDigital = {3,  3, 4, 5};
    Bus busAnalog = {2,  4, 5};
    BusDA bus = {busDigital, busAnalog};
    ioBlock.init(1, 0, 2, bus, 3, 3);
    ioBlock.start();
}

void loop() {
    //ring.loop(tick);
    if(teste == 0) {
        Platoon senders;
        senders.first = 0;
        senders.length = 1;
        
        ioBlock.pack.messageList.length = 1;
        ioBlock.pack.messageList.messages[0].make(124);
        ioBlock.pack.make(senders, ioBlock.pack.messageList, 2, RING_PACKTYPE_ANALOG);
        ioBlock.send(ioBlock.pack);
        
        ioBlock.pack.messageList.length = 1;
        ioBlock.pack.messageList.messages[0].make(221);
        ioBlock.pack.make(senders, ioBlock.pack.messageList, 4, RING_PACKTYPE_ANALOG);
        ioBlock.send(ioBlock.pack);
        teste = 1;
    }
    
}

