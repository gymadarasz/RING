#include "error.cpp"

#define RING_INIT_DELAY 10
#define RING_BUS_LENGTH 3
#define RING_MASTER true
#define RING_SLAVE false
#define RING(master, prev, next, clock, bus0, bus1, bus2) Bus bus = {bus0, bus1, bus2}; int length = ring.init(master, prev, next, clock, bus)

typedef int Bus[RING_BUS_LENGTH];

class Ring {
public:
    int address;
    Bus bus;
    int error;
    int init(bool master, int prev, int next, int clock, Bus bus);
    static void queueInterruptHandler();
    static void clockInterruptHandler();
    int send(int data);
    int read();
};

int Ring::init(bool master, int prev, int next, int clock, Bus bus) {
    digitalWrite(prev, LOW);
    digitalWrite(next, LOW);
    delay(RING_INIT_DELAY);
    long start = millis();
    while(!master && digitalRead(prev) != HIGH);
    address = master ? 0 : (millis() - start) / RING_INIT_DELAY;
    delay(RING_INIT_DELAY);
    digitalWrite(next, HIGH);
    while(master && digitalRead(prev) != HIGH);
    int length = (millis() - start) / RING_INIT_DELAY;
    for(int i=0; i < RING_BUS_LENGTH; i++) {
        this->bus[i] = bus[i];
    }
    error = 0;
    attachInterrupt(digitalPinToInterrupt(prev), queueInterruptHandler, CHANGE);
    attachInterrupt(digitalPinToInterrupt(clock), clockInterruptHandler, CHANGE);
    sei();
    return length-1;
}

void Ring::queueInterruptHandler() {
    cli();
    // .. send data if you need
    sei();
}

void Ring::clockInterruptHandler() {
    cli();
    // .. read data to buffer
    sei();
}

int Ring::send(int data) {
    
}

int Ring::read() {
    
}
