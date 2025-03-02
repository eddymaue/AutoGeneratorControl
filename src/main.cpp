// Include necessary Arduino libraries
#include <Arduino.h>
#include <string.h>

// Define pin numbers for relays and inputs
#define powerOnRelayPin 7  // Relay for powering on the generator (Relay 1)
#define chokeRelayPin 6    // Relay for choke (Relay 2)
#define startRelayPin 5    // Relay for starting the generator (Relay 3)
#define atsRelayPin 4      // Relay for ATS contactor (Relay 4)

// Generator running detection
#define genCheckOutPin 12   // Sends signal to check if generator is running
#define genCheckInPin 13    // Receives signal (pulls LOW) if generator is running

// Grid presence detection
#define gridCheckOutPin 11  // Sends signal to check if grid is present
#define gridCheckInPin 10   // Receives signal (pulls LOW) if grid is present

// Voltage monitoring
#define genVoltagePin A0    // Analog input for generator voltage monitoring
#define gridVoltagePin A1   // Analog input for grid voltage monitoring
#define unTest A2
#define unTest1 A3
#define untest2 A4

// State machine enum
enum GeneratorState {
  IDLE,                     // Normal grid operation
  GRID_LOSS_DETECTED,       // Grid loss detected, waiting before starting
  START_POWER_ON,           // Power on relay activated
  START_CHOKE_ON,           // Choke relay activated
  START_CRANKING,           // Starter relay activated
  START_CHOKE_OFF,          // Choke relay deactivated
  CHECK_RUNNING,            // Checking if generator started
  RUNNING_WAIT_ATS,         // Generator running, waiting to connect ATS
  RUNNING_WITH_ATS,         // Generator running and powering the load
  GRID_RESTORED_WAIT,       // Grid restored, waiting before shutdown
  COOLING_DOWN,             // Generator running but ATS disengaged
  SHUTTING_DOWN             // Final shutdown phase
};

// Timing variables
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long gridLossTime = 0;            // Timestamp when grid is lost
unsigned long gridRestoreTime = 0;         // Timestamp when grid returns
unsigned long stateStartTime = 0;          // Timestamp when current state started

// Timing constants
const unsigned long chokeOnTime = 3500;           // Choke stays on for 3.5 seconds
const unsigned long atsDelayTime = 120000;        // 2 minutes delay AFTER choke turns off
const unsigned long generatorRunTime = 14400000;  // 4 hours of running time
const unsigned long shutdownDelayTime = 120000;   // 2 minutes delay for shutdown
const unsigned long gridLossWaitTime = 300000;    // 5 minutes wait after grid is lost
const unsigned long startCheckDelay = 5000;       // 5 seconds to check if generator started
const unsigned long startRetryDelay = 5000;       // 5 seconds between start attempts
const unsigned long debounceDelay = 50;           // Debounce delay in milliseconds
const unsigned long logInterval = 60000;          // Log status every minute

// State variables
bool atsEngaged = false;
bool generatorRunning = false;
bool gridWasLost = false;

// Debounce variables
bool lastGridState = false;
bool lastGenState = false;
unsigned long lastGridDebounceTime = 0;
unsigned long lastGenDebounceTime = 0;
bool gridStateStable = false;
bool genStateStable = false;
bool gridPresent = false;
bool generatorConfirmed = false;

// Voltage monitoring variables
const float minGenVoltage = 210.0;  // Minimum generator voltage to engage ATS (adjust as needed)
const float voltageFactor = 0.465;  // Voltage divider calibration factor (adjust based on your voltage divider)
float genVoltage = 0.0;
float gridVoltage = 0.0;

// Logging variables
unsigned long lastLogTime = 0;
int eventLogIndex = 0;
const int maxEventLogs = 10;
char eventLog[10][50];  // Circular buffer for storing the last 10 events

// Start retry variables
int startAttempts = 0;
const int maxStartAttempts = 3;

GeneratorState currentState = IDLE;

// Function prototypes
void changeState(GeneratorState newState);
void logEvent(const char* event);
float readVoltage(int pin);
void printEventLog();

