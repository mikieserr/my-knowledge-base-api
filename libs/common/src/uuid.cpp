#include "kb/common/uuid.hpp"

#include <openssl/rand.h>

#include <cstdio>
#include <stdexcept>

namespace kb::common {

    std::string generate_uuid() {
        unsigned char buf[16];

        if (RAND_bytes(buf, sizeof(buf)) != 1) {
            throw std::runtime_error("RAND_bytes failed");
        }

        // устанавливаем биты версии и варианта
        buf[6] = (buf[6] & 0x0F) | 0x40;   // version = 4
        buf[8] = (buf[8] & 0x3F) | 0x80;   // variant = RFC 4122

        char out[37];
        std::snprintf(out, sizeof(out),
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            buf[0],  buf[1],  buf[2],  buf[3],
            buf[4],  buf[5],
            buf[6],  buf[7],
            buf[8],  buf[9],
            buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

        return std::string(out, 36);
    }

}