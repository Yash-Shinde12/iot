//IOT FINAL PRJCT
#define BLYNK_TEMPLATE_ID "TMPL3FvqA0wYe"
#define BLYNK_TEMPLATE_NAME "Gas Detection"
#define BLYNK_AUTH_TOKEN "220FyglVSMoxcnSruI9MDISmjf5CPHaU"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <HX711.h>
#include <Wire.h>
#include <Servo.h>

// Pin Configurations
#define DOUT D6         // HX711 data pin
#define CLK D7          // HX711 clock pin
#define SMOKE_SENSOR A0
#define RED_LED D5      // RED LED
#define GREEN_LED D4    // GREEN LED
#define BUZZER D8       // Buzzer
#define SERVO_PIN D3    // Servo Motor
#define RELAY_PIN D0   // Relay module for fans
#define SENSOR_THRESHOLD 300
#define FAN_RUN_EXTRA_TIME 15000 // Extra time in milliseconds fans run after gas level normalizes
#define WEIGHT_THRESHOLD 350 // Weight threshold value for triggering the loadcell event

// WiFi Credentials
char ssid[] = "CONNECTED";  // Replace with your WiFi SSID
char pass[] = "rohi@123";  // Replace with your WiFi password

// HX711 Setup
HX711 scale;
float calibration_factor = 463.150; // Calibration factor
float weight;

// Servo Motor Setup
Servo servo;

// Variables for Gas Sensor
int smokeLevel = 0;
bool fansRunning = false;     // Indicates if fans are currently running
unsigned long fanOffTime = 0; // Time to stop fans after gas level decreases

// Blynk Virtual Pins
#define V0_WEIGHT V1
#define V1_SMOKE V0

void setup() {
  // Begin Serial for Debugging
  Serial.begin(9600);

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Pin Configurations
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  
  // Indication on Startup
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  Serial.print("led turned on");
  digitalWrite(BUZZER, LOW);
  
  // Initialize Scale
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();

  // Initialize Servo Motor
  servo.attach(SERVO_PIN);
  servo.write(0);

  // Ensure relay starts in OFF state (Active-Low logic, relay OFF = HIGH)
  digitalWrite(RELAY_PIN, HIGH); // Relay OFF (Active-Low)

  Serial.println("Setup complete!");
}

void loop() {
  // Run Blynk
  Blynk.run();

  // Read Weight
  weight = scale.get_units(10);
  if (weight < 0) weight = 0;

  // Read Gas Sensor
  smokeLevel = analogRead(SMOKE_SENSOR);

  // Display Readings Locally
  Serial.print("Weight: ");
  Serial.print(weight);
  Serial.println(" grams");

  Serial.print("Gas Sensor: ");
  Serial.println(smokeLevel);

  // Send Data to Blynk App
  Blynk.virtualWrite(V0_WEIGHT, weight);
  Blynk.virtualWrite(V1_SMOKE, smokeLevel);

  // Handle Gas Detection and Relay Control
  if (smokeLevel > SENSOR_THRESHOLD) {
    Serial.println("Gas detected! Activating safety mechanisms.");
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BUZZER, HIGH);
    servo.write(162); // Rotate servo to 90 degrees
    activateFans();  // Turn on fans
    sendBlynkAlert("High gas level detected! Take action.");
  } else {
    Serial.println("Gas level safe. Deactivating safety mechanisms.");
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(BUZZER, LOW);
    servo.write(0); // Rotate servo back to 0 degrees
    deactivateFansWithDelay(); // Handle fan deactivation with delay
  }

// Check if weight is below threshold for triggering loadcell alert
if (weight > 250 && weight < 350) {
    sendBlynkLoadCell(weight); // Trigger loadcell event
}
 delay(1000); // Delay between readings
}
// Function to trigger Blynk loadcell event
void sendBlynkLoadCell(float weight) {
  Blynk.logEvent("gas_booked", "Subject: Gas Booking Notification\nBody:\nDear [User's Name],\nWe noticed that your gas weight is low. A new gas cylinder has been successfully booked for you.\n\nThank you for using our service!\nBest regards.");
  Serial.println("Blynk Loadcell Alert Sent: Weight is below threshold.");
}


// Turn on the fans via the relay module (Active-Low Logic)
void activateFans() {
  if (!fansRunning) { // Only activate fans if they are not already running
    digitalWrite(RELAY_PIN, LOW); // Relay ON (Active-Low)
    fansRunning = true;
    fanOffTime = 0; // Reset the timer
    Serial.println("Fans activated.");
    Serial.print("Relay state: ");
    Serial.println(digitalRead(RELAY_PIN) == HIGH ? "OFF" : "ON");
  }
}

// Turn off the fans after the extra time delay if no gas is detected
void deactivateFansWithDelay() {
  if (smokeLevel <= SENSOR_THRESHOLD) {
    if (fansRunning) {
      if (fanOffTime == 0) {
        fanOffTime = millis() + FAN_RUN_EXTRA_TIME; // Set timer
        Serial.println("Fan stop timer initiated.");
      }

      if (millis() >= fanOffTime) {
        digitalWrite(RELAY_PIN, HIGH); // Relay OFF (Active-Low)
        fansRunning = false;
        Serial.println("Fans deactivated.");
        Serial.print("Relay state: ");
        Serial.println(digitalRead(RELAY_PIN) == HIGH ? "OFF" : "ON");
      }
    }
  } else {
    fanOffTime = 0; // Reset the timer if gas is detected again
  }
}
bool alertSent = false; // Flag to track if the alert has been sent

// Send an alert to the Blynk app
void sendBlynkAlert(String message) {
  // Only send the alert if it hasn't been sent already
  if (!alertSent) {
    Blynk.logEvent("alert", message);
    Serial.println("Blynk Alert Sent: " + message);
    
    // Set the flag to true to prevent sending duplicate alerts
    alertSent = true;
  }
}