// Helper function to read voltage from analog pin
float readVoltage(int pin) {
  // Read analog value (0-1023)
  int rawValue = analogRead(pin);
  
  // Convert to voltage (assuming voltage divider calibrated for mains voltage)
  // The voltageFactor should be calibrated based on your voltage divider
  float voltage = rawValue * voltageFactor;
  
  return voltage;
}

// Helper function to log events
void logEvent(const char* event) {
  // Get current timestamp
  unsigned long currentTime = millis();
  
  // Format the timestamp as hours:minutes:seconds
  unsigned long seconds = currentTime / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  // Print to serial
  Serial.print(hours % 24);
  Serial.print(":");
  Serial.print(minutes % 60);
  Serial.print(":");
  Serial.print(seconds % 60);
  Serial.print(" - ");
  Serial.println(event);
  
  // Store in circular buffer
  strncpy(eventLog[eventLogIndex], event, 49);
  eventLog[eventLogIndex][49] = '\0';  // Ensure null termination
  eventLogIndex = (eventLogIndex + 1) % maxEventLogs;
}

// Helper function to change states and log
void changeState(GeneratorState newState) {
  currentState = newState;
  stateStartTime = millis();
  
  char stateBuffer[30];
  
  switch(newState) {
    case IDLE: 
      strcpy(stateBuffer, "IDLE"); 
      break;
    case GRID_LOSS_DETECTED: 
      strcpy(stateBuffer, "GRID_LOSS_DETECTED"); 
      break;
    case START_POWER_ON: 
      strcpy(stateBuffer, "START_POWER_ON"); 
      break;
    case START_CHOKE_ON: 
      strcpy(stateBuffer, "START_CHOKE_ON"); 
      break;
    case START_CRANKING: 
      strcpy(stateBuffer, "START_CRANKING"); 
      break;
    case START_CHOKE_OFF: 
      strcpy(stateBuffer, "START_CHOKE_OFF"); 
      break;
    case CHECK_RUNNING: 
      strcpy(stateBuffer, "CHECK_RUNNING"); 
      break;
    case RUNNING_WAIT_ATS: 
      strcpy(stateBuffer, "RUNNING_WAIT_ATS"); 
      break;
    case RUNNING_WITH_ATS: 
      strcpy(stateBuffer, "RUNNING_WITH_ATS"); 
      break;
    case GRID_RESTORED_WAIT: 
      strcpy(stateBuffer, "GRID_RESTORED_WAIT"); 
      break;
    case COOLING_DOWN: 
      strcpy(stateBuffer, "COOLING_DOWN"); 
      break;
    case SHUTTING_DOWN: 
      strcpy(stateBuffer, "SHUTTING_DOWN"); 
      break;
  }
  
  char logBuffer[50];
  sprintf(logBuffer, "Changed to state: %s", stateBuffer);
  logEvent(logBuffer);
}

// Helper function to print the event log
void printEventLog() {
  Serial.println("=== EVENT LOG ===");
  
  // Print from oldest to newest
  for (int i = 0; i < maxEventLogs; i++) {
    int index = (eventLogIndex + i) % maxEventLogs;
    if (eventLog[index][0] != '\0') {  // Only print non-empty entries
      Serial.print(i);
      Serial.print(": ");
      Serial.println(eventLog[index]);
    }
  }
  
  Serial.println("================");
}

void setup() {
  Serial.begin(9600);

  pinMode(powerOnRelayPin, OUTPUT);
  pinMode(chokeRelayPin, OUTPUT);
  pinMode(startRelayPin, OUTPUT);
  pinMode(atsRelayPin, OUTPUT);
  pinMode(genCheckOutPin, OUTPUT);
  pinMode(genCheckInPin, INPUT_PULLUP);  // Changed from INPUT_PULLDOWN to INPUT_PULLUP
  pinMode(gridCheckOutPin, OUTPUT);
  pinMode(gridCheckInPin, INPUT_PULLUP); // Changed from INPUT_PULLDOWN to INPUT_PULLUP
  pinMode(genVoltagePin, INPUT);
  pinMode(gridVoltagePin, INPUT);

  // Initialize all relays to OFF (LOW)
  digitalWrite(powerOnRelayPin, LOW);
  digitalWrite(chokeRelayPin, LOW);
  digitalWrite(startRelayPin, LOW);
  digitalWrite(atsRelayPin, LOW);
  
  // Turn on sensing pins
  digitalWrite(genCheckOutPin, HIGH);
  digitalWrite(gridCheckOutPin, HIGH);

  // Initialize event log
  logEvent("System initialized. State: IDLE");
}

