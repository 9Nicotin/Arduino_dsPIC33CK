/*
 * NanoRoundMacro - what Arduino's round() really is, and when it bites
 *
 * dsPIC33CK256MC005 Curiosity Nano (EV08P02A)
 *
 * NO EXTERNAL WIRING NEEDED. Everything here is printed over Serial.
 *
 * THE POINT OF THIS SKETCH
 *
 * Arduino.h defines round() as a MACRO, not a function:
 *
 *     #define round(x)  ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
 *
 * math.h -- which Arduino.h includes a few lines earlier -- declares the real
 * C99 function:
 *
 *     double round(double);
 *
 * The macro is defined last, so from the moment a sketch includes Arduino.h the
 * function is unreachable by that name. The compiler says nothing about it: GCC
 * suppresses "macro redefined" when the earlier definition came from a system
 * header, and the toolchain headers are system headers. Silent shadowing.
 *
 * Three consequences, in the order they are likely to cost you an evening:
 *
 *   1. IT RETURNS long, NOT double. Fine for 2.5 -> 3, wrong for anything that
 *      does not fit a 32-bit long, and it silently truncates in expressions that
 *      expected a floating-point result: round(x)/2 is integer division.
 *
 *   2. IT EVALUATES ITS ARGUMENT TWICE. Once for the >=0 test, once in the
 *      branch taken. round(readSensor()) reads the sensor twice and rounds the
 *      second reading. round(i++) increments twice. This is the dangerous one,
 *      because the result usually still looks plausible.
 *
 *   3. THE NAME round IS NOW UNUSABLE. A variable, a struct member, a method or
 *      a std::round call named round expands into the macro body and produces a
 *      wall of syntax errors far from the real cause.
 *
 * What the macro gets RIGHT: the rounding rule. Both the macro and C99 round()
 * round halves away from zero (2.5 -> 3, -2.5 -> -3), which is NOT what most
 * other languages do and not what rint()/nearbyint() do -- those round half to
 * even under the default rounding mode. So the value is usually right; the type
 * and the side effects are the traps.
 *
 * ON THIS TOOLCHAIN double IS 64-BIT. xc-dsc v4.00 reports
 * __DBL_MANT_DIG__ == 53 and sizeof(double) == 8, so there is no float/double
 * confusion to worry about on top of the above, and the header's own
 * "map every math function onto its f-suffixed variant" path is inactive.
 * Classic XC16 defaulted to 32-bit double, so a sketch moved from there gains
 * precision rather than losing it.
 *
 * Serial Monitor at 115200.
 *
 * Board: Tools > Board > "Arduino_dsPIC33CK (dsPIC33CK256MC005 Curiosity Nano)"
 */

#include <Arduino.h>

#if defined(LED_BUILTIN_ACTIVE_LOW) && LED_BUILTIN_ACTIVE_LOW
  #define LED_ON   LOW
  #define LED_OFF  HIGH
#else
  #define LED_ON   HIGH
  #define LED_OFF  LOW
#endif

/* Escape hatch 1: wrap the name in parentheses. `round` is then not followed by
 * `(`, so the function-like macro does not expand, and the call resolves to the
 * libm function -- which returns double. This is the cheapest fix and it needs
 * no #undef, so the macro stays available elsewhere in the file. */
static double libmRound(double v)
{
    return (round)(v);
}

/* Escape hatch 2: a small named helper, so intent is explicit at every call
 * site and nobody has to remember the parenthesis trick. One evaluation of v,
 * because v is a parameter. */
static long roundToLong(double v)
{
    return (long)(round)(v);
}

/* Escape hatch 3: lround() is C99's "round and convert to long" in one step. It
 * is a real function, so it evaluates its argument once. This is what the macro
 * was trying to be. */
static long lroundHelper(double v)
{
    return lround(v);
}

/* Out of range for a 32-bit long. volatile so the value reaches the macro at run
 * time instead of being folded at compile time. */
static volatile double g_big = 3.0e9;

/* A function with an observable side effect, to make double evaluation visible. */
static unsigned int g_calls = 0;

static double nextSample(void)
{
    g_calls++;
    return 2.5;
}

static void compareOne(const char *label, double v)
{
    Serial.print("  ");
    Serial.print(label);
    Serial.print("  macro=");
    Serial.print(round(v));              /* long */
    Serial.print("   libm=");
    Serial.print(libmRound(v), 1);       /* double */
    Serial.print("   lround=");
    Serial.print(lroundHelper(v));
    Serial.print("   trunc=");
    Serial.print((long)v);
    Serial.print("   rint=");
    Serial.println(rint(v), 1);
}

static void sectionValues(void)
{
    Serial.println();
    Serial.println("-- 1. the rounding rule -------------------------------");
    Serial.println("   macro and libm agree: halves go AWAY from zero.");
    Serial.println("   rint() differs: it rounds halves to EVEN.");
    Serial.println();

    compareOne("+2.4 ", 2.4);
    compareOne("+2.5 ", 2.5);
    compareOne("+2.6 ", 2.6);
    compareOne("+3.5 ", 3.5);
    compareOne("-2.4 ", -2.4);
    compareOne("-2.5 ", -2.5);
    compareOne("-2.6 ", -2.6);
    compareOne(" 0.5 ", 0.5);
    compareOne("-0.5 ", -0.5);
}

