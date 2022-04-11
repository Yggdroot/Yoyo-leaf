#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fuzzyMatch.h"

namespace leaf
{

#if defined(_MSC_VER) && \
    (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))

    #define FM_BITSCAN_WINDOWS

    #include <intrin.h>
    #pragma intrinsic(_BitScanReverse)

    #if defined(_M_AMD64) || defined(_M_X64)
        #define FM_BITSCAN_WINDOWS64
        #pragma intrinsic(_BitScanReverse64)
    #endif

#endif

#if defined(FM_BITSCAN_WINDOWS)

    uint32_t FM_BitLength(uint64_t x)
    {
        unsigned long index;
        #if defined(FM_BITSCAN_WINDOWS64)
        if ( !_BitScanReverse64(&index, x) )
            return 0;
        else
            return index + 1;
        #else
        if ( (x & 0xFFFFFFFF00000000) == 0 ) {
            if ( !_BitScanReverse(&index, (unsigned long)x) )
                return 0;
            else
                return index + 1;
        }
        else {
            _BitScanReverse(&index, (unsigned long)(x >> 32));
            return index + 33;
        }
        #endif
    }

    #define FM_BIT_LENGTH(x) FM_BitLength(x)

#elif defined(__GNUC__)

    #define FM_BIT_LENGTH(x) ((uint32_t)((sizeof(unsigned long long) << 3) - __builtin_clzll(x)))

#elif defined(__clang__)

    #if __has_builtin(__builtin_clzll)
        #define FM_BIT_LENGTH(x) ((uint32_t)((sizeof(unsigned long long) << 3) - __builtin_clzll(x)))
    #endif

#endif

