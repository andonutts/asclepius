#include <DmxSimple.h>

// Pin definitions
#define DMX_MASTER_MODE_PIN 2
#define DMX_DATA_TX_PIN 4
#define AUDIO_TRIGGER_PIN 7
#define AUDIO_ACTIVITY_PIN 8

// query the sound board's Act pin to see if audio is currently playing
int audioIsPlaying() {
    return (digitalRead(AUDIO_ACTIVITY_PIN) == LOW ? 1 : 0);
}

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

void setup () {

    // set the pin used to trigger audio playback (active low)
    pinMode(AUDIO_TRIGGER_PIN, OUTPUT);
    digitalWrite(AUDIO_TRIGGER_PIN, HIGH);

    // set the pin used to detect audio activity
    pinMode(AUDIO_ACTIVITY_PIN, INPUT);

    // set the DMX module to Master mode
    pinMode(DMX_MASTER_MODE_PIN, OUTPUT);
    digitalWrite(DMX_MASTER_MODE_PIN, HIGH);

    // set the pin that the DMX module will receive commands on
    DmxSimple.usePin(DMX_DATA_TX_PIN);

    // set number of channels
    DmxSimple.maxChannel(1);

    // set LED to lowest brightness initially
    DmxSimple.write(1, 0);

    // wait 3 seconds, then turn on LED to full brightness and trigger audio playback
    delay(3000);
    triggerAudio();
    DmxSimple.write(1, 100);
}

void loop () {

    // when audio activity stops, turn off the LED
    int audio_activity_pin = digitalRead(AUDIO_ACTIVITY_PIN);
    if (audio_activity_pin == HIGH) {
        DmxSimple.write(1, 0);
    }
    delay(250);
}
