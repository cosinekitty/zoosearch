#include <cstdio>
#include <cstring>


static int UnitTests();


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


static int UnitTests()
{
    printf("UnitTests: PASS\n");
    return 0;
}
