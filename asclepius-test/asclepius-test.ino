/* asclepius-main.ino
 * This Arduino program uses the CQRobot DMX Shield with the DmxSimple library
 * and a DS3231 RTC module to create a dimming schedule for a single LED strip.
 */

#include <RTClib.h>
#include <DmxSimple.h>

// TRANSITION_DURATION_MINS defines the number of minutes it takes to transition
// from 0% to 100% brightness, or vice versa
#define TRANSITION_DURATION_MINS 20

// ON_DURATION_MINS defines the number of minutes for the LED to remain on after
// reaching full brightness
#define ON_DURATION_MINS 20

RTC_DS3231 rtc;
int now;
int on_time;

void fadeUp(int duration) {
    int brightness;
    int period = duration * 240;
    for (brightness = 1; brightness <= 255; brightness++) {
        DmxSimple.write(1, brightness);
        delay(period);
    }
}

void fadeDown(int duration) {
    int brightness;
    int period = duration * 240;
    for (brightness = 254; brightness >= 0; brightness--) {
        DmxSimple.write(1, brightness);
        delay(period);
    }
}

int convertToMilitaryTime(DateTime now) {
    return (now.hour() * 100) + now.minute();
}

void setup () {
    Serial.begin(9600);

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

    Serial.println("Initializing RTC");

    if (!rtc.begin()) {
        abort();
    }

    if (rtc.lostPower()) {
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    Serial.println("Turning off LED");

    // turn off LED initially
    DmxSimple.write(1, 0);
}

void loop () {
    Serial.println("Waiting 5 seconds");

    // wait 5 seconds
    delay(5000);

    Serial.println("Triggering sound board and increasing LED brightness");

    // trigger the sound board on pin 7 and begin increasing the LED brightness
    digitalWrite(7, LOW);
    fadeUp(TRANSITION_DURATION_MINS);
    digitalWrite(7, HIGH);

    Serial.println("Full brightness reached; waiting for specified duration");

    // obtain the current time and convert it to a military time integer
    on_time = convertToMilitaryTime(rtc.now());
    now = on_time;

    // leave LED on for specified duration
    while(now < on_time + ON_DURATION_MINS) {
        now = convertToMilitaryTime(rtc.now());

        // delay 5 seconds to avoid slamming the RTC with requests
        delay(5000);
    }

    Serial.println("Decreasing LED brightness");
    
    // begin decreasing the LED brightness
    fadeDown(TRANSITION_DURATION_MINS);

    Serial.println("LED is now off");
}
