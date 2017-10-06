#include "error.cpp"

#define RING_PACK_DEBUG // TODO: remove or comment out this on live system

#define RING_DELAY_INIT 5
#define RING_DELAY_TOKEN 5
#define RING_DELAY_CLOCK 5

#define RING_BUS_LENGTH 3
// TODO:
//#define RING_TYPE_DIGITAL 0
//#define RING_TYPE_ANALOG 1
//#define RING_ADDRESS_BITS 12
//#define RING_TYPE_BITS 1
#define RING_DATA_BITS 12
#define RING_QUEUE_LENGTH 100
#define RING_BUFFER_LENGTH 100

#define RING_DATA_SIGNED true
#define RING_MASTER true
#define RING_SLAVE false
#define RING_ADDR_MASTER 0
#define RING_ADDR_BROADCAST -1

#define RING_DEFAULT_HANDLER loop   // arduino sketch loop()


// --------------------------------------------------------
// ------------------------- PACK -------------------------
// --------------------------------------------------------

class Pack {
public:
    int from;
    int to;
    int length;
    int* buffer;
    Pack* make(int from, int to = RING_ADDR_BROADCAST, int length = 0, int* buffer = NULL);
    void copy(Pack* pack);
    bool isBroadcast();
    static bool isBroadcast(int to);
    
    // TODO: pass Ring to these two function instead using address and multiple (or make a struct)
    bool isAddressedBetween(int address, int multiple);
    static bool isAddressedBetween(int to, int address, int multiple);
};

Pack* Pack::make(int from, int to, int length, int* buffer) {
    this->from = from;
    this->to = to;
    if(NULL == buffer) { // only one data sending?
        this->length = 1;
        this->buffer[0] = length;
    } else {
        this->length = length;
        this->buffer = buffer;
    }
    return this;
}

void Pack::copy(Pack* pack) {
    pack->from = from;
    pack->to = to;
    pack->length = length;
    for(int i=0; i < length; i++) {
        pack->buffer[i] = buffer[i];
    }
}

bool Pack::isBroadcast() {
    return isBroadcast(to);
}

bool Pack::isBroadcast(int to) {
    return to == RING_ADDR_BROADCAST;
}

bool Pack::isAddressedBetween(int address, int multiple) {
    return isAddressedBetween(to, address, multiple);
}

bool Pack::isAddressedBetween(int to, int address, int multiple) {
    return to >= address && to < address+multiple;
}

#ifdef RING_PACK_DEBUG

void debug(const char* msg, Pack* pack);

void debug(const char* msg, Pack* pack) {
    debug(msg);
    debug("from:", pack->from);
    debug("to:", pack->to);
    debug("length:", pack->length);    
    debug("buffer[]:", pack->buffer, pack->length);
}
#endif



// --------------------------------------------------------
// ------------------------ QUEUE -------------------------
// --------------------------------------------------------


class Queue {
private:
    Pack packs[RING_QUEUE_LENGTH];
    int buffers[RING_QUEUE_LENGTH][RING_BUFFER_LENGTH];
    int current;
public:
    Queue();
    void push(Pack* pack);
    Pack* top();
    Pack* pop();
    bool isEmpty();
    bool isFull();
};

Queue::Queue() {
    for(int i=0; i < RING_QUEUE_LENGTH; i++) {
        packs[i].buffer = buffers[i];
    }
    current = -1;
}

void Queue::push(Pack* pack) {
    current++;
    pack->copy(&packs[current]);
}

Pack* Queue::top() {
    return &packs[current];
}

Pack* Queue::pop() {
    current--;
    return &packs[current+1];
}

bool Queue::isEmpty() {
    return current == -1;
}

bool Queue::isFull() {
    return current == RING_QUEUE_LENGTH-1;
}



// -----------------------------------------------------------------
// ----------------------------- RING ------------------------------
// -----------------------------------------------------------------

typedef void (*Receiver)(Pack*);

/**
 * Token Ring
 */
class Ring {
public:
    
    int init(const bool master, const int prev, const int next, const int clock, 
            const int* bus, Receiver onPackReceivedHandler, 
            const int multiple = 1, 
            Handler onClockWaitingLoopHandler = RING_DEFAULT_HANDLER,
            Handler onTokenOwnedLoopHandler = RING_DEFAULT_HANDLER, 
            Handler onTokenWaitingLoopHandler = RING_DEFAULT_HANDLER
    );
      
