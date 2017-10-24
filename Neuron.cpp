
#include "Ring.cpp"

#define NODE_VERSION 0x0001

#define NODE_INPUTS_MAX 256   // maximum input per neuron
#define NODE_LIST_MAX 8           // maximum neuron per block

// TODO: normalize for analog ADC/DAC I/O values (10 bits => value[0..1023])
#define NODE_NORMALIZE_BITS 10 // ADC/DAC data length
#define NODE_VALUE_MAX 1<<NODE_NORMALIZE_BITS - 1  // ADC/DAC value max (weight and I/O value max)
#define NODE_VALUE_MIN 0 // force values between 0-1023 for analog Read/Write

#define BASE_ADDRESS -2

#define BASE_CMD_SET_STATE 1
#define BASE_CMD_GET_STATE 2
#define BASE_CMD_RANDOM_SEED 3
#define BASE_CMD_RANDOM_CHANGE 4
#define BASE_CMD_STORE 5
#define BASE_CMD_RESTORE 6

#define BASE_MSG_STATE 2


typedef struct {
    int numerator;
    int denominator;
} Fraction;

class Node {
public:
    // Connection descriptors
    int address;
    int begin; // first non input neuron address
    Platoon inputs;
    int weights[NODE_INPUTS_MAX];   // NEURON_VALUE_MAX - 10 bits integer, max value: [0..1023] for ADC/DAC I/O
    
    // Activation function parameters
    int bias;
    int leak;       // NEURON_NORMALIZE_BITS - bits for rotating out, below bias threshold
    int momentum;   // NEURON_VALUE_MAX - 10 bits integer, max value: [0..1023] for ADC/DAC I/O
    int previous;
    
    // for SUM and momentum
    int sum;
    int counter;
    int output;     // NEURON_VALUE_MAX - 10 bits integer, max value: [0..1023] for ADC/DAC I/O
    
    void init(
        int address, int begin, Platoon inputs = {0, 0}, 
        int bias = (NODE_VALUE_MAX-NODE_VALUE_MIN) / 2, 
        int leak = 0, int momentum = 0
    );
    void randomize(Fraction chance);
    void randomize();
    void clear();
    bool collect(int from, int value);
    void activate();
    int exportState(int* buffer);
    void importState(int* buffer);
};


void Node::init(
        int address, int begin, Platoon inputs, 
        int bias, int leak, int momentum
) {
    this->address = address;
    this->begin = begin;
    if(inputs.first <= 0 && inputs.length <= 0) {
        inputs.first = address - NODE_INPUTS_MAX;
        inputs.length = NODE_INPUTS_MAX;
    }
    if(inputs.first < 0) {
        inputs.first = 0;
    }
    if(inputs.first+inputs.length >= address) {
        inputs.length = address - 1;
    }
    this->inputs.first = inputs.first;
    this->inputs.length = inputs.length;
    this->bias = bias;
    this->leak = leak;
    this->momentum = momentum;
    clear();
    randomize();
}


void Node::randomize(Fraction chance) {
    if(random(1, chance.denominator) <= chance.numerator) 
        momentum = random(NODE_VALUE_MIN, NODE_VALUE_MAX);
    if(random(1, chance.denominator) <= chance.numerator) 
        bias = random(NODE_VALUE_MIN, NODE_VALUE_MAX * (inputs.length + (momentum ? 1 : 0)));
    if(random(1, chance.denominator) <= chance.numerator) 
        leak = random(NODE_VALUE_MIN, NODE_NORMALIZE_BITS);
    if(random(1, chance.denominator) <= chance.numerator) {
        do {
            inputs.first = random(0, address-1);
            inputs.length = random(0, address-begin) + 1;
        } while(
                inputs.first + inputs.length > address
        );
    }
    for(int i=0; i < inputs.length; i++) {
        if(random(1, chance.denominator) <= chance.numerator) 
            weights[i] = random(NODE_VALUE_MIN, NODE_VALUE_MAX);
    }
}

void Node::randomize() {
    const Fraction nochance = {1, 1};
    randomize(nochance);
}

void Node::clear() {
    sum = 0;
    counter = 0;
    previous = 0;
}

// value should be a NEURON_VALUE_MAX - 10 bits integer, max value: [0..1023] for ADC/DAC I/O
bool Node::collect(int from, int value) {
    sum += (value * weights[from-inputs.first]);
    counter++;
    if(counter > inputs.length) {
        activate();
        clear();
        return true;
    }
    return false;
}

void Node::activate() {
    
    // Add Momentum and AVG normalization
    sum += ((previous * momentum) / (inputs.length + (momentum ? 1 : 0)));
    
    // Leaky ReLU, Normalized for DAC
    output = (sum > bias ? sum : sum >> leak) >> NODE_NORMALIZE_BITS;
    
    // Store output to next momentum calculation
    previous = output;
    
}