void loop() {
  currentMillis = millis();
  
  // Read sensor states with debouncing (inverted logic due to INPUT_PULLUP)
  bool gridReading = !digitalRead(gridCheckInPin);  // LOW means signal present with PULLUP
  bool genReading = !digitalRead(genCheckInPin);    // LOW means signal present with PULLUP
  
  // Debounce grid state
  if (gridReading != lastGridState) {
    lastGridDebounceTime = currentMillis;
    lastGridState = gridReading;
    gridStateStable = false;
  }
  
  if ((currentMillis - lastGridDebounceTime) > debounceDelay && !gridStateStable) {
    gridStateStable = true;
    if (gridReading != gridPresent) {
      char logBuffer[50];
      sprintf(logBuffer, "Grid state changed to: %s", gridReading ? "PRESENT" : "ABSENT");
      logEvent(logBuffer);
    }
  }
  
  // Debounce generator state
  if (genReading != lastGenState) {
    lastGenDebounceTime = currentMillis;
    lastGenState = genReading;
    genStateStable = false;
  }
  
  if ((currentMillis - lastGenDebounceTime) > debounceDelay && !genStateStable) {
    genStateStable = true;
    if (genReading != generatorConfirmed) {
      char logBuffer[50];
      sprintf(logBuffer, "Generator state changed to: %s", genReading ? "RUNNING" : "STOPPED");
      logEvent(logBuffer);
    }
  }
  
  // Update states only if they're stable after debouncing
  if (gridStateStable) {
    gridPresent = lastGridState;
  }
  
  if (genStateStable) {
    generatorConfirmed = lastGenState;
  }
  
  // Read voltage levels
  genVoltage = readVoltage(genVoltagePin);
  gridVoltage = readVoltage(gridVoltagePin);
  
  // Periodic logging of system status
  if (currentMillis - lastLogTime >= logInterval) {
    lastLogTime = currentMillis;
    char statusBuffer[50];
    sprintf(statusBuffer, "State: %d, Grid: %d, Gen: %d, V: %.1f", 
            currentState, gridPresent, generatorConfirmed, genVoltage);
    logEvent(statusBuffer);
  }

  // State machine
  switch (currentState) {
    
    case IDLE:
      // Check for grid loss
      if (!gridPresent && !gridWasLost) {
        logEvent("Grid power lost. Starting timer...");
        gridLossTime = currentMillis;
        gridWasLost = true;
        changeState(GRID_LOSS_DETECTED);
      }
      break;

    case GRID_LOSS_DETECTED:
      // Wait for 5 minutes to confirm grid loss is not temporary
      if (currentMillis - gridLossTime >= gridLossWaitTime) {
        startAttempts = 0;
        changeState(START_POWER_ON);
      } else if (gridPresent) {
        // Grid came back during wait period
        logEvent("Grid restored during wait period. Returning to IDLE.");
        gridWasLost = false;
        changeState(IDLE);
      }
      break;

    case START_POWER_ON:
      // Power on the generator
      digitalWrite(powerOnRelayPin, HIGH);
      logEvent("Powering on generator...");
      
      // Wait 1 second then move to next state
      if (currentMillis - stateStartTime >= 1000) {
        changeState(START_CHOKE_ON);
      }
      break;

    case START_CHOKE_ON:
      // Turn on the choke
      digitalWrite(chokeRelayPin, HIGH);
      
      // Wait 3 seconds then move to cranking
      if (currentMillis - stateStartTime >= 3000) {
        changeState(START_CRANKING);
      }
      break;

    case START_CRANKING:
      // Activate the starter
      digitalWrite(startRelayPin, HIGH);
      
      // Crank for 3 seconds
      if (currentMillis - stateStartTime >= 3000) {
        digitalWrite(startRelayPin, LOW);
        changeState(START_CHOKE_OFF);
      }
      break;

    case START_CHOKE_OFF:
      // Keep choke on for specified time
      if (currentMillis - stateStartTime >= chokeOnTime) {
        digitalWrite(chokeRelayPin, LOW);
        changeState(CHECK_RUNNING);
      }
      break;

    case CHECK_RUNNING:
      // Check if generator is running after a delay
      if (currentMillis - stateStartTime >= startCheckDelay) {
        if (generatorConfirmed) {
          logEvent("Generator started successfully!");
          generatorRunning = true;
          changeState(RUNNING_WAIT_ATS);
        } else {
          startAttempts++;
          if (startAttempts < maxStartAttempts) {
            logEvent("Generator failed to start, retrying...");
            // Wait a bit before trying again
            if (currentMillis - stateStartTime >= startCheckDelay + startRetryDelay) {
              changeState(START_POWER_ON);
            }
          } else {
            logEvent("Generator failed to start after multiple attempts! Initiating shutdown...");
            changeState(SHUTTING_DOWN);
          }
        }
      }
      break;

    case RUNNING_WAIT_ATS:
      // Wait for specified time before engaging ATS
      if (currentMillis - stateStartTime >= atsDelayTime) {
        // Check generator voltage before engaging ATS
        if (genVoltage >= minGenVoltage) {
          logEvent("Generator voltage OK. Engaging ATS...");
          digitalWrite(atsRelayPin, HIGH);
          atsEngaged = true;
          changeState(RUNNING_WITH_ATS);
        } else {
          logEvent("Generator voltage too low! Cannot engage ATS.");
          // Stay in current state and keep checking
          // If this persists too long, we might want to add a timeout
          if (currentMillis - stateStartTime >= atsDelayTime + 60000) {  // Extra 1 minute timeout
            logEvent("Generator voltage unstable. Shutting down.");
            changeState(SHUTTING_DOWN);
          }
        }
      }
      
      // Check if generator stopped unexpectedly
      if (!generatorConfirmed) {
        logEvent("Generator stopped unexpectedly during wait period!");
        changeState(SHUTTING_DOWN);
      }
      break;

    case RUNNING_WITH_ATS:
      // Monitor for grid restoration or scheduled shutdown
      
      // Check for scheduled shutdown after 4 hours
      if (currentMillis - stateStartTime >= generatorRunTime) {
        logEvent("Scheduled generator shutdown after 4 hours runtime...");
        changeState(COOLING_DOWN);
      }
      
      // Check for grid restoration
      if (gridPresent) {
        if (gridRestoreTime == 0) {
          gridRestoreTime = currentMillis;
          logEvent("Grid restored. Waiting 2 minutes before switching back...");
        }
        
        if (currentMillis - gridRestoreTime >= shutdownDelayTime) {
          logEvent("Grid stable. Initiating generator shutdown...");
          changeState(COOLING_DOWN);
        }
      } else {
        gridRestoreTime = 0;
      }
      
      // Check if generator stopped unexpectedly
      if (!generatorConfirmed) {
        logEvent("Generator stopped unexpectedly while running!");
        changeState(SHUTTING_DOWN);
      }
      break;

    case GRID_RESTORED_WAIT:
      // Wait for 2 minutes after grid is restored
      if (currentMillis - stateStartTime >= shutdownDelayTime) {
        changeState(COOLING_DOWN);
      }
      break;

    case COOLING_DOWN:
      // Disengage ATS and let generator cool down
      digitalWrite(atsRelayPin, LOW);
      atsEngaged = false;
      
      // Wait for cooldown period
      if (currentMillis - stateStartTime >= shutdownDelayTime) {
        changeState(SHUTTING_DOWN);
      }
      break;

    case SHUTTING_DOWN:
      // Turn off generator
      digitalWrite(powerOnRelayPin, LOW);
      generatorRunning = false;
      gridWasLost = false;
      gridRestoreTime = 0;
      logEvent("Generator shut down. Returning to IDLE state.");
      changeState(IDLE);
      break;
  }
}