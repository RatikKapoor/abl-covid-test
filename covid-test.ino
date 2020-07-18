/**
 * covid-test
 * (c) Ratik Kapoor 2020
 * 
 * This code is to be used in the COVID testing kit created at the Advanced Biofabrication Laboratory at the University of Calgary
 * Please see LICENSE for use, this code is not to be reproduced, modified, or derived from without express permission from the author.
 * 
 */

// Include servo software
#include <ESP32_Servo.h>

// Include Adafruit Graphics Library (to be used with ST7735 display)
#include <Adafruit_GFX.h>     // Core graphics library
#include <XTronical_ST7735.h> // Hardware-specific library
#include <SPI.h>

// Include Wifi module drivers
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
// Wifi AP Setup
const char *ssid = "ESP32_ABL_NET";
const char *password = "password";

// Define pin designations
#define HEATER_PIN 32
#define LED_ARRAY_PIN 33
#define SERVO_PIN 25
#define TFT_DC 2
#define TFT_RST -1
#define TFT_CS 5

/**
 * 
 * MotorController
 * 
 */
class MotorController
{
private:
    Servo servo;
    int power = 0;
    int ms = 1500;
    int pin;

public:
    MotorController(int pin);
    ~MotorController();
    void setPower(int power);
    int getPower();
    void run();
};

MotorController::MotorController(int pin)
{
    this->pin = pin;
    servo.attach(pin);
    servo.writeMicroseconds(ms);
}

MotorController::~MotorController()
{
}

int MotorController::getPower()
{
    return power;
}

void MotorController::setPower(int power)
{
    ms = map(power, 0, 100, 1500, 1700);
}

void MotorController::run()
{
    servo.writeMicroseconds(this->ms);
}

/**
 * 
 * RelayController
 * 
 */
class RelayController
{
private:
    int pin;
    bool state = false;

public:
    RelayController(int pin);
    ~RelayController();
    void setState(bool state);
    bool getState();
};

RelayController::RelayController(int pin)
{
    this->pin = pin;
    pinMode(this->pin, OUTPUT);
}

RelayController::~RelayController()
{
}

bool RelayController::getState()
{
    return this->state;
}

void RelayController::setState(bool state)
{
    this->state = state;
}

/**
 * 
 * IO
 * 
 */
class IO
{
private:
    MotorController *motorController;
    RelayController *heaterController;
    RelayController *ledArrayController;

public:
    IO(int motorPin, int heaterPin, int ledArrayPin);
    ~IO();
    void setMotorPower(int power);
    int getMotorPower();
    void runMotor();
    void setHeaterState(bool state);
    bool getHeaterState();
    void setLedArrayState(bool state);
    bool getLedArrayState();
};

IO::IO(int motorPin, int heaterPin, int ledArrayPin)
{
    this->motorController = new MotorController(motorPin);
    this->heaterController = new RelayController(heaterPin);
    this->ledArrayController = new RelayController(ledArrayPin);
}

IO::~IO()
{
}

void IO::setMotorPower(int power)
{
    motorController->setPower(power);
    return;
}

int IO::getMotorPower()
{
    return motorController->getPower();
}

void IO::runMotor()
{
    motorController->run();
    return;
}

void IO::setHeaterState(bool state)
{
    heaterController->setState(state);
    return;
}

bool IO::getHeaterState()
{
    return heaterController->getState();
}

void IO::setLedArrayState(bool state)
{
    ledArrayController->setState(state);
    return;
}

bool IO::getLedArrayState()
{
    return ledArrayController->getState();
}

/**
 * 
 * TFT
 * 
 */
class TFT
{
private:
    Adafruit_ST7735 *tft;

public:
    TFT(int CS, int DC);
    ~TFT();
    void text(String text);
    void text(String text, int x, int y);
    void text(String text, int x, int y, unsigned short color);
};

TFT::TFT(int CS, int DC)
{
    this->tft = new Adafruit_ST7735(CS, DC, -1);
    this->tft->init();
    this->tft->setRotation(3);
    this->tft->fillScreen(ST7735_BLACK);
}

TFT::~TFT()
{
}

void TFT::text(String text)
{
    this->text(text, 5, 5);
}

void TFT::text(String text, int x, int y)
{
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(x, y);
    tft.setTextColor(0xFFFF);
    tft.setTextWrap(true);
    tft.print(text);
}

/**
 * 
 * WifiAP
 * 
 */
class WifiAP
{
private:
    WiFiServer *server = new WiFiServer(80);

public:
    WifiAP(const char *ssid, const char *password);
    ~WifiAP();
    WiFiClient available();
};

WifiAP::WifiAP(const char *ssid, const char *password)
{
    WiFi.softAP(ssid, password);
    server->begin();
}

WifiAP::~WifiAP()
{
}

WiFiClient WifiAP::available()
{
    return this->server->available();
}

/**
 * 
 * Setup
 * 
 */
void setup()
{
    TFT *tft = new TFT(TFT_CS, TFT_DC);
    IO *io = new IO(SERVO_PIN, HEATER_PIN, LED_ARRAY_PIN);
    WifiAP *wifiAP = new WifiAP(ssid, password);
}

/**
 * 
 * Loop
 * 
 */
void loop()
{
}
