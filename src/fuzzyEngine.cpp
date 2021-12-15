#include <algorithm>
#include <cstring>
#include "fuzzyEngine.h"

namespace leaf
{

#define MAX_TASK_COUNT(cpu_count) ((cpu_count) << 3)

Result FuzzyEngine::fuzzyMatch(const StrContainer::const_iterator& source_begin,
                               uint32_t source_size,
                               const std::string& pattern,
                               Preference preference,
                               DigestFn get_digest,
                               bool sort_results)
{
    if ( source_begin == nullptr || source_size == 0 ) {
        return Result();
    }

    if ( pattern != pattern_ ) {
        pattern_ = pattern;
        pattern_ctxt_.reset(initPattern(pattern_.c_str(), pattern_.length()));
    }

    if ( thread_pool_.size() == 0 ) {
        thread_pool_.start(cpu_count_);
    }

    uint32_t max_task_count  = MAX_TASK_COUNT(cpu_count_);
    uint32_t chunk_size = (source_size + max_task_count - 1) / max_task_count;
    if ( cpu_count_ == 1 ) {
        chunk_size = source_size;
    }
    else if ( chunk_size < 4096 ) {
        chunk_size = std::max(4096u, (source_size + cpu_count_ - 1) / cpu_count_);
    }

    std::unique_ptr<MatchResult[]> the_results(new MatchResult[source_size]);
    auto results = the_results.get();
    for ( uint32_t offset = 0; offset < source_size; offset += chunk_size ) {
        uint32_t length = std::min(chunk_size, source_size - offset);

        thread_pool_.enqueueTask([&source_begin, this, results, offset, length, preference] {
            for ( auto i = offset; i < offset + length; ++i ) {
                results[i].weight = getWeight((source_begin + i)->str, (source_begin + i)->len,
                                              pattern_ctxt_.get(), preference);
                results[i].index = i;
            }
        });
    }

    // blocks until all tasks are done
    thread_pool_.join();

    uint32_t results_count = 0;
    for (uint32_t i = 0; i < source_size; ++i ) {
        if ( results[i].weight > MIN_WEIGHT ) {
            if ( i > results_count ) {
                results[results_count] = results[i];
            }
            ++results_count;
        }
    }

    if ( results_count == 0 ) {
        return Result();
    }

    if ( sort_results ) {
        if ( cpu_count_ == 1 || results_count < 50000 ) {
            std::sort(results, results + results_count,
                      [](const MatchResult& a, const MatchResult& b) {
                          return a.weight > b.weight;
                      });
        }
        else {
            chunk_size = (results_count + max_task_count - 1) / max_task_count;
            if ( chunk_size < 4096 ) {
                chunk_size = std::max(4096u, (source_size + cpu_count_ - 1) / cpu_count_);
            }
            for ( uint32_t offset = 0; offset < results_count; offset += chunk_size ) {
                uint32_t length = std::min(chunk_size, results_count - offset);

                thread_pool_.enqueueTask([results, offset, length] {
                    std::sort(results + offset, results + (offset + length),
                              [](const MatchResult& a, const MatchResult& b) {
                                  return a.weight > b.weight;
                              });
                });
            }

            // blocks until all tasks are done
            thread_pool_.join();

            auto task_count = (results_count + chunk_size - 1) / chunk_size;
            std::unique_ptr<MatchResult[]> result_buffer(new MatchResult[chunk_size * (task_count >> 1)]);
            while ( chunk_size < results_count ) {
                uint32_t two_chunk_size = chunk_size << 1;
                uint32_t q = results_count / two_chunk_size;
                uint32_t r = results_count % two_chunk_size;
                for (uint32_t i = 0; i < q; ++i ) {
                    auto offset_1 = i * two_chunk_size;
                    auto length_1 = chunk_size;
                    auto length_2 = chunk_size;
                    auto buffer = result_buffer.get() + (offset_1 >> 1);

                    thread_pool_.enqueueTask([this, results, offset_1, length_1, length_2, buffer] {
                        _merge(results, buffer, offset_1, length_1, length_2);
                    });
                }

                if ( r > chunk_size ) {
                    auto offset_1 = q * two_chunk_size;
                    auto length_1 = chunk_size;
                    auto length_2 = r - chunk_size;
                    auto buffer = result_buffer.get() + (offset_1 >> 1);

                    thread_pool_.enqueueTask([this, results, offset_1, length_1, length_2, buffer] {
                        _merge(results, buffer, offset_1, length_1, length_2);
                    });
                }

                chunk_size <<= 1;

                // blocks until all tasks are done
                thread_pool_.join();
            }
        }
    }

    Result r{ WeightContainer(results_count), StrContainer(results_count) };
    auto& weight_list = std::get<0>(r);
    auto& str_list = std::get<1>(r);
    if ( cpu_count_ == 1 || results_count < 50000 ) {
        for (uint32_t i = 0; i < results_count; ++i ) {
            weight_list[i] = results[i].weight;
            str_list[i] = *(source_begin + results[i].index);
        }
    }
    else
    {
        chunk_size = (results_count + max_task_count - 1) / max_task_count;
        if ( chunk_size < 4096 ) {
            chunk_size = std::max(4096u, (source_size + cpu_count_ - 1) / cpu_count_);
        }
        for ( uint32_t offset = 0; offset < results_count; offset += chunk_size ) {
            uint32_t length = std::min(chunk_size, results_count - offset);

            thread_pool_.enqueueTask([&source_begin, &weight_list, &str_list, results, offset, length] {
                for ( auto i = offset; i < offset + length; ++i ) {
                    weight_list[i] = results[i].weight;
                    str_list[i] = *(source_begin + results[i].index);
                }
            });
        }
        thread_pool_.join();
    }

    return r;
}

Result FuzzyEngine::merge(const Result& a, const Result& b)
{
    const auto& weights_a = std::get<0>(a);
    auto size_a = weights_a.size();
    if ( size_a == 0 ) {
        return b;
    }
    const auto& weights_b = std::get<0>(b);
    auto size_b = weights_b.size();
    if ( size_b == 0 ) {
        return a;
    }

    Result result{ WeightContainer(size_a + size_b), StrContainer(size_a + size_b) };
    auto& weight_list = std::get<0>(result);
    auto& str_list = std::get<1>(result);

    decltype(size_a) i = 0;
    decltype(size_b) j = 0;

    const auto& weights_a_iter = weights_a.begin();
    const auto& weights_b_iter = weights_b.begin();
    const auto& str_list_a_iter = std::get<1>(a).begin();
    const auto& str_list_b_iter = std::get<1>(b).begin();
    while ( i < size_a && j < size_b ) {
        if ( *(weights_a_iter + i) > *(weights_b_iter + j) ) {
            weight_list[i + j] = *(weights_a_iter + i);
            str_list[i + j] = *(str_list_a_iter + i);
            ++i;
        }
        else {
            weight_list[i + j] = *(weights_b_iter + j);
            str_list[i + j] = *(str_list_b_iter + j);
            ++j;
        }
    }

    if ( i < size_a ) {
        // move tail_ pointer back
        weight_list.pop_back(size_a - i);
        weight_list.push_back(weights_a_iter + i, weights_a.cend());
        // move tail_ pointer back
        str_list.pop_back(size_a - i);
        str_list.push_back(str_list_a_iter + i, str_list_a_iter + size_a);
    }

    if ( j < size_b ) {
        // move tail_ pointer back
        weight_list.pop_back(size_b - j);
        weight_list.push_back(weights_b.cbegin() + j, weights_b.cend());
        // move tail_ pointer back
        str_list.pop_back(size_b - j);
        str_list.push_back(str_list_b_iter + j, str_list_b_iter + size_b);
    }

    return result;
}

std::vector<Unique_ptr<HighlightContext>>
FuzzyEngine::getHighlights(const StrContainer::const_iterator& source_begin,
                           uint32_t source_size,
                           const std::string& pattern,
                           DigestFn get_digest)
{
    std::vector<Unique_ptr<HighlightContext>> res;

    if ( source_begin == nullptr || source_size == 0 ) {
        return res;
    }

    PatternContextPtr pattern_ctxt(initPattern(pattern.c_str(), pattern.length()));
    res.reserve(source_size);
    auto end = source_begin + source_size;
    for ( auto iter = source_begin; iter != end; ++iter ) {
        res.emplace_back(FuzzyMatch::getHighlights(iter->str, iter->len, pattern_ctxt.get()));
    }

    return res;
}

void FuzzyEngine::_merge(MatchResult* results,
                         MatchResult* buffer,
                         uint32_t offset_1,
                         uint32_t length_1,
                         uint32_t length_2)
{
    auto list_1 = results + offset_1;
    auto list_2 = list_1 + length_1;
    memcpy(buffer, list_2, length_2 * sizeof(MatchResult));
    int32_t i = length_1 - 1;
    int32_t j = length_2 - 1;
    int32_t k = length_1 + j;
    while ( i >= 0 && j >= 0 ) {
        if ( list_1[i].weight < buffer[j].weight ) {
            list_1[k--] = list_1[i--];
        }
        else {
            list_1[k--] = buffer[j--];
        }
    }
    while ( j >= 0 ) {
        list_1[k--] = buffer[j--];
    }
}


} // end namespace leaf
