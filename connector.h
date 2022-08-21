#pragma once

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <stdexcept>
#include <string>

#ifdef __MSCVER__ || __WINRT__
#define COM 1
#else  // other, LINUX, UNIX, MACOS
#define TTY 1
#endif

namespace connector {
enum PortID {
    port0,
    port1,
    port2,
    port3,
    port4,
    port5,
    port6,
    port7,
    port8,
    port9,
    port10,
    port11,
    port12,
    port13,
    port14,
    port15
};

class Connector;

// device memory pointer
struct dptr;

class ConnectorUtility;

class Port {
    PortID _id;

   public:
    Port();
    Port(PortID port);

    std::string name();
    std::string fullname();
    const PortID getPortID();
    friend class Connector;
};

struct DeviceData {
    int memorySize;
    int memoryFree;
    int lcdRows;
    int lcdCols;
};

class Connector {
    int _fd;
    Port __port;
    termios tty;
    DeviceData devdata;

   private:
    bool init();
    void reset();

   public:
    Connector();
    Connector(const Port& port);

    inline const int handle() const;

    // check verify connection status
    bool isConnected() const;
    // give connected port
    Port port();
    // find free port, and connect - auto
    bool connect();
    // connect to port
    bool connect(Port port);
    // disconnect port
    void disconnect() throw();

    // immediatily update init information
    bool update();

    // this function for set information to device
    template <typename Request>
    bool set(const Request& req);

    // this function for get information from device
    template <typename T1, typename T2>
    bool get(const T1& request, T2& responce);

    const DeviceData& data() const;

    // check connected state
    operator bool() const;

    friend class ConnectorUtility;
};

class ConnectorUtility {
    Connector* _dev;

   public:
    ConnectorUtility(Connector& device);

    const bool connected() const;

    // get state led
    bool isBacklight();
    // set state led's
    void setBacklight(bool state);
    // show text to LCD
    void print(const std::string& text);

    void setCursorView(bool state);

    void setCursorPos(int row, int col);

    // get row's from lcd display
    int getRows();

    // get cell's from lcd display
    int getCols();

    // clear text from LCD
    void clear();

    // memory allocate from device
    dptr* malloc(uint8_t size);

    // memory free from device
    void mfree(dptr* memory);

    dptr* memset(dptr* dest, int value, int size);
    dptr* memcpy(dptr* dest, const dptr* source, int size);
};
}  // namespace connector
