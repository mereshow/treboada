# Treboada project sensors

The Treboada project...

![Project diagram](./doc/sensors_drawing.jpg "Project diagram")

![Arduino core sketch](./doc/sensors_drawing.jpg "Arduino core sketch")


This sketch uses interrupts to read pulses from pluviometers (or any sensor equipped with reed switches).
The board enters sleep mode when not reading a sensor or sending data: sleep modes allow a significant drop in the power usage of a board while it does nothing waiting for an event to happen (thus enhancing battery life significantly).

The internal RTC will wake up the processor every 10 minutes (at 00:00, 10:00, 20:00, etc). Absolute  time is kept using an DS3231 RTC. When the processor is woken up, it sends the data collected during those 10 minutes to SigFox.

>Please **NOTE** that, if the processor is sleeping, a new sketch can't be uploaded. To overcome this, manually reset the board (usually with a single or double tap to the RESET button). **On the Arduino MKR Fox 1200 (and others in the MKR family?) you have to press the reset button for a couple of second, release and press it again quickly and briefly. An orange led should turn on, allowing to upload the sketch.**