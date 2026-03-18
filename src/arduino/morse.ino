// morse.ino — Arduino morse blinker
// Receives serial commands at 9600 baud:
//   S:<wpm>\n  — set speed (5-25 WPM)
//   M:<text>\n — blink text in morse on LED_BUILTIN

static const char* MORSE_TABLE[] = {
    // A-Z (index 0-25)
    ".-",    "-...",  "-.-.",  "-..",   ".",     "..-.",  "--.",
    "....",  "..",    ".---",  "-.-",   ".-..",  "--",    "-.",
    "---",   ".--.",  "--.-",  ".-.",   "...",   "-",     "..-",
    "...-",  ".--",   "-..-",  "-.--",  "--..",
    // 0-9 (index 26-35)
    "-----", ".----", "..---", "...--", "....-",
    ".....", "-....", "--...", "---..", "----.",
    // Punctuation (index 36+): . , ? ! / ( ) & : ; = + - _ " $ @
    ".-.-.-", "--..--", "..--..", "-.-.--", "-..-.",
    "-.--.",  "-.--.-", ".-...",  "---...",  "-.-.-.",
    "-...-",  ".-.-.",  "-....-", ".-..-.",  "...-..-", ".--.-."
};

// Maps ASCII char to MORSE_TABLE index; returns -1 if unknown
static int morse_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return 26 + (c - '0');
    switch (c) {
        case '.': return 36; case ',': return 37; case '?': return 38;
        case '!': return 39; case '/': return 40; case '(': return 41;
        case ')': return 42; case '&': return 43; case ':': return 44;
        case ';': return 45; case '=': return 46; case '+': return 47;
        case '-': return 48; case '_': return 49; case '"': return 50;
        case '$': return 51; case '@': return 52;
        default:  return -1;
    }
}

static int g_wpm = 15;
static bool g_busy = false;

static void blink_morse(const char* text, int wpm) {
    if (wpm < 5)  wpm = 5;
    if (wpm > 25) wpm = 25;
    int dot_ms = 1200 / wpm;

    g_busy = true;
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c == ' ') {
            // inter-word gap = 7 dots; 3 already elapsed as inter-char after prev char
            delay(4 * dot_ms);
            continue;
        }
        int idx = morse_index(c);
        if (idx < 0) continue;

        const char* sym = MORSE_TABLE[idx];
        for (int j = 0; sym[j] != '\0'; j++) {
            digitalWrite(LED_BUILTIN, HIGH);
            if (sym[j] == '.') {
                delay(dot_ms);
            } else {
                delay(3 * dot_ms);
            }
            digitalWrite(LED_BUILTIN, LOW);
            if (sym[j + 1] != '\0') {
                delay(dot_ms);  // intra-char gap
            }
        }
        // inter-char gap = 3 dots
        delay(3 * dot_ms);
    }
    g_busy = false;
}

void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
    if (Serial.available() == 0) return;
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() < 3) return;

    if (line.startsWith("S:")) {
        int wpm = line.substring(2).toInt();
        if (wpm >= 5 && wpm <= 25) g_wpm = wpm;
        return;
    }

    if (line.startsWith("M:")) {
        if (g_busy) return;  // ignore while blinking
        String text = line.substring(2);
        text.toUpperCase();
        blink_morse(text.c_str(), g_wpm);
        return;
    }
}
