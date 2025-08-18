#include <cstdio>
#include <cstring>
#include "sapphire_prog_chaos.hpp"


static int UnitTests();
static int Test_Rucklidge();


int main(int argc, const char *argv[])
{
    if (argc >= 2)
    {
        const char *verb = argv[1];
        if (0 == strcmp(verb, "test"))
            return UnitTests();
    }
    printf("zoosearch: Invalid command line arguments.\n");
    return 1;
}


static int Test_Rucklidge()
{
    Sapphire::ProgOscillator osc(
        0.01,
        -3.4423733871317674, 9.699573232290314, 0.006054606899164795,
        -1, +1,
        -1, +1,
        -1, +1,
        0.15, 0.20, 0.04
    );

    printf("Test_Rucklidge: PASS\n");
    return 0;
}



static int UnitTests()
{
    if (Test_Rucklidge()) return 1;
    printf("UnitTests: PASS\n");
    return 0;
}
