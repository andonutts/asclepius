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

const uint8_t dimming_curve[256] PROGMEM = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
    2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5,
    5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17,
    17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25, 25,
    26, 27, 27, 28, 29, 29, 30, 31, 31, 32, 33, 33, 34, 35, 36, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 43, 44, 45, 46, 47, 48, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 57, 58, 59, 60, 61, 62, 63, 64,
    65, 66, 67, 68, 69, 70, 71, 72, 74, 75, 76, 77, 78, 79, 80, 81,
    82, 83, 85, 86, 87, 88, 89, 90, 92, 93, 94, 95, 96, 98, 99, 100,
    101, 103, 104, 105, 106, 108, 109, 110, 112, 113, 114, 116, 117, 118, 120, 121,
    122, 124, 125, 127, 128, 129, 131, 132, 134, 135, 137, 138, 140, 141, 143, 144,
    146, 147, 149, 150, 152, 153, 155, 156, 158, 159, 161, 163, 164, 166, 167, 169,
    171, 172, 174, 176, 177, 179, 181, 182, 184, 186, 187, 189, 191, 193, 194, 196,
    198, 200, 201, 203, 205, 207, 208, 210, 212, 214, 216, 218, 219, 221, 223, 225,
    227, 229, 231, 233, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254, 255
};

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
