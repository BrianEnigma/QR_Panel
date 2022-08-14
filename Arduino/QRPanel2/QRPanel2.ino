#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
    #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define LED_PIN    14
#define LED_COUNT 52
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

WiFiClientSecure iot = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org"); // prep NTP server connectivity
X509List clientCertificate(AWS_CRT);
PrivateKey clientPrivateKey(AWS_PRIVATE);
X509List caCertificate(AWS_CA);

uint32_t alertColor1;
uint32_t alertColor2;
bool shouldAlert = false;

/// Display a progress indicator up both sides of the QR code.
/// @param step The current step number.
/// @param steps The total number of steps.
void displayProgress(unsigned char step, unsigned char steps)
{
    unsigned int ledCount = LED_COUNT / 2;
    float percent = (float) step / (float) steps;
    ledCount = (int)((float) ledCount * percent);
    Serial.print("Progress: ");
    Serial.print(step);
    Serial.print(" of ");
    Serial.print(steps);
    Serial.print(" -- ");
    Serial.println(percent);
    for (int pos = 0; pos < LED_COUNT; pos++)
    {
        if (pos < ledCount || pos > LED_COUNT - ledCount)
        {
            strip.setPixelColor(pos, strip.Color(0x00, 0xff, 0x00));
        } else {
            strip.setPixelColor(pos, strip.Color(0x20, 0x00, 0x00));
        }
    }
    strip.show();
}

/// Display a fade-in/fade-out red ring around the QR code.
/// @param limitedError If true, only blink the error a few times, then return. If false, 
/// then this function does not return. You will need to manually reset the
/// device.
void displayError(bool limitedError)
{
#define RED_MIN 0x20
#define RED_MAX 0xD0
#define RED_STEP 0x04
    int redDirection = 1;
    int redValue = RED_MIN;
    int blinkCount = 0;
    Serial.println("***** ERROR *****");
    while (true)
    {
        for (int pos = 0; pos < LED_COUNT; pos++)
            strip.setPixelColor(pos, strip.Color(redValue, 0x00, 0x00));
        strip.show();
        delay(50);
        redValue += redDirection * RED_STEP;
        if (redValue <= RED_MIN)
        {
            redDirection = 1;
            redValue = RED_MIN;
            if (limitedError)
            {
                if (++blinkCount >= 3)
                    return;
            }
        }
        if (redValue >= RED_MAX)
        {
            redDirection = -1;
            redValue = RED_MAX;
        }
    }
}

void displayWhite()
{
    for (int pos = 0; pos < LED_COUNT; pos++)
        strip.setPixelColor(pos, strip.Color(0xff, 0xff, 0xff));
    strip.show();
}

/// Attempt to connect to wifi using the information in secrets.h.
/// @return true on success, false on fail.
bool connectToWiFi() // connect to local wifi network
{
    Serial.println("\nConnecting to Wi-Fi");
    
    //WiFi.mode(WIFI_STA);
    WiFi.begin((char *)WIFI_SSID, (char *)WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(200);
        Serial.print("..");
        // TODO: TIMEOUT CONDITION
    }

    Serial.println("\nSUCCESS: Connected to Wi-Fi");
    Serial.print("Wi-Fi SSID:  ");
    Serial.println(WIFI_SSID);
    return true;
}

/// Callback to indicate a MQTT message has been received.
void messageReceived(String &topic, String &payload) 
{
    unsigned char r, g, b;
    Serial.println("incoming: " + topic + " - " + payload);
    digitalWrite(LED_BUILTIN, LOW);
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        displayError(true);
        for (int pos = 0; pos < LED_COUNT; pos++)
            strip.setPixelColor(pos, strip.Color(0xff, 0xff, 0xff));
        strip.show();
        return;
    }
    /*
Example doc. This library doesn't seem to want to do nested arrays.
{
    "color0":[255,0,0],
    "color1":[255,0,0]
}
echo '{"color0":[255,0,255],"color1":[0,255,255]}' | base64
aws --profile home iot-data publish --topic qr_trigger --payload eyJjb2xvcjAiOlsyNTUsMCwwXSwiY29sb3IxIjpbMCwwLDI1NV19Cg==
aws --profile home iot-data publish --topic qr_trigger --payload eyJjb2xvcjAiOlsyNTUsMCwwXSwiY29sb3IxIjpbMjU1LDI1NSwwXX0K
aws --profile home iot-data publish --topic qr_trigger --payload eyJjb2xvcjAiOlsyNTUsMCwyNTVdLCJjb2xvcjEiOlswLDI1NSwyNTVdfQo=
     */
    r = doc["color0"][0];
    g = doc["color0"][1];
    b = doc["color0"][2];
    Serial.print(r); Serial.print(","); Serial.print(g); Serial.print(","); Serial.println(b);
    alertColor1 = Adafruit_NeoPixel::Color(r, g, b);
    r = doc["color1"][0];
    g = doc["color1"][1];
    b = doc["color1"][2];
    Serial.print(r); Serial.print(","); Serial.print(g); Serial.print(","); Serial.println(b);
    alertColor2 = Adafruit_NeoPixel::Color(r, g, b);
    shouldAlert = true;
    digitalWrite(LED_BUILTIN, HIGH);
    // Note: Do not use the client in the callback to publish, subscribe or
    // unsubscribe as it may cause deadlocks when other things arrive while
    // sending and receiving acknowledgments. Instead, change a global variable,
    // or push to a queue and handle it in the loop after calling `client.loop()`.
}

