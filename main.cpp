#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

#define SERVO_F1_PIN 13
#define SERVO_F2_PIN 12
#define SERVO_F3_PIN 14
#define SERVO_F4_PIN 27

#define I2C_SDA 21
#define I2C_SCL 22

Servo servoF1;
Servo servoF2;
Servo servoF3;
Servo servoF4;

const char* ssid = "Rocky_V1_Avionics";
const char* password = "rockets4me";

WebServer server(80);

enum FlightMode {
    MODE_CINEMATIC_STARTUP = 0,
    MODE_PRE_RECORDED_FLIGHT = 1,
    MODE_ACTIVE_STABILIZATION = 2,
    MODE_IDLE = 3
};

FlightMode currentMode = MODE_IDLE;
unsigned long modeStartTime = 0;

const char* htmlDashboard = 
"<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
"<style>body{font-family:Arial; text-align:center; background:#111; color:#fff;}"
".btn{display:block; width:80%; max-width:300px; margin:15px auto; padding:15px; font-size:18px; color:white; border:none; border-radius:5px; cursor:pointer; font-weight:bold;}"
".btn-0{background:#ff5722;}.btn-1{background:#2196f3;}.btn-2{background:#4caf50;}.btn-3{background:#777;}"
"h1{color:#ffeb3b;}</style></head><body>"
"<h1>🚀 Rocky V1 Mission Control</h1>"
"<p>Select Actuation Flight Mode:</p>"
"<button class='btn btn-0' onclick='location.href=\"/setmode?m=0\"'>1. Cinematic Startup</button>"
"<button class='btn btn-1' onclick='location.href=\"/setmode?m=1\"'>2. Pre-Recorded Routine</button>"
"<button class='btn btn-2' onclick='location.href=\"/setmode?m=2\"'>3. Active Gyro Stabilization</button>"
"<button class='btn btn-3' onclick='location.href=\"/setmode?m=3\"'>Disarm / Idle</button>"
"</body></html>";

void setAllFins(int angle) {
    servoF1.write(angle);
    servoF2.write(angle);
    servoF3.write(angle);
    servoF4.write(angle);
}

void runCinematicStartup() {
    unsigned long elapsed = millis() - modeStartTime;
    
    if (elapsed < 2000) {
        int sweep = map(elapsed, 0, 2000, 90, 135);
        setAllFins(sweep);
    } 
    else if (elapsed < 4000) {
        int sweep = map(elapsed, 2000, 4000, 135, 45);
        setAllFins(sweep);
    } 
    else if (elapsed < 6000) {
        int oscillation = 25 * sin((elapsed - 4000) * 0.01);
        servoF1.write(90 + oscillation);
        servoF2.write(90 - oscillation);
        servoF3.write(90 + oscillation);
        servoF4.write(90 - oscillation);
    } 
    else {
        setAllFins(90);
        currentMode = MODE_IDLE;
    }
}

void runPreRecordedFlight() {
    unsigned long elapsed = millis() - modeStartTime;
    
    if (elapsed < 5000) {
        int rollFactor = 20 * sin(elapsed * 0.005);
        servoF1.write(90 + rollFactor);
        servoF2.write(90 + rollFactor);
        servoF3.write(90 + rollFactor);
        servoF4.write(90 + rollFactor);
    } else {
        setAllFins(90);
        currentMode = MODE_IDLE;
    }
}

void runActiveStabilization() {
    float gyroX = 0.0; 
    float gyroY = 0.0; 

    float Kp = 1.5; 
    int correctionX = constrain(90 + (gyroX * Kp), 45, 135);
    int correctionY = constrain(90 + (gyroY * Kp), 45, 135);

    servoF1.write(correctionX);
    servoF3.write(180 - correctionX); 
    servoF2.write(correctionY);
    servoF4.write(180 - correctionY);
}

void handleRoot() {
    server.send(200, "text/html", htmlDashboard);
}

void handleSetMode() {
    if (server.hasArg("m")) {
        int modeNum = server.arg("m").toInt();
        currentMode = (FlightMode)constrain(modeNum, 0, 3);
        modeStartTime = millis();
        setAllFins(90); 
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    
    servoF1.attach(SERVO_F1_PIN, 500, 2400);
    servoF2.attach(SERVO_F2_PIN, 500, 2400);
    servoF3.attach(SERVO_F3_PIN, 500, 2400);
    servoF4.attach(SERVO_F4_PIN, 500, 2400);

    setAllFins(90); 

    WiFi.softAP(ssid, password);
    Serial.print("Access Point Launched. IP Address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/setmode", handleSetMode);
    server.begin();
}

void loop() {
    server.handleClient(); 

    switch (currentMode) {
        case MODE_CINEMATIC_STARTUP:
            runCinematicStartup();
            break;
        case MODE_PRE_RECORDED_FLIGHT:
            runPreRecordedFlight();
            break;
        case MODE_ACTIVE_STABILIZATION:
            runActiveStabilization();
            break;
        case MODE_IDLE:
        default:
            setAllFins(90); 
            break;
    }
    
    delay(15); 
}