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

#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED

// Wifi AP Credentials
const char *ssid = "ESP32_ABL_NET";
const char *password = "password";

// LCD GPIO Pins
#define TFT_DC 2
#define TFT_RST -1
#define TFT_CS 5

WiFiServer server(80);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);  

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
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

  digitalWrite(LED_BUILTIN, LOW);

  Serial.println("Server started");
  text("Server started\nLED Off");
}

void loop() {
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
            client.print("SUCCCESS");
            // client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");

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
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);
          text("Server started\nLED On");
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);
          text("Server started\nLED Off");
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
