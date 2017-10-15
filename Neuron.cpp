
#include "Ring.cpp"

#define NEURON_VERSION 0x0001

#define NEURON_INPUTS_MAX 256   // maximum input per neuron
#define NEURON_LIST_MAX 8           // maximum neuron per block

// TODO: normalize for analog ADC/DAC I/O values (10 bits => value[0..1023])
#define NEURON_NORMALIZE_BITS 10 // ADC/DAC data length
#define NEURON_VALUE_MAX 1<<NEURON_NORMALIZE_BITS - 1  // ADC/DAC value max (weight and I/O value max)
#define NEURON_VALUE_MIN 0 // force values between 0-1023 for analog Read/Write

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

class Neuron {
public:
    // Connection descriptors
    int address;
    int begin; // first non input neuron address
    Platoon inputs;
    int weights[NEURON_INPUTS_MAX];   // NEURON_VALUE_MAX - 10 bits integer, max value: [0..1023] for ADC/DAC I/O
    
    // Activation function parameters
    int bias;
    int leak;       // NEURON_NORMALIZE_BITS - bits for rotating out, below bias threshold
    int momentum;   // NEURON_VALUE_MAX - 10 bits integer, max value: [0..1023] for ADC/DAC I/O
    int previous;
    
    // for SUM and momentum
    int sum;
    int counter;
    int output;     // NEURON_VALUE_MAX - 10 bits integer, max value: [0..1023] for ADC/DAC I/O
    
    void init(int address, int begin, Platoon inputs = {0, 0}, int bias = (NEURON_VALUE_MAX-NEURON_VALUE_MIN) / 2, int leak = 0, int momentum = 0);
    void randomize(Fraction chance);
    void randomize();
    void clear();
    bool collect(int from, int value);
    void activate();
    int exportState(int* buffer);
    void importState(int* buffer);
};


void Neuron::init(int address, int begin, Platoon inputs, int bias, int leak, int momentum) {
    this->address = address;
    this->begin = begin;
    this->inputs.first = inputs.first;
    this->inputs.length = inputs.length;
    this->bias = bias;
    this->leak = leak;
    this->momentum = momentum;
    clear();
    randomize();
}


void Neuron::randomize(Fraction chance) {
    if(random(1, chance.denominator) <= chance.numerator) 
        momentum = random(NEURON_VALUE_MIN, NEURON_VALUE_MAX);
    if(random(1, chance.denominator) <= chance.numerator) 
        bias = random(NEURON_VALUE_MIN, NEURON_VALUE_MAX * (inputs.length + (momentum ? 1 : 0)));
    if(random(1, chance.denominator) <= chance.numerator) 
        leak = random(NEURON_VALUE_MIN, NEURON_NORMALIZE_BITS);
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
            weights[i] = random(NEURON_VALUE_MIN, NEURON_VALUE_MAX);
    }
}

void Neuron::randomize() {
    const Fraction nochance = {1, 1};
    randomize(nochance);
}

void Neuron::clear() {
    sum = 0;
    counter = 0;
    previous = 0;
}

// value should be a NEURON_VALUE_MAX - 10 bits integer, max value: [0..1023] for ADC/DAC I/O
bool Neuron::collect(int from, int value) {
    sum += (value * weights[from-inputs.first]);
    counter++;
    if(counter > inputs.length) {
        activate();
        clear();
        return true;
    }
    return false;
}

void Neuron::activate() {
    
    // Add Momentum and AVG normalization
    sum += ((previous * momentum) / (inputs.length + (momentum ? 1 : 0)));
    
    // Leaky ReLU, Normalized for DAC
    output = (sum > bias ? sum : sum >> leak) >> NEURON_NORMALIZE_BITS;
    
    // Store output to next momentum calculation
    previous = output;
    
}

