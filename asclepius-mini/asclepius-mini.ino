/* asclepius-mini.ino
 * This Arduino program uses a DS3231 RTC module and the DFPlayer Mini MP3
 * Player module to play an audio file at scheduled times.
 */

#include <avr/pgmspace.h>
#include <SoftwareSerial.h>
#include <RTClib.h>
#include <DFMiniMp3.h>
#include "dallas_sunrise_sunset.h"

// volume setting for the mp3 module. Can be set 0-30
// setting the mp3 module to maximum volume maximizes signal to noise ratio
#define MP3_VOLUME 30

// implement a notification class
class Mp3Notify
{
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
    if (source & DfMp3_PlaySources_Sd) 
    {
        Serial.print("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) 
    {
        Serial.print("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) 
    {
        Serial.print("Flash, ");
    }
    Serial.println(action);
  }
  static void OnError(uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }
  static void OnPlayFinished(DfMp3_PlaySources source, uint16_t track)
  {
    Serial.print("Play finished for #");
    Serial.println(track);  
  }
  static void OnPlaySourceOnline(DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "online");
  }
  static void OnPlaySourceInserted(DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "inserted");
  }
  static void OnPlaySourceRemoved(DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "removed");
  }
};

// instantiate a DFMiniMp3 object using SoftwareSerial
SoftwareSerial secondarySerial(10, 11); // RX, TX
DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(secondarySerial);

RTC_DS3231 rtc;
int track_count;

// convert a DateTime format to 
int convertTo24HrFormat(DateTime now) {
    return (now.hour() * 100) + now.minute();
}

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    // calling mp3.loop() periodically allows for notifications 
    // to be handled without interrupts
    mp3.loop();
    delay(1);
  }
}

void setup () {
    Serial.begin(115200);

    // begin I2C communication with RTC module
    if (!rtc.begin()) {
        abort();
    }

    // set the RTC time to the last compile time of this code
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    // initialize mp3 module
    mp3.begin();
    mp3.enableDac();
    mp3.setVolume(MP3_VOLUME);
    track_count = mp3.getTotalTrackCount(DfMp3_PlaySource_Sd);
    mp3.stop();
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
        mp3.playMp3FolderTrack(1);  // sd:/mp3/0001.mp3
        waitMilliseconds(5000);

        // wait for time to change to avoid immediately triggering the same alarm again
        while(time_24hr == trigger_time) {
            now = rtc.now();
            time_24hr = convertTo24HrFormat(now);

            // delay to avoid slamming the RTC with requests
            delay(5000);
        }
    }

    // delay to avoid slamming the RTC with requests
    delay(1000);
}
