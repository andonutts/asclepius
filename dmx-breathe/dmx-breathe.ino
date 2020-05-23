#include <DmxSimple.h>

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
        delay(10);
    }
    for (brightness = 255; brightness >= 0; brightness--) {
        DmxSimple.write(1, brightness);
        delay(10);
    }
}