int Neuron::exportState(int* buffer) {
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

void Neuron::importState(int* buffer) {
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

class NeuronList {
public:
    int length;
    Neuron neurons[NEURON_LIST_MAX];
    int stack[NEURON_LIST_MAX][sizeof(Neuron)];
    NeuronList();
    void store();
    void restore();
};

NeuronList::NeuronList() {
    length = -1;
}

void NeuronList::store() {
    for(int i = 0; i < length; i++) {
        neurons[i].exportState(stack[i]);
    }
}

void NeuronList::restore() {
    for(int i = 0; i < length; i++) {
        neurons[i].importState(stack[i]);
    }
}

//------------


class Block: public Ring {
public:
    static Block* block;
    
    NeuronList neuronList;
    
    int counter;
    
    Pack packOut;
    Pack packResp;
    
    void init(
        bool master, int prev, int next, int clock, 
        BusDA bus, int begin, int multiple = 1, 
        Handler onClockWaitingLoopHandler = RING_DEFAULT_HANDLER,
        Handler onTokenOwnedLoopHandler = RING_DEFAULT_HANDLER, 
        Handler onTokenWaitingLoopHandler = RING_DEFAULT_HANDLER
    );
    static void onBlockPackReceiveHandler(Pack pack);
};

Block* Block::block;

void Block::init(
    bool master, int prev, int next, int clock, 
    BusDA bus, int begin, int multiple, 
    Handler onClockWaitingLoopHandler,
    Handler onTokenOwnedLoopHandler, 
    Handler onTokenWaitingLoopHandler
) {
    block = this;
    
    Ring::init(master, prev, next, clock, bus, onBlockPackReceiveHandler, multiple,
        onClockWaitingLoopHandler, onTokenOwnedLoopHandler, onTokenWaitingLoopHandler);
    
    // TODO randomSeed
    counter = neuronList.length = multiple;
    for(int i=0; i < multiple; i++) {
        neuronList.neurons[i].init(address.first+i, begin);
    }
    
}

void Block::onBlockPackReceiveHandler(Pack pack) {
    debug("onBlockPackReceiveHandler:", pack);
    // Todo: use 'that' as 'this' self
    if(pack.from.length != pack.messageList.length) {
        error("Ambiguous incoming package length.", 1);
    }
    
    // if message came from BASE and the neuron in this block.. (or BC)
    if(
        pack.from.first == BASE_ADDRESS
    ) {
        // handle base commands
        if(
            pack.to >= block->neuronList.neurons[0].address &&
            (pack.to < block->neuronList.neurons[0].address + block->neuronList.length)
        ) {
            if(pack.messageList.messages[0].buffer[0] == BASE_CMD_GET_STATE) {
                block->packResp.messageList.messages[0].buffer[0] = BASE_MSG_STATE;
                block->packResp.messageList.messages[0].length = 
                    block->neuronList.neurons[pack.to - block->neuronList.neurons[0].address]
                        .exportState(
                            &block->packResp.messageList.messages[0].buffer[1]
                        );
            
                block->packResp.from.first = pack.to;
                block->packResp.from.length = 1;
                block->packResp.to = BASE_ADDRESS;
                block->packResp.type = RING_PACKTYPE_DIGITAL;
                block->packResp.messageList.length = 1;
                
                block->send(block->packResp);
            } else if(pack.messageList.messages[0].buffer[0] == BASE_CMD_SET_STATE) {
                block->neuronList.neurons[pack.to - block->neuronList.neurons[0].address]
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
                Fraction chance;
                chance.denominator = pack.messageList.messages[0].buffer[1];
                chance.numerator = pack.messageList.messages[0].buffer[2];
                block->neuronList.neurons[pack.to - block->neuronList.neurons[0].address]
                    .randomize(chance);
            } else if(pack.messageList.messages[0].buffer[0] == BASE_CMD_STORE) {
                block->neuronList.store();
            } else if(pack.messageList.messages[0].buffer[0] == BASE_CMD_RESTORE) {
                block->neuronList.restore();
            } else {
                error("Unknown base command. (BC)", 2);
            }
        }
    } else {    
        // goes through each incoming message
        for(int i = 0; i < pack.from.length; i++) {
            // goes through each neuron in this block
            for(int j = 0; j < block->neuronList.length; j++) {
                int packFrom = pack.from.first+i;
                // if current message for current neuron..
                if(
                        packFrom >= block->neuronList.neurons[j].inputs.first && 
                        packFrom < 
                            block->neuronList.neurons[j].inputs.first +
                            block->neuronList.neurons[j].inputs.length
                ) {
                    // collect to SUM
                    bool activated = block->neuronList.neurons[j].collect(
                        packFrom, 
                        pack.messageList.messages[j].buffer[0]
                    );
                    // send output if neuron is activated
                    if(activated) {
                        block->packOut.messageList.length = 1;
                        block->packOut.messageList.messages[j].buffer[0] = 
                                block->neuronList.neurons[j].output;
                        block->counter--;
                        if(block->counter == 0) {                    
                            block->packOut.from.first = block->address.first;
                            block->packOut.from.length = block->address.length;
                            block->packOut.to = RING_ADDR_BROADCAST;
                            block->packOut.type = RING_PACKTYPE_ANALOG;
                            block->send(block->packOut);
                            block->counter = block->neuronList.length;
                        }
                    }
                }
            }
        }
    }
}
