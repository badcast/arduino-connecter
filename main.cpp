#include <iostream>

#include "connector.h"

const char defaultUsbTty[] = "/dev/ttyUSB%d";

void awake(connector::Connector& con);

int main() {
    int x;
    bool result = false;
    connector::Connector con;

    std::cout << "Openning usb port ..." << std::endl;

    // retrive
    for (x = 0; !result && x < 5; ++x) {
        // find free port, and connect to
        result = con.connect();

        if (con) {
            std::cout << "port: " << con.port().name() << " connected!" << std::endl;

            std::cout << "Memory size: " << con.data().memorySize << std::endl;
            std::cout << "Memory free: " << con.data().memoryFree << std::endl;
            std::cout << "Memory used: " << (con.data().memorySize - con.data().memoryFree) << std::endl;
            std::cout << "LCD Rows: " << con.data().lcdRows << ", Cols: " << con.data().lcdCols << std::endl;

            awake(con);

        } else {
            std::cout << "Error opening port, retrive " << x + 1 << std::endl;
            connector::delay(1000);
        }
    }

    if (!result) {
        // todo: error
        std::cout << "5 retrive opening usb port failed. Exiting." << std::endl;
        return EXIT_FAILURE;
    }

    con.disconnect();
    std::cout << "Port disconnected!" << std::endl;
}

void awake(connector::Connector& con) {
    connector::ConnectorUtility util(con);

    std::cout << "Check memory allocs: ";
    con.update();
    int memsz = con.data().memoryFree;
    int memsz0;
    auto mem = util.malloc(100);

    con.update();
    memsz0 = con.data().memoryFree;

    std::cout << ((memsz == memsz0) ? "not allocated" : "allocated") << std::endl;

    while (1) {
        util.setBacklight(false);
        util.setBacklight(true);

        util.printText("xx");
    }
}
