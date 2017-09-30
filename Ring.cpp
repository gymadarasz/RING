#include "error.cpp"

#define RING_INIT_DELAY 10
#define RING_TOKEN_DELAY 1
#define RING_BUS_LENGTH 3
#define RING_DATA_BITS 16
#define RING_MASTER true
#define RING_SLAVE false

typedef int Bus[RING_BUS_LENGTH];

class Ring {
public:
    int address;
    Bus bus;
    int error;
    int prev;
    int next;
    int state;
    int init(bool master, int prev, int next, int clock, Bus bus);
    void loop(Handler tickHandler);
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
    this->prev = prev;
    this->next = next;
    state = master ? LOW : HIGH;
    if(master) digitalWrite(next, LOW);
    return length-1;
}

void Ring::loop(Handler tickHandler) {
    if(digitalRead(prev) != state) {
        
        // .. do stuff
        tickHandler();
        delay(RING_TOKEN_DELAY);
        
        state = !state;
        digitalWrite(next, state);
    }    
}

