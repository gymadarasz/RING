
#include <fstream>

#define DEBUG_PREFIX "****** "
#define DEBUG_SUFFIX ""

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

void error(int code) {
    while(simulation->logBlock);
    simulation->logBlock = true;
    show_device();
    show_time();
    simulation->log << "[ERROR] " << code << endl;
    simulation->log.flush();
    throw code;
    simulation->logBlock = false;
}

void error(int code, int info) {
    while(simulation->logBlock);
    simulation->logBlock = true;
    show_device();
    show_time();
    simulation->log << "[ERROR] " << code << ", info: " << info << endl;
    simulation->log.flush();
    throw code;
    simulation->logBlock = false;
}

void warn(int code) {
    while(simulation->logBlock);
    simulation->logBlock = true;
    show_device();
    show_time();
    simulation->log << "[WARN] " << code << endl;
    simulation->log.flush();
    //throw code;
    simulation->logBlock = false;
}

void panic(int code) {
    while(simulation->logBlock);
    simulation->logBlock = true;
    show_device();
    show_time();
    simulation->log << "[PANIC] " << code << endl;
    simulation->log.flush();
    //throw code;
    simulation->logBlock = false;
}

void info(const char* msg) {
    while(simulation->logBlock);
    simulation->logBlock = true;
    show_device();
    show_time();
    simulation->log << "[INFO] " << msg << endl;
    simulation->log.flush();
    //throw code;
    simulation->logBlock = false;
}

void info(const char* msg, int nfo) {
    while(simulation->logBlock);
    simulation->logBlock = true;
    show_device();
    show_time();
    simulation->log << "[INFO] " << msg << " " << nfo << endl;
    simulation->log.flush();
    //throw code;
    simulation->logBlock = false;
}


void debug(const char* msg) {
    while(simulation->logBlock);
    simulation->logBlock = true;
    show_device();
    show_time();
    simulation->log << "[DEBUG] " << DEBUG_PREFIX << msg << DEBUG_SUFFIX << endl;
    simulation->log.flush();
    //throw code;
    simulation->logBlock = false;
}

void debug(const char* msg, int nfo) {
    while(simulation->logBlock);
    simulation->logBlock = true;
    show_device();
    show_time();
    simulation->log << "[DEBUG] " << DEBUG_PREFIX << msg << " " << nfo << DEBUG_SUFFIX << endl;
    simulation->log.flush();
    //throw code;
    simulation->logBlock = false;
}

void debug(const char* msg0, int nfo0, const char* msg1, int nfo1) {
    while(simulation->logBlock);
    simulation->logBlock = true;
    show_device();
    show_time();
    simulation->log << "[DEBUG] " << DEBUG_PREFIX << msg0 << nfo0 << msg1 << " " << nfo1 << DEBUG_SUFFIX << endl;
    simulation->log.flush();
    //throw code;
    simulation->logBlock = false;
}