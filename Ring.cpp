#include "error.cpp"


#define RING_DELAY_INIT 100
#define RING_DELAY_TOKEN 10
#define RING_DELAY_CLOCK 25

#define RING_BUS_LENGTH 3
#define RING_DATA_BITS 16
#define RING_QUEUE_LENGTH 100
#define RING_MULTIPLE_DEFAULT 1

#define RING_DATA_SIGNED true
#define RING_IS_MASTER true
#define RING_IS_SLAVE false
#define RING_ADDR_BROADCAST -1
#define RING_ADDR_IGNORED -2

typedef int Bus[RING_BUS_LENGTH];

typedef struct {
    int from;
    int to;
    int length;
    int* buffer;
} Pack;

void debug(const char* msg, Pack pack) {
    debug(msg);
    debug("from:", pack.from);
    debug("to:", pack.to);
    debug("length:", pack.length);    
    for(int i=0; i < pack.length; i++) {
        debug(" - buffer[",i,"]:", pack.buffer[i]);
    }
}

typedef void (*Receiver)(Pack);

/**
 * Ring class for Token Ring
 */
class Ring {
public:
    // @int address used address for communication
    int address;
    
    // @int count of emulated element (address extension)
    int multiple;
    
    // @Bus pins of bus
    Bus bus;
    
    // @int store for last error code
    // @todo not implemented yet
    int error;
    
    // @int pin for previous element of token ring
    int tokenPrev;
    
    // @int pin for next element of token ring
    int tokenNext;
    
    // @int state of token to ckeck when we given the token
    int tokenState;
    
    // @int pin for clock the communication on BUS
    int clock;
    
    // @int state of clock to check the change of bitbaing tick-tack
    int clockState;
    
    // @Pack pack store the given data
    Pack pack;
    
    // @Receiver receiver callback to handle the received data
    Receiver receiver;
    
    // @bool (private) do we have a token at the moment - for send()
    bool tick;
    
    /**
     * Initialize token ring
     * 
     * @bool master         it is the master or a slave element of token ring
     * @int prev            pin for previous element to get the token
     * @int next            pin for next element to pass the token
     * @int clock           pin for clock while passing the data on BUS
     * @Bus bus             pins for BUS of communication
     * @Reader reader       callback when data received
     * @int* buffer         buffer for received data
     * @int multiple        greater than 0, 1 is not multiple, 1+ more then one device emulation on ring
     * @int                 return: length of initialized token ring
     */
    int init(
        bool master, int prev, int next, int clock, Bus bus, 
        Receiver reader, int* buffer, 
        int multiple = RING_MULTIPLE_DEFAULT
    );
    
    /**
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     * !!!! CALL ONLY ONCE IN MAIN SKETCH LOOP!! !!!!
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     * 
     * @param Handler tickHandler will be called at every token cycle
     */
    void loop(Handler tickHandler);
    
    /**
     * Sending package
     * 
     * @param pack
     */
    void send(Pack pack);
    
private:
    
    // @Pack queue[] queue for sending response messages
    Pack queue[RING_QUEUE_LENGTH];
    
    // @int element of queue
    int queueNext;
    
    /**
     * Sending data
     * 
     * @param data
     */
    void send(int data);
    
    /**
     * Reading data
     * 
     * @return data
     */
    int read();
    
    /**
     * Sending into a queue, realy send when we given the token
     * 
     * @param pack
     */
    void sendQueue(Pack pack);
    
    /**
     * receiving messages into pack 
     *  return true if message addressed to us, false if message ignored
     * 
     * @bool 
     */
    bool receive();
    
    /**
     * Is message addressed for this device
     * 
     * @param to
     * @return 
     */
    bool is4me(int to);
    
    /**
     * Is message is a broadcast
     * 
     * @param to
     * @return 
     */
    bool is4all(int to);
    
    
};

