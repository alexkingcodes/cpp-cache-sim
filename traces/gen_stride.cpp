#include <cstdint>
#include <iostream>
#include <vector>

/*
  Generate hex addresses that stride through an array.
  Args: bytes length, line size, stride lines, count
  Example: gen-stride 65536 64 4 20000 > traces/stride.hex
*/
int main(int argc, char **argv)
{
    if (argc < 5)
    {
        std::cerr << "Usage: gen-stride <bytes> <lineBytes> <strideLines> <count>\n";
        return 1;
    }
    uint64_t bytes = std::stoull(argv[1]);
    uint64_t lineBytes = std::stoull(argv[2]);
    uint64_t strideLines = std::stoull(argv[3]);
    uint64_t count = std::stoull(argv[4]);

    uint64_t lines = bytes / lineBytes;
    uint64_t idx = 0;
    for (uint64_t i = 0; i < count; i++)
    {
        uint64_t addr = (idx * lineBytes) & ~((uint64_t)lineBytes - 1ULL);
        std::cout << std::hex << addr << "\n";
        idx = (idx + strideLines) % lines;
    }
    return 0;
}
