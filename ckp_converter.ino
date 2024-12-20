const int ckpInputPin = 2; // Input pin for the 60-2 trigger wheel signal

// Configurable constants
const uint8_t maxToothCount = 60;       // Total number of teeth on the trigger wheel
const uint8_t missingToothCount = 2;    // Number of missing teeth
const uint8_t hallWindow = 60;          // Distributor hall window to emulate in degrees
const uint8_t teethWindow = hallWindow / (360.0 / maxToothCount); // Number of teeth in one hall window
const uint8_t tdcTeeth = 15;            // Top Dead Center (TDC) tooth position for cylinder 1
const uint8_t intervalMultiplier = 2 + (missingToothCount / 2); // Threshold multiplier (e.g., 150% for 60-2)

// Timing variables (using 32-bit integers for overflow safety)
volatile unsigned long lastToothTime = 0;  // Timestamp of the last detected tooth
volatile unsigned long currentToothTime = 0; // Timestamp of the current tooth
volatile unsigned long toothInterval = 0; // Interval between teeth (in microseconds)
volatile unsigned long maxIntervalThreshold = 0; // Max interval for a valid signal (computed dynamically)

// Tooth counting
volatile uint8_t toothCount = 0; // Tracks the current tooth position
volatile uint8_t lastSync = 11;  // Sync counter for handling the missing teeth

// CKP (Crankshaft Position) output state
volatile bool ckpOutputState = false; // Tracks the current state of the CKP output

void setup() {
    pinMode(ckpInputPin, INPUT);        // Configure the input pin for the CKP signal
    DDRB |= (1 << DDB0);               // Set PB0 (Pin 8) as an output pin
    attachInterrupt(digitalPinToInterrupt(ckpInputPin), toothDetected, RISING); // Attach interrupt to CKP signal
}

void loop() {
    // Generate a HIGH signal when the tooth count is within the hall window
    if (toothCount >= tdcTeeth && toothCount < tdcTeeth + teethWindow && !ckpOutputState) {
        PORTB |= (1 << PB0); // Set PB0 HIGH (ON)
        ckpOutputState = true;
    } 
    // Generate a LOW signal when the tooth count is within the second hall window (180° later)
    else if (toothCount >= tdcTeeth + (maxToothCount / 2) && 
             toothCount < tdcTeeth + (maxToothCount / 2) + teethWindow && 
             ckpOutputState) {
        PORTB &= ~(1 << PB0); // Set PB0 LOW (OFF)
        ckpOutputState = false;
    }
}

void toothDetected() {
    currentToothTime = micros(); // Record the current time (in microseconds)

    if (lastToothTime > 0) {
        unsigned long interval = currentToothTime - lastToothTime; // Compute interval since last tooth

        // Detect missing teeth and synchronize
        if (lastSync >= 10 && interval > maxIntervalThreshold) {
            toothCount = 1; // Reset tooth count after detecting missing teeth
            lastSync = 0;   // Reset sync counter
        } 
        else if (toothCount >= (maxToothCount - missingToothCount)) {
            toothCount = 1; // Reset tooth count after a full revolution
            lastSync++;
        } 
        else {
            toothCount++; // Increment the tooth count
            toothInterval = interval; // Update the interval between teeth
        }

        // Update the maximum valid interval threshold
        maxIntervalThreshold = (toothInterval * intervalMultiplier) / 2;
    }

    lastToothTime = currentToothTime; // Update the last tooth timestamp
}