    bool send(Pack* pack);
      
private:
    
    // --------------------------------------------------------
    // ------------------------- BUS --------------------------
    // --------------------------------------------------------
    
    const int* bus;
    
    void busReset(int level);
    
    
    // --------------------------------------------------------
    // ------------------------ QUEUE -------------------------
    // --------------------------------------------------------
    
    Queue queue;
    
    
    // --------------------------------------------------------
    // ------------------- INITIALIZATION ---------------------
    // --------------------------------------------------------
    
    bool initialized;
    static Ring* that;
    int slaveCounter;
    
    static void initReceiveHandler(Pack* pack);
    static void initTokenOwnedHandler();

    
    // --------------------------------------------------------
    // ------------------------ TOKEN -------------------------
    // --------------------------------------------------------
    
    int tokenPrev;
    int tokenNext;
    
    int tokenPrevState;
    int tokenNextState;
    
    bool tokenOwned;
    
    Handler onTokenWaitingLoopHandler;
    Handler onTokenOwnedLoopHandler;
    
    void tokenReset(bool master);
    void tokenPass();
    bool tokenCheck();
    
    
    // --------------------------------------------------------
    // ---------------------- CLOCK ---------------------------
    // --------------------------------------------------------
    
    int clock;
    int clockState;
    
    bool clockReset(int level);
    bool isClockTick();
    void clockTick();
    void clockWaiting();
    
    
    // --------------------------------------------------------
    // ----------------------- PACK ---------------------------
    // --------------------------------------------------------
    
    int buffer[RING_BUFFER_LENGTH];
        
    Pack pack;
    
    
    // --------------------------------------------------------
    // ------------------- COMMUNICATION ----------------------
    // --------------------------------------------------------
    
    int address;
    int multiple;
    
    void sendClock();
    void readClock();
    
    void send(int data);
    int read();
    bool receive();
    bool listen();
    
    Receiver onPackReceivedHandler;
    Handler onClockWaitingLoopHandler;
    
};



// -----------------------------------------------------------------
// -----------------------------------------------------------------
// ----------------------------- RING ------------------------------
// -----------------------------------------------------------------
// -----------------------------------------------------------------


int Ring::init(const bool master, const int prev, const int next, 
        const int clock, const int* bus, Receiver onPackReceivedHandler, 
        const int multiple, Handler onClockWaitingLoopHandler, 
        Handler onTokenOwnedLoopHandler, Handler onTokenWaitingLoopHandler
) {
    
    this->tokenPrev = prev;
    this->tokenNext = next;
    this->clock = clock;
    this->bus = bus;
    this->multiple = multiple;
    
    that = this;
    pack.buffer = buffer;
    initialized = false;
    
    clockReset(HIGH);
    busReset(HIGH);
    tokenReset(master);
    
    if(master) {
        delay(RING_DELAY_INIT);
        address = RING_ADDR_MASTER;
        tokenPass();
    } else {
        this->onTokenOwnedLoopHandler = initTokenOwnedHandler;
        this->onPackReceivedHandler = initReceiveHandler;
        while(!initialized) {
            while(!listen()) delay(1);
        }
    }
    
    while(!tokenCheck()) delay(1);
    tokenPass();

    
    debug("Initialization finished, address:", address);
    this->onPackReceivedHandler = onPackReceivedHandler;
    this->onClockWaitingLoopHandler = onClockWaitingLoopHandler;
    this->onTokenWaitingLoopHandler = onTokenWaitingLoopHandler;
    this->onTokenOwnedLoopHandler = onTokenOwnedLoopHandler;
    
    while(true) {
        //debug("...");
        listen();
        delay(1);
        tokenPass();
    }
    
}

// ------------ BUS -------------

void Ring::busReset(int level) {
    for(int i=0; i < RING_BUS_LENGTH; i++) {
        //this->bus[i] = bus[i];
        digitalWrite(bus[i], HIGH);
    }
}

// ----------- TOKEN ------------

void Ring::tokenReset(bool master) {
    tokenPrevState = digitalRead(tokenPrev);
    tokenNextState = digitalRead(tokenNext);
    tokenOwned = master;
}

void Ring::tokenPass() {
    tokenNextState = !tokenNextState;
    digitalWrite(tokenNext, tokenNextState);
    //debug("Token passed");
    tokenOwned = false;
}

bool Ring::tokenCheck() {
    if(digitalRead(tokenPrev) != tokenPrevState) {
        tokenPrevState = !tokenPrevState;
        //debug("Token given");
        tokenOwned = true;
    }
    return tokenOwned;
}

