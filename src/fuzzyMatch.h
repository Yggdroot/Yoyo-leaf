_Pragma("once");

#include <cstdint>
#include <memory>
#include "config.h"

namespace leaf
{

#define MIN_WEIGHT (-2147483648)

struct PatternContext
{
    const uint8_t* pattern;
    int64_t pattern_mask[256];
    uint16_t pattern_len;
    uint16_t actual_pattern_len;
    bool is_lower;
};

struct HighlightPos
{
    uint16_t col;
    uint16_t len;
};

struct HighlightContext
{
    int32_t  score;
    uint16_t beg;
    uint16_t end;
    HighlightPos positions[64];
    uint16_t end_index;
};


struct Destroyer
{
    template <typename T>
    void operator()(T* p) {
        free(p);
    }
};

template <typename T>
using Unique_ptr = std::unique_ptr<T, Destroyer>;

class FuzzyMatch
{
public:

    PatternContext* initPattern(const char* pattern,
                                uint16_t pattern_len);

    int32_t getWeight(const char* text,
                      uint16_t text_len,
                      PatternContext* p_pattern_ctxt,
                      Preference preference);

    Unique_ptr<HighlightContext> getHighlights(const char* text,
                                               uint16_t text_len,
                                               PatternContext* p_pattern_ctxt);

    uint32_t getPathWeight(const char* filename,
                           const char* suffix,
                           const char* dirname,
                           const char* path, uint32_t path_len);

};

} // end namespace leaf
