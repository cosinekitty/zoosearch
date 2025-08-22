#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include "sapphire_prog_chaos.hpp"


static int Search();
static int UnitTests();
static int Test_Rucklidge();


int main(int argc, const char *argv[])
{
    if (argc >= 2)
    {
        const char *verb = argv[1];
        if (0 == strcmp(verb, "test"))
            return UnitTests();

        if (0 == strcmp(verb, "search"))
            return Search();
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


template <unsigned xBins, unsigned yBins, unsigned zBins>
class Hologram
{
private:
    static_assert(xBins > 0 && xBins <= 1024);
    static_assert(yBins > 0 && yBins <= 1024);
    static_assert(zBins > 0 && zBins <= 1024);

    static constexpr unsigned nFlatSize = xBins * yBins * zBins;

    std::vector<unsigned> count;
    double radius = 1;

    unsigned index(double u, unsigned nbins) const
    {
        assert(std::isfinite(u));
        double realIndex = (nbins-1)*((u + radius) / (2*radius));
        realIndex = std::max<double>(0.0, realIndex);
        unsigned intIndex = static_cast<unsigned>(std::round(realIndex));
        return std::min<unsigned>(intIndex, nbins-1);
    }

    const unsigned& access(double x, double y, double z) const
    {
        unsigned i = index(x, xBins);
        unsigned j = index(y, yBins);
        unsigned k = index(z, zBins);
        return count.at(i + xBins*(j + yBins*k));
    }

    unsigned& access(double x, double y, double z)
    {
        return const_cast<unsigned&>(const_cast<const Hologram *>(this)->access(x, y, z));
    }

public:
    static constexpr unsigned xBinCount = xBins;
    static constexpr unsigned yBinCount = yBins;
    static constexpr unsigned zBinCount = zBins;

    explicit Hologram(double _radius)
        : radius(_radius)
    {
        count.resize(nFlatSize);
        initialize();
    }

    void initialize()
    {
        for (unsigned& k : count)
            k = 0;
    }

    void tally(double x, double y, double z)
    {
        ++access(x, y, z);
    }

    unsigned hits(unsigned i, unsigned j, unsigned k) const
    {
        if (i < 0 || i >= xBins) return 0;
        if (j < 0 || j >= xBins) return 0;
        if (k < 0 || k >= xBins) return 0;
        return count.at(i + xBins*(j + yBins*k));
    }
};


using holo_t = Hologram<40, 40, 40>;


static void PrintHolo(const holo_t& holo)
{
    for (unsigned j = 0; j < holo_t::yBinCount; ++j)
    {
        for (unsigned i = 0; i < holo_t::xBinCount; ++i)
        {
            unsigned sum = 0;
            for (unsigned k = 0; k < holo_t::zBinCount; ++k)
                sum += holo.hits(i, j, k);

            char c = (sum > 0) ? '@' : ' ';
            printf("%c", c);
        }
        printf("\n");
    }
}


static Behavior Fly(Sapphire::ProgOscillator& osc)
{
    try
    {
        holo_t holo(6.0);

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

            if (OutOfBounds(x) || OutOfBounds(y) || OutOfBounds(z))
                return Behavior::Diverge;

            if (IsFixedPoint(px-x, py-y, pz-z))
                return Behavior::FixedPoint;

            holo.tally(x, y, z);

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
        PrintHolo(holo);
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
        printf("Infix compile failure [%s]: %s\n", infix.c_str(), result.message.c_str());
        return 1;
    }
    return 0;
}


static int CompilePostfix(Sapphire::ProgOscillator& osc, std::string postfix)
{
    Sapphire::BytecodeResult result = osc.compilePostfix(postfix);
    if (result.failure())
    {
        printf("Postfix compile failure [%s]: %s\n", postfix.c_str(), result.message.c_str());
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
    osc.prog.print();
    if (bv != Behavior::Stable)
    {
        printf("Test_Rucklidge: incorrect behavior result: %s\n", BehaviorText(bv));
        return 1;
    }

    printf("Test_Rucklidge: PASS\n");
    return 0;
}


using string_list_t = std::vector<std::string>;


class ExpressionEnumerator
{
public:
    static constexpr int CACHESIZE = 3;

private:
    string_list_t cache[CACHESIZE];
    const string_list_t emptyList;

    static string_list_t makeVars(const char *varlist)
    {
        string_list_t vlist;
        if (varlist)
            for (int i = 0; varlist[i]; ++i)
                vlist.push_back(std::string{varlist[i]});
        return vlist;
    }

public:
    explicit ExpressionEnumerator(const char* varlist)
    {
        cache[0] = makeVars(varlist);
    }

    const string_list_t& postfixExpressions(int opcount)
    {
        if (opcount >= 0 && opcount < CACHESIZE)
        {
            if (cache[opcount].size() == 0)
            {
                for (int leftCount = 0; leftCount < opcount; ++leftCount)
                {
                    int rightCount = (opcount-1) - leftCount;
                    for (const std::string& u : postfixExpressions(leftCount))
                    {
                        for (const std::string& v : postfixExpressions(rightCount))
                        {
                            if (u != v)
                                cache[opcount].push_back(u + v + "-");
                            if (u < v)
                                cache[opcount].push_back(u + v + "+");
                            if (u <= v)
                                cache[opcount].push_back(u + v + "*");
                        }
                    }
                }
            }
            return cache[opcount];
        }
        return emptyList;
    }
};


static int Test_ExpressionEnumerator()
{
    const char *outFileName = "output/expressions.txt";
    FILE *outfile = fopen(outFileName, "wt");
    if (!outfile)
    {
        printf("Test_ExpressionEnumerator: Cannot open file for output: %s\n", outFileName);
        return 1;
    }

    ExpressionEnumerator ee("abxy");

    for (int opcount = 0; opcount < ExpressionEnumerator::CACHESIZE; ++opcount)
    {
        fprintf(outfile, "opcount=%d\n", opcount);
        const string_list_t& list = ee.postfixExpressions(opcount);
        for (const std::string& s : list)
            fprintf(outfile, "    %s\n", s.c_str());
        fprintf(outfile, "\n");
    }

    fclose(outfile);
    printf("Test_ExpressionEnumerator: PASS\n");
    return 0;
}


static bool IsCandidateFunction(const std::string& postfix)
{
    // It does not make sense to have a function like vx = a+b,
    // because that is inherently unstable (assuming a+b != 0).
    // Exclude any function that does not contain at least one reference
    // to a variable: 'xyz'.
    for (char c : postfix)
        if (c >= 'x' && c <= 'z')
            return true;

    return false;
}


static int Search()
{
    using namespace Sapphire;

    ExpressionEnumerator ee("abcdxyz");

    constexpr double x0 = +0.123;
    constexpr double y0 = -0.157;
    constexpr double z0 = +0.109;

    ProgOscillator osc(
        0.005,
        x0, y0, z0,
        -CHAOS_AMPLITUDE, +CHAOS_AMPLITUDE,
        -CHAOS_AMPLITUDE, +CHAOS_AMPLITUDE,
        -CHAOS_AMPLITUDE, +CHAOS_AMPLITUDE,
        1, 1, 1
    );

    osc.knobMap[0].center = +1.0;
    osc.knobMap[0].spread = 0.9;
    osc.knobMap[1].center = -1.0;
    osc.knobMap[1].spread = 0.9;
    osc.knobMap[2].center = +0.5;
    osc.knobMap[2].spread = 0.9;
    osc.knobMap[3].center = -0.5;
    osc.knobMap[3].spread = 0.9;
    osc.setMode(0);
    osc.setKnob(0.0);

    string_list_t exprlist;
    int rejectCount = 0;
    for (int opcount = 0; opcount <= 1; ++opcount)
    {
        const string_list_t& list = ee.postfixExpressions(opcount);
        for (const std::string& postfix : list)
            if (IsCandidateFunction(postfix))
                exprlist.push_back(postfix);
            else
                ++rejectCount;
    }
    printf("Search: expression list length = %d, rejected %d\n", static_cast<int>(exprlist.size()), rejectCount);

    for (const std::string& xPostfix : exprlist)
    {
        for (const std::string& yPostfix : exprlist)
        {
            for (const std::string& zPostfix : exprlist)
            {
                printf("vx[%s], vy[%s], vz[%s]\n", xPostfix.c_str(), yPostfix.c_str(), zPostfix.c_str());
                fflush(stdout);

                osc.resetProgram();
                if (CompilePostfix(osc, xPostfix)) return 1;
                if (CompilePostfix(osc, yPostfix)) return 1;
                if (CompilePostfix(osc, zPostfix)) return 1;

                //printf("Program before running:\n");
                //osc.prog.print();
                Behavior bv = Fly(osc);
                if (bv != Behavior::Diverge)
                {
                    printf("RESULT: %s\n", BehaviorText(bv));
                    osc.prog.print();
                    goto done;
                }
            }
        }
    }

done:
    printf("Search: PASS\n");
    return 0;
}


static int UnitTests()
{
    if (Test_Rucklidge()) return 1;
    if (Test_ExpressionEnumerator()) return 1;
    printf("UnitTests: PASS\n");
    return 0;
}
