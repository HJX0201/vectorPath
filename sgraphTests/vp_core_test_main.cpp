#include <cstring>
#include <iostream>

#define VECTORPATH_CORE_TEST(name, entry) int entry();
#include "vp_core_test_registry.inc"
#undef VECTORPATH_CORE_TEST

namespace
{

struct VpCoreSuite
{
    const char* name;
    int (*run)();
};

constexpr VpCoreSuite kSuites[] = {
#define VECTORPATH_CORE_TEST(name, entry) {#name, &entry},
#include "vp_core_test_registry.inc"
#undef VECTORPATH_CORE_TEST
};

} // namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--list-suites") == 0)
    {
        for (const auto& suite : kSuites)
        {
            std::cout << suite.name << '\n';
        }
        return 0;
    }
    if (argc == 2)
    {
        for (const auto& suite : kSuites)
        {
            if (std::strcmp(argv[1], suite.name) == 0)
            {
                return suite.run();
            }
        }
    }
    std::cerr << "Usage: vectorPathCoreTests <suite> or --list-suites\n";
    return 2;
}