static void sectionReturnType(void)
{
    const double v = 7.5;

    Serial.println();
    Serial.println("-- 2. the return type is long ------------------------");

    /* The macro's result is a long, so dividing it divides integers. The libm
     * form keeps the fractional part. Same source text, different arithmetic. */
    Serial.print  ("   round(7.5)/2   with the macro : ");
    Serial.println(round(v) / 2);                 /* 8/2 = 4  (long) */

    Serial.print  ("   round(7.5)/2   with libm      : ");
    Serial.println(libmRound(v) / 2, 2);          /* 8.0/2 = 4.00 */

    /* Integer division inside the argument is a classic: the division happens
     * before round() ever sees a value, so there is nothing left to round. */
    Serial.print  ("   round(7/2)     (int division) : ");
    Serial.println(round(7 / 2));                 /* round(3) = 3, not 4 */

    Serial.print  ("   round(7.0/2)   (real division): ");
    Serial.println(round(7.0 / 2));               /* round(3.5) = 4 */

    /* Out of long's range the (long) cast in the macro is undefined behaviour.
     * libm round() stays in double and is well defined. Printed side by side so
     * the difference is concrete rather than theoretical. Kept in a variable so
     * the conversion happens at run time -- as a literal the compiler folds it
     * and warns about the overflow instead, which is the friendlier outcome but
     * not the one a real sketch gets. */
    Serial.print  ("   round(big)     macro (UB!)    : ");
    Serial.println(round(g_big));
    Serial.print  ("   round(big)     libm  3.0e9    : ");
    Serial.println(libmRound(g_big), 0);
}

static void sectionDoubleEval(void)
{
    long r;

    Serial.println();
    Serial.println("-- 3. the argument is evaluated twice ----------------");

    g_calls = 0;
    r = round(nextSample());
    Serial.print  ("   round(nextSample())      -> ");
    Serial.print(r);
    Serial.print  ("   nextSample() ran ");
    Serial.print(g_calls);
    Serial.println(" time(s)   <-- expected 1");

    g_calls = 0;
    r = lroundHelper(nextSample());
    Serial.print  ("   lround(nextSample())     -> ");
    Serial.print(r);
    Serial.print  ("   nextSample() ran ");
    Serial.print(g_calls);
    Serial.println(" time(s)");

    /* The same shape with ++ is worse, because the two evaluations see
     * different values and the *second* one is the one that gets rounded. */
    {
        int i = 0;
        long got = round((double)i++);
        Serial.print  ("   round((double)i++)       -> ");
        Serial.print(got);
        Serial.print  ("   i ended at ");
        Serial.print(i);
        Serial.println("           <-- expected 1");
    }

    Serial.println();
    Serial.println("   RULE: never put a call, ++, -- or an assignment inside");
    Serial.println("   round(). Assign to a local first, or use lround().");
}

static void sectionNameClash(void)
{
    Serial.println();
    Serial.println("-- 4. the name is taken ------------------------------");
    Serial.println("   These all fail to compile while the macro is live:");
    Serial.println("     int round = 3;                  // 'int ((3)>=0? ...'");
    Serial.println("     struct S { long round; };");
    Serial.println("     double y = std::round(x);       // <cmath> is no help");
    Serial.println("     myMath.round(x);                // a method named round");
    Serial.println();
    Serial.println("   Fixes, in order of preference:");
    Serial.println("     (round)(x)      parenthesised name, macro not expanded");
    Serial.println("     lround(x)       C99, one evaluation, returns long");
    Serial.println("     #undef round    after #include <Arduino.h>, whole file");
    Serial.println();
    Serial.print  ("   roundToLong(2.5) via (round)() = ");
    Serial.println(roundToLong(2.5));
}

static void sectionOtherMacros(void)
{
    int a = 5;
    int b = 4;
    int m;

    Serial.println();
    Serial.println("-- 5. round() is not alone ---------------------------");
    Serial.println("   Arduino.h defines these the same way, with the same");
    Serial.println("   double-evaluation trap:");
    Serial.println("     min max abs constrain sq radians degrees");
    Serial.println();

    Serial.print  ("   sq(2.5)                  = ");
    Serial.println(sq(2.5), 2);

    /* max(a,b) is ((a)>(b)?(a):(b)), so when the left operand wins it is
     * evaluated a second time -- and the SECOND value is what comes back.
     * a=5, b=4: the comparison sees 5, the result is 6, and a ends at 7. */
    m = max(a++, b);
    Serial.print  ("   max(a++, b)   a was 5, b 4 -> ");
    Serial.print(m);
    Serial.print  ("        <-- max(5,4) returned ");
    Serial.println(m);
    Serial.print  ("                  a ended at ");
    Serial.print(a);
    Serial.println("        <-- incremented twice");
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" round() on Arduino_dsPIC33CK: macro, not function");
    Serial.println("======================================================");

    Serial.print  ("sizeof(double)     : ");
    Serial.println((unsigned long)sizeof(double));
    Serial.print  ("double mantissa    : ");
    Serial.print(__DBL_MANT_DIG__);
    Serial.println(" bits");
    Serial.print  ("sizeof(long)       : ");
    Serial.println((unsigned long)sizeof(long));
    Serial.print  ("PI as stored       : ");
    Serial.println(PI, 9);

    sectionValues();
    sectionReturnType();
    sectionDoubleEval();
    sectionNameClash();
    sectionOtherMacros();

    Serial.println();
    Serial.println("------------------------------------------------------");
    Serial.println("done - LED0 now blinks so you know the sketch is live");
}

void loop()
{
    /* Nothing left to say; blink so a silent monitor is distinguishable from a
     * dead board. */
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(100);
    digitalWrite(LED_BUILTIN, LED_OFF);
    delay(1400);
}
