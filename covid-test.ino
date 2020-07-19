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
#include <Adafruit_GFX.h> // Core graphics library
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Include Wifi module drivers
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include <ESPmDNS.h>
// Wifi AP Setup
const char *ssid = "ESP32_ABL_NET";
const char *password = "password";

// Define pin designations
#define HEATER_PIN 32
#define LED_ARRAY_PIN 33
#define SERVO_PIN 25
#define TEMP_SENSOR_PIN 36
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
    void ease();

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
    this->power = constrain(power, 0, 100);
}

void MotorController::run()
{
    this->ease();
    servo.writeMicroseconds(this->ms);
}

void MotorController::ease()
{
    int finalMs = map(power, 0, 100, 1500, 1600);
    if (finalMs < this->ms)
    {
        this->ms -= 1;
    }
    else if (finalMs > this->ms)
    {
        this->ms += 1;
    }
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
    void updatePin();

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
    this->updatePin();
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
    this->updatePin();
    return;
}

void RelayController::updatePin()
{
    digitalWrite(this->pin, state ? HIGH : LOW);
    return;
}

/**
 * 
 * TemperatureSensor
 * 
 */
class TemperatureSensor
{
private:
    int pin;
//    double temps[] = new double[10];
    double currentTemp;
    int currentPtr = 0;

    double adcMax = 1023.0, Vs = 3.3;
    double R1 = 100000.0; // voltage divider resistor value
    double Beta = 3950.0; // Beta value
    double To = 298.15;   // Temperature in Kelvin for 25 degree Celsius
    double Ro = 100000.0; // Resistance of Thermistor at 25 degree Celsius

    void calculateTemp();

public:
    TemperatureSensor(int pin);
    ~TemperatureSensor();
    double getCurrentTemp();
};

TemperatureSensor::TemperatureSensor(int pin)
{
    this->pin = pin;
    pinMode(this->pin, INPUT);
}

TemperatureSensor::~TemperatureSensor()
{
}

void TemperatureSensor::calculateTemp()
{
    double Vout, Rt = 0;
    double T, Tc, Tf = 0;

    Vout = analogRead(this->pin) * this->Vs / this->adcMax;
    Rt = this->R1 * Vout / (this->Vs - Vout);
    T = 1 / (1 / this->To + log(Rt / this->Ro) / this->Beta);
    Tc = T - 273.15;
    this->currentTemp = Tc;
    return;
}

double TemperatureSensor::getCurrentTemp()
{
    this->calculateTemp();
    return this->currentTemp;
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
    TemperatureSensor *temperatureSensor;

public:
    IO(int motorPin, int heaterPin, int ledArrayPin, int tempPin);
    ~IO();
    void setMotorPower(int power);
    int getMotorPower();
    void runMotor();
    void setHeaterState(bool state);
    bool getHeaterState();
    void setLedArrayState(bool state);
    bool getLedArrayState();
    double getCurrentTemperature();
    String getJsonState();
};

