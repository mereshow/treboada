/*
  Treboada sensors

  This sketch uses interrupts to read pulses from 2 sensors (equipped with reed switches).
  The board enters sleep mode when not reading a sensor or sending data.
  Sleep modes allow a significant drop in the power usage of a board while it does nothing waiting for an event to happen. 
  Battery powered application can take advantage of these modes to enhance battery life significantly.
  In this sketch, the internal RTC will wake up the processor every 10 minutes. Absolute  time is kept using an DS3231 RTC.
  When the processor is woken up, it sends the data collected during 10 minutes to SigFox.
  
  Please note that, if the processor is sleeping, a new sketch can't be uploaded. To overcome this, manually reset the board (usually with a single or double tap to the RESET button)
*/

#include "Arduino.h"
#include <ArduinoLowPower.h>
#include "RTClib.h"
#include <SigFox.h>
#include <SimplyAtomic.h>

RTC_DS3231 externalRTC;

const byte sensor1Pin = 0;
const bool useSensor2 = false; //Indicates whether we are using a second sensor
const byte sensor2Pin = 1;
volatile byte sensor1PinIntCter = 0; //Counter for sensor 1 interruptions between send periods
volatile byte sensor2PinIntCter = 0; //Counter for sensor 2 interruptions between send periods
volatile bool sendData = false;      //Indicates if it is time  to send the data so SigFox

/**
 *  Increments the variable that counts how many pulses the sensor 1 sent
 * */
void incrementSensor1IntCounter()
{
    sensor1PinIntCter++;
}

/**
 *  Increments the variable that counts how many pulses the sensor 2 sent
 * */
void incrementSensor2IntCounter()
{
    sensor2PinIntCter++;
}

/**
 *  Sets the flag to send data when the RTC_ALARM is triggered
 * */
void alarmWakeUp()
{
    sendData = true;
}

/**
 *  Send data to Sigfox
 * */
void sendData2Sigfox()
{
    byte value1;
    byte value2;

    ATOMIC() // Execute atomic code: Alternative to noInterrupts(); and interrupts(); saving interruptions state
    {
        value1 = sensor1PinIntCter;
        sensor1PinIntCter = 0;
        if (useSensor2)
        {
            value2 = sensor2PinIntCter;
            sensor2PinIntCter = 0;
        }
    }

    // Keep the SigFox module on as low as possible
    SigFox.begin();
    // Wait at least 30mS after first configuration (100mS before)
    delay(100);
    // Clears all pending interrupts
    //SigFox.status();
    //delay(1);
    SigFox.beginPacket();
    SigFox.write(value1);
    if (useSensor2)
        SigFox.write(value2);
    SigFox.endPacket();
    SigFox.end(); // shut down module, back to standby
}

/**
 * Sleeps until it is time to send the data (each 10 minutes)
*/
void sleep()
{
    DateTime now = externalRTC.now();
    DateTime nextWakeUp = now + TimeSpan(0, 0, 10 - 1 - (now.minute() % 10), 60 - (now.second() % 60));
    uint32_t sleepTime = nextWakeUp.unixtime() - now.unixtime();
    LowPower.deepSleep(sleepTime * 1000);
}

void setup()
{
    if (!SigFox.begin()) //something is really wrong, try rebooting
    {
        NVIC_SystemReset();
        while (1)
            ;
    }

    SigFox.end(); //Send module to standby until we need to send a message

    if (!externalRTC.begin())
    {
        //TODO flash 3 times
    }

    //Call alarmWakeUp function when the RTC wakes up the system
    LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, alarmWakeUp, CHANGE);

    pinMode(sensor1Pin, INPUT_PULLUP); // The sensor 1 uses a reed switch, we have to use a pullup resistor
    LowPower.attachInterruptWakeup(digitalPinToInterrupt(sensor1Pin), incrementSensor1IntCounter, RISING);
    if (useSensor2)
    {
        pinMode(sensor2Pin, INPUT_PULLUP); // The sensor 2 uses a reed switch, we have to use a pullup resistor
        LowPower.attachInterruptWakeup(digitalPinToInterrupt(sensor2Pin), incrementSensor2IntCounter, RISING);
    }
}

void loop()
{
    sleep(); // Puts the system to sleep: the power consumption of the chip will drop consistently
    if (sendData)
    {
        sendData2Sigfox();
        //BUG: SigFox.endpacket causes RTC alarm handler to be called again (setting sendData = true).
        //We have to set "sendData = false" after sending. Using SigFox.status() does not solve this!
        sendData = false;
    }
}