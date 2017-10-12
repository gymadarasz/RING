
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
#define BASE_CMD_UNDO 5

#define BASE_MSG_STATE 2


typedef struct {
    int numerator;
    int denominator;
} Fraction;

class Neuron {
public:
    // Connection descriptors
    int address;
    
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
    
    void init(int address, int bias = (NEURON_VALUE_MAX-NEURON_VALUE_MIN) / 2, int leak = 0, int momentum = 0);
    void setInputs(Platoon inputs);
    void randomize(Fraction chance);
    void randomize();
    void clear();
    bool collect(int from, int value);
    void activate();
};


void Neuron::init(int address, int bias, int leak, int momentum) {
    this->address = address;
    this->bias = bias;
    this->leak = leak;
    this->momentum = momentum;
    clear();
    randomize();
}

void Neuron::setInputs(Platoon inputs) {
    this->inputs.first = inputs.first;
    this->inputs.length = inputs.length;
}

void Neuron::randomize(Fraction chance) {
    if(random(1, chance.denominator) <= chance.numerator) 
        momentum = random(NEURON_VALUE_MIN, NEURON_VALUE_MAX);
    if(random(1, chance.denominator) <= chance.numerator) 
        bias = random(NEURON_VALUE_MIN, NEURON_VALUE_MAX * (inputs.length + (momentum ? 1 : 0)));
    if(random(1, chance.denominator) <= chance.numerator) 
        leak = random(NEURON_VALUE_MIN, NEURON_NORMALIZE_BITS);
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

//---------------

class NeuronList {
public:
    int length;
    Neuron neurons[NEURON_LIST_MAX];
    NeuronList();
};

NeuronList::NeuronList() {
    length = -1;
}

//------------


class Block: public Ring {
public:
    static Block* that;
    
    NeuronList neuronList;
    
    int counter;
    
    Pack packOut;
    
    void init(
        const bool master, const int prev, const int next, const int clock, 
        const Bus bus, const int multiple = 1, 
        Handler onClockWaitingLoopHandler = RING_DEFAULT_HANDLER,
        Handler onTokenOwnedLoopHandler = RING_DEFAULT_HANDLER, 
        Handler onTokenWaitingLoopHandler = RING_DEFAULT_HANDLER
    );
    void setInputs(Platoon inputs);
    static void onPackReceiveHandler(Pack pack);
};

Block* Block::that;

void Block::init(
    const bool master, const int prev, const int next, const int clock, 
    const Bus bus, const int multiple, 
    Handler onClockWaitingLoopHandler,
    Handler onTokenOwnedLoopHandler, 
    Handler onTokenWaitingLoopHandler
) {
    that = this;
    
    Ring::init(master, prev, next, clock, bus, onPackReceivedHandler, multiple,
        onClockWaitingLoopHandler, onTokenOwnedLoopHandler, onTokenWaitingLoopHandler);
    
    // TODO randomSeed
    counter = neuronList.length = multiple;
    for(int i=0; i < multiple; i++) {
        neuronList.neurons[i].init(address.first+i);
    }
    
    packOut.from.first = address.first;
    packOut.from.length = address.length;
    packOut.to = RING_ADDR_BROADCAST;
    packOut.type = RING_PACKTYPE_ANALOG;
    
}

void Block::onPackReceiveHandler(Pack pack) {
    // Todo: use 'that' as 'this' self
    if(pack.from.length != pack.messageList.length) {
        error("Ambiguous incoming package length.", 1);
    }
    // goes through each incoming message
    for(int i = 0; i < pack.from.length; i++) {
        // goes through each neuron in this block
        for(int j = 0; j < that->neuronList.length; j++) {
            int packFrom = pack.from.first+i;
            // if current message for current neuron..
            if(
                    packFrom >= that->neuronList.neurons[j].inputs.first && 
                    packFrom < 
                        that->neuronList.neurons[j].inputs.first +
                        that->neuronList.neurons[j].inputs.length
            ) {
                // collect to SUM
                bool activated = that->neuronList.neurons[j].collect(
                    packFrom, 
                    pack.messageList.messages[j].buffer[0]
                );
                // send output if neuron is activated
                if(activated) {
                    that->packOut.messageList.length = 1;
                    that->packOut.messageList.messages[j].buffer[0] = 
                            that->neuronList.neurons[j].output;
                    that->counter--;
                    if(that->counter == 0) {
                        that->counter = that->neuronList.length;
                        that->send(that->packOut);
                    }
                }
            }
        }
    }
}





//class Neuron {
//private:
//    int sum;
//    int counter;
//public:
//    int address;
//    int first;
//    int last;
//    int min;
//    int max;
//    int weights[NEURON_INPUTS];
//    int bias;
//    int momentum;
//    int weight;
//    void init(
//        int address, int first = 0, int last = NEURON_INPUTS, 
//        int min = NEURON_VALUE_MIN, int max = NEURON_VALUE_MAX, 
//        bool randomize = true
//    );
//    void clear();
//    void copy(Neuron* neuron);
//    void randomize(int numerator = 1, int denominator = 1);
//    void collect(Ring* ring, int from, int value);
//};
//
//void Neuron::init(
//    int address, int first, int last, 
//    int min, int max, bool randomize
//) {
//    this->address = address;
//    this->first = first;
//    this->last = last;
//    this->min = min;
//    this->max = max;
//    bias = 0;
//    momentum = 0;
//    weight = 0;
//    for(int i=0; i < last; i++) {
//        weights[i] = 0;
//    }
//    if(randomize) {
//        this->randomize();
//    }
//    
//    clear();
//}
//
//void Neuron::clear() {
//    sum = 0;
//    counter = first;
//}
//
//void Neuron::copy(Neuron* neuron) {
//    neuron->first = first;
//    neuron->last = last;
//    neuron->bias = bias;
//    neuron->momentum = momentum;
//    neuron->weight = weight;
//    for(int i=0; i < last-first; i++) {
//        neuron->weights[i] = weights[i];
//    }
//}
//
//void Neuron::randomize(int numerator, int denominator) {
//    int length = last-first;
//    if(random(1, denominator) <= numerator) bias = random(min*length, max*length);
//    if(random(1, denominator) <= numerator) momentum = random(min*length, max*length);
//    if(random(1, denominator) <= numerator) weight = random(min, max);
//    for(int i=0; i < length; i++) {
//        if(random(1, denominator) <= numerator) weights[i] = random(min, max);
//    }
//}
//
//void Neuron::collect(Ring* ring, int from, int value) {
//    if(from >= first && from <= last) {
//        sum += (value * weights[from-first]);
//        counter++;
//        if(counter > last) {
//            int relu = sum > bias ? sum : bias;
//            ring->send(ring->pack.make(
//                from, RING_ADDR_BROADCAST, relu, NULL, RING_PACKTYPE_ANALOG));
//            clear();
//        }
//    }
//}



//class Neuron {
//private:
//    State stack;
//    
//public:
//    Ring* ring;
//    
//    int min;
//    int max;
//    
//    int address;
//    
//    int countdown;
//    int sum;
//    
//    State state;
//    
//    void init(
//        Ring* ring, int address, int first = 0, int inputs = INPUTS, 
//        const int min = VALUE_MIN, const int max = VALUE_MAX
//    );
//    
//    void clean();
//    void clear();
//    void store();
//    void restore();
//    void change(int chance);
//    void input(int input, int value);
//    virtual int activate();
//    
//};
//
//void Neuron::init(
//    Ring* ring, int address, int first, int inputs,
//    const int min, const int max
//) {
//    this->ring = ring;
//    this->address = address;
//    this->min = min;
//    this->max = max;
//    clear();
//}
//
//void Neuron::clean() {
//    countdown = inputs;
//    sum = 0;
//}
//
//void Neuron::clear() {
//    clean();
//    state.clear();
//}
//
//void Neuron::store() {
//    state.copy(&stack);
//}
//
//void Neuron::restore() {
//    stack.copy(&state);
//}
//
//void Neuron::change(int chance) {
//    state.change(chance, min, max);
//}
//
//void Neuron::input(int input, int value) {
//    sum += (value*state.weights[input-first]);
//    countdown--;
//    if(countdown == 0) {
//        int output = activate();
//        
//        ring->pack.from = address;
//        ring->pack.to = RING_ADDR_BROADCAST;
//        ring->pack.length = 1;
//        ring->pack.buffer[0] = output;
//        ring->pack.type = RING_PACKTYPE_ANALOG;
//        
////        Pack* pack = ring->pack.make(
////            address, RING_ADDR_BROADCAST, 
////            output, 
////            NULL, RING_PACKTYPE_ANALOG
////        );
//
//        ring->send(&ring->pack);
//        
//        clean();
//    }
//}
//
//int Neuron::activate() {
//    // relu
//    return sum > state.bias ? sum : state.bias;
//}
//
//
//
//class NeuronRing: public Ring {
//public:
//    
//    static NeuronRing* that;
//    Neuron neurons[NEURONS];
//    
//    void init(
//        const bool master, const int prev, const int next, const int clock, 
//        const Bus bus, const int multiple = NEURONS, 
//        int first = 0, int inputs = INPUTS, 
//        const int min = VALUE_MIN, const int max = VALUE_MAX
//    ) {
//        that = this;
//        Ring::init(master, prev, next, clock, bus, onReceive, multiple, NULL, NULL, NULL);
//        
//        clear(first, inputs, min, max);
//        
//        Ring::start();
//    }
//    
//    void clear(
//        int first = 0, int inputs = INPUTS, 
//        const int min = VALUE_MIN, const int max = VALUE_MAX
//    ) {
//        for(int n=0; n < multiple; n++) {
//            neurons[n].init(this, address+n, first, inputs, min, max);
//            neurons[n].clear();
//        }
//    }
//    
//    void store() {
//        for(int n=0; n < multiple; n++) {
//            neurons[address+n].store();
//        }
//    }
//    
//    void restore() {
//        for(int n=0; n < multiple; n++) {
//            neurons[address+n].restore();
//        }
//    }
//    
//    void change(int chance) {
//        for(int n=0; n < multiple; n++) {
//            neurons[address+n].change(chance);
//        }
//    }
//    
//    void static onReceive(Pack* pack) {
//        if(pack->from == BASE_ADDRESS) {
//            if(pack->buffer[0] == BASE_CMD_SET_STATE) {
//                that->neurons[pack->to - that->address].clear();
//                that->neurons[pack->to - that->address].first = pack->buffer[1];
//                that->neurons[pack->to - that->address].inputs = pack->buffer[2];
//                ((State*)pack->buffer[3])->copy(&that->neurons[pack->to - that->address].state);
//            } else if(pack->buffer[0] == BASE_CMD_GET_STATE) {
//                pack->from = pack->to - that->address;
//                pack->to = BASE_ADDRESS;
//                pack->length = that->neurons[pack->from].inputs+2;
//                pack->buffer[0] = BASE_MSG_STATE;
//                pack->buffer[1] = that->neurons[pack->from].first;
//                pack->buffer[2] = that->neurons[pack->from].inputs;
//                that->neurons[pack->from].state.copy((State*)pack->buffer[3]);
//                pack->type = RING_PACKTYPE_ANALOG;
//                that->send(pack);
//            } else if(pack->buffer[0] == BASE_CMD_RANDOM_SEED) { 
//                randomSeed(pack->buffer[1]);
//            } else if(pack->buffer[0] == BASE_CMD_RANDOM_CHANGE) {
//                that->store();
//                that->change(pack->buffer[1]);
//            } else if(pack->buffer[0] == BASE_CMD_UNDO) {
//                that->restore();
//            } else {
//                // unknown command
//                error("Unknown command", 1);
//            }
//        } else {
//        
//            for(int i=0; i < that->multiple; i++) {
//                if(
//                    pack->from >= that->neurons[i].first && 
//                    pack->from < that->neurons[i].first+that->neurons[i].inputs)
//                {
//                    that->neurons[i].input(pack->from, pack->buffer[0]);
//                }
//            }
//        
//        }
//    }
//    
//};
