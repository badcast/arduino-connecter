#include "connector.h"

#include <chrono>
#include <thread>

constexpr char port_place_holder[] =
#if COM
    "\\COM%d";
#elif TTY
    "/dev/ttyUSB%d";
#endif

constexpr char determ =
#if COM
    '\\';
#elif TTY
    '/';
#endif

using namespace connector;

Port::Port() : Port(PortID::port0) {}

Port::Port(PortID port) : _id(port) {}

std::string Port::name() {
    std::string _str = fullname();
    return _str.c_str() + _str.find_last_of(determ);
}

std::string Port::fullname() {
    char portname[64];
    sprintf(portname, port_place_holder, (int)_id);
    return portname;
}

const PortID Port::getPortID() { return _id; }

void connector::delay(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

int connector::millis() {
    using namespace std::chrono;
    uint64_t tms = (duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    static uint64_t tcapture = tms;
    return static_cast<int>(tms - tcapture);
}

int Connector::command_write(const void* buffer, int sz) {
    // write attribute for indentification
    int nwrit = write(_fd, buffer, sz);
    tcdrain(_fd);  // sync
    return nwrit;
}

int Connector::command_read(command_responce* commandResponce, int sz) {
    return connector::read_timeout(_fd, commandResponce, sz);
}

bool Connector::send_wait(const command_request* requestCmd, command_responce* responceCmd, uint8_t size_req,
                          uint8_t size_resp) {
    int alpha;

    if (!isConnected()) {
        throw std::runtime_error("Device is not connected");
    }

    if (requestCmd == nullptr) throw std::runtime_error("requestCmd is null");

    if ((requestCmd->commandFlag & 0x1) == SET) {
        if (responceCmd) throw std::runtime_error("request method not requirable responce arg");
    } else if (responceCmd == nullptr)
        throw std::runtime_error("responceCmd is null");

    tcflush(_fd, TCIOFLUSH);  // flushing

    // send content
    alpha = connector::send(_fd, &size_req, 1);

    // send test echo
    if (!connector::echo_test(_fd)) {
        return false;
    }

    delay(10);  // custom delay

    // send request
    alpha = connector::send(_fd, requestCmd, size_req);

    // send test echo
    if (!echo_test(_fd)) {
        return false;
    }

    alpha = command_read(responceCmd, size_resp);
    if (!(alpha == size_resp)) return false;

    return true;
}

template <typename T1, typename T2>
bool Connector::request_get(const T1& request, T2& responce) {
    return send_wait(&request, &responce, sizeof request, sizeof responce);
}

template <typename Request>
bool Connector::request_set(const Request& req) {
    return send_wait(&req, nullptr, sizeof req, 0);
}

void Connector::reset() {
    uint8_t cmdReset = cmd_reset;
    write(_fd, &cmdReset, 1);
    tcdrain(_fd);
}

bool Connector::init() {
    uint8_t attrib;
    if (!connector::read_timeout(_fd, &attrib, 1, 3000)) {
        return false;
    }

    if (attrib != default_attribute) {
        return false;
    }

    return update();
}

Connector::Connector() { _fd = ~0; }

Connector::Connector(const Port& port) : __port(port) {
    if (!connect(__port)) throw std::runtime_error("connection failed");
}

bool Connector::isConnected() { return _fd != ~0; }

Port Connector::getPort() { return __port; }

bool Connector::connect() {
    // auto connect
    bool status;
    // start find from 0
    Port p;
    do {
        status = connect(p);
    } while (!status && (*(int*)(&p._id))++ < PortID::port15);
    return status;
}

bool Connector::connect(Port port) {
    int alpha;
    bool status;
    _fd = open(port.fullname().c_str(), O_RDWR);
    while (status = _fd != -1) {
        tty = {0};

        // error handling
        alpha = tcgetattr(_fd, &tty);
        if (alpha != 0) {
            status = false;
            break;
        }

        // set baud rate
        cfsetspeed(&tty, B9600);

        // Setting other Port Stuff
        tty.c_cflag &= ~PARENB;  // Make 8n1
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_cflag &= ~CRTSCTS;  // no flow control
        tty.c_lflag = 0;          // no signaling chars, no echo, no canonical processing
        tty.c_oflag = 0;          // no remapping, no delays
        tty.c_cc[VMIN] = 0;       // read doesn't block
        tty.c_cc[VTIME] = 5;      // 0.5 seconds read timeout

        tty.c_cflag |= CREAD | CLOCAL;                   // turn on READ & ignore ctrl lines
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);          // turn off s/w flow ctrl
        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  // make raw
        tty.c_oflag &= ~OPOST;                           // make raw

        /* Flush Port, then applies attributes */
        // tcflush(fd, TCIOFLUSH);

        alpha = tcsetattr(_fd, TCSANOW, &tty);
        if (alpha != 0) {
            status = false;
            break;
        }

        if (!init()) {
            status = false;
            break;
        }

        break;
    }

    if (!status) {
        if (_fd != -1) close(_fd);
        _fd = ~0;
    }
    return status;
}

void Connector::disconnect() throw() {
    if (!isConnected()) throw std::runtime_error("is not connected");

    int alpha = close(_fd);

    _fd = ~0;
    if (alpha) throw std::runtime_error(strerror(errno));
}

bool Connector::update() {
    RequestStatus req;
    ResponceStatus resp;

    bool alpha = request_get(req, resp);
    if (alpha) {
        devdata.memorySize = resp.memsize;
        devdata.memoryFree = resp.memfree;
        devdata.lcdRows = resp.lcd_data & 0x0f;
        devdata.lcdCols = resp.lcd_data >> 8;
    }
    return alpha;
}

const DeviceData& Connector::data() {
    if (!isConnected()) throw std::runtime_error("Device not connected");
    return devdata;
}

// declaration

int connector::read_timeout(int fd, void* buffer, const uint16_t& len, int timeout) {
    uint8_t alpha;
    int nreaded = 0;
    timeout = millis() + timeout;
    do {
        // read left
        alpha = read(fd, (reinterpret_cast<int8_t*>(buffer) + nreaded), len - nreaded);
        if (alpha == -1) alpha = 0;
        nreaded += alpha;
    } while (timeout > millis() && nreaded < len);
    return nreaded;
}

uint16_t connector::send(int fd, const void* buffer, const uint16_t& len) {
    uint16_t alpha = write(fd, buffer, len);

    tcdrain(fd);  // sync

    return alpha;
}

inline bool connector::echo_input(int fd) {
    uint8_t echo;
    return read_timeout(fd, &echo, 1, echo_timeout) && echo == cmd_echo;
}

inline bool connector::echo_output(int fd) { return send(fd, &cmd_echo, 1); }

// state is connected from "echo"
bool connector::echo_test(int fd) { return echo_input(fd) && echo_output(fd); }

ConnectorUtility::ConnectorUtility(Connector& device) : _dev(&device) {}

bool ConnectorUtility::isBacklight() {
    RequestBacklight rb;
    ResponceBacklight rs;
    _dev->request_get(rb, rs);
    return rs.mode;
}

void ConnectorUtility::setBacklight(bool state) {
    RequestBacklight rb(state);
    _dev->request_set(rb);
}

void ConnectorUtility::printText(std::string text) {
    // TODO: next function
}

void ConnectorUtility::setCursorView(bool state) {
    // TODO: next function
}

void ConnectorUtility::setCursorPos(int row, int col) {
    // TODO: next function
}

int ConnectorUtility::getRows() { return _dev->data().lcdRows; }

int ConnectorUtility::getCols() { return _dev->data().lcdCols; }

void ConnectorUtility::clear() {
    // TODO: next function
}