int Node::exportState(int* buffer) {
    int size = 0;
    
    buffer[size] = address;         size += sizeof (address);
    buffer[size] = begin;           size += sizeof (begin);
    buffer[size] = bias;            size += sizeof (bias);
    buffer[size] = counter;         size += sizeof (counter);
    buffer[size] = inputs.first;    size += sizeof (inputs.first);
    buffer[size] = inputs.length;   size += sizeof (inputs.length);
    buffer[size] = leak;            size += sizeof (leak);
    buffer[size] = momentum;        size += sizeof (momentum);
    buffer[size] = output;          size += sizeof (output);
    buffer[size] = previous;        size += sizeof (previous);
    buffer[size] = sum;             size += sizeof (sum);
    
    for (int j = 0; j < inputs.length; j++) {
        buffer[size] = weights[j]; size += sizeof (weights[j]);
    }
    
    return size;
}

void Node::importState(int* buffer) {
    int i = 0;
    
    address         = buffer[i]; i += sizeof(address);
    begin           = buffer[i]; i += sizeof(begin);
    bias            = buffer[i]; i += sizeof(bias);
    counter         = buffer[i]; i += sizeof(counter);
    inputs.first    = buffer[i]; i += sizeof(inputs.first);
    inputs.length   = buffer[i]; i += sizeof(inputs.length);
    leak            = buffer[i]; i += sizeof(leak);
    momentum        = buffer[i]; i += sizeof(momentum);
    output          = buffer[i]; i += sizeof(output);
    previous        = buffer[i]; i += sizeof(previous);
    sum             = buffer[i]; i += sizeof(sum);
    
    for (int j = 0; j < inputs.length; j++) {
        weights[j] = buffer[i]; i += sizeof(weights[j]);
    }   
}

//---------------

class NodeList {
public:
    int length;
    Node nodes[NODE_LIST_MAX];
    int stack[NODE_LIST_MAX][sizeof(Node)];
    NodeList();
    void store();
    void restore();
};

NodeList::NodeList() {
    length = -1;
}

void NodeList::store() {
    for(int i = 0; i < length; i++) {
        nodes[i].exportState(stack[i]);
    }
}

void NodeList::restore() {
    for(int i = 0; i < length; i++) {
        nodes[i].importState(stack[i]);
    }
}

//--------------

class HiddenBlock: public Ring {
public:
    static HiddenBlock* hiddenBlock;// todo: can we use the super::ring instead?
    
    NodeList nodeList;
    
    int counter;
    
    Pack packOut;
    Pack packResp;
    
    void init(
        int prev, int next, int clock, 
        BusDA bus, int begin, int multiple = 1, 
        Handler onClockWaitingLoopHandler = RING_DEFAULT_HANDLER,
        Handler onTokenOwnedLoopHandler = RING_DEFAULT_HANDLER, 
        Handler onTokenWaitingLoopHandler = RING_DEFAULT_HANDLER
    );
    static void onHiddenBlockPackReceiveHandler(Pack pack);
};

HiddenBlock* HiddenBlock::hiddenBlock;

void HiddenBlock::init(
    int prev, int next, int clock, 
    BusDA bus, int begin, int multiple, 
    Handler onClockWaitingLoopHandler,
    Handler onTokenOwnedLoopHandler, 
    Handler onTokenWaitingLoopHandler
) {
    hiddenBlock = this;
    
    Ring::init(RING_SLAVE, prev, next, clock, bus, 
        onHiddenBlockPackReceiveHandler, multiple,
        onClockWaitingLoopHandler, 
        onTokenOwnedLoopHandler, 
        onTokenWaitingLoopHandler
    );
    
    // TODO randomSeed
    counter = nodeList.length = multiple;
    for(int i=0; i < multiple; i++) {
        nodeList.nodes[i].init(address.first+i, begin);
    }
    
    debug("****** HiddenBlock/Slave Initialized ******");
}

