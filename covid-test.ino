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

//#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED
#define LED1 32
#define LED2 33
//#define LED3 25

// Motor
#include <ESP32_Servo.h>
int servoPin = 25;
Servo servo;


// Wifi AP Credentials
const char *ssid = "ESP32_ABL_NET";
const char *password = "password";

// LCD GPIO Pins
#define TFT_DC 2
#define TFT_RST -1
#define TFT_CS 5

WiFiServer server(80);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

bool oneState = false;
bool twoState = false;
bool threeState = false;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
//  pinMode(LED3, OUTPUT);
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);

  Serial.begin(115200);
  Serial.println();
  text("Configuring access point...");
  Serial.println("Configuring access point...");

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  pinOutput(LED1, LOW);
  pinOutput(LED2, LOW);
//  pinOutput(LED3, LOW);

  Serial.println("Server started");
  text("Server started, LEDs Off");
  servo.attach(servoPin);
  servo.writeMicroseconds(1500); // send "stop" signal to ESC.
  delay(3000);
}

void loop() {
  int signal = 1700;
  servo.writeMicroseconds(signal);
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
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
//            client.print("SUCCCESS");
           client.print("Change LED1 state: <a href=\"/H1\">On</a> or <a href=\"/L1\">Off</a><br>");
           client.print("Change LED2 state: <a href=\"/H2\">On</a> or <a href=\"/L2\">Off</a><br>");
           client.print("Change LED3 state: <a href=\"/H3\">On</a> or <a href=\"/L3\">Off</a><br>");

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
          pinOutput(LED1, HIGH);
          tftStatus();
        }
        if (currentLine.endsWith("GET /L1")) {
          pinOutput(LED1, LOW);
          tftStatus();
        }
        if (currentLine.endsWith("GET /H2")) {
          pinOutput(LED2, HIGH);
          tftStatus();
        }
        if (currentLine.endsWith("GET /L2")) {
          pinOutput(LED2, LOW);
          tftStatus();
        }
        if (currentLine.endsWith("GET /H3")) {
//          pinOutput(LED3, HIGH);
          tftStatus();
        }
        if (currentLine.endsWith("GET /L3")) {
//          pinOutput(LED3, LOW);
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
    case LED1:
      oneState = state;
      break;
    case LED2:
      twoState = state;
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
  text(oneState ? "LED1: On" : "LED1: Off", 5, 20);
  text(twoState ? "LED2: On" : "LED2: Off", 5, 30);
  text(threeState ? "LED3: On" : "LED3: Off", 5, 40);
}
