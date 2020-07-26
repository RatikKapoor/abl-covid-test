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
#define TEMP1_SENSOR_PIN 39
#define TEMP2_SENSOR_PIN 34
#define TEMP3_SENSOR_PIN 35
#define TFT_DC 2
#define TFT_RST -1
#define TFT_CS 5
#define UP_PIN 17
#define DOWN_PIN 16
#define OK_PIN 15

/**
 * 
 * Data Structs & Enums
 * 
 */
struct StatusMessage
{
    String message;
    short colour;
};

struct IoState
{
    bool heaterEnabled;
    bool heaterActive;
    bool ledArrayEnabled;
    int motorPower;
    double currentTemp;
};

struct ButtonsPressed
{
    bool wasUpPressed;
    bool wasDownPressed;
    bool wasOkPressed;
    unsigned long lastInterruptTime;
};

enum CurrentSelection
{
    Heater = 0,
    LedArray = 1,
    Motor = 2
};

/**
 * 
 * Globals
 * 
 */
struct StatusMessage tftStatus = {
    "Initializing...",
    0x057F};

struct IoState ioState;
struct ButtonsPressed buttonsPressed;

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
    bool enabled = false;
    bool active = true;
    void updatePin();

public:
    RelayController(int pin);
    ~RelayController();
    void setEnabled(bool enabled);
    bool getEnabled();
    void setActive(bool active);
    bool getActive();
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

bool RelayController::getEnabled()
{
    return this->enabled;
}

void RelayController::setEnabled(bool enabled)
{
    this->enabled = enabled;
    this->updatePin();
    return;
}

void RelayController::updatePin()
{
    if (this->enabled && this->active)
    {
        digitalWrite(this->pin, HIGH);
    }
    else
    {
        digitalWrite(this->pin, LOW);
    }
    return;
}

void RelayController::setActive(bool active)
{
    this->active = active;
    this->updatePin();
    return;
}

bool RelayController::getActive()
{
    return this->active;
}

/**
 * 
 * TemperatureSensor
 * 
 */
class TemperatureSensor
{
private:
    int pin1;
    int pin2;
    int pin3;
    //    double temps[] = new double[10];
    double currentTemp;
    //    int currentPtr = 0;

    double adcMax = 4095.0, Vs = 3.3;
    double R1 = 100000.0; // voltage divider resistor value
    double Beta = 3950.0; // Beta value
    double To = 298.15;   // Temperature in Kelvin for 25 degree Celsius
    double Ro = 100000.0; // Resistance of Thermistor at 25 degree Celsius

    double calculateTemp(int pin);

public:
    TemperatureSensor(int pin1, int pin2, int pin3);
    ~TemperatureSensor();
    double getCurrentTemp();
};

TemperatureSensor::TemperatureSensor(int pin1, int pin2, int pin3)
{
    this->pin1 = pin1;
    this->pin2 = pin2;
    this->pin3 = pin3;
}

TemperatureSensor::~TemperatureSensor()
{
}

double TemperatureSensor::calculateTemp(int pin)
{
    double Vout, Rt = 0;
    double T, Tc, Tf = 0;

    Vout = analogRead(pin) * this->Vs / this->adcMax;
    Rt = this->R1 * Vout / (this->Vs - Vout);
    T = 1 / (1 / this->To - log(Rt / this->Ro) / this->Beta);
    Tc = T - 273.15;
    // this->currentTemp = Tc;
    return Tc;
}

double TemperatureSensor::getCurrentTemp()
{
    double temps[3];
    temps[0] = this->calculateTemp(this->pin1);
    temps[1] = this->calculateTemp(this->pin2);
    temps[2] = this->calculateTemp(this->pin3);
    this->currentTemp = (temps[0] + temps[1] + temps[2]) / 3;
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
    IO(int motorPin, int heaterPin, int ledArrayPin, int tempPin1, int tempPin2, int tempPin3);
    ~IO();
    void setMotorPower(int power);
    int getMotorPower();
    void run();
    void setHeaterEnabled(bool enabled);
    bool getHeaterEnabled();
    bool getHeaterActive();
    void setLedArrayEnabled(bool enabled);
    bool getLedArrayEnabled();
    double getCurrentTemperature();
    String getJsonState();
    void updateIoState();
};

IO::IO(int motorPin, int heaterPin, int ledArrayPin, int tempPin1, int tempPin2, int tempPin3)
{
    this->motorController = new MotorController(motorPin);
    this->heaterController = new RelayController(heaterPin);
    this->ledArrayController = new RelayController(ledArrayPin);
    this->temperatureSensor = new TemperatureSensor(tempPin1, tempPin2, tempPin3);
    this->updateIoState();
}

