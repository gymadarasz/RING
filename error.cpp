
#include <fstream>

#define DEBUG_PREFIX "****** "
#define DEBUG_SUFFIX ""

#define LOG simulation->log

#define SHOW_ERROR  true
#define SHOW_WARN   true
#define SHOW_PANIC  true
#define SHOW_INFO   true
#define SHOW_FAIL   true
#define SHOW_DEBUG  true // set true for debuging
#define SHOW_TEST   true // set true for self testing

// only if SHOW_TEST is true
#define SHOW_TEST_ACCEPTANCE        SHOW_TEST    
#define SHOW_TEST_ADDITIONAL_INFO   SHOW_TEST && SHOW_DEBUG   

#define THROW_ERROR true
#define THROW_WARN  false
#define THROW_PANIC false
#define THROW_INFO  false
#define THROW_FAIL  true

#define DEBUG_MAX_INTPTR 5

using namespace std;

void show_time() {
    simulation->log << "[" << millis() << "] ";
}

void show_device() {
    if(simulation->logLastDevice != device->id) {
        simulation->logLastDevice = device->id;
        simulation->log << endl;
        simulation->log.flush();
    }
    simulation->log << "[" << device->id << "] ";
}

void log_start() {
    while(simulation->logBlock) simulation->delay(device->id, 1);
    simulation->logBlock = true;
    show_device();
    show_time();
}

void log_end() {
    simulation->log.flush();
    simulation->logBlock = false;
}


void error(int code) {
    if(SHOW_ERROR) {
        log_start();
        LOG << "[ERROR] " << code << endl;
        log_end();
    }
    if(THROW_ERROR) {
        throw code;
    }
}

void error(const char* msg, int code) {
    if(SHOW_ERROR) {
        log_start();
        LOG << "[ERROR] " << msg << " - " << code << endl;
        log_end();
    }
    if(THROW_ERROR) {
        throw code;
    }
}

void error(int code, int info) {
    if(SHOW_ERROR) {
        log_start();
        LOG << "[ERROR] " << code << ", info: " << info << endl;
        log_end();
    }
    if(THROW_ERROR) {
        throw code;
    }
}

void error(const char* msg) {
    if(SHOW_ERROR) {
        log_start();
        LOG << "[ERROR] " << msg << " - " << endl;
        log_end();
    }
    if(THROW_ERROR) {
        throw msg;
    }
}

void warn(const char* msg, int code) {
    if(SHOW_WARN) {
        log_start();
        LOG << "[WARN] " << msg << " - " << code << endl;
        log_end();
    }
    if(THROW_WARN) {
        throw code;
    }
}

void warn(int code) {
    if(SHOW_WARN) {
        log_start();
        LOG << "[WARN] " << code << endl;
        log_end();
    }
    if(THROW_WARN) {
        throw code;
    }
}


void panic(int code) {
    if(SHOW_PANIC) {
        log_start();
        LOG << "[PANIC] " << code << endl;
        log_end();
    }
    if(THROW_PANIC) {
        throw code;
    }
}


void info(const char* msg) {
    if(SHOW_INFO) {
        log_start();
        LOG << "[INFO] " << msg << endl;
        log_end();
    }
    if(THROW_INFO) {
        throw msg;
    }
}

void info(const char* msg, int nfo) {
    if(SHOW_INFO) {
        log_start();
        LOG << "[INFO] " << msg << " " << nfo << endl;
        log_end();
    }
    if(THROW_INFO) {
        throw msg;
    }
}


void debug(const char* msg) {
    if(SHOW_DEBUG) {
        log_start();
        LOG << "[DEBUG] " << DEBUG_PREFIX << msg << DEBUG_SUFFIX << endl;
        log_end();
    }
}

void debug(const char* msg, int nfo) {
    if(SHOW_DEBUG) {
        log_start();
        LOG << "[DEBUG] " << DEBUG_PREFIX << msg << " " << nfo << DEBUG_SUFFIX << endl;
        log_end();
    }
}

