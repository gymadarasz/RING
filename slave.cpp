
#include "Neuron.cpp"

static HiddenBlock hiddenBlock;

void setup() {
    debug("Slave start..");
    
    Bus busDigital = {3,  3, 4, 5};
    Bus busAnalog = {2,  4, 5};
    BusDA bus = {busDigital, busAnalog};
    hiddenBlock.init(1, 0, 2, bus, 3, 3);
    hiddenBlock.start();
}

void loop() {
    //ring.loop(tick);
}