IO::~IO()
{
}

void IO::setMotorPower(int power)
{
    motorController->setPower(power);
    this->updateIoState();
    return;
}

int IO::getMotorPower()
{
    return motorController->getPower();
}

void IO::run()
{
    motorController->run();
    if (temperatureSensor->getCurrentTemp() > 61.5 && heaterController->getEnabled())
    {
        heaterController->setActive(false);
    }
    else if (temperatureSensor->getCurrentTemp() < 58.5 && heaterController->getEnabled())
    {
        heaterController->setActive(true);
    }
    return;
}

void IO::setHeaterEnabled(bool enabled)
{
    heaterController->setEnabled(enabled);
    this->updateIoState();
    return;
}

bool IO::getHeaterEnabled()
{
    return heaterController->getEnabled();
}

bool IO::getHeaterActive()
{
    return this->heaterController->getActive();
}

void IO::setLedArrayEnabled(bool enabled)
{
    ledArrayController->setEnabled(enabled);
    this->updateIoState();
    return;
}

bool IO::getLedArrayEnabled()
{
    return ledArrayController->getEnabled();
}

double IO::getCurrentTemperature()
{
    return this->temperatureSensor->getCurrentTemp();
}

String IO::getJsonState()
{
    return (String) "{heaterEnabled:" + (this->getHeaterEnabled() ? (String) "true," : (String) "false,") + (String) "motorPower:" + this->getMotorPower() + (String) ",ledArrayEnabled:" + (this->getLedArrayEnabled() ? (String) "true," : (String) "false,") + (String) "currentTemp:" + (String)this->getCurrentTemperature() + (String) "}";
}

void IO::updateIoState()
{
    ioState.currentTemp = this->getCurrentTemperature();
    ioState.heaterActive = this->getHeaterActive();
    ioState.heaterEnabled = this->getHeaterEnabled();
    ioState.ledArrayEnabled = this->getLedArrayEnabled();
    ioState.motorPower = this->getMotorPower();
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
    IO *io;
    CurrentSelection currentSelection = CurrentSelection::Heater;

public:
    TFT(int CS, int DC, IO *io);
    ~TFT();
    void clear();
    void text(String text);
    void text(String text, int x, int y);
    void text(String text, unsigned short color);
    void text(String text, int x, int y, unsigned short color);
    void run();
};

