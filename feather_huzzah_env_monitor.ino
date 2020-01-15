// Temp & Humidity Monitor
// Donal Heidenblad
// Licensed under the GPLv3 license.
//

/************************** Configuration ***********************************/
// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

/************************ Example Starts Here *******************************/


#include <Adafruit_Si7021.h>

// set up the adafruit io feeds
AdafruitIO_Feed *temperature = io.feed("env.temp");
AdafruitIO_Feed *humidity = io.feed("env.humidity");
AdafruitIO_Feed *battery = io.feed("env.battery");
AdafruitIO_Feed *raw_battery = io.feed("env.raw-battery");

AdafruitIO_Feed *sample_rate = io.feed("env.samplerate");

// use the Si7021 sensor to check the temp and humidity
Adafruit_Si7021 sensor = Adafruit_Si7021();


#define DEFAULT_WAIT_DELAY 5000

unsigned long lastUpdate = 0;
unsigned int wait_delay = DEFAULT_WAIT_DELAY;

void setup() {
  // start the serial connection
  Serial.begin(115200);

  // connect to io.adafruit.com
  Serial.println();
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  sample_rate->onMessage(handleMessage);
  
  // wait for a connection
  while (io.mqttStatus() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  sample_rate->get();

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}

void loop() {
  io.run();

  //Serial.println(battery_count);
  if (millis() > (lastUpdate + wait_delay)) {
    
    Serial.println(millis());
    battery_level();
    env_check();

    lastUpdate = millis();
    
    sample_rate->get();
  }

}


void env_check() {
  if (!sensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    return;
  }
  float celsius = sensor.readTemperature();
  float fahrenheit = (celsius * 1.8) + 32;

  Serial.print("fahrenheit: ");
  Serial.print(fahrenheit);
  Serial.println("F");

  float relative_humidity = sensor.readHumidity();

  Serial.print("humidity: ");
  Serial.print(relative_humidity);
  Serial.println("%");

  temperature->save(fahrenheit);
  humidity->save(relative_humidity);

}

void battery_level() {
  // read the battery level from the ESP8266 analog in pin.
  // analog read level is 10 bit 0-1023 (0V-1V).
  // our 1M & 220K voltage divider takes the max
  // lipo value of 4.2V and drops it to 0.758V max.
  // this means our min analog read value should be 580 (3.14V)
  // and the max analog read value should be 774 (4.2V).
  // **** Since I didn't have a 220K resistor I used 2 100k
  // and that means that the math is:
  // using 1m & 200k voltage divider, the max is 0.7v
  // min should be 535 (3.14V) and max should be 716 (4.2V)

  int raw = analogRead(A0);
  int level = 0;

  Serial.print("Raw Battery: ");
  Serial.println(raw);
  if (raw > 0 && raw < 1024) {
    // convert battery level to percent
    //level = map(level, 580, 774, 0, 100);
    level = map(raw, 536, 717, 0, 100);
    Serial.print("Battery level: ");
    Serial.print(level);
    Serial.println("%");
  } else {
    Serial.println("Error Reading Battery");
  }

  // send battery level to AIO
  battery->save(level);
  raw_battery->save(raw);
}


// this function is called whenever a 'sample_rate' message
// is received from Adafruit IO. it was attached to
// the counter feed in the setup() function above.
void handleMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");
  Serial.println(data->value());
  
  String inValue = data->value();
  if (isValidNumber(inValue)) {
    wait_delay = inValue.toInt() * 1000;
  }
  if (wait_delay < 30000) {
    wait_delay = 30000;
  }
}

boolean isValidNumber(String str)
{
   boolean isNum=false;
   if(!(str.charAt(0) == '+' || str.charAt(0) == '-' || isDigit(str.charAt(0)))) return false;

   for(byte i=1;i<str.length();i++)
   {
       if(!(isDigit(str.charAt(i)) || str.charAt(i) == '.')) return false;
   }
   return true;
}
