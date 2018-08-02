/**
 * Blink
 *
 * Read any c
 * then off for one second, repeatedly.
 */
#include "Arduino.h"
#include <SigFox.h>
#include <SimplyAtomic.h>
#include <ArduinoLowPower.h>
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

// Set LED_BUILTIN if it is not defined by Arduino framework
// #define LED_BUILTIN 13

// Pin the pluviometer is connected to
const byte interruptPin = 2;
volatile byte counter = 0;

const int dataSendPeriod = 10 * 60; //Time the Arduino sleeps between sending data to Sigfox (in second)

// Increments the variable that counts how many times the pluviometer empties
void incrementCounter()
{
    Serial.write("Interrupted by pin");
    counter++;
}

void sendData()
{
    Serial.write("Interrupted by RTC");
    // This function will be called once on device wakeup
    // You can do some little operations here (like changing variables which will be used in the loop)
    // Remember to avoid calling delay() and long running functions since this functions executes in interrupt context

    // We can wake from an interruption or from a scheduled sleep, so we need to check if it's time to
    // send the data every time we wake up
    // TODO check if its time to send data when we wake up
    byte value;
    // Execute atomic code: Alternative to noInterrupts(); and interrupts(); saving interruptions state
    ATOMIC()
    {
        value = counter;
        counter = 0;
    }

    // TODO Write value to Sigfox
    digitalWrite(LED_BUILTIN, value);
}

void printTime(String header, DateTime time)
{
    Serial.print(header);
    Serial.print('= ');
    Serial.print(time.year(), DEC);
    Serial.print('-');
    Serial.print(time.month(), DEC);
    Serial.print('-');
    Serial.print(time.day(), DEC);
    Serial.print(" ");
    Serial.print(time.hour(), DEC);
    Serial.print(':');
    Serial.print(time.minute(), DEC);
    Serial.print(':');
    Serial.print(time.second(), DEC);
    Serial.println();
}

void setup()
{
    if (!SigFox.begin())
    {
        // Something is really wrong, try rebooting
        // Reboot is useful if we are powering the board using an unreliable power source
        // (eg. solar panels or other energy harvesting methods)
        NVIC_SystemReset();
        while (1)
            ;
    }

    //If RTC is not up, use internal?
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }

    // Set the interruption in the pin that read the pluviometer
    // The pluviometer used a reed switch, we have to use a pullup resistor
    pinMode(interruptPin, INPUT_PULLUP);
    // Attach a wakeup interrupt on interruptPin, calling incrementCounter when the device is woken up
    LowPower.attachInterruptWakeup(digitalPinToInterrupt(interruptPin), incrementCounter, LOW);

    // Initialize and set the RTC
    // TODO set an alarm that sends the data via Sigfox: does it call a function??
    // Uncomment this function if you wish to attach function dummy when RTC wakes up the chip
    LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, sendData, CHANGE);

    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    // Triggers a XX ms sleep (the device will be woken up only by the registered wakeup sources and by internal RTC)
    // The power consumption of the chip will drop consistently
    //TODO get the current time and sleep for 10 - current_time_minutes MOD 10 minutes BUT IN SECONDS
    DateTime now = rtc.now();
    DateTime nextWakeUp = DateTime(now.year,now.month,now.day,now.hour,now.minute, 0) + TimeSpan(0,0,10 - (now.minute % 10),0);
    //TODO write nextWakeUp to console
    printTime("Now", now);
    printTime("nextWakeUp", nextWakeUp);

    LowPower.sleep(nextWakeUp.unixtime-now.unixtime);
}