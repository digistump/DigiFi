// DigiX WiFi module example - released by Digistump LLC/Erik Kettenburg under CC-BY-SA 3.0
// Inspired by HttpClient library by MCQN Ltd.

#include <DigiFi.h>

DigiFi wifi;

void setup()
{
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
  wifi.begin(9600);

  //DigiX trick - since we are on serial over USB wait for character to be entered in serial terminal
  while(!Serial.available()){
    Serial.println("Enter any key to begin");
    delay(1000);
  }

  Serial.println("Starting");

  while (wifi.ready() != 1)
  {
    Serial.println("Error connecting to network");
    delay(15000);
  }  
  
  Serial.println("Connected to wifi!");
  
  String ret = wifi.get("digistump.com","/test.txt");
  Serial.println(ret);

  wifi.close();
}

void loop()
{
  int err =0;

}
  
  /*err = wifi.get('api.thingspeak.com', '/channels/1417/field/1/last.txt');
  if (err == 0)
  {
    Serial.println("startedRequest ok");

    err = wifi.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = wifi.skipResponseHeaders();
      if (err >= 0)
      {
        int bodyLen = wifi.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");
      
        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // unless we time out or reach the end of the body
        while ( (wifi.connected() || wifi.available()) &&
               ((millis() - timeoutStart) < 30*1000) ) //30 second timeout for the network
        {
            if (http.available())
            {
                c = http.read();
                // Print out this character
                Serial.print(c);
               
                bodyLen--;
                // We read something, reset the timeout counter
                timeoutStart = millis();
            }
            else
            {
                // We haven't got any data, so let's pause to allow some to
                // arrive
                delay(1000);
            }
        }
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else
    {    
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();

  // And just stop, now that we've tried a download
  while(1);
}
*/