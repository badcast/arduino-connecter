#include <errno.h>
#include <string.h>

#include <iostream>

#include "connector-pipe.h"

constexpr int CMDN = 3;
const char *commands_details[CMDN][2] = {
    {"--help", "Show this help"}, {"--version", "Show version"}, {"-p", "Print text to connected device"}};
const char errcmd[] = "Invalid command \"%s\"\n";

void showhelp() {
    for (int x = 0; x < CMDN; ++x) {
        std::cout << commands_details[x][0] << "\t" << commands_details[x][1] << std::endl;
    }
    exit(EXIT_SUCCESS);
}

void showVersion() {
    std::cout << "Arduino connector version: 1.0.0" << std::endl;

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    std::string argVal;
    // show help
    if (argc == 1 || !strncmp(argv[1], commands_details[0][0], 3)) {
        showhelp();
    }

    // show version
    if (!strncmp(argv[1], commands_details[1][0], 3)) {
        showVersion();
    }

    for (int x = 1; argVal.empty() && x < argc; ++x) {
        for (int y = 2; y < CMDN; ++y) {
            if (!strcmp(argv[x], commands_details[y][0])) {
                argVal = argv[x + 1];
                break;
            }
        }
    }

    if (argVal.empty()) {
        printf(errcmd, argv[1]);
        showhelp();
    }

    if (!connect_pipe()) {
        return EXIT_FAILURE;
    }

    send_message(argVal.data(), argVal.size());

    return EXIT_SUCCESS;
}
