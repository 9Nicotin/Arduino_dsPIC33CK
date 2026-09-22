/*
 * NanoSerialCommand - reading FROM the Serial Monitor on the
 *                     dsPIC33CK256MC005 Curiosity Nano (EV08P02A)
 *
 * Every other example in this folder only ever prints. This one listens. It is a
 * complete non-blocking command interpreter built on the four receive functions
 * -- Serial.available(), Serial.read(), Serial.peek(), Serial.flush() -- none of
 * which had an example before, which means nothing in this repository ever proved
 * the receive path worked at all.
 *
 * WIRING: none. The Curiosity Nano's on-board debugger is also the USB-serial
 * bridge, so the cable you program with is the cable you type over.
 *
 * WHAT THIS CORE DOES NOT HAVE
 *   No String class. No Serial.parseInt(). No readStringUntil(). No
 *   readBytesUntil(), setTimeout(), find() or available ForWrite().
 *
 *   That is not an oversight to work around quietly -- it is the reason this
 *   example exists. Sketch code that expects them will not compile, so this file
 *   shows the char-buffer idiom they replace: accumulate bytes into a fixed
 *   array, terminate on a newline, then parse in place. It is what Arduino's own
 *   String-based helpers do internally, minus the heap.
 *
 * HOW TO USE IT
 *   Open the Serial Monitor at 115200, type  help  and press Enter.
 *
 *   Set the line-ending dropdown to "New Line", "Carriage Return" or "Both" --
 *   all three work. If you leave it on "No Line Ending" a command would normally
 *   never run, because nothing tells the sketch the line is finished. This sketch
 *   works around that with an idle timeout (see IDLE_SUBMIT_MS): if you stop
 *   typing for a moment, a partial line is submitted anyway. Real sketches
 *   usually should not do that -- it is here so that no dropdown setting leaves
 *   you staring at a dead prompt.
 *
 * FOUR THINGS THE RECEIVE PATH DOES THAT WILL SURPRISE YOU
 *
 * 1. The receive buffer holds 63 bytes, not 64. SERIAL_BUFFER_SIZE is 64, but a
 *    head==tail ring buffer has to sacrifice one slot to tell "empty" from
 *    "full", so available() can never return more than 63.
 *
 * 2. On overflow the NEWEST byte is dropped, not the oldest. The receive ISR
 *    checks for room first and, finding none, reads the hardware register and
 *    throws the byte away to clear the interrupt. So a burst longer than 63 bytes
 *    leaves you the START of the burst and silently loses the END -- the reverse
 *    of what you may expect, and it matters: the newline you were waiting for is
 *    the part that vanishes.
 *
 * 3. Nothing in this core ever checks or clears the hardware overrun flag. If
 *    U1STAbits.OERR ever sets, this UART's receiver stops accepting data
 *    PERMANENTLY until the bit is cleared, and no core code clears it -- so
 *    receive would be dead for the rest of the run. The "stat" command reports
 *    the flag and "clearerr" clears it. That code is worth copying into any
 *    sketch that must keep receiving no matter what.
 *
 *    How OERR happens: the receive interrupt is IPL3, but Timer1/millis() is
 *    IPL4 and tone() is IPL5, so both can hold it off. Two unread bytes arriving
 *    while it is held off is all it takes. At 115200 that is 174 us.
 *
 * 4. Serial.flush() waits for TRANSMIT to finish. It does NOT discard received
 *    input, despite the name -- that meaning was dropped from Arduino in 1.0.
 *    To throw away input you must loop on read(), which is what "clear" does.
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

/*
 * Deliberately smaller than the 63-byte receive buffer so that line overflow is
 * reachable by typing rather than only in theory. Type more than 32 characters
 * and watch what the sketch reports.
 */
#define LINE_MAX          32
#define IDLE_SUBMIT_MS    400UL

static char          g_line[LINE_MAX + 1];
static unsigned int  g_len       = 0;
static uint8_t       g_overflow  = 0;
static uint8_t       g_echo      = 1;
static unsigned long g_lastByte  = 0;
static unsigned long g_commands  = 0;
static unsigned int  g_maxSeen   = 0;   /* high-water mark of available() */

/* --- parsing, by hand, because there is no String ------------------------- */

static uint8_t isSpace(char c)
{
    return (uint8_t)((c == ' ') || (c == '\t'));
}

/* Advance *pos past any run of spaces. */
static void skipSpaces(const char *s, unsigned int *pos)
{
    while (s[*pos] != '\0' && isSpace(s[*pos])) { (*pos)++; }
}

/*
 * Compare the word at *pos against a literal. On a match, *pos is advanced past
 * the word; on no match it is left exactly where it was, so the caller can try
 * the next candidate. This is the whole of what strcmp-on-a-token needs to do.
 */