void debug(const char* msg, int* nfos, int length) {
    if(SHOW_DEBUG) {
        log_start();
        LOG << "[DEBUG] " << DEBUG_PREFIX << msg << " ";
        int i;
        for(i=0; i<length-1 && i<DEBUG_MAX_INTPTR; i++) {
            LOG << nfos[i] << ", ";
        }
        if(i<length) {
            LOG << nfos[i];
        } else if(i>=DEBUG_MAX_INTPTR){
            LOG << "...";
        }
        LOG << DEBUG_SUFFIX << endl;
        log_end();
    }
}

void debug(const char* msg0, int nfo0, const char* msg1, int nfo1) {
    if(SHOW_DEBUG) {
        log_start();
        LOG << "[DEBUG] " << DEBUG_PREFIX << msg0 << nfo0 << msg1 << " " << nfo1 << DEBUG_SUFFIX << endl;
        log_end();
    }
}




void test(const char* acceptance, const char* additional, int info) {
    if(SHOW_TEST) {
        log_start();
        LOG << "[TEST] "
#if SHOW_TEST_ACCEPTANCE
                << acceptance 
#endif
#if SHOW_TEST_ADDITIONAL_INFO
                << " - " << additional << " - info: " << info 
#endif
                << endl;
        log_end();
    }
}

void test(const char* acceptance, const char* additional, int info0, int info1) {
    if(SHOW_TEST) {
        log_start();        
        LOG << "[TEST] " 
#if SHOW_TEST_ACCEPTANCE
                << acceptance 
#endif
#if SHOW_TEST_ADDITIONAL_INFO
                << " - " << additional << " - info: " << info0 << ", " << info1
#endif 
                << endl;
        log_end();
    }
}


void fail(const char* err) {
    if(SHOW_FAIL) {
        log_start();
        LOG << "[FAIL] " << err << endl;
        log_end();
    }
    if(THROW_FAIL) {
        throw(err);
    }
}



void test_true(const char* name, bool a, const char* err) {
    test(name, "Testing assert true (bool):", a);
    if(!a) {
        fail(err);
    }
}
void test_true(const char* name, bool a) {
    test_true(name, a, name);
}




void test_false(const char* name, bool a, const char* err) {
    test(name, "Testing assert false (bool):", a);
    if(a) {
        fail(err);
    }
}
void test_false(const char* name, bool a) {
    test_false(name, a, name);
}



void test_equ(const char* name, bool a, bool b, const char* err) {
    test(name, "Testing assert equals (bool):", a, b);
    if(a == !b) {
        fail(err);
    }
}
void test_equ(const char* name, bool a, bool b) {
    test_equ(name, a, b, name);
}



void test_nequ(const char* name, bool a, bool b, const char* err) {
    test(name, "Testing assert non-equals (bool):", a, b);
    if(a && b) {
        fail(err);
    }
}
void test_nequ(const char* name, bool a, bool b) {
    test_nequ(name, a, b, name);
}



void test_equ(const char* name, int a, int b, const char* err) {
    test(name, "Testing assert equals (int):", a, b);
    if(a != b) {
        fail(err);
    }
}
void test_equ(const char* name, int a, int b) {
    test_equ(name, a, b, name);
}



void test_nequ(const char* name, int a, int b, const char* err) {
    test(name, "Testing assert non-equals (int):", a, b);
    if(a == b) {
        fail(err);
    }
}
void test_nequ(const char* name, int a, int b) {
    test_nequ(name, a, b, name);
}


void test_equ(const char* name, float a, float b, const char* err) {
    test(name, "Testing assert equals (float):", a, b);
    if(a != b) {
        fail(err);
    }
}
void test_equ(const char* name, float a, float b) {
    test_equ(name, a, b, name);
}




void test_nequ(const char* name, float a, float b, const char* err) {
    test(name, "Testing assert non-equals (float):", a, b);
    if(a == b) {
        fail(err);
    }
}
void test_nequ(const char* name, float a, float b) {
    test_nequ(name, a, b, name);
}

