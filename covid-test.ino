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
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_ILI9341.h>
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
#define TEMP2_SENSOR_PIN 36
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
    unsigned long lastTempCheck;
};

struct Timer
{
    unsigned long currentTime;
    unsigned long lastTimerTick;
    unsigned long motorTimer;
    unsigned long ledArrayTimer;
    unsigned long heaterTimer;
};

struct ButtonsPressed
{
    bool wasUpPressed;
    bool wasDownPressed;
    bool wasOkPressed;
    unsigned long lastInterruptTime;
};

enum Controls
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
struct Timer timer;
unsigned long screenRefresh = 0;

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
    unsigned long lastIncrementTime;

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
        if (millis() - this->lastIncrementTime > 20)
        {
            this->ms -= 1;
            this->lastIncrementTime = millis();
        }
    }
    else if (finalMs > this->ms)
    {
        if (millis() - this->lastIncrementTime > 20)
        {
            this->ms += 1;
            this->lastIncrementTime = millis();
        }
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
    //    buttonsPressed.lastInterruptTime = millis() + 200; // Debounced here to prevent cross-talk between buttons and relay pins
    return;
}

void RelayController::updatePin()
{
    buttonsPressed.lastInterruptTime = millis();
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
    double currentTemp;

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

    double temps[15];

    for (int i = 0; i < 15; i++)
    {
        Vout = analogRead(pin) * this->Vs / this->adcMax;
        Rt = this->R1 * Vout / (this->Vs - Vout);
        T = 1 / (1 / this->To - log(Rt / this->Ro) / this->Beta);
        temps[i] = T - 273.15;
    }

    Tc = (temps[0] + temps[1] + temps[2] + temps[3] + temps[4] + temps[5] + temps[6] + temps[7] + temps[8] + temps[9] + temps[10] + temps[11] + temps[12] + temps[13] + temps[14]) / 15;
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
    int lastMotorPower = 95;
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
    if (power != 0)
    {
        if (timer.motorTimer == 0)
        {
            timer.motorTimer = timer.currentTime;
        }
        this->lastMotorPower = power;
    }
    else
    {
        timer.motorTimer = 0;
    }
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
    if (millis() - ioState.lastTempCheck > 5000)
    {
        if (temperatureSensor->getCurrentTemp() > 62.5 && heaterController->getEnabled())
        {
            heaterController->setActive(false);
        }
        else if (temperatureSensor->getCurrentTemp() < 59.0 && temperatureSensor->getCurrentTemp() > -15.0 && heaterController->getEnabled())
        {
            heaterController->setActive(true);
        }
        else if (temperatureSensor->getCurrentTemp() < -15.0)
        {
            heaterController->setActive(false);
        }
        ioState.lastTempCheck = millis();
    }
    if (millis() - timer.lastTimerTick > 1000)
    {
        timer.lastTimerTick = millis();
        timer.currentTime += 1;
    }
    return;
}

void IO::setHeaterEnabled(bool enabled)
{
    heaterController->setEnabled(enabled);
    if (enabled && timer.heaterTimer == 0)
    {
        timer.heaterTimer = timer.currentTime;
    }
    else if (!enabled)
    {
        timer.heaterTimer = 0;
    }
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
    if (enabled && timer.ledArrayTimer == 0)
    {
        timer.ledArrayTimer = timer.currentTime;
    }
    else if (!enabled)
    {
        timer.ledArrayTimer = 0;
    }
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
    return (String) "{\"heaterEnabled\":" + (this->getHeaterEnabled() ? (String) "true," : (String) "false,") 
    + (String) "\"motorPower\":" + this->getMotorPower() 
    + (String) ",\"ledArrayEnabled\":" + (this->getLedArrayEnabled() ? (String) "true," : (String) "false,") 
    + (String) "\"currentTemp\":" + (String)this->getCurrentTemperature()
    + (String) ",\"motorTimer\":" + (String)(timer.currentTime - timer.motorTimer)
    + (String) ",\"ledArrayTimer\":" + (String)(timer.currentTime - timer.ledArrayTimer)
    + (String) ",\"heaterTimer\":" + (String)(timer.currentTime - timer.heaterTimer)
    + (String) "}";
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
    Adafruit_ILI9341 *tft;
    IO *io;
    Controls currentSelection = Controls::Heater;
    String getTimerText(Controls controls);

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
    this->tft = new Adafruit_ILI9341(CS, DC, -1);
    this->tft->begin();
    this->tft->setRotation(1);
    this->tft->fillScreen(ILI9341_BLACK);
    this->tft->setFont(&FreeSans9pt7b);
    this->io = io;
}

TFT::~TFT()
{
}

