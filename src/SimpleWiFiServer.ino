#include <WiFi.h>
#include <arduino-timer.h>
#include <ezButton.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <secrets.h>

auto timer = timer_create_default(); // create a timer with default settings

const int photoSensorPin = 25;
const int buzzerPin = 27;
const int tonePWMChannel = 0;
const int buttonPin = 14;
const int ledPin = 33;

int photoSensorVal = 0;
int photoState = 0;

int highCount = 0; // In the dark
int lowCount = 0;  // Exposed to light

ezButton button(buttonPin);
AsyncWebServer server(80);

bool buzzerState = false;
bool ledState = false;

void startLed()
{
    if (ledState == false)
    {
        digitalWrite(ledPin, HIGH);

        ledState = true;
    }
}

void stopLed(bool skipStateUpdate = false)
{
    if (ledState == true)
    {
        digitalWrite(ledPin, LOW);

        if (skipStateUpdate == false)
        {
            ledState = false;
        }
    }
}

void startBuzzer()
{
    if (buzzerState == false)
    {
        ledcAttachPin(buzzerPin, tonePWMChannel);
        ledcWriteNote(tonePWMChannel, NOTE_B, 4);

        buzzerState = true;
    }
}

void stopBuzzer(bool skipStateUpdate = false)
{
    if (buzzerState == true)
    {
        ledcDetachPin(buzzerPin);

        if (skipStateUpdate == false)
        {
            buzzerState = false;
        }
    }
}

bool check_sensor(void *)
{

    if (lowCount > highCount)
    {
        startBuzzer();
        startLed();
        photoState = 1;
    }
    else
    {
        stopBuzzer();
        stopLed();
        photoState = 0;
    }

    highCount = 0;
    lowCount = 0;

    return true;
}

bool perform_action(void *)
{
    return true;
}

void setupWifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PW);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void setup()
{
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT); // set the LED pin mode
    pinMode(photoSensorPin, INPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);

    delay(10);

    setupWifi();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "application/json", "{\"power\":" + String(photoState) + "}"); });

    server.begin();

    timer.every(1000, check_sensor);
}

void loop()
{
    button.loop();

    if (button.isReleased() && buzzerState == true)
    {
        stopBuzzer(true);
    }

    photoSensorVal = digitalRead(photoSensorPin);

    if (photoSensorVal == HIGH)
    {
        highCount++;
    }
    else
    {
        lowCount++;
    }

    timer.tick();
}