// ---------- CLOCK -----------

bool Ring::clockReset(int level) {
    digitalWrite(clock, level);
    clockState = level;
}

bool Ring::isClockTick() {
    if(digitalRead(clock) != clockState) {
        clockState = !clockState;
        //debug("Clock ticked");
        return true;
    }
    return false;
}

void Ring::clockTick() {
    //debug("Clock tick!!");
    clockState = !clockState;
    digitalWrite(clock, clockState);
}

void Ring::clockWaiting() {       
    while(!isClockTick()) {
        if(NULL != onClockWaitingLoopHandler) onClockWaitingLoopHandler();
        delay(1);
    }
}
    
// -------------- RING ---------------

bool Ring::listen() {
    if(tokenCheck()) {
        
        while(!queue.isEmpty()) {
            if(queue.top()->isAddressedBetween(address, multiple)) {
                onPackReceivedHandler(queue.pop());
            } else {
                send(queue.pop());
            }
        }
        
        // .. do stuff
        if(NULL != onTokenOwnedLoopHandler) onTokenOwnedLoopHandler();
        
        tokenPass();
        return true;
    } else if(isClockTick()) {
        // .. read pack stuff
        if(receive()) {
            //debug("Package received:", &pack);
            onPackReceivedHandler(&pack);
        }
          
    } else {
        if(NULL != onTokenWaitingLoopHandler) onTokenWaitingLoopHandler();
    }
    return false;
}

bool Ring::send(Pack* pack) {
    //debug("Package sending:", pack);
    if(queue.isFull()) {
        error("queue full",1);
        return false;
    }
    
    if(tokenCheck()) {
        send(pack->from);
        send(pack->to);
        send(pack->length);
        for(int i=0; i < pack->length; i++) {
            send(pack->buffer[i]);
        }
        sendClock();
        if(NULL != onPackReceivedHandler) onPackReceivedHandler(pack);
    } else {
        queue.push(pack);
    }
    
    return true;
}

void Ring::sendClock() {
    //debug("Clocking out");
    clockTick();
    delay(RING_DELAY_CLOCK);
}

void Ring::send(int data) {
    //debug("Send:", data);
    for(int i=0; i < RING_DATA_BITS;) {
        for(int j=0; j < RING_BUS_LENGTH && i < RING_DATA_BITS; j++) {
            int bit = data & 1;
            data >>= 1;
            //debug("Write bit to bus[",j,"] =>", bit);
            digitalWrite(bus[j], bit);
            i++;
        }
        sendClock();
    }
}


bool Ring::receive() {
    // read additional meta info of message
    int from = read();
    int to = read();
    int length = read();
    
    // check if it's sent to us
    bool our = pack.isBroadcast(to) || pack.isAddressedBetween(to, address, multiple);
    // read message contents
    if(our) {
        pack.to = to;
        pack.from = from;
        pack.length = length;
    }
    for(int i=0; i<length; i++) {
        int data = read();
        if(our) {
            // store if it's for us
            pack.buffer[i] = data;
        }
    }
    
    return our;
}

void Ring::readClock() {
    //debug("Waiting for clock...");
    clockWaiting();
    //debug("Clock ticked!!");
}

int Ring::read() {
    //debug("Reading start...");
    int data = 0;
    for(int i=0; i < RING_DATA_BITS;) {
        for(int j=0; j < RING_BUS_LENGTH && i < RING_DATA_BITS; j++) {
            int bit = digitalRead(bus[j]);
            //debug("Read bit on bus[",j,"] <=", bit);
            data = data | (bit << i);
            i++;
        }
        if(i < RING_DATA_BITS) {
            readClock();
        }
    }
    readClock();
    
    // sign
    if(RING_DATA_SIGNED && data > (1<<RING_DATA_BITS)/2) {
        data -= 1<<RING_DATA_BITS;
    }
    //debug("Read:", data);
    return data;
}

Ring* Ring::that;

void Ring::initReceiveHandler(Pack* pack) {
    if(!that->initialized) {
        that->address = pack->buffer[0];
        //debug("Slave set the address:", that->address);
    }
}

void Ring::initTokenOwnedHandler() {
    if(!that->initialized) {
        that->send(that->pack.make(that->address, RING_ADDR_BROADCAST, that->address+that->multiple));
        that->initialized = true;
    }
}