TFT::TFT(int CS, int DC, IO *io)
{
    this->tft = new Adafruit_ST7735(CS, DC, -1);
    this->tft->initR(INITR_144GREENTAB);
    this->tft->setRotation(1);
    this->tft->fillScreen(ST7735_BLACK);
    this->io = io;
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

void TFT::text(String text, int x, int y)
{
    this->text(text, x, y, 0xFFFF);
}

void TFT::text(String text, unsigned short color)
{
    this->text(text, 0, 0, color);
}

void TFT::text(String text, int x, int y, unsigned short color)
{
    tft->setCursor(x, y);
    tft->setTextColor(color);
    tft->setTextWrap(true);
    tft->print(text);
}

void TFT::run()
{
    this->clear();

    if (buttonsPressed.wasDownPressed || buttonsPressed.wasOkPressed || buttonsPressed.wasUpPressed)
    {
        if (buttonsPressed.wasUpPressed)
        {
            this->text("UP", 0, 60);
            switch (currentSelection)
            {
            case CurrentSelection::LedArray:
                currentSelection = CurrentSelection::Heater;
                break;

            case CurrentSelection::Motor:
                currentSelection = CurrentSelection::LedArray;
                break;

            default:
                break;
            }
            buttonsPressed.wasUpPressed = false;
        }
        if (buttonsPressed.wasDownPressed)
        {
            this->text("DOWN", 0, 70);
            switch (currentSelection)
            {
            case CurrentSelection::Heater:
                currentSelection = CurrentSelection::LedArray;
                break;

            case CurrentSelection::LedArray:
                currentSelection = CurrentSelection::Motor;
                break;

            default:
                break;
            }
            buttonsPressed.wasDownPressed = false;
        }
        if (buttonsPressed.wasOkPressed)
        {
            this->text("OK", 0, 80);
            switch (currentSelection)
            {
            case CurrentSelection::Heater:
            {
                io->getHeaterEnabled() ? io->setHeaterEnabled(false) : io->setHeaterEnabled(true);
                break;
            }

            case CurrentSelection::LedArray:
            {
                io->getLedArrayEnabled() ? io->setLedArrayEnabled(false) : io->setLedArrayEnabled(true);
                break;
            }

            case CurrentSelection::Motor:
            {
                int currentPower = io->getMotorPower();
                if (currentPower > 0)
                {
                    io->setMotorPower(0);
                }
                else
                {
                    io->setMotorPower(95);
                }
                break;
            }

            default:
                break;
            }
            buttonsPressed.wasOkPressed = false;
        }
    }

    this->text(tftStatus.message, tftStatus.colour);
    this->text((String) "Temp: " + (String)ioState.currentTemp, 0, 10, 0xFFFF);
    this->text((String) "Heater: " + (String)(ioState.heaterEnabled ? "On" : "Off"), 0, 25, currentSelection == CurrentSelection::Heater ? 0xFFFF : 0xDDDD);
    this->text((String) "LED Array: " + (String)(ioState.ledArrayEnabled ? "On" : "Off"), 0, 35, currentSelection == CurrentSelection::LedArray ? 0xFFFF : 0xDDDD);
    this->text((String) "Motor Power: " + (String)ioState.motorPower + (String) "%", 0, 45, currentSelection == CurrentSelection::Motor ? 0xFFFF : 0xDDDD);
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
            tftStatus.message = "Good Request";
            tftStatus.colour = 0x17E0;
        }
        else
        {
            this->server->send(404, "text/plain", "Bad request");
            tftStatus.message = "Bad Request";
            tftStatus.colour = 0xF800;
        }
        this->tft->run();
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
            this->io->setHeaterEnabled(true);
            return true;
        }
        else if (newState == "off")
        {
            this->io->setHeaterEnabled(false);
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
            this->io->setLedArrayEnabled(true);
            return true;
        }
        else if (newState == "off")
        {
            this->io->setLedArrayEnabled(false);
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
 * Buttons
 * 
 */
class Buttons
{
private:
    int upPin;
    int downPin;
    int okPin;
    bool upPresssed;
    bool downPressed;
    bool okPressed;
    TFT *tft;

public:
    Buttons(int upPin, int downPin, int okPin, TFT *tft);
    ~Buttons();
    static void onUpPress();
    static void onDownPress();
    static void onOkPress();
};

Buttons::Buttons(int upPin, int downPin, int okPin, TFT *tft)
{
    this->upPin = upPin;
    this->downPin = downPin;
    this->okPin = okPin;
    pinMode(upPin, INPUT_PULLUP);
    pinMode(downPin, INPUT_PULLUP);
    pinMode(okPin, INPUT_PULLUP);
    attachInterrupt(upPin, this->onUpPress, RISING);
    attachInterrupt(downPin, this->onDownPress, RISING);
    attachInterrupt(okPin, this->onOkPress, RISING);
    this->tft = tft;
}

Buttons::~Buttons()
{
}

void Buttons::onUpPress()
{
    if (millis() - buttonsPressed.lastInterruptTime > 200)
    {
        buttonsPressed.wasUpPressed = true;
        Serial.println("upPressed");
    }
    buttonsPressed.lastInterruptTime = millis();
}

void Buttons::onDownPress()
{
    if (millis() - buttonsPressed.lastInterruptTime > 200)
    {
        buttonsPressed.wasDownPressed = true;
        Serial.println("downPressed");
    }
    buttonsPressed.lastInterruptTime = millis();
}

void Buttons::onOkPress()
{
    if (millis() - buttonsPressed.lastInterruptTime > 200)
    {
        buttonsPressed.wasOkPressed = true;
        Serial.println("okPressed");
    }
    buttonsPressed.lastInterruptTime = millis();
}

/**
 * 
 * Setup
 * 
 */
void setup()
{
    Serial.begin(9600);
    IO *io = new IO(SERVO_PIN, HEATER_PIN, LED_ARRAY_PIN, TEMP1_SENSOR_PIN, TEMP2_SENSOR_PIN, TEMP3_SENSOR_PIN);
    TFT *tft = new TFT(TFT_CS, TFT_DC, io);
    tft->text("ABL Covid Test", 0, 0, 0xFCA0);
    tft->text("\n\nInitializing...\nPlease wait", 0, 0, 0xFFFF);
    WifiAP *wifiAP = new WifiAP(ssid, password, io, tft);
    delay(2000);
    tftStatus.message = "Ready";
    tftStatus.colour = 0x17E0;
    tft->run();
    Buttons *buttons = new Buttons(UP_PIN, DOWN_PIN, OK_PIN, tft);

    while (true)
    {
        wifiAP->handleClient();
        io->run();
        if (buttonsPressed.wasDownPressed || buttonsPressed.wasOkPressed || buttonsPressed.wasUpPressed)
            tft->run();
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
