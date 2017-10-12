#include "error.cpp"

// TODO: remove or comment out these lines on live system
#define RING_PACK_DEBUG
#define RING_DELAY_SIMULATION delay(1)

#define RING_VERSION 0x0001

#define RING_DELAY_INIT 5
#define RING_DELAY_TOKEN 5
#define RING_DELAY_CLOCK 5

#define RING_PACKTYPE_DIGITAL 0
#define RING_PACKTYPE_ANALOG 1

#define RING_ADDRESS_FIRST_BITS 16
#define RING_ADDRESS_LENGTH_BITS 12 // or less that 16
#define RING_DATA_TYPE_BITS 1
#define RING_DATA_LENGTH_BITS 6
#define RING_DATA_VALUE_BITS 12 // or 10

#define RING_QUEUE_LENGTH 3
#define RING_BUFFER_LENGTH 10
#define RING_PACK_MESSAGES_MAX 10

#define RING_DATA_SIGNED true
#define RING_MASTER true
#define RING_SLAVE false
#define RING_ADDR_MASTER 0
#define RING_ADDR_BROADCAST -1

#define RING_DEFAULT_HANDLER loop   // arduino sketch loop()

class Platoon {
public:
    int first;
    int length = 1;
    Platoon(int first = 0, int length = 1);
};

Platoon::Platoon(int first, int length) {
    this->first = first;
    this->length = length;
}

// --------------------------------------------------------
// ------------------------- BUS --------------------------
// --------------------------------------------------------

typedef struct {
    const int length;
    const int* pins;
    const int length_analog;
    const int* pins_analog;
} Bus;


// --------------------------------------------------------
// ------------------------- PACK -------------------------
// --------------------------------------------------------

class Message {
public:
    int length;
    int buffer[RING_BUFFER_LENGTH];
    void make(int length = 0, int* buffer = NULL);
};

void Message::make(int length, int* buffer) {
    if(NULL == buffer) { // only one data sending?
        this->length = 1;
        this->buffer[0] = length;
    } else {
        this->length = length;
        for(int i=0; i<this->length; i++) {
            this->buffer[i] = buffer[i];
        }
    }
}

// -------------

class MessageList {
public:
    int length;
    Message messages[RING_PACK_MESSAGES_MAX];
    MessageList();
    void clear();
    void put(Message message);
};

MessageList::MessageList() {
    clear();
}

void MessageList::clear() {
    length = -1;
}

void MessageList::put(Message message) {
    messages[length] = message;
    length++;
}


// -------- 

class Pack {
public:
    Platoon from;
    MessageList messageList;
    int to;
    int type;
    void make(
        Platoon from, MessageList messageList, 
        int to = RING_ADDR_BROADCAST, 
        int type = RING_PACKTYPE_DIGITAL
    );
    void copy(Pack* pack);
    bool isBroadcast();
    static bool isBroadcast(int to);
    
    // TODO: pass Ring to these two function instead using address and multiple (or make a struct)
    bool isAddressedBetween(Platoon address);
    static bool isAddressedBetween(int to, Platoon address);
};

void Pack::make(Platoon from, MessageList messageList, int to, int type) {
    this->from.first = from.first;
    this->from.length = from.length;
    this->to = to;
    this->type = type;
    this->messageList = messageList;
}

void Pack::copy(Pack* pack) {
    pack->from.first = from.first;
    pack->from.length = from.length;
    pack->to = to;
    pack->type = type;
    pack->messageList.length = messageList.length;
    for(int j=0; j < pack->messageList.length; j++) {
        pack->messageList.messages[j].length = messageList.messages[j].length;
        for(int i=0; i < messageList.messages[j].length; i++) {
            pack->messageList.messages[j].buffer[i] = messageList.messages[j].buffer[i];
        }
    }
}

bool Pack::isBroadcast() {
    return isBroadcast(to);
}

bool Pack::isBroadcast(int to) {
    return to == RING_ADDR_BROADCAST;
}

bool Pack::isAddressedBetween(Platoon address) {
    return isAddressedBetween(to, address);
}

bool Pack::isAddressedBetween(int to, Platoon address) {
    return to >= address.first && to < address.first + address.length;
}

#ifdef RING_PACK_DEBUG

void debug(const char* msg, Pack pack);

void debug(const char* msg, Pack pack) {
    debug(msg);
    debug("from.first:", pack.from.first);
    debug("from.length:", pack.from.length);
    debug("to:", pack.to);
    debug("type:", pack.type);
    debug("messageList.length:", pack.messageList.length);    
    for(int i=0; i < pack.messageList.length; i++) {
        debug("messageList.messages[].buffer[]:", pack.messageList.messages[i].buffer, pack.messageList.messages[i].length);
    }
}
#endif


// --------------------------------------------------------
// ------------------------ QUEUE -------------------------
// --------------------------------------------------------


