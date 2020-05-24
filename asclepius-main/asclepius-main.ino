/* asclepius-main.ino
 * This Arduino program uses the CQRobot DMX Shield with the DmxSimple library
 * and a DS3231 RTC module to create a dimming schedule for a single LED strip.
 */

#include <RTClib.h>
#include <DmxSimple.h>

// DIM_DELAY_MS defines the number of milliseconds between each brightness value when fading up/down
#define DIM_DELAY_MS 1000

// FADE_DOWN_TIME and FADE_UP_TIME respectively define the times when the LED strip is faded up/down
#define FADE_DOWN_TIME 600
#define FADE_UP_TIME 1735

RTC_DS3231 rtc;
int now;
int alarm1Triggered;
int alarm2Triggered;

void fadeUp(int period) {
    int brightness;
    for (brightness = 1; brightness <= 255; brightness++) {
        DmxSimple.write(1, brightness);
        delay(period);
    }
}

void fadeDown(int period) {
    int brightness;
    for (brightness = 254; brightness >= 0; brightness--) {
        DmxSimple.write(1, brightness);
        delay(period);
    }
}

int convertToMilitaryTime(DateTime now) {
    return (now.hour() * 100) + now.minute();
}

void setup () {
    // set the pin used to trigger audio playback (active low)
    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH);

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
    if (now > FADE_DOWN_TIME && now < FADE_UP_TIME) {
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
    } else if (now == FADE_DOWN_TIME && !alarm1Triggered) {
        alarm1Triggered = 1;
        digitalWrite(7, LOW);
        fadeDown(DIM_DELAY_MS);
        digitalWrite(7, HIGH);
    } else if (now == FADE_UP_TIME && !alarm2Triggered) {
        alarm2Triggered = 1;
        digitalWrite(7, LOW);
        fadeUp(DIM_DELAY_MS);
        digitalWrite(7, HIGH);
    }

    delay(10000);
}
