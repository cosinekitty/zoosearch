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


enum class Behavior
{
    Stable,
    FixedPoint,
    Diverge,
    Fault,
};


const char* BehaviorText(Behavior bv)
{
    switch (bv)
    {
    case Behavior::Stable:      return "Stable";
    case Behavior::FixedPoint:  return "FixedPoint";
    case Behavior::Diverge:     return "Diverge";
    case Behavior::Fault:       return "Fault";
    default:                    return "Undefined";
    }
}


inline bool OutOfBounds(double u)
{
    return !std::isfinite(u) || (std::abs(u) > 100.0);
}


inline bool IsFixedPoint(double dx, double dy, double dz)
{
    double dist2 = dx*dx + dy*dy + dz*dz;
    static constexpr double epsilon = 1.0e-8;
    static constexpr double eps2 = epsilon * epsilon;
    //printf("dx=%0.16f dy=%0.16f dz=%0.16f dist=%0.16f\n", dx, dy, dz, std::sqrt(dist2));
    return dist2 < eps2;
}


static Behavior Fly(Sapphire::ProgOscillator& osc)
{
    try
    {
        const long SAMPLE_RATE = 44100;
        const long SIM_SECONDS = 300;
        const long SIM_SAMPLES = SIM_SECONDS * SAMPLE_RATE;
        const double dt = 1.0 / SAMPLE_RATE;

        double xMin = osc.xpos();
        double xMax = osc.xpos();
        double yMin = osc.ypos();
        double yMax = osc.ypos();
        double zMin = osc.zpos();
        double zMax = osc.zpos();

        double px = osc.xpos();
        double py = osc.ypos();
        double pz = osc.zpos();

        for (long i = 0; i < SIM_SAMPLES; ++i)
        {
            osc.update(dt, 1);
            const double x = osc.xpos();
            const double y = osc.ypos();
            const double z = osc.zpos();

            if (i == 0) osc.prog.print();

            if (OutOfBounds(x) || OutOfBounds(y) || OutOfBounds(z))
                return Behavior::Diverge;

            if (IsFixedPoint(px-x, py-y, pz-z))
                return Behavior::FixedPoint;

            xMin = std::min(xMin, x);
            xMax = std::max(xMax, x);
            yMin = std::min(yMin, y);
            yMax = std::max(yMax, y);
            zMin = std::min(zMin, z);
            zMax = std::max(zMax, z);

            px = x;
            py = y;
            pz = z;
        }

        printf("Fly: xrange:[%0.3f, %0.3f], yrange:[%0.3f, %0.3f]\n", xMin, xMax, yMin, yMax);
        return Behavior::Stable;
    }
    catch (const Sapphire::CalcError& ex)
    {
        printf("Fly(EXCEPTION): %s\n", ex.what());
        return Behavior::Fault;
    }
}


static int Compile(Sapphire::ProgOscillator& osc, std::string infix)
{
    Sapphire::BytecodeResult result = osc.compile(infix);
    if (result.failure())
    {
        printf("Compile failure [%s]: %s\n", infix.c_str(), result.message.c_str());
        return 1;
    }
    return 0;
}


static int Test_Rucklidge()
{
    using namespace Sapphire;

    constexpr double x0 = +1.5;
    constexpr double y0 = -0.5;
    constexpr double z0 = +0.1;

    ProgOscillator osc(
        0.005,
        x0, y0, z0,
        -CHAOS_AMPLITUDE, +CHAOS_AMPLITUDE,
        -CHAOS_AMPLITUDE, +CHAOS_AMPLITUDE,
        -CHAOS_AMPLITUDE, +CHAOS_AMPLITUDE,
        1, 1, 1
    );

    osc.knobMap[0].center = 5.25;
    osc.knobMap[0].spread = 1.45;
    osc.setKnob(0.0);

    if (Compile(osc, "-2*x + a*y - y*z")) return 1;
    if (Compile(osc, "x")) return 1;
    if (Compile(osc, "-z + y*y")) return 1;

    Behavior bv = Fly(osc);
    if (bv != Behavior::Stable)
    {
        printf("Test_Rucklidge: incorrect behavior result: %s\n", BehaviorText(bv));
        return 1;
    }

    printf("Test_Rucklidge: PASS\n");
    return 0;
}



static int UnitTests()
{
    if (Test_Rucklidge()) return 1;
    printf("UnitTests: PASS\n");
    return 0;
}
