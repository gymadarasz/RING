#include "error.cpp"

#define RING_INIT_DELAY 10
#define RING_TOKEN_DELAY 1
#define RING_CLOCK_DELAY 5
#define RING_BUS_LENGTH 3
#define RING_DATA_BITS 12
#define RING_MASTER true
#define RING_SLAVE false

typedef int Bus[RING_BUS_LENGTH];

class Ring {
public:
    int address;
    Bus bus;
    int error;
    int tokenPrev;
    int tokenNext;
    int tokenState;
    int clock;
    int clockState;
    int init(bool master, int prev, int next, int clock, Bus bus);
    void loop(Handler tickHandler);
    void send(unsigned int data);
    unsigned int read();
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
    tokenPrev = prev;
    tokenNext = next;
    tokenState = master ? LOW : HIGH;
    this->clock = clock;
    clockState = LOW;
    digitalWrite(clock, clockState);
    if(master) digitalWrite(next, tokenState);
    return length-1;
}

void Ring::loop(Handler tickHandler) {
    if(digitalRead(tokenPrev) != tokenState) {
        
        // .. do stuff
        tickHandler();
        
        delay(RING_TOKEN_DELAY);
        
        tokenState = !tokenState;
        digitalWrite(tokenNext, tokenState);
    } else if(digitalRead(clock) != clockState) {
            
        // .. read stuff
        int data = read();
        if(device->id == 1) {
        debug("receive: ", data);
        }
          
    }
}

void Ring::send(unsigned int data) {
    //debug("send:", data);
    for(int i=0; i < RING_DATA_BITS;) {
        for(int j=0; j < RING_BUS_LENGTH && i < RING_DATA_BITS; j++) {
            int bit = data & 1;
            data >>= 1;
            //debug("nth:", i);
            //debug("bit:", bit);
            digitalWrite(bus[j], bit);
            i++;
        }
        clockState = !clockState;
        digitalWrite(clock, clockState);
        delay(RING_CLOCK_DELAY);
    }
}

unsigned int Ring::read() {
    unsigned int data = 0;
    for(int i=0; i < RING_DATA_BITS;) {
        while(digitalRead(clock) == clockState);
        clockState = !clockState;
        //data >>= RING_BUS_LENGTH;
        for(int j=0; j < RING_BUS_LENGTH && i < RING_DATA_BITS; j++) {
            //data >>= 1;
            data = data | (digitalRead(bus[j]) << i);
            //data = data | (bit << RING_DATA_BITS);
            i++;
        }
    }
    return data;
}

