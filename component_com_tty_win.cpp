#include "tty.h"
#include <iostream>
#include <assert.h>
#include <windows.h>
    using namespace std;
static int TIMEOUT = 1000;
TTY::TTY() { m_Handle = INVALID_HANDLE_VALUE; }
TTY::~TTY() { Disconnect(); }
bool TTY::IsOK() const { return m_Handle != INVALID_HANDLE_VALUE; }
void TTY::Connect(const string& port, int baudrate)
{
    Disconnect();
    m_Handle = CreateFile(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_Handle == INVALID_HANDLE_VALUE) {
        throw TTYException();
    }
    SetCommMask(m_Handle, EV_RXCHAR);
    SetupComm(m_Handle, 1500, 1500);
    COMMTIMEOUTS CommTimeOuts;
    CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
    CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
    CommTimeOuts.ReadTotalTimeoutConstant = TIMEOUT;
    CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
    CommTimeOuts.WriteTotalTimeoutConstant = TIMEOUT;
    if (!SetCommTimeouts(m_Handle, &CommTimeOuts)) {
        CloseHandle(m_Handle);
        m_Handle = INVALID_HANDLE_VALUE;
        throw TTYException();
    }
    DCB ComDCM;
    memset(&ComDCM, 0, sizeof(ComDCM));
    ComDCM.DCBlength = sizeof(DCB);
    GetCommState(m_Handle, &ComDCM);
    ComDCM.BaudRate = DWORD(baudrate);
    ComDCM.ByteSize = 8;
    ComDCM.Parity = NOPARITY;
    ComDCM.StopBits = ONESTOPBIT;
    ComDCM.fAbortOnError = TRUE;
    ComDCM.fDtrControl = DTR_CONTROL_DISABLE;
    ComDCM.fRtsControl = RTS_CONTROL_DISABLE;
    ComDCM.fBinary = TRUE;
    ComDCM.fParity = FALSE;
    ComDCM.fInX = FALSE;
    ComDCM.fOutX = FALSE;
    ComDCM.XonChar = 0;
    ComDCM.XoffChar = (unsigned char)0xFF;
    ComDCM.fErrorChar = FALSE;
    ComDCM.fNull = FALSE;
    ComDCM.fOutxCtsFlow = FALSE;
    ComDCM.fOutxDsrFlow = FALSE;
    ComDCM.XonLim = 128;
    ComDCM.XoffLim = 128;
    if (!SetCommState(m_Handle, &ComDCM)) {
        CloseHandle(m_Handle);
        m_Handle = INVALID_HANDLE_VALUE;
        throw TTYException();
    }
}
void TTY::Disconnect()
{
    if (m_Handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_Handle);
        m_Handle = INVALID_HANDLE_VALUE;
    }
}
void TTY::Write(const vector<unsigned char>& data)
{
    if (m_Handle == INVALID_HANDLE_VALUE) {
        throw TTYException();
    }
    DWORD feedback;
    if (!WriteFile(m_Handle, &data[0], (DWORD)data.size(), &feedback, 0) || feedback != (DWORD)data.size()) {
        CloseHandle(m_Handle);
        m_Handle = INVALID_HANDLE_VALUE;
        throw TTYException();
    }
    // In some cases it's worth uncommenting
    // FlushFileBuffers(m_Handle);
}
void TTY::Read(vector<unsigned char>& data)
{
    if (m_Handle == INVALID_HANDLE_VALUE) {
        throw TTYException();
    }
    DWORD begin = GetTickCount();
    DWORD feedback = 0;
    unsigned char* buf = &data[0];
    DWORD len = (DWORD)data.size();
    int attempts = 3;
    while (len && (attempts || (GetTickCount() - begin) < (DWORD)TIMEOUT / 3)) {
        if (attempts)
            attempts--;
        if (!ReadFile(m_Handle, buf, len, &feedback, NULL)) {
            CloseHandle(m_Handle);
            m_Handle = INVALID_HANDLE_VALUE;
            throw TTYException();
        }
    }
    assert(feedback <= len);
    len -= feedback;
    buf += feedback;
    if (len) {
        CloseHandle(m_Handle);
        m_Handle = INVALID_HANDLE_VALUE;
        throw TTYException();
    }
}
using namespace std;
int main(int argc, char* argv[])
{
    TTY tty;
    tty.Connect("COM4", 9600);
    for (int i = 0; i < 1000; i++) {
        std::vector<unsigned char> the_vectsor;
        the_vectsor.push_back(5);
        tty.Read(the_vectsor);
    }
    std::cout << (char)(the_vectsor[0]); // output text
}
system("PAUSE");
re