#if !defined(FM_BIT_LENGTH)

    static uint8_t clz_table_8bit[256] =
    {
        8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    uint32_t FM_BIT_LENGTH(uint64_t x)
    {
        uint32_t n = 0;
        if ((x & 0xFFFFFFFF00000000) == 0) {n  = 32; x <<= 32;}
        if ((x & 0xFFFF000000000000) == 0) {n += 16; x <<= 16;}
        if ((x & 0xFF00000000000000) == 0) {n +=  8; x <<=  8;}
        n += (uint32_t)clz_table_8bit[x >> 56];

        return 64 - n;
    }

#endif

static uint64_t deBruijn = 0x022FDD63CC95386D;

static uint8_t MultiplyDeBruijnBitPosition[64] =
{
    0,  1,  2,  53, 3,  7,  54, 27,
    4,  38, 41, 8,  34, 55, 48, 28,
    62, 5,  39, 46, 44, 42, 22, 9,
    24, 35, 59, 56, 49, 18, 29, 11,
    63, 52, 6,  26, 37, 40, 33, 47,
    61, 45, 43, 21, 23, 58, 17, 10,
    51, 25, 36, 32, 60, 20, 57, 16,
    50, 31, 19, 15, 30, 14, 13, 12,
};

#define FM_CTZ(x) MultiplyDeBruijnBitPosition[((uint64_t)((x) & -(int64_t)(x)) * deBruijn) >> 58]

static int32_t valTable[64] =
{
    0,       10000,   40000,   70000,   130000,  190000,  250000,  310000,
    370000,  430000,  490000,  550000,  610000,  670000,  730000,  790000,
    850000,  910000,  970000,  1030000, 1090000, 1150000, 1210000, 1270000,
    1330000, 1390000, 1450000, 1510000, 1570000, 1630000, 1690000, 1750000,
    1810000, 1870000, 1930000, 1990000, 2050000, 2110000, 2170000, 2230000,
    2290000, 2350000, 2410000, 2470000, 2530000, 2590000, 2650000, 2710000,
    2770000, 2830000, 2890000, 2950000, 3010000, 3070000, 3130000, 3190000,
    3250000, 3310000, 3370000, 3430000, 3490000, 3550000, 3610000, 3670000
};

struct TextContext
{
    const uint8_t* text;
    uint64_t* text_mask;
    uint16_t text_len;
    uint16_t col_num;
    uint16_t offset;
};

struct ValueElements
{
    int32_t  score;
    uint16_t beg;
    uint16_t end;
};


PatternContext* FuzzyMatch::initPattern(const char* pattern, uint16_t pattern_len)
{
    PatternContext* p_pattern_ctxt = new PatternContext;
    if ( !p_pattern_ctxt ) {
        fprintf(stderr, "Out of memory in initPattern()!\n");
        return nullptr;
    }
    p_pattern_ctxt->actual_pattern_len = pattern_len;
    if ( pattern_len >= 64 ) {
        pattern_len = 63;
    }
    p_pattern_ctxt->pattern = reinterpret_cast<const uint8_t*>(pattern);
    p_pattern_ctxt->pattern_len = pattern_len;
    memset(p_pattern_ctxt->pattern_mask, -1, sizeof(p_pattern_ctxt->pattern_mask));

    for (uint16_t i = 0; i < pattern_len; ++i ) {
        p_pattern_ctxt->pattern_mask[(uint8_t)pattern[i]] ^= (1LL << i);
        if ( islower(pattern[i]) && p_pattern_ctxt->pattern_mask[(uint8_t)toupper(pattern[i])] != -1 ) {
            p_pattern_ctxt->pattern_mask[(uint8_t)toupper(pattern[i])] ^= (1LL << i);
        }
    }
    p_pattern_ctxt->is_lower = true;

    for (uint16_t i = 0; i < pattern_len; ++i ) {
        if ( isupper(pattern[i]) ) {
            p_pattern_ctxt->is_lower = false;
            break;
        }
    }

    return p_pattern_ctxt;
}

static ValueElements* evaluate(TextContext* p_text_ctxt,
                               PatternContext* p_pattern_ctxt,
                               uint16_t k,
                               ValueElements val[])
{
    uint64_t* text_mask = p_text_ctxt->text_mask;
    uint16_t col_num = p_text_ctxt->col_num;
    uint16_t j = p_text_ctxt->offset;

    const uint8_t* pattern = p_pattern_ctxt->pattern;
    uint16_t base_offset = pattern[k] * col_num;
    uint64_t x = text_mask[base_offset + (j >> 6)] >> (j & 63);
    uint16_t i = 0;

    if ( x == 0 ) {
        uint64_t bits = 0;
        uint16_t col = 0;
        for ( col = (j >> 6) + 1; col < col_num; ++col ) {
            if ( (bits = text_mask[base_offset + col]) != 0 )
                break;
        }
        if ( bits == 0 ) {
            memset(val, 0, sizeof(ValueElements));
            return val;
        }
        else {
            i = (col << 6) + FM_CTZ(bits);
        }
    }
    else {
        i = j + FM_CTZ(x);
    }

    /**
     * e.g., text = '~abc~~AbcD~~', pattern = 'abcd'
     * j > 0 means val[k].beg > 0, means k in val
     */
    if ( j > 0 && val[k].beg >= j )
        return val + k;

    uint16_t beg = 0;
    uint16_t end = 0;

    int32_t max_prefix_score = 0;
    int32_t max_score = MIN_WEIGHT;

    const uint8_t* text = p_text_ctxt->text;
    uint16_t text_len = p_text_ctxt->text_len;
    uint16_t pattern_len = p_pattern_ctxt->pattern_len - k;
    int64_t* pattern_mask = p_pattern_ctxt->pattern_mask;

    int32_t special = 0;
    if ( i == 0 )
        special = 50000;
#if defined(_MSC_VER)
    else if ( text[i-1] == '\\' || text[i-1] == '/' )
#else
    else if ( text[i-1] == '/' )
#endif
        special = k == 0 ? 50000 : 30000;
    else if ( isupper(text[i]) )
        special = !isupper(text[i-1]) || (i+1 < text_len && islower(text[i+1])) ? 30000 : 0;
    /* else if ( text[i-1] == '_' || text[i-1] == '-' || text[i-1] == ' ' ) */
    /*     special = 30000;                                                 */
    /* else if ( text[i-1] == '.' )                                         */
    /*     special = 30000;                                                 */
    else if ( !isalnum(text[i-1]) )
        special = 30000;
    else
        special = 0;
    ++i;
    int64_t d = -2;     /* ~1 */
    int64_t last = d;
    while ( i < text_len )
    {
        last = d;
        uint8_t c = text[i];
        /* c in pattern */
        if ( pattern_mask[c] != -1 )
            d = (d << 1) | (pattern_mask[c] >> k);
        /**
         * text = 'xxABC', pattern = 'abc'; text[i] == 'B'
         * text = 'xxABC', pattern = 'abc'; text[i] == 'C'
         * NOT text = 'xxABCd', pattern = 'abc'; text[i] == 'C'
         * 'Cd' is considered as a word
         */
        /* else if ( isupper(text[i-1]) && pattern_mask[(uint8_t)tolower(c)] != -1 */
        /*           && (i+1 == text_len || !islower(text[i+1])) )                 */
        else if ( pattern_mask[(uint8_t)tolower(c)] != -1 )
            d = (d << 1) | (pattern_mask[(uint8_t)tolower(c)] >> k);
        else
            d = ~0;

        if ( d >= last ) {
            int32_t score = MIN_WEIGHT;
            uint16_t end_pos = 0;
            uint16_t n = FM_BIT_LENGTH(~last);
            /* e.g., text = '~~abcd~~~~', pattern = 'abcd' */
            if ( n == pattern_len ) {
                score = special > 0 ? (n > 1 ? valTable[n+1] : valTable[n]) + special : valTable[n];
                if ( (k == 0 && special == 50000) || (k > 0 && special > 0) ) {
                    val[k].score = score;
                    val[k].beg = i - n;
                    val[k].end = i;
                    return val + k;
                }
                else
                    end_pos = i;
            }
            else {
                int32_t prefix_score = special > 0 ? (n > 1 ? valTable[n+1] : valTable[n]) + special : valTable[n];
                /**
                 * e.g., text = 'AbcxxAbcyyde', pattern = 'abcde'
                 * prefer matching 'Abcyyde'
                 */
                if ( prefix_score > max_prefix_score
                     || (special > 0 && prefix_score == max_prefix_score) )
                {
                    max_prefix_score = prefix_score;
                    p_text_ctxt->offset = i;
                    ValueElements* p_val = evaluate(p_text_ctxt, p_pattern_ctxt, k + n, val);
                    if ( p_val->end ) {
                        score = prefix_score + p_val->score - 3000 * (p_val->beg - i);
                        end_pos = p_val->end;
                    }
                    else {
                        break;
                    }
                }
            }

            if ( score > max_score ) {
                max_score = score;
                beg = i - n;
                end = end_pos;
            }
            /* e.g., text = '~_ababc~~~~', pattern = 'abc' */
            special = 0;
        }

        /*
         * e.g., text = 'a~c~~~~ab~c', pattern = 'abc',
         * to find the index of the second 'a'
         * `d == last` is for the case when text = 'kpi_oos1', pattern = 'kos'
         */
        if ( d == ~0 || d == last ) {
            x = text_mask[base_offset + (i >> 6)] >> (i & 63);

            if ( x == 0 ) {
                uint64_t bits = 0;
                uint16_t col = 0;
                for ( col = (i >> 6) + 1; col < col_num; ++col ) {
                    if ( (bits = text_mask[base_offset + col]) != 0 )
                        break;
                }
                if ( bits == 0 )
                    break;
                else
                    i = (col << 6) + FM_CTZ(bits);
            }
            else {
                i += FM_CTZ(x);
            }

#if defined(_MSC_VER)
            if ( text[i-1] == '\\' || text[i-1] == '/' )
#else
            if ( text[i-1] == '/' )
#endif
                special = k == 0 ? 50000 : 30000;
            else if ( isupper(text[i]) )
                special = !isupper(text[i-1]) || (i+1 < text_len && islower(text[i+1])) ? 30000 : 0;
            /* else if ( text[i-1] == '_' || text[i-1] == '-' || text[i-1] == ' ' ) */
            /*     special = 30000;                                                 */
            /* else if ( text[i-1] == '.' )                                         */
            /*     special = 30000;                                                 */
            else if ( !isalnum(text[i-1]) )
                special = 30000;
            else
                special = 0;
            d = -2;
            ++i;
        }
        else
            ++i;
    }

    /* e.g., text = '~~~~abcd', pattern = 'abcd' */
    if ( i == text_len ) {
        if ( ~d >> (pattern_len - 1) ) {
            int32_t score = special > 0 ? (pattern_len > 1 ? valTable[pattern_len + 1] : valTable[pattern_len]) + special
                            : valTable[pattern_len];
            if ( score > max_score ) {
                max_score = score;
                beg = i - pattern_len;
                end = i;
            }
        }
    }

    val[k].score = max_score;
    val[k].beg = beg;
    val[k].end = end;

    return val + k;
}

int32_t FuzzyMatch::getWeight(const char* p_text,
                              uint16_t text_len,
                              PatternContext* p_pattern_ctxt,
                              Preference preference)
{
    if ( !p_text || !p_pattern_ctxt )
        return MIN_WEIGHT;

    uint16_t j = 0;
    uint16_t col_num = 0;
    uint64_t* text_mask = nullptr;
    const uint8_t* pattern = p_pattern_ctxt->pattern;
    const uint8_t* text = reinterpret_cast<const uint8_t*>(p_text);
    uint16_t pattern_len = p_pattern_ctxt->pattern_len;
    int64_t* pattern_mask = p_pattern_ctxt->pattern_mask;
    uint8_t first_char = pattern[0];
    uint8_t last_char = pattern[pattern_len - 1];

    /* maximum number of int16_t is (1 << 15) - 1 */
    if ( text_len >= (1 << 15) ) {
        text_len = (1 << 15) - 1;
    }

    if ( pattern_len == 1 ) {
        if ( isupper(first_char) ) {
            int16_t first_char_pos = -1;
            for ( uint16_t i = 0; i < text_len; ++i ) {
                if ( text[i] == first_char ) {
                    first_char_pos = i;
                    break;
                }
            }
            if ( first_char_pos == -1 )
                return MIN_WEIGHT;
            else
                return 10000/(first_char_pos + 1) + 10000/text_len;
        }
        else {
            int16_t first_char_pos = -1;
            for ( uint16_t i = 0; i < text_len; ++i ) {
                if ( tolower(text[i]) == first_char ) {
                    if ( first_char_pos == -1 )
                        first_char_pos = i;

                    if ( isupper(text[i]) || i == 0 || !isalnum(text[i-1]) )
                        return 2 + 10000/(i + 1) + 10000/text_len;
                }
            }
            if ( first_char_pos == -1 )
                return MIN_WEIGHT;
            else
                return 10000/(first_char_pos + 1) + 10000/text_len;
        }
    }

    int16_t first_char_pos = -1;
    if ( p_pattern_ctxt->is_lower ) {
        for ( uint16_t i = 0; i < text_len; ++i ) {
            if ( tolower(text[i]) == first_char ) {
                first_char_pos = i;
                break;
            }
        }
        if ( first_char_pos == -1 )
            return MIN_WEIGHT;

        int16_t last_char_pos = -1;
        for ( int16_t i = text_len - 1; i >= first_char_pos; --i ) {
            if ( tolower(text[i]) == last_char ) {
                last_char_pos = i;
                break;
            }
        }
        if ( last_char_pos == -1 )
            return MIN_WEIGHT;

        col_num = (text_len + 63) >> 6;     /* (text_len + 63)/64 */
        /* uint64_t text_mask[256][col_num] */
        text_mask = static_cast<uint64_t*>(calloc(col_num << 8, sizeof(uint64_t)));
        if ( !text_mask ) {
            fprintf(stderr, "Out of memory in getWeight()!\n");
            return MIN_WEIGHT;
        }
        for ( int16_t i = first_char_pos; i <= last_char_pos; ++i ) {
            uint8_t c = tolower(text[i]);
            /* c in pattern */
            if ( pattern_mask[c] != -1 ) {
                text_mask[c * col_num + (i >> 6)] |= 1ULL << (i & 63);
                if ( j < pattern_len && c == pattern[j] )
                    ++j;
            }
        }
    }
    else {
        if ( isupper(first_char) ) {
            for ( int16_t i = 0; i < text_len; ++i ) {
                if ( text[i] == first_char ) {
                    first_char_pos = i;
                    break;
                }
            }
        }
        else {
            for ( int16_t  i = 0; i < text_len; ++i ) {
                if ( tolower(text[i]) == first_char ) {
                    first_char_pos = i;
                    break;
                }
            }
        }
        if ( first_char_pos == -1 )
            return MIN_WEIGHT;

        int16_t last_char_pos = -1;
        if ( isupper(last_char) ) {
            for ( int16_t i = text_len - 1; i >= first_char_pos; --i ) {
                if ( text[i] == last_char ) {
                    last_char_pos = i;
                    break;
                }
            }
        }
        else {
            for ( int16_t i = text_len - 1; i >= first_char_pos; --i ) {
                if ( tolower(text[i]) == last_char ) {
                    last_char_pos = i;
                    break;
                }
            }
        }
        if ( last_char_pos == -1 )
            return MIN_WEIGHT;

        col_num = (text_len + 63) >> 6;
        /* uint64_t text_mask[256][col_num] */
        text_mask = static_cast<uint64_t*>(calloc(col_num << 8, sizeof(uint64_t)));
        if ( !text_mask ) {
            fprintf(stderr, "Out of memory in getWeight()!\n");
            return MIN_WEIGHT;
        }
        for ( int16_t i = first_char_pos; i <= last_char_pos; ++i ) {
            uint8_t c = text[i];
            if ( isupper(c) ) {
                /* c in pattern */
                if ( pattern_mask[c] != -1 )
                    text_mask[c * col_num + (i >> 6)] |= 1ULL << (i & 63);
                if ( pattern_mask[(uint8_t)tolower(c)] != -1 )
                    text_mask[(uint8_t)tolower(c) * col_num + (i >> 6)] |= 1ULL << (i & 63);
                if ( j < pattern_len && c == toupper(pattern[j]) )
                    ++j;
            }
            else {
                /* c in pattern */
                if ( pattern_mask[(uint8_t)c] != -1 ) {
                    text_mask[(uint8_t)c * col_num + (i >> 6)] |= 1ULL << (i & 63);
                    if ( j < pattern_len && c == pattern[j] )
                        ++j;
                }
            }
        }
    }

    if ( j < pattern_len ) {
        free(text_mask);
        return MIN_WEIGHT;
    }

    if ( p_pattern_ctxt->actual_pattern_len >= 64 ) {
        j = 0;
        for ( int16_t i = first_char_pos; i < text_len; ++i ) {
            if ( j < p_pattern_ctxt->actual_pattern_len ) {
                if ( (p_pattern_ctxt->is_lower && tolower(text[i]) == pattern[j])
                     || text[i] == pattern[j] ) {
                    ++j;
                }
            }
            else
                break;
        }

        if ( j < p_pattern_ctxt->actual_pattern_len ) {
            free(text_mask);
            return MIN_WEIGHT;
        }
    }

    TextContext text_ctxt;
    text_ctxt.text = text;
    text_ctxt.text_len = text_len;
    text_ctxt.text_mask = text_mask;
    text_ctxt.col_num = col_num;
    text_ctxt.offset = 0;

    ValueElements val[64];
    memset(val, 0, sizeof(val));

    ValueElements* p_val = evaluate(&text_ctxt, p_pattern_ctxt, 0, val);
    int32_t score = p_val->score;
    uint16_t beg = p_val->beg;
    uint16_t end = p_val->end;

    free(text_mask);

    if ( preference == Preference::Begin ) {
        return score + 10000/text_len + 20000 * pattern_len/(beg + end);
    }
    else {
        return score + 10000 * pattern_len/text_len + 20000 * pattern_len/(text_len - beg);
    }
}


static HighlightContext* evaluateHighlights(TextContext* p_text_ctxt,
                                            PatternContext* p_pattern_ctxt,
                                            uint16_t k,
                                            HighlightContext* groups[])
{
    uint16_t j = p_text_ctxt->offset;

    if ( groups[k] && groups[k]->beg >= j )
        return groups[k];

    uint64_t* text_mask = p_text_ctxt->text_mask;
    uint16_t col_num = p_text_ctxt->col_num;

    const uint8_t* pattern = p_pattern_ctxt->pattern;
    uint16_t base_offset = pattern[k] * col_num;
    uint64_t x = text_mask[base_offset + (j >> 6)] >> (j & 63);
    uint16_t i = 0;

    if ( x == 0 ) {
        uint64_t bits = 0;
        uint16_t col = 0;
        for ( col = (j >> 6) + 1; col < col_num; ++col ) {
            if ( (bits = text_mask[base_offset + col]) != 0 )
                break;
        }
        if ( bits == 0 ) {
            return nullptr;
        }
        else {
            i = (col << 6) + FM_CTZ(bits);
        }
    }
    else {
        i = j + FM_CTZ(x);
    }

    int32_t max_prefix_score = 0;
    int32_t max_score = MIN_WEIGHT;

    if ( !groups[k] ) {
        groups[k] = static_cast<HighlightContext*>(calloc(1, sizeof(HighlightContext)));
        if ( !groups[k] ) {
            fprintf(stderr, "Out of memory in evaluateHighlights()!\n");
            return nullptr;
        }
    }
    else {
        memset(groups[k], 0, sizeof(HighlightContext));
    }

    HighlightContext cur_highlights;
    memset(&cur_highlights, 0, sizeof(HighlightContext));

    const uint8_t* text = p_text_ctxt->text;
    uint16_t text_len = p_text_ctxt->text_len;
    uint16_t pattern_len = p_pattern_ctxt->pattern_len - k;
    int64_t* pattern_mask = p_pattern_ctxt->pattern_mask;

    int32_t special = 0;
    if ( i == 0 )
        special = 50000;
#if defined(_MSC_VER)
    else if ( text[i-1] == '\\' || text[i-1] == '/' )
#else
    else if ( text[i-1] == '/' )
#endif
        special = k == 0 ? 50000 : 30000;
    else if ( isupper(text[i]) )
        special = !isupper(text[i-1]) || (i+1 < text_len && islower(text[i+1])) ? 30000 : 0;
    /* else if ( text[i-1] == '_' || text[i-1] == '-' || text[i-1] == ' ' ) */
    /*     special = 3;                                                     */
    /* else if ( text[i-1] == '.' )                                         */
    /*     special = 3;                                                     */
    else if ( !isalnum(text[i-1]) )
        special = 30000;
    else
        special = 0;
    ++i;
    int64_t d = -2;     /* ~1 */
    int64_t last = d;
    while ( i < text_len )
    {
        last = d;
        auto c = text[i];
        /* c in pattern */
        if ( pattern_mask[c] != -1 )
            d = (d << 1) | (pattern_mask[c] >> k);
        /**
         * text = 'xxABC', pattern = 'abc'; text[i] == 'B'
         * text = 'xxABC', pattern = 'abc'; text[i] == 'C'
         * NOT text = 'xxABCd', pattern = 'abc'; text[i] == 'C'
         * 'Cd' is considered as a word
         */
        /* else if ( isupper(text[i-1]) && pattern_mask[(uint8_t)tolower(c)] != -1 */
        /*           && (i+1 == text_len || !islower(text[i+1])) )                 */
        else if ( pattern_mask[(uint8_t)tolower(c)] != -1 )
            d = (d << 1) | (pattern_mask[(uint8_t)tolower(c)] >> k);
        else
            d = ~0;

        if ( d >= last ) {
            int32_t score = MIN_WEIGHT;
            uint16_t n = FM_BIT_LENGTH(~last);
            /* e.g., text = '~~abcd~~~~', pattern = 'abcd' */
            if ( n == pattern_len ) {
                score = special > 0 ? (n > 1 ? valTable[n+1] : valTable[n]) + special : valTable[n];
                cur_highlights.score = score;
                cur_highlights.beg = i - n;
                cur_highlights.end = i;
                cur_highlights.end_index = 1;
                cur_highlights.positions[0].col = i - n;
                cur_highlights.positions[0].len = n;
                if ( (k == 0 && special == 5) || (k > 0 && special > 0) ) {
                    memcpy(groups[k], &cur_highlights, sizeof(HighlightContext));
                    return groups[k];
                }
            }
            else {
                int32_t prefix_score = special > 0 ? (n > 1 ? valTable[n+1] : valTable[n]) + special : valTable[n];
                /**
                 * e.g., text = 'AbcxxAbcyyde', pattern = 'abcde'
                 * prefer matching 'Abcyyde'
                 */
                if ( prefix_score > max_prefix_score
                     || (special > 0 && prefix_score == max_prefix_score) )
                {
                    max_prefix_score = prefix_score;
                    p_text_ctxt->offset = i;
                    HighlightContext* p_group = evaluateHighlights(p_text_ctxt, p_pattern_ctxt, k + n, groups);
                    if ( p_group ) {
                        if ( p_group->end ) {
                            score = prefix_score + p_group->score - 3000 * (p_group->beg - i);
                            cur_highlights.score = score;
                            cur_highlights.beg = i - n;
                            cur_highlights.end = p_group->end;
                            cur_highlights.positions[0].col = i - n;
                            cur_highlights.positions[0].len = n;
                            memcpy(cur_highlights.positions + 1, p_group->positions, p_group->end_index * sizeof(HighlightPos));
                            cur_highlights.end_index = p_group->end_index + 1;
                        }
                        else {
                            break;
                        }
                    }
                }
            }

            if ( score > max_score ) {
                max_score = score;
                memcpy(groups[k], &cur_highlights, sizeof(HighlightContext));
            }
            /* e.g., text = '~_ababc~~~~', pattern = 'abc' */
            special = 0;
        }

        /*
         * e.g., text = 'a~c~~~~ab~c', pattern = 'abc',
         * to find the index of the second 'a'
         * `d == last` is for the case when text = 'kpi_oos1', pattern = 'kos'
         */
        if ( d == ~0 || d == last ) {
            x = text_mask[base_offset + (i >> 6)] >> (i & 63);

            if ( x == 0 ) {
                uint64_t bits = 0;
                uint16_t col = 0;
                for ( col = (i >> 6) + 1; col < col_num; ++col ) {
                    if ( (bits = text_mask[base_offset + col]) != 0 )
                        break;
                }
                if ( bits == 0 )
                    break;
                else
                    i = (col << 6) + FM_CTZ(bits);
            }
            else {
                i += FM_CTZ(x);
            }

#if defined(_MSC_VER)
            if ( text[i-1] == '\\' || text[i-1] == '/' )
#else
            if ( text[i-1] == '/' )
#endif
                special = k == 0 ? 50000 : 30000;
            else if ( isupper(text[i]) )
                special = !isupper(text[i-1]) || (i+1 < text_len && islower(text[i+1])) ? 30000 : 0;
            /* else if ( text[i-1] == '_' || text[i-1] == '-' || text[i-1] == ' ' ) */
            /*     special = 3;                                                     */
            /* else if ( text[i-1] == '.' )                                         */
            /*     special = 3;                                                     */
            else if ( !isalnum(text[i-1]) )
                special = 30000;
            else
                special = 0;
            d = -2;
            ++i;
        }
        else
            ++i;
    }

    /* e.g., text = '~~~~abcd', pattern = 'abcd' */
    if ( i == text_len ) {
        if ( ~d >> (pattern_len - 1) ) {
            int32_t score = special > 0 ? (pattern_len > 1 ? valTable[pattern_len + 1] : valTable[pattern_len]) + special
                            : valTable[pattern_len];
            if ( score > max_score ) {
                groups[k]->score = score;
                groups[k]->beg = i - pattern_len;
                groups[k]->end = i;
                groups[k]->positions[0].col = i - pattern_len;
                groups[k]->positions[0].len = pattern_len;
                groups[k]->end_index = 1;
            }
        }
    }

    return groups[k];
}

/**
 * return a list of pair [col, length], where `col` is the column number(start
 * from 0, the value must correspond to the byte index of `text`) and `length`
 * is the length of the highlight in bytes.
 * e.g., [ [2,3], [6,2], [10,4], ... ]
 */
Unique_ptr<HighlightContext> FuzzyMatch::getHighlights(const char* p_text,
                                                       uint16_t text_len,
                                                       PatternContext* p_pattern_ctxt)
{
    Destroyer destroyer;

    if ( !p_text || !p_pattern_ctxt )
        return { nullptr, destroyer };

    uint16_t col_num = 0;
    Unique_ptr<uint64_t> p_text_mask(nullptr, destroyer);
    const uint8_t* pattern = p_pattern_ctxt->pattern;
    const uint8_t* text = reinterpret_cast<const uint8_t*>(p_text);
    uint16_t pattern_len = p_pattern_ctxt->pattern_len;
    int64_t* pattern_mask = p_pattern_ctxt->pattern_mask;
    uint8_t first_char = pattern[0];
    uint8_t last_char = pattern[pattern_len - 1];

    /* maximum number of int16_t is (1 << 15) - 1 */
    if ( text_len >= (1 << 15) ) {
        text_len = (1 << 15) - 1;
    }

    if ( pattern_len == 1 ) {
        if ( isupper(first_char) ) {
            int16_t first_char_pos = -1;
            int16_t i;
            for ( i = 0; i < text_len; ++i ) {
                if ( text[i] == first_char ) {
                    first_char_pos = i;
                    break;
                }
            }
            if ( first_char_pos == -1 )
                return { nullptr, destroyer };
            else {
                HighlightContext* p_group = static_cast<HighlightContext*>(malloc(sizeof(HighlightContext)));
                if ( !p_group ) {
                    fprintf(stderr, "Out of memory in getHighlights()!\n");
                    return { nullptr, destroyer };
                }
                p_group->positions[0].col = first_char_pos;
                p_group->positions[0].len = 1;
                p_group->end_index = 1;

                return { p_group, destroyer };
            }
        }
        else {
            int16_t first_char_pos = -1;
            int16_t i;
            for ( i = 0; i < text_len; ++i ) {
                if ( tolower(text[i]) == first_char ) {
                    if ( first_char_pos == -1 )
                        first_char_pos = i;

                    if ( isupper(text[i]) || i == 0 || !isalnum(text[i-1]) ) {
                        first_char_pos = i;
                        break;
                    }
                }
            }
            if ( first_char_pos == -1 )
                return { nullptr, destroyer };
            else {
                HighlightContext* p_group = static_cast<HighlightContext*>(malloc(sizeof(HighlightContext)));
                if ( !p_group ) {
                    fprintf(stderr, "Out of memory in getHighlights()!\n");
                    return { nullptr, destroyer };
                }
                p_group->positions[0].col = first_char_pos;
                p_group->positions[0].len = 1;
                p_group->end_index = 1;

                return { p_group, destroyer };
            }
        }
    }

    if ( p_pattern_ctxt->is_lower ) {
        int16_t first_char_pos = -1;
        int16_t i;
        for ( i = 0; i < text_len; ++i ) {
            if ( tolower(text[i]) == first_char ) {
                first_char_pos = i;
                break;
            }
        }

        int16_t last_char_pos = -1;
        for ( i = text_len - 1; i >= first_char_pos; --i ) {
            if ( tolower(text[i]) == last_char ) {
                last_char_pos = i;
                break;
            }
        }

        col_num = (text_len + 63) >> 6;     /* (text_len + 63)/64 */
        /* uint64_t text_mask[256][col_num] */
        p_text_mask.reset(static_cast<uint64_t*>(calloc(col_num << 8, sizeof(uint64_t))));
        if ( !p_text_mask ) {
            fprintf(stderr, "Out of memory in getHighlights()!\n");
            return { nullptr, destroyer };
        }
        auto text_mask = p_text_mask.get();
        for ( i = first_char_pos; i <= last_char_pos; ++i ) {
            uint8_t c = tolower(text[i]);
            /* c in pattern */
            if ( pattern_mask[c] != -1 )
                text_mask[c * col_num + (i >> 6)] |= 1ULL << (i & 63);
        }
    }
    else {
        int16_t first_char_pos = -1;
        if ( isupper(first_char) ) {
            int16_t i;
            for ( i = 0; i < text_len; ++i ) {
                if ( text[i] == first_char ) {
                    first_char_pos = i;
                    break;
                }
            }
        }
        else {
            int16_t i;
            for ( i = 0; i < text_len; ++i ) {
                if ( tolower(text[i]) == first_char ) {
                    first_char_pos = i;
                    break;
                }
            }
        }

        int16_t last_char_pos = -1;
        if ( isupper(last_char) ) {
            for (int16_t i = text_len - 1; i >= first_char_pos; --i ) {
                if ( text[i] == last_char ) {
                    last_char_pos = i;
                    break;
                }
            }
        }
        else {
            for ( int16_t i = text_len - 1; i >= first_char_pos; --i ) {
                if ( tolower(text[i]) == last_char ) {
                    last_char_pos = i;
                    break;
                }
            }
        }

        col_num = (text_len + 63) >> 6;
        /* uint64_t text_mask[256][col_num] */
        p_text_mask.reset(static_cast<uint64_t*>(calloc(col_num << 8, sizeof(uint64_t))));
        if ( !p_text_mask ) {
            fprintf(stderr, "Out of memory in getHighlights()!\n");
            return { nullptr, destroyer };
        }

        auto text_mask = p_text_mask.get();

        for ( int16_t i = first_char_pos; i <= last_char_pos; ++i ) {
            uint8_t c = text[i];
            if ( isupper(c) ) {
                /* c in pattern */
                if ( pattern_mask[c] != -1 )
                    text_mask[c * col_num + (i >> 6)] |= 1ULL << (i & 63);
                if ( pattern_mask[(uint8_t)tolower(c)] != -1 )
                    text_mask[(uint8_t)tolower(c) * col_num + (i >> 6)] |= 1ULL << (i & 63);
            }
            else {
                /* c in pattern */
                if ( pattern_mask[c] != -1 )
                    text_mask[c * col_num + (i >> 6)] |= 1ULL << (i & 63);
            }
        }
    }

    TextContext text_ctxt;
    text_ctxt.text = text;
    text_ctxt.text_len = text_len;
    text_ctxt.text_mask = p_text_mask.get();
    text_ctxt.col_num = col_num;
    text_ctxt.offset = 0;

    /* HighlightContext* groups[pattern_len] */
    HighlightContext** groups = static_cast<HighlightContext**>(calloc(pattern_len, sizeof(HighlightContext*)));
    if ( !groups ) {
        fprintf(stderr, "Out of memory in getHighlights()!\n");
        return { nullptr, destroyer };
    }

    HighlightContext* p_group = nullptr;
    p_group = evaluateHighlights(&text_ctxt, p_pattern_ctxt, 0, groups);

    for (uint16_t i = 0; i < pattern_len; ++i ) {
        if ( groups[i] && groups[i] != p_group )
            free(groups[i]);
    }
    free(groups);

    return { p_group, destroyer };
}

/**
 * e.g., /usr/src/example.tar.gz
 * `dirname` is "/usr/src"
 * `basename` is "example.tar.gz"
 * `filename` is "example.tar", `suffix` is ".gz"
 */
uint32_t getPathWeight(const char* filename,
                       const char* suffix,
                       const char* dirname,
                       const char* path, uint32_t path_len)
{
    uint32_t filename_lcp = 0;
    uint32_t filename_prefix = 0;
    uint32_t dirname_lcp = 0;
    uint32_t is_suffix_diff = 0;
    uint32_t is_basename_same = 0;
    uint32_t is_dirname_same = 0;

    const char* filename_start = path;
    const char* p = path + path_len;
    const char* p1 = nullptr;

    while ( p >= path )
    {
#if defined(_MSC_VER)
        if ( *p == '\\' || *p == '/' )
#else
        if ( *p == '/' )
#endif
        {
            filename_start = p + 1;
            break;
        }
        --p;
    }

    if ( *suffix != '\0' ) {
        p = filename_start;
        p1 = filename;
        while ( *p != '\0' && *p == *p1 )
        {
            ++filename_lcp;
            ++p;
            ++p1;
        }

        filename_prefix = filename_lcp;

        if ( filename_lcp > 0 ) {
            if ( (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')
                 || (*p1 >= 'a' && *p1 <= 'z') || (*p1 >= '0' && *p1 <= '9') )
            {
                --p;
                while ( p > filename_start )
                {
                    if ( *p >= 'a' && *p <= 'z' ) {
                        --p;
                    }
                    else {
                        break;
                    }
                }
                filename_prefix = (uint32_t)(p - filename_start);
            }
            else if ( (*p >= 'A' && *p <= 'Z') && (*p1 >= 'A' && *p1 <= 'Z')
                      && (*(p-1) >= 'A' && *(p-1) <= 'Z') )
            {
                --p;
                while ( p > filename_start )
                {
                    if ( *p >= 'A' && *p <= 'Z' ) {
                        --p;
                    }
                    else {
                        break;
                    }
                }
                filename_prefix = (uint32_t)(p - filename_start);
            }
        }

        p = path + path_len - 1;
        while ( p > filename_start )
        {
            if ( *p == '.' ) {
                if ( strcmp(suffix, p) != 0 ) {
                    if ( filename_lcp > 0 )
                        is_suffix_diff = 1;
                }
                else if ( *p1 == '\0' && filename_lcp == p - filename_start ) {
                    is_basename_same = 1;
                }
                break;
            }
            --p;
        }
    }
    else {
        is_basename_same = strcmp(filename, filename_start) == 0;
    }

    p = path;
    p1 = dirname;
#if defined(_MSC_VER)
    while ( p < filename_start )
    {
        if ( *p1 == '\\' ) {
            if ( *p == '\\' || *p == '/' ) {
                ++dirname_lcp;
            }
            else {
                break;
            }
        }
        else if ( *p != *p1 ) {
            break;
        }
        ++p;
        ++p1;
    }
#else
    while ( p < filename_start && *p == *p1 )
    {
        if ( *p == '/' ) {
            ++dirname_lcp;
        }
        ++p;
        ++p1;
    }
#endif
    /**
     * e.g., dirname = "abc" , path = "abc/test.h"
     * p1 != dirname is to avoid such a case:
     * e.g., buffer name is "aaa.h", path is "/abc/def.h"
     */
#if defined(_MSC_VER)
    if ( *p1 == '\0' && p1 != dirname && (*p == '\\' || *p == '/') )
#else
    if ( *p1 == '\0' && p1 != dirname && *p == '/' )
#endif
    {
        ++dirname_lcp;
    }

    /* if dirname is empty, filename_start == path */
    is_dirname_same = filename_start - p == 1 || (*dirname == '\0' && filename_start == path) ;

    /* dirname/filename+suffix is the same as path */
    if ( is_basename_same && is_dirname_same ) {
        return 0;
    }

    if ( filename_start == path && *dirname == '\0') {
        dirname_lcp = 1;
    }

    return (((filename_prefix + 1) << 24) | (dirname_lcp << 12) | (is_dirname_same << 11)
            | filename_lcp) + (is_suffix_diff << 2) - path_len;
}


} // end namespace leaf
