#include "utils.h"

namespace utils
{

    bool is_utf8_boundary(uint8_t c) {
        return c < 128 || (c & (1 << 6));
    }
}
