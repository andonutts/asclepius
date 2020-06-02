/* asclepius-main.ino
 * This Arduino program uses the CQRobot DMX Shield with the DmxSimple library
 * and a DS3231 RTC module to create a dimming schedule for a single LED strip.
 */

#include <avr/pgmspace.h>
#include <RTClib.h>
#include <DmxSimple.h>
#include "dallas_sunrise_sunset.h"

// TRANSITION_DURATION_MINS defines the number of minutes it takes to transition
// from 0% to 100% brightness, or vice versa
#define TRANSITION_DURATION_MINS 20

// ON_DURATION_MINS defines the number of minutes for the LED to remain on after
// reaching full brightness
#define ON_DURATION_MINS 20

// Pin definitions
#define DMX_MASTER_MODE_PIN 2
#define DMX_DATA_TX_PIN 4
#define AUDIO_TRIGGER_PIN 7

RTC_DS3231 rtc;

void fadeUp(int duration) {
    int brightness;
    int period = duration * 240;
    for (brightness = 2; brightness <= 255; brightness++) {
        DmxSimple.write(1, brightness);
        delay(period);
    }
    return;
}

void fadeDown(int duration) {
    int brightness;
    int period = duration * 240;
    for (brightness = 254; brightness >= 1; brightness--) {
        DmxSimple.write(1, brightness);
        delay(period);
    }
    return;
}

int convertTo24HrFormat(DateTime now) {
    return (now.hour() * 100) + now.minute();
}

void triggerAudio() {
    // send a trigger pulse (must be > 125ms) to the audio trigger pin
    digitalWrite(AUDIO_TRIGGER_PIN, LOW);
    delay(500);
    digitalWrite(AUDIO_TRIGGER_PIN, HIGH);
    return;
}

void setup () {
    // set the pin used to trigger audio playback (active low)
    pinMode(AUDIO_TRIGGER_PIN, OUTPUT);
    digitalWrite(AUDIO_TRIGGER_PIN, HIGH);

    // set the DMX module to Master mode
    pinMode(DMX_MASTER_MODE_PIN, OUTPUT);
    digitalWrite(DMX_MASTER_MODE_PIN, HIGH);

    // set the pin that the module will receive commands on
    DmxSimple.usePin(DMX_DATA_TX_PIN);

    // set number of channels
    DmxSimple.maxChannel(1);

    // begin I2C communication with RTC module
    if (!rtc.begin()) {
        abort();
    }

    if (rtc.lostPower()) {
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // set LED to lowest brightness initially
    DmxSimple.write(1, 1);
}

void loop () {
    // obtain the current time
    DateTime now = rtc.now();
    int time_24hr = convertTo24HrFormat(now);
    int month = now.month();
    int day = now.day();

    uint16_t sunrise_time = pgm_read_word(&sunrise_time_table[month-1][day-1]);
    uint16_t sunset_time = pgm_read_word(&sunset_time_table[month-1][day-1]);

    if (time_24hr == sunrise_time || time_24hr == sunset_time) {
        int trigger_time = time_24hr;
        int fade_down_time = time_24hr + ON_DURATION_MINS >= 2400 ? time_24hr + ON_DURATION_MINS - 2400 : time_24hr + ON_DURATION_MINS;
        triggerAudio();

        fadeUp(TRANSITION_DURATION_MINS);
        while(time_24hr != fade_down_time) {
            now = rtc.now();
            time_24hr = convertTo24HrFormat(now);

            // delay to avoid slamming the RTC with requests
            delay(2000);
        }
        fadeDown(TRANSITION_DURATION_MINS);

        // wait for time to change to avoid immediately triggering the same alarm again
        while(time_24hr == trigger_time) {
            now = rtc.now();
            time_24hr = convertTo24HrFormat(now);

            // delay to avoid slamming the RTC with requests
            delay(2000);
        }
    }

    // delay to avoid slamming the RTC with requests
    delay(5000);
}
