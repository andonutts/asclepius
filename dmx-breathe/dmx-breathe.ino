/* dmx-breathe.ino
 * This Arduino program uses the CQRobot DMX Shield with the DMXSimple library
 * to repeatedly ramp up and ramp down the brightness of a single LED strip,
 * creating a "breathing" effect.
 */

#include <DmxSimple.h>

// DIM_DELAY_MS defines the number of milliseconds between each brightness value when fading up/down
#define DIM_DELAY_MS 10

void setup() {
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    DmxSimple.usePin(4);

    DmxSimple.maxChannel(1);
}

void loop() {
    int brightness;
    // Simple loop to ramp up and down brightness
    for (brightness = 0; brightness <= 255; brightness++) {
        DmxSimple.write(1, brightness);
        delay(DIM_DELAY_MS);
    }
    for (brightness = 255; brightness >= 0; brightness--) {
        DmxSimple.write(1, brightness);
        delay(DIM_DELAY_MS);
    }
}