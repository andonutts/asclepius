/* asclepius-main.ino
 * This Arduino program uses the CQRobot DMX Shield with the DmxSimple library
 * and a DS3231 RTC module to create a dimming schedule for a single LED strip.
 */

#include <RTClib.h>
#include <DmxSimple.h>

// DIM_DELAY_MS defines the number of milliseconds between each brightness value when fading up/down
#define DIM_DELAY_MS 1000

// ALARM1_TIME and ALARM2_TIME respectively define the morning and evening alarm times
#define ALARM1_TIME 600
#define ALARM2_TIME 1735

RTC_DS3231 rtc;
int now;
int alarm1Triggered;
int alarm2Triggered;

void fadeUp(int period) {
    int brightness;
    for (brightness = 0; brightness <= 255; brightness++) {
        DmxSimple.write(1, brightness);
        delay(period);
    }
}

void fadeDown(int period) {
    int brightness;
    for (brightness = 255; brightness >= 0; brightness--) {
        DmxSimple.write(1, brightness);
        delay(period);
    }
}

int convertToMilitaryTime(DateTime now) {
    return (now.hour() * 100) + now.minute();
}

void setup () {

    // set the DMX module to Master mode
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);

    // set the pin that the module will receive commands on
    DmxSimple.usePin(4);

    // set number of channels
    DmxSimple.maxChannel(1);

    if (!rtc.begin()) {
        abort();
    }

    if (rtc.lostPower()) {
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    now = convertToMilitaryTime(rtc.now());
    if (now > ALARM1_TIME && now < ALARM2_TIME) {
        DmxSimple.write(1, 0);
    } else {
        DmxSimple.write(1, 255);
    }
}

void loop () {
    // obtain the current time and convert it to a military time integer
    now = convertToMilitaryTime(rtc.now());

    if (now == 0) {
        // reset all alarmTriggered flags at midnight
        alarm1Triggered = 0;
        alarm2Triggered = 0;
    } else if (now == ALARM1_TIME && !alarm1Triggered) {
        alarm1Triggered = 1;
        fadeDown(DIM_DELAY_MS);
    } else if (now == ALARM2_TIME && !alarm2Triggered) {
        alarm2Triggered = 1;
        fadeUp(DIM_DELAY_MS);
    }

    delay(10000);
}