static uint8_t matchWord(const char *s, unsigned int *pos, const char *word)
{
    unsigned int i = 0;
    unsigned int p = *pos;

    while (word[i] != '\0') {
        if (s[p + i] != word[i]) { return 0U; }
        i++;
    }
    /* The literal must end at a word boundary, so "ledx" does not match "led". */
    if (s[p + i] != '\0' && !isSpace(s[p + i])) { return 0U; }

    *pos = p + i;
    return 1U;
}

/*
 * The replacement for parseInt(). Returns 1 and writes *out on success; returns 0
 * and leaves *out alone if there is no digit where a number was expected -- which
 * is the distinction parseInt() cannot make, since it returns 0 both for the
 * number zero and for "no number found".
 */
static uint8_t parseUInt(const char *s, unsigned int *pos, unsigned long *out)
{
    unsigned long v = 0;
    unsigned int  n = 0;

    skipSpaces(s, pos);
    while (s[*pos] >= '0' && s[*pos] <= '9') {
        v = (v * 10UL) + (unsigned long)(s[*pos] - '0');
        (*pos)++;
        n++;
        if (n > 9U) { return 0U; }   /* would overflow 32 bits */
    }
    if (n == 0U) { return 0U; }
    *out = v;
    return 1U;
}

/* --- commands ------------------------------------------------------------- */

static void cmdHelp(void)
{
    Serial.println("commands:");
    Serial.println("  help              this list");
    Serial.println("  led on|off|toggle drive LED_BUILTIN");
    Serial.println("  read <pin>        digitalRead a pin  (0..38)");
    Serial.println("  ain <ch>          analogRead a channel");
    Serial.println("  pwm <pin> <0-255> analogWrite  (pins 5..8 only)");
    Serial.println("  hex <n>           print n in DEC, HEX, OCT, BIN");
    Serial.println("  echo on|off       echo characters as you type");
    Serial.println("  stat              buffer + UART error status");
    Serial.println("  clear             discard pending input (flush does not)");
    Serial.println("  clearerr          clear a latched UART error flag");
    Serial.println("  peektest          show peek() not consuming a byte");
}

static void cmdStat(void)
{
    Serial.print  ("  available()      : ");
    Serial.print(Serial.available());
    Serial.println(" byte(s) waiting");
    Serial.print  ("  buffer capacity  : ");
    Serial.print(SERIAL_BUFFER_SIZE - 1);
    Serial.print  (" usable of ");
    Serial.print(SERIAL_BUFFER_SIZE);
    Serial.println(" (one slot marks 'full')");
    Serial.print  ("  high-water mark  : ");
    Serial.println(g_maxSeen);
    Serial.print  ("  commands run     : ");
    Serial.println(g_commands);
    Serial.print  ("  uptime ms        : ");
    Serial.println(millis());

    Serial.print  ("  OERR (overrun)   : ");
    Serial.println(U1STAbits.OERR ? "SET  <- receive is DEAD until cleared" : "clear");
    Serial.print  ("  FERR (framing)   : ");
    Serial.println(U1STAbits.FERR ? "SET  <- baud mismatch or noise" : "clear");
    Serial.print  ("  PERR (parity)    : ");
    Serial.println(U1STAbits.PERR ? "SET" : "clear");
    Serial.println("  The core never inspects these. A sketch that must stay");
    Serial.println("  receiving should check OERR itself, as 'clearerr' does.");
}

static void cmdClearErr(void)
{
    if (U1STAbits.OERR) {
        /*
         * Clearing OERR also empties the hardware receive FIFO, so whatever was
         * mid-flight is gone either way. Drain the software ring too, otherwise
         * the sketch parses the surviving fragment of a corrupted line.
         */
        U1STAbits.OERR = 0;
        while (Serial.read() != -1) { }
        Serial.println("  OERR cleared, FIFO and ring drained. Receive is alive.");
    } else {
        Serial.println("  nothing latched; OERR was already clear.");
    }
    U1STAbits.FERR = 0;
    U1STAbits.PERR = 0;
}

static void cmdClear(void)
{
    int n = 0;

    Serial.flush();     /* drains TX only - proves the point below */
    while (Serial.read() != -1) { n++; }
    Serial.print  ("  flush() waited for TX; the RX discard took a read() loop: ");
    Serial.print(n);
    Serial.println(" byte(s) thrown away.");
}

