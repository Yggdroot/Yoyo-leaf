_Pragma("once");

#include <string>
#include <memory>
#include <algorithm>
#include <functional>
#include <vector>
#include "constString.h"
#include "fuzzyMatch.h"
#include "threadPool.h"
#include "ringBuffer.h"

namespace leaf
{

using weight_t = int32_t;
using StrType = ConstString;
using StrContainer = RingBuffer<StrType>;
using WeightContainer = RingBuffer<weight_t>;
using Result = std::tuple<WeightContainer, StrContainer>;
using DigestFn = std::function<StrType(const StrType&)>;

struct MatchResult
{
    weight_t weight;
    uint32_t index;
};


class FuzzyEngine : private FuzzyMatch
{
public:
    explicit FuzzyEngine(uint32_t cpus): cpu_count_(cpus) {}

    Result fuzzyMatch(const StrContainer::const_iterator& source_begin,
                      uint32_t source_size,
                      const std::string& pattern,
                      Preference preference=Preference::Begin,
                      DigestFn get_digest=DigestFn(),
                      bool sort_results=true);

    Result merge(const Result& a, const Result& b);

    std::vector<Unique_ptr<HighlightContext>>
        getHighlights(const StrContainer::const_iterator& source_begin,
                      uint32_t source_size,
                      const std::string& pattern,
                      DigestFn get_digest=DigestFn());
private:
    void _merge(MatchResult* results,
                MatchResult* buffer,
                uint32_t offset_1,
                uint32_t length_1,
                uint32_t length_2);
private:
    using PatternContextPtr = std::unique_ptr<PatternContext>;
    uint32_t          cpu_count_;
    ThreadPool        thread_pool_;
    std::string       pattern_;
    PatternContextPtr pattern_ctxt_;

};


} // end namespace leaf
