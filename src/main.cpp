#include "sim.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>

static void usage()
{
    std::cerr << "cache-sim --size <KB> --line <bytes> --assoc <1|4> --trace <path>\n"
                 "Example: cache-sim --size 32 --line 64 --assoc 1 --trace traces/toy.hex\n";
}

int main(int argc, char **argv)
{
    std::size_t sizeKB = 0, line = 0, assoc = 1;
    std::string tracePath;

    for (int i = 1; i < argc; i++)
    {
        std::string a = argv[i];
        auto need = [&](const char *name) -> std::string
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing value for " << name << "\n";
                usage();
                std::exit(1);
            }
            return std::string(argv[++i]);
        };
        if (a == "--size")
            sizeKB = std::stoul(need("--size"));
        else if (a == "--line")
            line = std::stoul(need("--line"));
        else if (a == "--assoc")
            assoc = std::stoul(need("--assoc"));
        else if (a == "--trace")
            tracePath = need("--trace");
        else
        {
            std::cerr << "Unknown arg: " << a << "\n";
            usage();
            return 1;
        }
    }
    if (!sizeKB || !line || !assoc || tracePath.empty())
    {
        usage();
        return 1;
    }

    std::ifstream f(tracePath);
    if (!f)
    {
        std::cerr << "Cannot open trace: " << tracePath << "\n";
        return 1;
    }

    // run sim
    SimParams p{sizeKB * 1024, line, assoc, true};
    CacheSim sim(p);

    std::string s;
    while (std::getline(f, s))
    {
        if (s.empty())
            continue;
        uint64_t addr = 0;
        std::stringstream ss;
        ss << std::hex << s;
        ss >> addr;
        sim.access(addr);
    }

    const auto &st = sim.stats();
    std::cout << CacheSim::csvHeader() << "\n";
    std::cout << sim.csvRow(assoc == 1 ? "DM" : "4way") << "\n";

    // Also print a human summary to stream for screenshot
    double missRate = st.total ? double(st.misses) / double(st.total) : 0.0;
    std::cerr << "Total: " << st.total
              << "  Hits: " << st.hits
              << "  Misses: " << st.misses
              << "  MissRate: " << missRate
              << "  [compulsory=" << st.compulsory
              << " capacity=" << st.capacity
              << " conflict=" << st.conflict << "]\n";
    return 0;
}