void TFT::clear()
{
    tft->fillScreen(ILI9341_BLACK);
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
            switch (currentSelection)
            {
            case Controls::LedArray:
                currentSelection = Controls::Heater;
                break;

            case Controls::Motor:
                currentSelection = Controls::LedArray;
                break;

            default:
                break;
            }
            buttonsPressed.wasUpPressed = false;
        }
        if (buttonsPressed.wasDownPressed)
        {
            switch (currentSelection)
            {
            case Controls::Heater:
                currentSelection = Controls::LedArray;
                break;

            case Controls::LedArray:
                currentSelection = Controls::Motor;
                break;

            default:
                break;
            }
            buttonsPressed.wasDownPressed = false;
        }
        if (buttonsPressed.wasOkPressed)
        {
            switch (currentSelection)
            {
            case Controls::Heater:
            {
                io->getHeaterEnabled() ? io->setHeaterEnabled(false) : io->setHeaterEnabled(true);
                break;
            }

            case Controls::LedArray:
            {
                io->getLedArrayEnabled() ? io->setLedArrayEnabled(false) : io->setLedArrayEnabled(true);
                break;
            }

            case Controls::Motor:
            {
                int currentPower = io->getMotorPower();
                if (currentPower > 0)
                {
                    io->setMotorPower(0);
                }
                else
                {
                    io->setMotorPower(io->lastMotorPower);
                }
                break;
            }

            default:
                break;
            }
            buttonsPressed.wasOkPressed = false;
        }
    }

    // Base
    this->text(tftStatus.message, 0, 15, tftStatus.colour);
    this->text((String) "Temp: " + (String)ioState.currentTemp, 0, 35, 0xFFFF);
    this->text((String) "Heater: " + (String)(ioState.heaterEnabled ? "On" : "Off"), 0, 75, currentSelection == Controls::Heater ? 0xFF80 : 0xBDD7);
    this->text((String) "LED Array: " + (String)(ioState.ledArrayEnabled ? "On" : "Off"), 0, 100, currentSelection == Controls::LedArray ? 0xFF80 : 0xBDD7);
    this->text((String) "Motor Power: " + (String)ioState.motorPower + (String) "%", 0, 125, currentSelection == Controls::Motor ? 0xFF80 : 0xBDD7);
    this->text((String) "Press Up/Down/Ok to toggle state.", 0, 225, 0xDEDB);

    // Timers
    if (timer.heaterTimer > 0)
        this->text(this->getTimerText(Controls::Heater), 250, 75, 0x073F);
    if (timer.ledArrayTimer > 0)
        this->text(this->getTimerText(Controls::LedArray), 250, 100, 0x073F);
    if (timer.motorTimer > 0)
        this->text(this->getTimerText(Controls::Motor), 250, 125, 0x073F);
}

String TFT::getTimerText(Controls controls)
{
    unsigned long startTime;
    // String name;
    switch (controls)
    {
    case Controls::Heater:
        startTime = timer.heaterTimer;
        break;

    case Controls::LedArray:
        startTime = timer.ledArrayTimer;
        break;

    case Controls::Motor:
        startTime = timer.motorTimer;
        break;

    default:
        break;
    }
    unsigned long elapsedSeconds = timer.currentTime - startTime;
    int seconds = elapsedSeconds % 60;
    int minutes = elapsedSeconds / 60;
    int hours = elapsedSeconds / 3600;
    String strMinutes;
    String strSeconds;
    String strHours;
    if (minutes / 10 == 0)
        strMinutes = (String) "0" + (String)minutes;
    else
        strMinutes = (String)minutes;
    if (seconds / 10 == 0)
        strSeconds = (String) "0" + (String)seconds;
    else
        strSeconds = (String)seconds;
    if (hours / 10 == 0)
        strHours = (String) "0" + (String)hours;
    else
        strHours = (String)hours;

    return strHours + (String) ":" + strMinutes + (String) ":" + strSeconds;
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
            tftStatus.message = "Ready";
            tftStatus.colour = 0x17E0;
        }
        else
        {
            this->server->send(404, "text/plain", "Bad request");
            tftStatus.message = "Error: Bad Request";
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
    if (millis() - buttonsPressed.lastInterruptTime > 350)
    {
        buttonsPressed.wasUpPressed = true;
        Serial.println("upPressed");
    }
    buttonsPressed.lastInterruptTime = millis();
}

void Buttons::onDownPress()
{
    if (millis() - buttonsPressed.lastInterruptTime > 350)
    {
        buttonsPressed.wasDownPressed = true;
        Serial.println("downPressed");
    }
    buttonsPressed.lastInterruptTime = millis();
}

void Buttons::onOkPress()
{
    if (millis() - buttonsPressed.lastInterruptTime > 350)
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
IO *io;
TFT *tft;
WifiAP *wifiAP;
Buttons *buttons;
void setup()
{
    Serial.begin(9600);
    io = new IO(SERVO_PIN, HEATER_PIN, LED_ARRAY_PIN, TEMP1_SENSOR_PIN, TEMP2_SENSOR_PIN, TEMP3_SENSOR_PIN);
    tft = new TFT(TFT_CS, TFT_DC, io);
    tft->text("ABL Covid Test", 0, 15, 0xFCA0);
    tft->text("Initializing...\nPlease wait", 0, 35, 0xFFFF);
    wifiAP = new WifiAP(ssid, password, io, tft);
    delay(2000);
    tftStatus.message = "Ready";
    tftStatus.colour = 0x17E0;
    tft->run();
    buttons = new Buttons(UP_PIN, DOWN_PIN, OK_PIN, tft);
}

/**
 * 
 * Loop
 * 
 */
void loop()
{
    wifiAP->handleClient();
    io->run();
    if (buttonsPressed.wasDownPressed || buttonsPressed.wasOkPressed || buttonsPressed.wasUpPressed)
    {
        tft->run();
    }
    if (millis() - screenRefresh > 5000)
    {
        io->updateIoState();
        tft->run();
        screenRefresh = millis();
    }
}
