/* asclepius-main.ino
 * This Arduino program uses the CQRobot DMX Shield with the DmxSimple library
 * and a DS3231 RTC module to create a dimming schedule for a single LED strip.
 */

#include <avr/pgmspace.h>
#include <RTClib.h>
#include <DmxSimple.h>
#include "dallas_sunrise_sunset.h"

// TRANSITION_DURATION_SEC defines the number of seconds it takes to transition
// from 0% to 100% brightness, or vice versa
#define TRANSITION_DURATION_SEC 1152

// MAX_LOOP_COUNT defines the total number of times the audio file will be looped
// each time a "scene" is executed
#define MAX_LOOP_COUNT 4

// Pin definitions
#define DMX_MASTER_MODE_PIN 2
#define DMX_DATA_TX_PIN 4
#define AUDIO_TRIGGER_PIN 7
#define AUDIO_ACTIVITY_PIN 8

// exponential dimming curve
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

RTC_DS3231 rtc;

// set the brightness level of the LED (0-255)
void setBrightness(int brightness) {
    if (brightness > 255) {
        brightness = 255;
    } else if (brightness < 0) {
        brightness = 0;
    }
    DmxSimple.write(1, brightness);
}

// convert a DateTime format to 
int convertTo24HrFormat(DateTime now) {
    return (now.hour() * 100) + now.minute();
}

// query the sound board's Act pin to see if audio is currently playing
int audioIsPlaying() {
    return (digitalRead(AUDIO_ACTIVITY_PIN) == LOW ? 1 : 0);
}

// send a trigger pulse (must be > 125ms) to the audio trigger pin
void triggerAudio() {
    digitalWrite(AUDIO_TRIGGER_PIN, LOW);
    delay(300);
    digitalWrite(AUDIO_TRIGGER_PIN, HIGH);

    // wait for Act pin to indicate playback with a 1 sec timeout
    unsigned long previous = millis();
    unsigned long current = previous;
    while(!audioIsPlaying() && current - previous < 1000) {
        current = millis();
        delay(50);
    }
    return;
}

void executeSunriseScene() {
    // calculate the delay 
    const unsigned long led_step_delay_ms = ((float)TRANSITION_DURATION_SEC / 256.0f) * 1000;
    Serial.print("led_step_delay_ms = ");
    Serial.println(led_step_delay_ms);

    // turn on the LED at the highest brightness level
    int led_step = 255;
    int brightness = pgm_read_byte(&dimming_curve[led_step]);
    setBrightness(brightness);

    // trigger audio playback
    int play_count = 1;
    triggerAudio();

    unsigned long previous = millis();
    unsigned long current = previous;

    int ramping_down_brightness = 0;
    int finished = 0;
    while(!finished) {

        if (!ramping_down_brightness) {
            current = millis();
            if (current - previous > led_step_delay_ms * 255) {
                ramping_down_brightness = 1;
            }
        } else {
            if (led_step > 0) {
                // increment the brightness step if the specified duration has
                // elapsed and the LED is not already at lowest brightness
                current = millis();
                if (current - previous > led_step_delay_ms) {
                    led_step--;
                    brightness = pgm_read_byte(&dimming_curve[led_step]);
                    setBrightness(brightness);

                    previous = millis();
                }
            }
        }

        if (!audioIsPlaying()) {
            if (play_count < MAX_LOOP_COUNT) {
                // loop audio file if max play count has not yet been reached
                triggerAudio();
                play_count++;
            } else {
                // once the final audio playback is completed, turn off the LED
                setBrightness(0);
                finished = 1;
            }
        }

        delay(100);
    }
}

void executeSunsetScene() {
    // calculate the delay 
    const unsigned long led_step_delay_ms = ((float)TRANSITION_DURATION_SEC / 256.0f) * 1000;
    Serial.print("led_step_delay_ms = ");
    Serial.println(led_step_delay_ms);

    // turn on the LED at the lowest brightness level
    int led_step = 0;
    int brightness = pgm_read_byte(&dimming_curve[led_step]);
    setBrightness(brightness);

    // trigger audio playback
    int play_count = 1;
    triggerAudio();

    unsigned long previous = millis();
    unsigned long current = previous;

    int finished = 0;
    while(!finished) {

        if (led_step < 255) {
            // increment the brightness step if the specified duration has
            // elapsed and the LED is not already at max brightness
            current = millis();
            if (current - previous > led_step_delay_ms) {
                led_step++;
                brightness = pgm_read_byte(&dimming_curve[led_step]);
                setBrightness(brightness);

                previous = millis();
            }
        }

        if (!audioIsPlaying()) {
            if (play_count < MAX_LOOP_COUNT) {
                // loop audio file if max play count has not yet been reached
                triggerAudio();
                play_count++;
            } else {
                // once the final audio playback is completed, turn off the LED
                setBrightness(0);
                finished = 1;
            }
        }

        delay(100);
    }
}

void setup () {
    Serial.begin(9600);
    delay(3000);
    Serial.println("PROGRAM START");

    // set the pin used to trigger audio playback (active low)
    pinMode(AUDIO_TRIGGER_PIN, OUTPUT);
    digitalWrite(AUDIO_TRIGGER_PIN, HIGH);

    // set the pin used to detect audio activity
    pinMode(AUDIO_ACTIVITY_PIN, INPUT_PULLUP);

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

    // turn off LED
    DmxSimple.write(1, 0);
}

void loop () {
    // obtain the current time
    DateTime now = rtc.now();
    int time_24hr = convertTo24HrFormat(now);
    int month = now.month();
    int day = now.day();

    Serial.print("Time is ");
    Serial.println(time_24hr);

    uint16_t sunrise_time = pgm_read_word(&sunrise_time_table[month-1][day-1]);
    uint16_t sunset_time = pgm_read_word(&sunset_time_table[month-1][day-1]);

    if (time_24hr == sunrise_time) {
        Serial.println("SUNRISE");

        executeSunriseScene();
        
        // wait for time to change to avoid immediately triggering the same alarm again
        while(time_24hr == sunrise_time) {
            now = rtc.now();
            time_24hr = convertTo24HrFormat(now);

            // delay to avoid slamming the RTC with requests
            delay(1000);
        }
        
    } else if (time_24hr == sunset_time) {
        Serial.println("SUNSET");

        executeSunsetScene();

        // wait for time to change to avoid immediately triggering the same alarm again
        while(time_24hr == sunset_time) {
            now = rtc.now();
            time_24hr = convertTo24HrFormat(now);

            // delay to avoid slamming the RTC with requests
            delay(1000);
        }
    }

    // delay to avoid slamming the RTC with requests
    delay(1000);
}
