#pragma once
#include <stdint.h>

#define function

#ifdef __unix__ || __MSVC__ || __WINRT__  // linux/unix/FreeDOS code

#include <unistd.h>
int millis();
#define ARG_FD_DEF int fd,
#define ARG_FD_SIN int fd
#define ARG_FD_USE fd,

#define SEND(BUFFER, LENGTH) write(fd, BUFFER, LENGTH)
#define READ(BUFFER, LENGTH) read(fd, BUFFER, LENGTH)

#else  // arduino code
#include <LiquidCrystal_I2C.h>  // подключаем библиотеку для QAPASS 1602
#define ARG_FD_DEF 
#define ARG_FD_SIN
#define ARG_FD_USE 
//#define USE_PRINT

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

typedef uint16_t dptr;

enum CommandMethod : uint8_t { GET, SET };

enum GeneralCommand : uint8_t {
    // Command a work lcd text
    LCD_TEXT = 2,
    LCD_BACKLIGHT_MODE = 4,
    LCD_CURSOR_POS = 6,
    DEVICE_MALLOC = 8
    // next 8, 10, 12, 14
    // TODO: do next command's
};

enum LowLevelCommand : uint8_t { GET_STATUS_INIT = 0xF0 };

struct command_queue {
    uint8_t commandFlag;
};

struct command_request : command_queue {};

struct command_responce {};

struct RequestInitStatus : command_request {
    RequestInitStatus() { commandFlag = LowLevelCommand::GET_STATUS_INIT; }
};

struct ResponceInitStatus : command_responce {
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
    const char* buffer;
    // get
    RequestText() { this->commandFlag = GeneralCommand::LCD_TEXT; }
    // set
    RequestText(const char* buffer, uint8_t length) : RequestText() {
        _length = length;
        this->buffer = buffer;
        this->commandFlag |= SET;
    }
};

struct RequestFreeMemory : command_request {
  dptr pointer;
};

struct RequestMalloc : command_request {
    uint8_t requestSize;
    RequestMalloc(uint8_t size) : requestSize(size) { commandFlag = DEVICE_MALLOC; }
};

struct ResponceMalloc : command_responce {
    dptr pointer;
};

function int read_timeout(ARG_FD_DEF void* buffer, const uint16_t& len, int timeout = default_timeout);
function uint16_t send(ARG_FD_DEF const void* buffer, const uint16_t& len);
function bool echo_input(ARG_FD_SIN);
function bool echo_output(ARG_FD_SIN);
function bool echo_test(ARG_FD_SIN);

inline bool echo_input(ARG_FD_SIN) {
    uint8_t echo;
    return read_timeout(ARG_FD_USE & echo, 1, echo_timeout) && echo == cmd_echo;
}

inline bool echo_output(ARG_FD_SIN) { return send(ARG_FD_USE & cmd_echo, 1); }

// state is connected from "echo"
bool echo_test(ARG_FD_SIN) {
#ifdef ARDUINO
    return echo_output() && echo_input();
#else
    return echo_input(fd) && echo_output(fd);
#endif
}

int read_timeout(ARG_FD_DEF void* buffer, const uint16_t& len, int timeout) {
    uint8_t alpha;
    int nreaded = 0;
    timeout = millis() + timeout;
    do {
        // read left
        alpha = READ((reinterpret_cast<int8_t*>(buffer) + nreaded), len - nreaded);
#ifndef ARDUINO
        if (alpha == -1) alpha = 0;
#endif
        nreaded += alpha;
    } while (timeout > millis() && nreaded < len);
    return nreaded;
}

uint16_t send(ARG_FD_DEF const void* buffer, const uint16_t& len) {
    uint16_t alpha = SEND(buffer, len);
#ifdef ARDUINO
    Serial.flush();  // sync
#else
    tcdrain(fd);  // sync
#endif
    return alpha;
}

#ifdef ARDUINO
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
// *** RequestMalloc *** *** *** *** *** *** *** ***
#define wrapper (reinterpret_cast<ResponceMalloc*>(pokets))
                case GeneralCommand::DEVICE_MALLOC:
                    pokets = ::malloc(sizeof(*wrapper));
                    wrapper->pointer = reinterpret_cast<dptr>(::malloc((reinterpret_cast<RequestMalloc*>(request))->requestSize));
                    break;
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
#undef wrapper
#endif
