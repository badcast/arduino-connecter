#pragma once
#include <stdint.h>

#define function

#ifdef __unix__ || __MSVC__ || __WINRT__  // linux/unix/FreeDOS code
#include <unistd.h>
namespace connector {
int millis();
#else  // arduino code
#define _USE_PRINT
#define SEND(BUFFER, LENGTH) Serial.write(reinterpret_cast<const char*>(BUFFER), LENGTH)
#define READ(BUFFER, LENGTH) Serial.readBytes(reinterpret_cast<char*>(BUFFER), LENGTH)
extern LiquidCrystal_I2C lcd(0x27, 16, 2);  // присваиваем имя LCD для дисплея
#endif

const int default_timeout = 1000;
const int echo_timeout = 5000;
const int max_text_length = 32;
const uint8_t default_attribute = 0xAA;
const uint8_t cmd_reset = 0xff;  // 255
const uint8_t cmd_quit = 0xfe;   // 254
const uint8_t cmd_echo = 0xfd;   // echo command for IO

enum CommandMethod : uint8_t { GET, SET };

enum GeneralCommand : uint8_t {
    // Command a work lcd text
    LCD_TEXT = 2,
    LCD_BACKLIGHT_MODE = 4,
    LCD_CURSOR_POS = 6
    // next 8, 10, 12, 14
    // TODO: do next command's
};

enum LowLevelCommand : uint8_t { GET_STATUS_INIT = 0xF0 };

struct command_queue {
    uint8_t commandFlag;
};

struct command_request : command_queue {};

struct command_responce {};

struct RequestStatus : command_request {
    RequestStatus() { commandFlag = LowLevelCommand::GET_STATUS_INIT; }
};

struct ResponceStatus : command_responce {
    uint16_t memsize;
    uint16_t memfree;
    uint16_t lcd_data;
};

struct RequestBacklight : command_request {
    uint8_t mode;
    // get
    RequestBacklight() { commandFlag = GeneralCommand::LCD_BACKLIGHT_MODE; }
    // set
    RequestBacklight(bool mode) : RequestBacklight() {
        this->mode = mode;
        commandFlag |= SET;
    }
};

struct ResponceBacklight : command_responce {
    bool mode;
};

struct RequestText : command_request {
    uint8_t _length;
    char* buffer;
    // get
    RequestText() {
        this->length = 0;
        this->commandFlag = GeneralCommand::LCD_TEXT;
    }
    // set
    RequestText(char* buffer, uint8_t length) : RequestText(), _length(length) {
        this->buffer = buffer;
        this->commandFlag |= SET;
    }
};

#ifdef ARDUINO
function int read_timeout(void* buffer, const uint16_t& len, int timeout = default_timeout);
function uint16_t send(const void* buffer, const uint16_t& len);
function bool echo_input();
function bool echo_output();
function bool echo_test();
#else
function int read_timeout(int fd, void* buffer, const uint16_t& len, int timeout = default_timeout);
function uint16_t send(int fd, const void* buffer, const uint16_t& len);
function bool echo_input(int fd);
function bool echo_output(int fd);
function bool echo_test(int fd);
#endif

#ifdef !ARDUINO

int read_timeout(void* buffer, const uint16_t& len, int timeout = default_timeout) {
    uint8_t alpha;
    int nreaded = 0;
    timeout = millis() + timeout;
    do {
        // read left
        alpha = READ((reinterpret_cast<int8_t*>(buffer) + nreaded), len - nreaded);
        nreaded += alpha;
    } while (timeout > millis() && nreaded < len);
    return nreaded;
}

uint16_t send(const void* buffer, const uint16_t& len) {
    uint16_t alpha = SEND(buffer, len);
    Serial.flush();  // sync
    return alpha;
}

inline bool echo_input() {
    uint8_t echo;
    return read_timeout(&echo, 1, echo_timeout) && echo == cmd_echo;
}

inline bool echo_output() { return send(&cmd_echo, 1); }

// state is connected from "echo"

bool echo_test() {
    // Arduino first send and reply
    return echo_output() && echo_input();
}

extern int __heap_start, *__brkval;
inline uint16_t heapstart() { return (uint16_t)(__brkval == 0 ? (uint16_t)&__heap_start : (uint16_t)__brkval); }
uint16_t freeRam() {
    uint16_t stack;
    stack = (uint16_t)&stack - heapstart();
    return stack;
}
inline uint16_t sizeRam() { return (uint16_t)heapstart() + freeRam(); }

void arduino_reply(command_request* request) {
    void* pokets = nullptr;
    uint16_t size;
    switch (request->commandFlag) {
        case LowLevelCommand::GET_STATUS_INIT: {
            // send complete
            size = freeRam();
            pokets = malloc(sizeof(ResponceStatus));
#define wrapper (reinterpret_cast<ResponceStatus*>(pokets))
            wrapper->memsize = sizeRam();
            wrapper->memfree = size;  // TODO: how to get used memory ?
            wrapper->lcd_data = lcd.getRows() | lcd.getCols() << 8;

            size = sizeof(*wrapper);
#ifdef USE_PRINT
            lcd.print("status_init");
#endif
            break;
        }
        default:
            // get command method, pop or push
            CommandMethod cmdMethod = request->commandFlag & 1;
            request->commandFlag &= ~1;
#ifdef USE_PRINT
            lcd.clear();
#endif
            switch (static_cast<GeneralCommand>(request->commandFlag)) {
// *** RequestBacklight *** *** *** *** *** *** *** ***
#define wrapper (reinterpret_cast<ResponceBacklight*>(pokets))
                case GeneralCommand::LCD_BACKLIGHT_MODE:

                    switch (cmdMethod) {
                        case GET:
                            pokets = malloc(size = sizeof(ResponceBacklight));
                            (*((bool*)pokets)) = lcd.isBacklight();
                            break;
                        case SET:
                            if (reinterpret_cast<RequestBacklight*>(request)->mode)
                                lcd.backlight();
                            else
                                lcd.noBacklight();
                            break;
                    }
                    break;

                    // *** RequestText *** *** *** *** *** *** *** ***
#define wrapper (reinterpret_cast<ResponceBacklight*>(pokets))
                case GeneralCommand::LCD_TEXT:
                    switch (cmdMethod) {
                        case GET:

                            break;
                        case SET:
                            break;
                    }

                    break;
            }
    }

    // send data
    if (pokets != nullptr) {
        send(pokets, size);
        free(pokets);  // free
    }
}
#endif

#undef wrapper

#ifndef ARDUINO
}  // namespace connector
#endif
