#pragma once

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

enum pipe_signals : int8_t { SIGNAL_FREE, SIGNAL_WRITED };

const char command_program_pipe[] = "connector_id_ppipe_badcast";
const size_t shm_default_size = 1024;
const size_t offset_size = sizeof(size_t) + 1;
size_t allocatedSize = 0;
int shm;
void* pshare;

inline bool status() {
    bool status;
    if (!(status = shm > -1)) {
        if (errno == 2) {
            std::cout << "daemon not started. Check systemctl status connectord" << std::endl;
        } else
            perror("shm");
    }

    return status;
}

inline bool sharing(size_t size = shm_default_size) {
    size = size + offset_size;
    pshare = mmap(nullptr, size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);

    if (pshare == MAP_FAILED)
        perror("mmap");
    else
        allocatedSize = size;

    return pshare != MAP_FAILED;
}

inline bool create_pipe() {
    shm = shm_open(command_program_pipe, O_RDWR | O_CREAT, 0777);
    bool _status = status() && sharing();

    if (_status) {
        ftruncate(shm, allocatedSize);
        memset(pshare, 0, allocatedSize);
    }

    return _status;
}

inline bool connect_pipe() {
    shm = shm_open(command_program_pipe, O_RDWR, 0777);
    return status() && sharing();
}

inline size_t peekLength() { return *(size_t*)((pshare) + 1); }

inline size_t read_message(char* buff, size_t len) {
    static size_t offsetBuffer = 0;
    if (shm == -1 || (*(int8_t*)pshare) == SIGNAL_FREE) return 0;
    size_t* addrLen = (size_t*)(pshare + 1);

    len = (len < (*addrLen) - offsetBuffer ? len : ((*addrLen) - offsetBuffer));
    //offsetBuffer += len;

    // copy to buffer
    memcpy(buff, pshare + sizeof(size_t) + 1, len);

    memset(pshare, SIGNAL_FREE, 1);

    return len;
}

inline void send_message(const char* buff, size_t len) {
    size_t* addrLen = (size_t*)(pshare + 1);

    if (shm == -1) return;

    mempcpy(pshare + offset_size, buff, len);
    *addrLen = len;

    memset(pshare, SIGNAL_WRITED, 1);
}

inline void close_pipe() {
    if (!shm) return;

    shm_unlink(command_program_pipe);
    close(shm);
    shm = -1;
}
