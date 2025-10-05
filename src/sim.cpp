#include "sim.hpp"
#include <cmath>
#include <cassert>
#include <sstream>
#include <iomanip>

static inline uint64_t blockTagOf(uint64_t addr, std::size_t lineBytes)
{
    return addr / lineBytes; // block number
}

unsigned CacheSim::log2u(std::size_t x)
{
    unsigned n = 0;
    while ((1ull << n) < x)
        ++n;
    return n;
}

CacheSim::CacheSim(SimParams p) : p_(p)
{
    assert(p_.lineBytes && p_.assoc && p_.totalBytes);
    sets_ = (p_.totalBytes / p_.lineBytes) / p_.assoc;
    if (sets_ == 0)
        sets_ = 1;

    offsetBits_ = log2u(p_.lineBytes);
    indexBits_ = log2u(sets_);
    indexMask_ = (sets_ - 1);

    cache_.assign(sets_, std::vector<Line>(p_.assoc));

    faCapacityLines_ = p_.totalBytes / p_.lineBytes;
}

void CacheSim::reset()
{
    for (auto &set : cache_)
        for (auto &l : set)
            l = Line{};
    faLRU_.clear();
    faPos_.clear();
    seenBlocks_.clear();
    stats_ = MissBreakdown{};
}

std::tuple<std::size_t, uint64_t> CacheSim::indexTag(uint64_t addr) const
{
    uint64_t block = addr >> offsetBits_;
    std::size_t idx = static_cast<std::size_t>(block & indexMask_);
    uint64_t tag = block >> indexBits_;
    return {idx, tag};
}

bool CacheSim::faHit(uint64_t blockTag)
{
    auto it = faPos_.find(blockTag);
    if (it == faPos_.end())
        return false;
    // move to front (MRU)
    faLRU_.splice(faLRU_.begin(), faLRU_, it->second);
    it->second = faLRU_.begin();
    return true;
}

void CacheSim::faTouch(uint64_t blockTag)
{
    auto it = faPos_.find(blockTag);
    if (it != faPos_.end())
    {
        // move to front
        faLRU_.splice(faLRU_.begin(), faLRU_, it->second);
        it->second = faLRU_.begin();
        return;
    }
    // insert new at front
    faLRU_.push_front(blockTag);
    faPos_[blockTag] = faLRU_.begin();
    if (faPos_.size() > faCapacityLines_)
    {
        // evict LRU (back)
        auto victim = faLRU_.back();
        faPos_.erase(victim);
        faLRU_.pop_back();
    }
}

void CacheSim::access(uint64_t addr)
{
    stats_.total++;
    const auto [idx, tag] = indexTag(addr);
    auto &set = cache_[idx];

    // tag match?
    for (std::size_t way = 0; way < p_.assoc; ++way)
    {
        auto &line = set[way];
        if (line.valid && line.tag == tag)
        {
            stats_.hits++;
            // update LRU ranks (smaller = more recent)
            if (p_.assoc > 1)
            {
                for (auto &l : set)
                    if (l.valid)
                        l.lruRank++;
                line.lruRank = 0;
            }
            // touch full-assoc shadow too
            faTouch((tag << indexBits_) | idx);
            return;
        }
    }

    // MISS path
    stats_.misses++;
    const uint64_t blockTag = (tag << indexBits_) | idx;

    bool firstTime = (seenBlocks_.find(blockTag) == seenBlocks_.end());
    if (firstTime)
    {
        stats_.compulsory++;
        seenBlocks_.insert(blockTag);
    }
    else
    {
        // distinguish conflict vs capacity with the fully associative shadow
        bool wouldHitFA = faHit(blockTag);
        if (wouldHitFA)
        {
            stats_.conflict++;
        }
        else
        {
            stats_.capacity++;
        }
    }

    // install into set (LRU/empty)
    std::size_t victim = 0;
    int worst = -1;
    bool hasEmpty = false;
    for (std::size_t way = 0; way < p_.assoc; ++way)
    {
        auto &l = set[way];
        if (!l.valid)
        {
            victim = way;
            hasEmpty = true;
            break;
        }
        if (l.lruRank > worst)
        {
            worst = l.lruRank;
            victim = way;
        }
    }
    // age all, reset chosen
    for (auto &l : set)
        if (l.valid)
            l.lruRank++;
    auto &v = set[victim];
    v.valid = true;
    v.tag = tag;
    v.lruRank = 0;

    // touch full-assoc shadow (after any potential faHit above)
    faTouch(blockTag);
}

std::string CacheSim::csvHeader()
{
    return "size_kb,line,assoc,policy,miss_rate,compulsory,capacity,conflict,total,hits,misses";
}

std::string CacheSim::csvRow(const std::string &tag) const
{
    double missRate = stats_.total ? (double)stats_.misses / (double)stats_.total : 0.0;
    std::ostringstream oss;
    oss << (p_.totalBytes / 1024) << ","
        << p_.lineBytes << ","
        << p_.assoc << ","
        << tag << ","
        << std::fixed << std::setprecision(6) << missRate << ","
        << stats_.compulsory << ","
        << stats_.capacity << ","
        << stats_.conflict << ","
        << stats_.total << ","
        << stats_.hits << ","
        << stats_.misses;
    return oss.str();
}