int Ring::init(
    bool master, int prev, int next, int clock, Bus bus, 
    Receiver receiver, int* buffer, 
    int multiple
) {
    // Initialize token ring default state as LOW,
    digitalWrite(prev, LOW);
    digitalWrite(next, LOW);
    // wait an initial delay, measure the start time,
    delay(RING_DELAY_INIT);
    long start = millis();
    // after wait for our moment with the token 
    while(!master && digitalRead(prev) != HIGH);
    // in the initialization queue and calculate the address(es).
    address = master ? 0 : (millis() - start) / RING_DELAY_INIT;
    // waiting for many times how multiple we are
    delay(RING_DELAY_INIT * multiple);
    // and then pass the token to the next (set next to HIGH)
    digitalWrite(next, HIGH);
    this->multiple = multiple;
    
    // Master wait when get back the token from the end of ring 
    // so the cycle is finished and we will know how long is it.
    while(master && digitalRead(prev) != HIGH);
    int length = (millis() - start) / RING_DELAY_INIT;
    
    // store the pins of BUS for later communication
    // and initialize all bit as LOW
    for(int i=0; i < RING_BUS_LENGTH; i++) {
        this->bus[i] = bus[i];
        digitalWrite(bus[i], LOW);
    }
    
    // initialize the error code for zero (no any errors)
    error = 0; // todo: need to implement it?
    
    // store the token ring pass pins
    tokenPrev = prev;
    tokenNext = next;
    // and master will start the token cycle (token state set to LOW first)
    tokenState = master ? LOW : HIGH;
    
    // store the clock pin and set it LOW by default
    this->clock = clock;
    clockState = LOW;
    digitalWrite(clock, clockState);
    
    // store the pointer of reading buffer and reading handler
    this->pack.buffer = buffer;
    this->receiver = receiver;
    
    // initialize the response queue pointer to 0 as empty
    queueNext = 0;
    
    // we don't have token at the moment
    tick = false;
    
    // master start the token ring dance 
    // and pass back the full length of ring
    if(master) digitalWrite(next, tokenState);
    return length-1;
}

void Ring::loop(Handler tickHandler) {
    if(digitalRead(tokenPrev) != tokenState) {
        tick = true;
        
        while(queueNext) {
            queueNext--;
            if(is4me(queue[queueNext].to)) {
                receiver(queue[queueNext]);
            } else {
                send(queue[queueNext]);
            }
        }
        
        // .. do stuff
        tickHandler();
        
        delay(RING_DELAY_TOKEN);
        
        tokenState = !tokenState;
        digitalWrite(tokenNext, tokenState);
        tick = false;
    } else if(digitalRead(clock) != clockState) {
            
        // .. read pack stuff
        if(receive()) {
            receiver(pack);
        }
//        if(device->id == 1) {
//        debug("receive: ", data);
//        }
          
    }
}

bool Ring::is4me(int to) {
    return to >= address && to <= address+(multiple-1);
}

bool Ring::is4all(int to) {
    return to == RING_ADDR_BROADCAST;
}

bool Ring::receive() {
    // read additional meta info of message
    int to = read();
    int from = read();
    int length = read();
    // check if it's sent to us
    bool me = is4all(to) || is4me(to);
    if(me) {
        // store meta addressing if its for us
        pack.to = to;
        pack.from = from;
        pack.length = length;
    }
    // read message contents
    for(int i=0; i<length; i++) {
        int data = read();
        if(me) {
            // store if it's for as
            pack.buffer[i] = data;
        }
    }
    return me;
}

void Ring::send(Pack pack) {
    // check if the message is emulated by us!
    if(tick && !is4me(pack.to)) {
        debug(" -> send to RING", pack.buffer[0]);
        send(pack.to);
        send(pack.from);
        send(pack.length);
        for(int i=0; i < pack.length; i++) {
            send(pack.buffer[i]);
        }
    } else {
        debug(" -> send to QUEUE", pack.buffer[0]);
        queue[queueNext].to = pack.to;
        queue[queueNext].from = pack.from;
        queue[queueNext].length = pack.length;
        queue[queueNext].buffer = pack.buffer;
        queueNext++;
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
        delay(RING_DELAY_CLOCK);
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

