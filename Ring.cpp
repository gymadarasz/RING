#include "error.cpp"

#define RING_INIT_DELAY 10
#define RING_TOKEN_DELAY 1
#define RING_CLOCK_DELAY 5
#define RING_BUS_LENGTH 3
#define RING_DATA_BITS 24
#define RING_DATA_SIGNED true
#define RING_MASTER true
#define RING_SLAVE false
#define RING_BROADCAST -1

typedef int Bus[RING_BUS_LENGTH];

typedef struct {
    int from;
    int length;
    int* buffer;
} Pack;

typedef void (*Reader)(Pack);

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
    Pack pack;
    Reader reader;
    int init(bool master, int prev, int next, int clock, Bus bus, Reader reader, int* buffer);
    void loop(Handler tickHandler);
    void send(int data);
    int read();
    void send(int to, Pack pack);
    bool receive();
    
};

int Ring::init(bool master, int prev, int next, int clock, Bus bus, Reader reader, int* buffer) {
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
    this->pack.buffer = buffer;
    this->reader = reader;
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
            
        // .. read pack stuff
        if(receive()) {
            reader(pack);
        }
//        if(device->id == 1) {
//        debug("receive: ", data);
//        }
          
    }
}

bool Ring::receive() {
    int to = read();
    int from = read();
    int length = read();
    bool me = to == RING_BROADCAST || to == address;
    if(me) {
        pack.from = from;
        pack.length = length;
    }
    for(int i=0; i<length; i++) {
        int data = read();
        if(me) {
            pack.buffer[i] = data;
        }
    }
    return me;
}

void Ring::send(int to, Pack pack) {
    send(to);
    send(address);
    send(pack.length);
    for(int i=0; i < pack.length; i++) {
        send(pack.buffer[i]);
    }
}


void Ring::send(int data) {
    for(int i=0; i < RING_DATA_BITS;) {
        for(int j=0; j < RING_BUS_LENGTH && i < RING_DATA_BITS; j++) {
            int bit = data & 1;
            data >>= 1;
            digitalWrite(bus[j], bit);
            i++;
        }
        clockState = !clockState;
        digitalWrite(clock, clockState);
        delay(RING_CLOCK_DELAY);
    }
}

int Ring::read() {
    int data = 0;
    for(int i=0; i < RING_DATA_BITS;) {
        while(digitalRead(clock) == clockState);
        clockState = !clockState;
        for(int j=0; j < RING_BUS_LENGTH && i < RING_DATA_BITS; j++) {
            data = data | (digitalRead(bus[j]) << i);
            i++;
        }
    }
    
    // sign
    if(RING_DATA_SIGNED && data > (1<<RING_DATA_BITS)/2) {
        data -= 1<<RING_DATA_BITS;
    }
    return data;
}

