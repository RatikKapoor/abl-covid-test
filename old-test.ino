/**
 * covid-test
 * (c) Ratik Kapoor 2020
 * 
 * This code is to be used in the COVID testing kit created at the Advanced Biofabrication Laboratory at the University of Calgary
 * Please see LICENSE for use, this code is not to be reproduced, modified, or derived from without express permission from the author.
 * 
 */

// Include Wifi module drivers
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

// Include Adafruit Graphics Library (to be used with ST7735 display)
#include <Adafruit_GFX.h>    // Core graphics library
#include <XTronical_ST7735.h> // Hardware-specific library
#include <SPI.h>

// Include servo software
#include <ESP32_Servo.h>

// Define pin designations
#define HEATER_PIN 32
#define LED_ARRAY_PIN 33
#define SERVO_PIN 25
#define TFT_DC 2
#define TFT_RST -1
#define TFT_CS 5

// Motor variables
Servo servo;

// Wifi AP Setup
//const char *ssid = "ESP32_ABL_NET";
//const char *password = "password";
WiFiServer server(80);

// TFT display setup
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// State holders
bool heaterState = false;
bool ledArrayState = false;
int motorPercent = 0;

void setup1() {
  Serial.begin(115200);
  Serial.println();

  // Set up servo
  servo.attach(SERVO_PIN);
//  servo.writeMicroseconds(servoSpeed); // send "stop" signal to ESC, by default servoSpeed is 1500

  // Define LED Array and Heater pins as 
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(LED_ARRAY_PIN, OUTPUT);

  // Initialize TFT module
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);

  // Configure access point
  text("Starting...");
  Serial.println("Configuring access point...");
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  
  // Ensure heater and LED start as OFF
  pinOutput(HEATER_PIN, LOW);
  pinOutput(LED_ARRAY_PIN, LOW);

  Serial.println("Server started");
  text("Server started, LEDs Off.\nPlease wait five seconds...");
  
  delay(5000);
}

void loop1() {
//  servo.writeMicroseconds(servoSpeed);
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:application/json");
            client.println();

            // the content of the HTTP response follows the header:
          //  client.print("Change HEATER_PIN state: <a href=\"/H1\">On</a> or <a href=\"/L1\">Off</a><br>");
          //  client.print("Change LED_ARRAY_PIN state: <a href=\"/H2\">On</a> or <a href=\"/L2\">Off</a><br>");
          //  client.print("Change LED3 state: <a href=\"/H3\">On</a> or <a href=\"/L3\">Off</a><br>");
          client.print("{heaterState:");
          if (heaterState) client.print("true,"); else client.print("false,");
          client.print("motorPercent:");
          client.print(motorPercent);
          client.print(",ledArrayState:");
          if (ledArrayState) client.print("true}"); else client.print("false}");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H1")) {
          pinOutput(HEATER_PIN, HIGH);
          tftStatus();
        }
        if (currentLine.endsWith("GET /L1")) {
          pinOutput(HEATER_PIN, LOW);
          tftStatus();
        }
        if (currentLine.endsWith("GET /H2")) {
          pinOutput(LED_ARRAY_PIN, HIGH);
          tftStatus();
        }
        if (currentLine.endsWith("GET /L2")) {
          pinOutput(LED_ARRAY_PIN, LOW);
          tftStatus();
        }
        if (currentLine.endsWith("GET /H3")) {
        //  servoSpeed = 1600;
          tftStatus();
        }
        if (currentLine.endsWith("GET /L3")) {
//          servoSpeed = 1500;
          tftStatus();
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void text(char *text) {
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(5,5);
  tft.setTextColor(0xFFFF);
  tft.setTextWrap(true);
  tft.print(text);
}

void text(const char *text, int x, int y) {
//  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(x,y);
  tft.setTextColor(0xFFFF);
  tft.setTextWrap(true);
  tft.print(text);
}

void pinOutput(int pin, bool state) {
  if (state) {
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
  switch (pin) {
    case HEATER_PIN:
      heaterState = state;
      break;
    case LED_ARRAY_PIN:
      ledArrayState = state;
      break;
//    case LED3:
//      threeState = state;
//      break;
    default:
      // Default
      break;
  }
}

void tftStatus() {
  text("Server runnning");
  text(heaterState ? "Heater: On" : "Heater: Off", 5, 20);
  text(ledArrayState ? "LED Array: On" : "LED Array: Off", 5, 30);
  //text("Motor Power: " + motorPercent + servoSpeed , 5, 40);
}

int getMotorPower() {

}

int setMotorPower() {
  
}