IO::IO(int motorPin, int heaterPin, int ledArrayPin, int tempPin)
{
    this->motorController = new MotorController(motorPin);
    this->heaterController = new RelayController(heaterPin);
    this->ledArrayController = new RelayController(ledArrayPin);
    this->temperatureSensor = new TemperatureSensor(tempPin);
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

double IO::getCurrentTemperature()
{
    return this->temperatureSensor->getCurrentTemp();
}

String IO::getJsonState()
{
    return (String) "{heaterState:" + (this->getHeaterState() ? (String) "true," : (String) "false,") + (String) "motorPower:" + this->getMotorPower() + (String) ",ledArrayState:" + (this->getLedArrayState() ? (String) "true," : (String) "false,") + (String) "currentTemp:" + (String)this->getCurrentTemperature() + (String) "}";
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
    void clear();
    void text(String text);
    void text(String text, unsigned short color);
    void text(String text, int x, int y);
    void text(String text, int x, int y, unsigned short color);
};

TFT::TFT(int CS, int DC)
{
    this->tft = new Adafruit_ST7735(CS, DC, -1);
    this->tft->initR(INITR_144GREENTAB);
    this->tft->setRotation(1);
    this->tft->fillScreen(ST7735_BLACK);
}

TFT::~TFT()
{
}

void TFT::clear()
{
    tft->fillScreen(ST7735_BLACK);
}

void TFT::text(String text)
{
    this->text(text, 0, 0);
}

void TFT::text(String text, unsigned short color)
{
    this->text(text, 0, 0, color);
}

void TFT::text(String text, int x, int y)
{
    this->text(text, x, y, 0xFFFF);
}

void TFT::text(String text, int x, int y, unsigned short color)
{
    tft->setCursor(x, y);
    tft->setTextColor(color);
    tft->setTextWrap(true);
    tft->print(text);
}

/**
 * 
 * WifiAP
 * 
 */
class WifiAP
{
private:
    WebServer *server = new WebServer(80);
    IO *io;
    TFT *tft;

public:
    WifiAP(const char *ssid, const char *password, IO *io, TFT *tft);
    ~WifiAP();
    void handleClient();
    bool handleRequest(String device, String newState);
};

WifiAP::WifiAP(const char *ssid, const char *password, IO *io, TFT *tft)
{
    WiFi.softAP(ssid, password);
    this->io = io;
    this->tft = tft;
    this->server->on("/", [this]() {
        this->server->send(200, "application/json", this->io->getJsonState());
    });
    this->server->on("/{}/{}", [this]() {
        if (this->handleRequest(this->server->pathArg(0), this->server->pathArg(1)))
        {
            this->server->send(200, "application/json", this->io->getJsonState());
            this->tft->clear();
            this->tft->text("Good Request", 0x17E0);
            this->tft->text(this->io->getJsonState(), 0, 10, 0xEEEE);
        }
        else
        {
            this->server->send(404, "text/plain", "Bad request");
            this->tft->clear();
            this->tft->text("Bad Request", 0xF800);
            this->tft->text(this->io->getJsonState(), 0, 10, 0xEEEE);
        }
    });
    server->begin();
}

WifiAP::~WifiAP()
{
}

void WifiAP::handleClient()
{
    this->server->handleClient();
    return;
}

bool WifiAP::handleRequest(String device, String newState)
{
    if (device == "heater")
    {
        if (newState == "on")
        {
            this->io->setHeaterState(true);
            return true;
        }
        else if (newState == "off")
        {
            this->io->setHeaterState(false);
            return true;
        }
        else
        {
            return false;
        }
    }
    if (device == "ledArray")
    {
        if (newState == "on")
        {
            this->io->setLedArrayState(true);
            return true;
        }
        else if (newState == "off")
        {
            this->io->setLedArrayState(false);
            return true;
        }
        else
        {
            return false;
        }
    }
    if (device == "motor")
    {
        this->io->setMotorPower(newState.toInt());
        return true;
    }
}

/**
 * 
 * Setup
 * 
 */
void setup()
{
    TFT *tft = new TFT(TFT_CS, TFT_DC);
    tft->text("ABL Covid Test", 0, 0, 0xFCA0);
    tft->text("\n\nInitializing...\nPlease wait", 0, 0, 0xFFFF);
    IO *io = new IO(SERVO_PIN, HEATER_PIN, LED_ARRAY_PIN, TEMP_SENSOR_PIN);
    WifiAP *wifiAP = new WifiAP(ssid, password, io, tft);
    delay(2000);
    tft->clear();
    tft->text("Ready", 0x17E0);
    tft->text(io->getJsonState(), 0, 10, 0xEEEE);

    while (true)
    {
        wifiAP->handleClient();
        io->runMotor();
    }
}

/**
 * 
 * Loop
 * 
 */
void loop()
{
}
