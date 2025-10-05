#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <tuple>

struct SimParams
{
    std::size_t totalBytes; // e.g. 32*1024
    std::size_t lineBytes;  // e.g. 64
    std::size_t assoc;      // 1 (DM) or 4
    bool lru = true;        // only LRU implemented for assoc>1
};

struct MissBreakdown
{
    std::size_t total = 0;
    std::size_t hits = 0;
    std::size_t misses = 0;
    std::size_t compulsory = 0;
    std::size_t conflict = 0;
    std::size_t capacity = 0;
};

class CacheSim
{
public:
    explicit CacheSim(SimParams p);

    // feed a 64-bit address
    void access(uint64_t addr);

    const MissBreakdown &stats() const { return stats_; }

    // reset internal state & stats
    void reset();

    // CSV header + one row
    static std::string csvHeader();
    std::string csvRow(const std::string &tag) const; // tag like "DM" or "4way"

private:
    SimParams p_;
    std::size_t sets_;
    std::size_t indexMask_;
    unsigned offsetBits_ = 0;
    unsigned indexBits_ = 0;

    struct Line
    {
        bool valid = false;
        uint64_t tag = 0;
        int lruRank = 0;
    };

    // target cache (DM or 4-way)
    std::vector<std::vector<Line>> cache_; // [set][way]

    // fully associative shadow (same total capacity) to split conflict vs capacity
    // classic LRU using list + hashmap
    std::size_t faCapacityLines_ = 0;
    std::list<uint64_t> faLRU_;
    std::unordered_map<uint64_t, std::list<uint64_t>::iterator> faPos_;

    // first-touch detector for compulsory misses (by block tag)
    std::unordered_set<uint64_t> seenBlocks_;

    MissBreakdown stats_{};

    // helpers
    static unsigned log2u(std::size_t x);
    std::tuple<std::size_t, uint64_t> indexTag(uint64_t addr) const;
    bool faHit(uint64_t blockTag);
    void faTouch(uint64_t blockTag);
};
