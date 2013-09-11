// DigiX WiFi module example - released by Digistump LLC/Erik Kettenburg under CC-BY-SA 3.0
// Inspired by HttpClient library by MCQN Ltd.


#ifndef DigiFi_h
#define DigiFi_h

#include <Arduino.h>
#include <string.h>

class DigiFi
{
    public:
        DigiFi();
        void begin(int aBaud);
        int ready();
        int connect(char *aHost);
        String get(char *aHost, char *aPath);
        void startATMode();
        void endATMode();
        void close();
    private:
        String readResponse(int contentLength);

        

};

#endif