class Queue {
private:
    Pack packs[RING_QUEUE_LENGTH];
    //int buffers[RING_QUEUE_LENGTH][RING_BUFFER_LENGTH];
    int current;
public:
    Queue();
    void push(Pack pack);
    Pack top();
    Pack pop();
    bool isEmpty();
    bool isFull();
};

Queue::Queue() {
//    for(int i=0; i < RING_QUEUE_LENGTH; i++) {
//        packs[i].message.buffer = buffers[i];
//    }
    current = -1;
}

void Queue::push(Pack pack) {
    current++;
    pack.copy(&packs[current]);
}

Pack Queue::top() {
    return packs[current];
}

Pack Queue::pop() {
    current--;
    return packs[current+1];
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

typedef void (*Receiver)(Pack);

/**
 * Token Ring
 */
class Ring {
public:
    
    Pack pack;
    
    int init(const bool master, const int prev, const int next, const int clock, 
            const Bus bus, Receiver onPackReceivedHandler, 
            const int multiple = 1, 
            Handler onClockWaitingLoopHandler = RING_DEFAULT_HANDLER,
            Handler onTokenOwnedLoopHandler = RING_DEFAULT_HANDLER, 
            Handler onTokenWaitingLoopHandler = RING_DEFAULT_HANDLER
    );
    
    void start();
      
    bool send(Pack pack);
    
protected:
    
    // --------------------------------------------------------
    // ------------------------- BUS --------------------------
    // --------------------------------------------------------
    
    const Bus* bus;
    
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
    
    static void initReceiveHandler(Pack pack);
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
    
//    int buffer[RING_BUFFER_LENGTH];
    
    
    // --------------------------------------------------------
    // ------------------- COMMUNICATION ----------------------
    // --------------------------------------------------------
    
    Platoon address;
    //int multiple;
    int length;
    
    void sendClock();
    
    void send(int data, int bits);
    int read(int bits);
    bool receive();
    bool receiveCheck();
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
        const int clock, const Bus bus, Receiver onPackReceivedHandler, 
        const int multiple, Handler onClockWaitingLoopHandler, 
        Handler onTokenOwnedLoopHandler, Handler onTokenWaitingLoopHandler
) {
    
    this->tokenPrev = prev;
    this->tokenNext = next;
    this->clock = clock;
    this->bus = &bus;
    this->address.length = multiple;
    
    that = this;
//    pack.message.buffer = buffer;
    initialized = master;
    
    clockReset(HIGH);
    busReset(HIGH);
    tokenReset(master);
    
    this->onTokenOwnedLoopHandler = initTokenOwnedHandler;
    this->onPackReceivedHandler = initReceiveHandler;
    if(master) {
        delay(RING_DELAY_INIT);
        address.first = RING_ADDR_MASTER;
        tokenPass();
    } else {
        while(!initialized) {
            while(!listen()) RING_DELAY_SIMULATION;
        }
    }
    
    while(!tokenCheck()) {
        receiveCheck();
        RING_DELAY_SIMULATION;
    }
    tokenPass();

    
    debug("Initialization finished");
    debug("address.first:", address.first);
    debug("address.length:", address.length);
    debug("length:", length);

    
    this->onPackReceivedHandler = onPackReceivedHandler;
    this->onClockWaitingLoopHandler = onClockWaitingLoopHandler;
    this->onTokenWaitingLoopHandler = onTokenWaitingLoopHandler;
    this->onTokenOwnedLoopHandler = onTokenOwnedLoopHandler;
    
}

void Ring::start() {
    
    while(true) {
        //debug("...");
        listen();
        RING_DELAY_SIMULATION;
    }
    
}

// ------------ BUS -------------

void Ring::busReset(int level) {
    for(int i=0; i < bus->length; i++) {
        //this->bus[i] = bus[i];
        digitalWrite(bus->pins[i], HIGH);
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
        RING_DELAY_SIMULATION;
    }
}
    
// -------------- RING ---------------


bool Ring::receiveCheck() {
    if(isClockTick()) {
        // .. read pack stuff
        if(receive()) {
            //debug("Package received:", &pack);
            this->onPackReceivedHandler(pack);
        }
        return true;
    }
    return false;
}

bool Ring::listen() {
    if(tokenCheck()) {
        
        while(!queue.isEmpty()) {
            if(queue.top().isAddressedBetween(address)) {
                onPackReceivedHandler(queue.pop());
            } else {
                send(queue.pop());
            }
        }
        
        // .. do stuff
        if(NULL != onTokenOwnedLoopHandler) onTokenOwnedLoopHandler();
        
        tokenPass();
        return true;
    }
    
    if(!receiveCheck() && NULL != onTokenWaitingLoopHandler) onTokenWaitingLoopHandler();
    return false;
}

void Ring::sendClock() {
    //debug("Clocking out");
    clockTick();
    delay(RING_DELAY_CLOCK);
}

bool Ring::send(Pack pack) {
    //debug("Package sending:", pack);
    if(queue.isFull()) {
        error("queue full", 1);
        return false;
    }
    
    if(tokenCheck()) {
        send(pack.type, RING_DATA_TYPE_BITS);
        send(pack.from.first, RING_ADDRESS_FIRST_BITS);
        send(pack.from.length, RING_ADDRESS_LENGTH_BITS);
        send(pack.to, RING_ADDRESS_FIRST_BITS);
        if(pack.from.length != pack.messageList.length) {
            error("Ambiguous message length", 5);
        }
        for(int k=0; k < pack.messageList.length; k++) {
            send(pack.messageList.messages[k].length, RING_DATA_LENGTH_BITS);
            int j = 0; // analog pin
            for(int i=0; i < pack.messageList.messages[k].length; i++) {
                if(pack.type == RING_PACKTYPE_DIGITAL) {
                    send(pack.messageList.messages[k].buffer[i], RING_DATA_VALUE_BITS);
                } else if(pack.type == RING_PACKTYPE_ANALOG) {
                    analogWrite(bus->pins_analog[j], pack.messageList.messages[k].buffer[i]);
                    j++;
                    if(j > bus->length_analog) {
                        j = 0;
                        sendClock();
                    }
                } else {
                    error("Wrong pack type:", pack.type);
                }
            }
        }
        sendClock();
        if(!initialized && NULL != onPackReceivedHandler) onPackReceivedHandler(pack);
    } else {
        queue.push(pack);
    }
    
    return true;
}

void Ring::send(int data, int bits) {
    //debug("Send:", data);
    for(int i=0; i < bits;) {
        for(int j=0; j < bus->length && i < bits; j++) {
            int bit = data & 1;
            data >>= 1;
            //debug("Write bit to bus[",j,"] =>", bit);
            digitalWrite(bus->pins[j], bit);
            i++;
        }
        sendClock();
    }
}

bool Ring::receive() {
    // read additional meta info of message
    int type = read(RING_DATA_TYPE_BITS);
    Platoon from;
    from.first = read(RING_ADDRESS_FIRST_BITS);
    from.length = read(RING_ADDRESS_LENGTH_BITS);
    int to = read(RING_ADDRESS_FIRST_BITS);
    
    // check if it's sent to us
    bool our = pack.isBroadcast(to) || pack.isAddressedBetween(to, address);
    
    if(our) {
        pack.to = to;
        pack.from.first = from.first;
        pack.from.length = from.length;
    }
    
    for(int k=0; k < from.length; k++) {
        int length = read(RING_DATA_LENGTH_BITS);
        // read message contents
        if(our) {
            pack.messageList.messages[k].length = length;
        }
        int j = 0; // analog pin
        for(int i=0; i<length; i++) {
            if(type == RING_PACKTYPE_DIGITAL) {
                int data = read(RING_DATA_VALUE_BITS);            
                if(our) {
                    // store if it's for us
                    pack.messageList.messages[k].buffer[i] = data;
                }
            } else if(type == RING_PACKTYPE_ANALOG) {
                int data = analogRead(bus->pins_analog[j]);            
                if(our) {
                    // store if it's for us
                    pack.messageList.messages[k].buffer[i] = data;
                }
                j++;
                if(j > bus->length_analog) {
                    j = 0;
                    clockWaiting();
                }
            } else {
                error("Wrong type read:", type);
            }
        }
    }
    
    return our;
}

int Ring::read(int bits) {
    //debug("Reading start...");
    int data = 0;
    for(int i=0; i < bits;) {
        for(int j=0; j < bus->length && i < bits; j++) {
            int bit = digitalRead(bus->pins[j]);
            //debug("Read bit on bus[",j,"] <=", bit);
            data = data | (bit << i);
            i++;
        }
        if(i < bits) {
            clockWaiting();
        }
    }
    clockWaiting();
    
    // sign
    if(RING_DATA_SIGNED && data > (1<<bits)/2) {
        data -= 1<<bits;
    }
    //debug("Read:", data);
    return data;
}

Ring* Ring::that;

void Ring::initReceiveHandler(Pack pack) {
    if(!that->initialized) {
        that->address.first = pack.messageList.messages[0].buffer[0];
        //debug("Slave set the address:", that->address);
    }
    that->length = pack.messageList.messages[0].buffer[0];
}

void Ring::initTokenOwnedHandler() {
    if(!that->initialized) {
        that->pack.from.first = that->address.first;
        that->pack.from.length = 1;
        that->pack.messageList.length = 1;
        that->pack.messageList.messages[0].make(that->address.first+that->address.length);
        that->pack.make(that->pack.from, that->pack.messageList, RING_ADDR_BROADCAST, RING_PACKTYPE_DIGITAL);
        that->send(that->pack);
        that->initialized = true;
    }
}