static void cmdPeekTest(void)
{
    int a;
    int b;
    int c;

    Serial.println("  send one character within 3 s (no Enter needed)...");
    {
        unsigned long deadline = millis() + 3000UL;
        while (Serial.available() == 0 && (long)(millis() - deadline) < 0L) { }
    }
    if (Serial.available() == 0) {
        Serial.println("  nothing arrived; test skipped.");
        return;
    }

    a = Serial.peek();
    b = Serial.peek();
    c = Serial.read();

    Serial.print  ("  peek() -> ");
    Serial.print(a);
    Serial.print  (", peek() again -> ");
    Serial.print(b);
    Serial.print  (", read() -> ");
    Serial.println(c);
    Serial.println(a == b && b == c
        ? "  Same byte all three times: peek() does not consume. PASS"
        : "  Mismatch - unexpected; another byte arrived mid-test.");

    /* Whatever line ending followed it would otherwise become a stray command. */
    while (Serial.read() != -1) { }
}

static void cmdRead(const char *s, unsigned int *pos)
{
    unsigned long pin;

    if (!parseUInt(s, pos, &pin)) {
        Serial.println("  usage: read <pin>");
        return;
    }
    if (pin >= NUM_DIGITAL_PINS) {
        Serial.print  ("  pin out of range; this variant has 0..");
        Serial.println(NUM_DIGITAL_PINS - 1);
        return;
    }
    if (pin == 10UL || pin == 11UL) {
        Serial.println("  D10/D11 are RB5/RB6 = the debugger's PGD3/PGC3.");
        Serial.println("  Reading them is harmless but driving them is not.");
    }
    pinMode((uint8_t)pin, INPUT_PULLUP);
    Serial.print  ("  D");
    Serial.print((unsigned int)pin);
    Serial.print  (" (INPUT_PULLUP) = ");
    Serial.println(digitalRead((uint8_t)pin) == HIGH ? "HIGH" : "LOW");
}

static void cmdAin(const char *s, unsigned int *pos)
{
    unsigned long ch;
    int raw;

    if (!parseUInt(s, pos, &ch)) {
        Serial.println("  usage: ain <channel>");
        return;
    }
    raw = analogRead((uint8_t)ch);
    Serial.print  ("  analogRead(");
    Serial.print((unsigned int)ch);
    Serial.print  (") = ");
    Serial.print(raw);
    Serial.print  ("  (");
    /* 12-bit, 3.3 V reference: mV without floating point. */
    Serial.print(((unsigned long)raw * 3300UL) / 4095UL);
    Serial.println(" mV)");
}

static void cmdPwm(const char *s, unsigned int *pos)
{
    unsigned long pin;
    unsigned long duty;

    if (!parseUInt(s, pos, &pin) || !parseUInt(s, pos, &duty)) {
        Serial.println("  usage: pwm <pin> <0-255>");
        return;
    }
    if (!digitalPinHasPWM(pin)) {
        Serial.println("  analogWrite() works on D5..D8 on this variant only.");
        return;
    }
    if (duty > 255UL) { duty = 255UL; }
    analogWrite((uint8_t)pin, (uint8_t)duty);
    Serial.print  ("  analogWrite(");
    Serial.print((unsigned int)pin);
    Serial.print  (", ");
    Serial.print((unsigned int)duty);
    Serial.println(") done");
    if (pin == 8UL) {
        Serial.println("  note: D8 is SCCP4, which tone() also borrows. See NanoTone.");
    }
}

static void cmdHex(const char *s, unsigned int *pos)
{
    unsigned long v;

    if (!parseUInt(s, pos, &v)) {
        Serial.println("  usage: hex <number>");
        return;
    }
    Serial.print  ("  DEC ");   Serial.print(v, DEC);
    Serial.print  ("   HEX ");  Serial.print(v, HEX);
    Serial.print  ("   OCT ");  Serial.print(v, OCT);
    Serial.print  ("   BIN ");  Serial.println(v, BIN);
}

/* --- dispatch ------------------------------------------------------------- */

static void runLine(char *s)
{
    unsigned int pos = 0;

    skipSpaces(s, &pos);
    if (s[pos] == '\0') { return; }     /* a bare Enter is not an error */

    g_commands++;

    if      (matchWord(s, &pos, "help"))     { cmdHelp(); }
    else if (matchWord(s, &pos, "stat"))     { cmdStat(); }
    else if (matchWord(s, &pos, "clearerr")) { cmdClearErr(); }
    else if (matchWord(s, &pos, "clear"))    { cmdClear(); }
    else if (matchWord(s, &pos, "peektest")) { cmdPeekTest(); }
    else if (matchWord(s, &pos, "read"))     { cmdRead(s, &pos); }
    else if (matchWord(s, &pos, "ain"))      { cmdAin(s, &pos); }
    else if (matchWord(s, &pos, "pwm"))      { cmdPwm(s, &pos); }
    else if (matchWord(s, &pos, "hex"))      { cmdHex(s, &pos); }
    else if (matchWord(s, &pos, "led")) {
        skipSpaces(s, &pos);
        if      (matchWord(s, &pos, "on"))  { digitalWrite(LED_BUILTIN, LED_ON);
                                              Serial.println("  LED on"); }
        else if (matchWord(s, &pos, "off")) { digitalWrite(LED_BUILTIN, LED_OFF);
                                              Serial.println("  LED off"); }
        else if (matchWord(s, &pos, "toggle")) {
            digitalWrite(LED_BUILTIN,
                digitalRead(LED_BUILTIN) == LED_ON ? LED_OFF : LED_ON);
            Serial.println("  LED toggled");
        }
        else { Serial.println("  usage: led on|off|toggle"); }
    }
    else if (matchWord(s, &pos, "echo")) {
        skipSpaces(s, &pos);
        if      (matchWord(s, &pos, "on"))  { g_echo = 1U; Serial.println("  echo on"); }
        else if (matchWord(s, &pos, "off")) { g_echo = 0U; Serial.println("  echo off"); }
        else { Serial.println("  usage: echo on|off"); }
    }
    else {
        Serial.print  ("  unknown command: ");
        Serial.println(s);
        Serial.println("  type 'help'");
    }
}

