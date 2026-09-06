#include "MemoryParityFixtures.h"
#include <iostream>

int main()
{
    const auto& Fixtures = Memoria::Tests::Fixtures();
    std::size_t Failed = 0;
    for (const auto& F : Fixtures)
    {
        const auto Errors = Memoria::Tests::Run(F);
        if (Errors.empty()) { std::cout << "PASS " << F.Id << '\n'; }
        else
        {
            ++Failed;
            for (const auto& Error : Errors) { std::cerr << "FAIL " << Error << '\n'; }
        }
    }
    std::cout << "MEMORIA_NATIVE_MEMORY_PARITY " << (Failed == 0 ? "PASS" : "FAIL")
        << " cases=" << Fixtures.size() << " failed=" << Failed << '\n';
    return Failed == 0 ? 0 : 1;
}