/// Make a connection to the AWS IOT service. Set up callbacks.
/// @return true on success, false on fail.
bool connectToAWSIoT()
{
    Serial.print("\n\nConnecting to AWS IoT");
    
    iot.setClientRSACert(&clientCertificate, &clientPrivateKey);
    iot.setTrustAnchors(&caCertificate);
    
    client.begin(AWS_ENDPOINT, 8883, iot);
    
    while (!client.connect(AWS_THINGNAME)) 
    {
        Serial.print(".");
        delay(200);
        // TODO: CATCH TIMEOUT
    }
    
    if (!client.connected()) 
    {
        Serial.println("\nERROR: AWS IoT Connection Timeout");
        return false;
    }
    
    client.subscribe(MQTT_SUB_TOPIC); // subscribe to MQTT topic
    client.onMessage(messageReceived);
    
    // display AWS IoT resource names in console
    Serial.println("\nSUCCESS: AWS IoT Endpoint Connected\n");
    Serial.print("\nAWS IoT Publish Topic:  ");
    Serial.println(MQTTT_PUB_TOPIC);
    Serial.print("\nAWS IoT Subscribe Topic:  ");
    Serial.println(MQTT_SUB_TOPIC);
    Serial.print("\nAWS IoT Thing Name:  ");
    Serial.println(AWS_THINGNAME);
    Serial.print("\nAWS IoT Endpoint:  ");
    Serial.println(AWS_ENDPOINT);
    Serial.println("\n");
    return true;
}

/// Publish a heartbeat payload to MQTT.
/// @return true on success, false on fail.
bool publishHeartbeat()
{
    StaticJsonDocument<200> doc;
    doc["time"] = timeClient.getEpochTime(); // get current NPT time/date in epoch format
    doc["heartbeat"] = 1;
    char jsonBuffer[512];
    
    serializeJson(doc, jsonBuffer);
    Serial.println(jsonBuffer);
    client.publish(MQTTT_PUB_TOPIC, jsonBuffer); // publish to MQTT topic
    // TODO: ERROR CONDITION
    return true;
}

/// Publish a heartbeat payload to MQTT, but only if 30 seconds has elapsed since the last heartbeat.
/// @return true on success, false on fail.
bool maybePublishHeartbeat()
{
    static unsigned char delayCounter = 0;
    // This is called roughly every second. We'll send a heartbeat roughly every 10 seconds.
    if (delayCounter++ >= 30)
    {
        delayCounter = 0;
        return publishHeartbeat();
    }
    return true;
}

/// Set up the hardware and libraries.
void setup() 
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Starting initialization.
    strip.begin();
    strip.show();
    strip.setBrightness(255);

    delay(2000); // some delay

    displayProgress(1, 4);
    if (!connectToWiFi())
        displayError(false);
    displayProgress(2, 4);
    if (!connectToAWSIoT())
        displayError(false);
    displayProgress(3, 4);
    if (!publishHeartbeat())
        displayError(false);
    delay(100);
    displayProgress(4, 4);
    delay(100);
    timeClient.begin();
    displayWhite();
    digitalWrite(LED_BUILTIN, HIGH); // Finished initialization.
}

void doAlertAnimation()
{
    const unsigned char COLOR_TABLE[] = {0, 0, 0, 0, 1, 1, 1, 1};
    const int COLOR_TABLE_SIZE = sizeof(COLOR_TABLE) / sizeof(COLOR_TABLE[0]);
    static unsigned char offset = 0;
    // Animate startup...
    strip.clear();
    strip.show();
    for (unsigned int pos = 0; pos < LED_COUNT; pos++)
    {
        unsigned char color = COLOR_TABLE[(pos + offset) % COLOR_TABLE_SIZE];
        if (color == 0)
        {
            strip.setPixelColor(pos, alertColor1);
        } else {
            strip.setPixelColor(pos, alertColor2);
        }
        strip.show();
        delay(20);
    }
    // Animate...
    for (int counter = 0; counter < 50; counter++)
    {
        for (unsigned int pos = 0; pos < LED_COUNT; pos++)
        {
            unsigned char color = COLOR_TABLE[(pos + offset) % COLOR_TABLE_SIZE];
            if (color == 0)
            {
                strip.setPixelColor(pos, alertColor1);
            } else {
                strip.setPixelColor(pos, alertColor2);
            }
        }
        strip.show();
        offset += COLOR_TABLE_SIZE - 1;
        offset = offset % COLOR_TABLE_SIZE;
        delay(100);
    }
    // Animate clear...
    for (unsigned int pos = 0; pos < LED_COUNT; pos++)
    {
        strip.setPixelColor(pos, strip.Color(0xff, 0xff, 0xff));
        strip.show();
        delay(20);
    }
}

unsigned char errorCount = 0;
/// Main control loop.
void loop() 
{
    bool error = false;
    // Update doesn't return a reliable error, only that the update did or did not happen.
    // So we're ignoring the return value and assuming a bad update will have side-effects
    // elsewhere that we'll catch (e.g. AWS IoT cert errors).
    timeClient.update();
    if (!maybePublishHeartbeat())
    {
        Serial.println("Error sending heartbeat.");
        error = true;
    }
    if (!client.loop())
    {
        Serial.println("Error in MQTT loop.");
        Serial.println(client.lastError());
        Serial.println(client.returnCode());
        error = true;
    }
    if (shouldAlert)
    {
        doAlertAnimation();
        shouldAlert = false;
    }
    if (!client.connected())
    {
        Serial.println("Got disconnected! Reconnecting.");
        displayProgress(2, 4);
        if (!connectToAWSIoT())
            error = true;
        else
            displayWhite();
    }
    if (error)
    {
        errorCount++;
        if (errorCount >= 10)
        {
            Serial.println("Too many errors. Aborting.");
            displayError(false);
        }
    } else {
        errorCount = 0;
    }
    delay(1000);
}