static void prompt(void)
{
    Serial.print("> ");
}

/*
 * Pull whatever has arrived out of the ring buffer and assemble a line.
 *
 * Never blocks and never calls delay(): the 63-byte buffer is only about 5.5 ms
 * of traffic at 115200, so a handler that waits is a handler that loses bytes.
 * Read greedily, act later.
 */
static void pumpSerial(void)
{
    int  n = Serial.available();
    int  ch;
    char c;

    if (n > (int)g_maxSeen) { g_maxSeen = (unsigned int)n; }

    while ((ch = Serial.read()) != -1) {
        c = (char)ch;
        g_lastByte = millis();

        if (c == '\r' || c == '\n') {
            /*
             * Handles LF, CR and CRLF without a state machine: the second
             * character of a CRLF pair simply terminates an already-empty line,
             * and an empty line is ignored by runLine().
             */
            if (g_echo) { Serial.println(); }

            if (g_overflow) {
                Serial.print  ("  line too long (>");
                Serial.print(LINE_MAX);
                Serial.println(" chars) - discarded, nothing was executed.");
                Serial.println("  A real protocol should say this too rather than");
                Serial.println("  silently acting on a truncated command.");
                g_overflow = 0U;
            } else {
                g_line[g_len] = '\0';
                runLine(g_line);
            }
            g_len = 0;
            prompt();
            continue;
        }

        if (c == 8 || c == 127) {                 /* backspace / delete */
            if (g_len > 0U && !g_overflow) {
                g_len--;
                if (g_echo) { Serial.print("\b \b"); }
            }
            continue;
        }

        if (c < 32) { continue; }                 /* ignore other control bytes */

        if (g_len >= LINE_MAX) {
            /* Keep consuming to the end of the line; do not act on a fragment. */
            g_overflow = 1U;
            continue;
        }
        g_line[g_len++] = c;
        if (g_echo) { Serial.print(c); }
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);

    Serial.begin(115200);

    Serial.println();
    Serial.println("======================================================");
    Serial.println(" Serial command interpreter - dsPIC33CK256MC005");
    Serial.println("======================================================");
    Serial.print  ("receive buffer : ");
    Serial.print(SERIAL_BUFFER_SIZE - 1);
    Serial.print  (" usable bytes (SERIAL_BUFFER_SIZE = ");
    Serial.print(SERIAL_BUFFER_SIZE);
    Serial.println(")");
    Serial.print  ("line buffer    : ");
    Serial.print(LINE_MAX);
    Serial.println(" chars - type more than that to see overflow handled");
    Serial.println("no String, no parseInt(), no readStringUntil() in this core");
    Serial.println();
    Serial.println("type 'help' and press Enter");
    Serial.println();
    prompt();
}

void loop()
{
    pumpSerial();

    /*
     * Escape hatch for the Serial Monitor's "No Line Ending" setting: submit a
     * line that has been sitting untouched. Most sketches want a real terminator
     * instead -- a timeout cannot tell a finished command from a slow typist.
     */
    if (g_len > 0U && (millis() - g_lastByte) > IDLE_SUBMIT_MS) {
        if (g_echo) { Serial.println("   <- submitted on idle timeout"); }
        g_line[g_len] = '\0';
        runLine(g_line);
        g_len = 0;
        prompt();
    }

    /*
     * A latched overrun kills receive for good, so notice it here rather than
     * waiting for the user to wonder why the board went quiet.
     */
    if (U1STAbits.OERR) {
        Serial.println();
        Serial.println("!! U1STAbits.OERR is set: no further bytes will be");
        Serial.println("!! received until it is cleared. Clearing it now.");
        U1STAbits.OERR = 0;
        while (Serial.read() != -1) { }
        g_len = 0;
        g_overflow = 0U;
        prompt();
    }
}