void HiddenBlock::onHiddenBlockPackReceiveHandler(Pack pack) {
    debug("onBlockPackReceiveHandler:", pack);
    // using 'that' as 'this' as self
    if(pack.from.length != pack.messageList.length) {
        error("Ambiguous incoming package length.", 1);
    }
    
    // if message came from BASE and the neuron in this block.. (or broadcast)
    if(
        pack.from.first == BASE_ADDRESS
    ) {
        // handle base commands
        if(
            pack.to >= hiddenBlock->nodeList.nodes[0].address &&
            (pack.to < hiddenBlock->nodeList.nodes[0].address + hiddenBlock->nodeList.length)
        ) {
            if(pack.messageList.messages[0].buffer[0] == BASE_CMD_GET_STATE) {
                hiddenBlock->packResp.messageList.messages[0].buffer[0] = BASE_MSG_STATE;
                hiddenBlock->packResp.messageList.messages[0].length = 
                    hiddenBlock->nodeList.nodes[pack.to - hiddenBlock->nodeList.nodes[0].address]
                        .exportState(
                            &hiddenBlock->packResp.messageList.messages[0].buffer[1]
                        );
            
                hiddenBlock->packResp.from.first = pack.to;
                hiddenBlock->packResp.from.length = 1;
                hiddenBlock->packResp.to = BASE_ADDRESS;
                hiddenBlock->packResp.type = RING_PACKTYPE_DIGITAL;
                hiddenBlock->packResp.messageList.length = 1;
                
                hiddenBlock->send(hiddenBlock->packResp);
            } else if(pack.messageList.messages[0].buffer[0] == BASE_CMD_SET_STATE) {
                hiddenBlock->nodeList.nodes[pack.to - hiddenBlock->nodeList.nodes[0].address]
                    .importState(
                        &pack.messageList.messages[0].buffer[1]
                    );
            } else if(pack.messageList.messages[0].buffer[0] == BASE_CMD_RANDOM_SEED) {
                randomSeed(pack.messageList.messages[0].buffer[1]);
            } else {
                error("Unknown base command.", 1);
            }
        } else if(pack.to == RING_ADDR_BROADCAST) {
            if(pack.messageList.messages[0].buffer[0] == BASE_CMD_RANDOM_CHANGE) {
                // back propagation
                Fraction chance;
                chance.denominator = pack.messageList.messages[0].buffer[1];
                chance.numerator = pack.messageList.messages[0].buffer[2];
                hiddenBlock->nodeList.nodes[pack.to - hiddenBlock->nodeList.nodes[0].address]
                    .randomize(chance);
            } else if(pack.messageList.messages[0].buffer[0] == BASE_CMD_STORE) {
                hiddenBlock->nodeList.store();
            } else if(pack.messageList.messages[0].buffer[0] == BASE_CMD_RESTORE) {
                hiddenBlock->nodeList.restore();
            } else {
                error("Unknown base command. (BC)", 2);
            }
        }
    } else {    
        // goes through each incoming message
        for(int i = 0; i < pack.from.length; i++) {
            // goes through each neuron in this block
            for(int j = 0; j < hiddenBlock->nodeList.length; j++) {
                int packFrom = pack.from.first+i;
                // if current message for current neuron..
                if(
                        packFrom >= hiddenBlock->nodeList.nodes[j].inputs.first && 
                        packFrom < 
                            hiddenBlock->nodeList.nodes[j].inputs.first +
                            hiddenBlock->nodeList.nodes[j].inputs.length
                ) {
                    // collect to SUM
                    bool activated = hiddenBlock->nodeList.nodes[j].collect(
                        packFrom, 
                        pack.messageList.messages[j].buffer[0]
                    );
                    // send output if neuron is activated
                    if(activated) {
                        hiddenBlock->packOut.messageList.length = 1;
                        hiddenBlock->packOut.messageList.messages[j].buffer[0] = 
                                hiddenBlock->nodeList.nodes[j].output;
                        hiddenBlock->counter--;
                        if(hiddenBlock->counter == 0) {                    
                            hiddenBlock->packOut.from.first = hiddenBlock->address.first;
                            hiddenBlock->packOut.from.length = hiddenBlock->address.length;
                            hiddenBlock->packOut.to = RING_ADDR_BROADCAST;
                            hiddenBlock->packOut.type = RING_PACKTYPE_ANALOG;
                            hiddenBlock->send(hiddenBlock->packOut);
                            hiddenBlock->counter = hiddenBlock->nodeList.length;
                        }
                    }
                }
            }
        }
    }
}

//----------------
//---------------- IO NODES
//----------------

//--------------

class IOBlock: public Ring {
public:
    
    int outputs;
    
    void init(
        int prev, int next, int clock, 
        BusDA bus, int inputs, int outputs, 
        Handler onClockWaitingLoopHandler = RING_DEFAULT_HANDLER,
        Handler onTokenOwnedLoopHandler = RING_DEFAULT_HANDLER, 
        Handler onTokenWaitingLoopHandler = RING_DEFAULT_HANDLER
    );
    static void onIOBlockPackReceiveHandler(Pack pack);
};

void IOBlock::init(
    int prev, int next, int clock, 
    BusDA bus, int inputs, int outputs, 
    Handler onClockWaitingLoopHandler,
    Handler onTokenOwnedLoopHandler, 
    Handler onTokenWaitingLoopHandler
) {
    this->outputs = outputs;
    
    Ring::init(RING_MASTER, prev, next, clock, bus, 
        onIOBlockPackReceiveHandler, inputs,
        onClockWaitingLoopHandler, 
        onTokenOwnedLoopHandler, 
        onTokenWaitingLoopHandler
    );
    
    debug("****** IOBlock/Mester/inputBlock Initialized ******");
    
}

void IOBlock::onIOBlockPackReceiveHandler(Pack pack) {
    debug("TODO: implement: onIOBlockPackReceiveHandler(), received:", pack);
}


