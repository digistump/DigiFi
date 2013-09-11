// DigiX WiFi module example - released by Digistump LLC/Erik Kettenburg under CC-BY-SA 3.0
// Inspired by HttpClient library by MCQN Ltd.


#include "DigiFi.h"


DigiFi::DigiFi()
{

}

void DigiFi::begin(int aBaud)
{
    Serial1.begin(aBaud);
    while(Serial1.available()){Serial1.read();} 
}

void DigiFi::startATMode()
{
    //silly init sequence for wifi module
    while(Serial1.available()){Serial1.read();} 
    //Serial.println("start at mode");
    Serial1.write("+");
    delay(1);
    Serial.println("next");
    Serial1.write("+");
    delay(1);
    Serial1.write("+");
    delay(1);
    //Serial.println("wait for a");
    while(!Serial1.available()){delay(1);}
    //Serial.println("clear buffer");
    while(Serial1.available()){Serial.write(Serial1.read());}
    Serial1.print("A"); 
    Serial.println(readResponse(0));

    Serial.println("echo off");
    Serial1.print("AT+E\r");
    Serial.println(readResponse(0));
}

void DigiFi::endATMode()
{
    //back to trasparent mode
    Serial1.print("AT+E\r");
    Serial.println(readResponse(0)); 
    Serial1.print("AT+ENTM\r");
    Serial.println(readResponse(0));
}
 

int DigiFi::ready(){
    startATMode();
    //Serial.println("send cmd");
    Serial1.print("AT+WSLK\r");
    //+ok=<ret><CR>< LF ><CR>< LF >
    //”Disconnected”, if no WiFi connection;
    //”AP’ SSID（AP’s MAC” ）, if WiFi connection available;
    //”RF Off”, if WiFi OFF;
    String ret = readResponse(0);
    Serial.println("OUT");
    Serial.println(ret);
    endATMode();
    //change this to report the AP it is connected to
    if(ret.substring(0,3) == "+ok" && ret != "+ok=RF Off" && ret != "+ok=Disconnected")
        return 1;
    else
        return 0;

}

int DigiFi::connect(char *aHost){
    Serial.println("Connect");
    startATMode();
    Serial.println("send client settings");
    //assuming port 80 for now
    Serial1.print("AT+NETP=TCP,CLIENT,80,");
    Serial1.print(aHost);
    Serial1.print("\r");
    //+ok
    String ret = readResponse(0);
    Serial.println(ret);
    endATMode();
    if(ret.substring(0,3) == "+ok")
        return 1;
    else
        return 0;

}


String DigiFi::get(char *aHost, char *aPath){
    if(connect(aHost) == 1){
        Serial1.print("GET ");
        Serial1.print(aPath);
        Serial1.print(" HTTP/1.1\r\nHost: ");
        Serial1.print(aHost);
        Serial1.print("\r\n\r\n");
        //cehck for link up?
        delay(100);
        String header = readResponse(0);
        Serial.println(header);
        //parse out content length
        //contentLength
//        Serial.println(header.substring(header.lastIndexOf("Content-Length: ")));
//        Content-Length: 6
//Content-Type: text/plain
        String contentLength = header.substring(header.lastIndexOf("Content-Length: "));
        contentLength = contentLength.substring(16,contentLength.indexOf("\n"));
        Serial.println(contentLength);
        //String contentLength = ,header.substring(header.lastIndexOf("Content-Length: ")).indexOf("\n"));
        //Serial.println(contentLength);
        String body = readResponse(contentLength.toInt());
        // Serial.println(body);
        return "1";
    }
    else
        return "0";

}




void DigiFi::close()
{
    //clear buffer
    while(Serial1.available()){Serial1.read();}
    Serial1.end();
}


String DigiFi::readResponse(int contentLength) //0 = cmd, 1 = header, 2=body
{
    String stringBuffer;
    char inByte;
    int rCount = 0;
    int nCount = 0;
    int curLength = 0;
    bool end = false;
    Serial1.flush();

    while (!end)
    {
        //look for this to be four bytes in a row
        if (Serial1.available())
        {
            inByte = Serial1.read();
            curLength++;
            Serial.write(inByte);

            if(contentLength == 0){
                if (inByte == '\n' && rCount == 2 && nCount == 1)
                {
                    end = true;
                    int strLength = stringBuffer.length()-3;
                    stringBuffer = stringBuffer.substring(0,strLength);
                }
                else if (inByte == '\r')
                {
                    rCount++;
                }
                else if (inByte == '\n')
                {
                    nCount++;
                }
                else{
                    rCount = 0;
                    nCount = 0;
                }
            }
            else if(curLength>=contentLength){
                end = true;
            }


            stringBuffer += inByte;

        }
    }

    return stringBuffer;
}

/*

void DigiFi::resetState()
{
  iState = eIdle;
  iStatusCode = 0;
  iContentLength = 0;
  iBodyLengthConsumed = 0;
  iContentLengthPtr = 0;
  iHttpResponseTimeout = kHttpResponseTimeout;
}



int DigiFi::startRequest(const char* aServerName, uint16_t aServerPort, const char* aURLPath, const char* aHttpMethod, const char* aUserAgent)
{
    tHttpState initialState = iState;
    if ((eIdle != iState) && (eRequestStarted != iState))
    {
        return HTTP_ERROR_API;
    }


    if (!iClient->connect(aServerName, aServerPort) > 0)
    {
#ifdef LOGGING
        Serial.println("Connection failed");
#endif
        return HTTP_ERROR_CONNECTION_FAILED;
    }
    

    // Now we're connected, send the first part of the request
    int ret = sendInitialHeaders(aServerName, IPAddress(0,0,0,0), aServerPort, aURLPath, aHttpMethod, aUserAgent);
    if ((initialState == eIdle) && (HTTP_SUCCESS == ret))
    {
        // This was a simple version of the API, so terminate the headers now
        finishHeaders();
    }
    // else we'll call it in endRequest or in the first call to print, etc.

    return ret;
}


int DigiFi::sendInitialHeaders(const char* aServerName, IPAddress aServerIP, uint16_t aPort, const char* aURLPath, const char* aHttpMethod, const char* aUserAgent)
{
#ifdef LOGGING
    Serial.println("Connected");
#endif
    // Send the HTTP command, i.e. "GET /somepath/ HTTP/1.0"
    iClient->print(aHttpMethod);
    iClient->print(" ");
    if (iProxyPort)
    {
      // We're going through a proxy, send a full URL
      iClient->print("http://");
      if (aServerName)
      {
        // We've got a server name, so use it
        iClient->print(aServerName);
      }
      else
      {
        // We'll have to use the IP address
        iClient->print(aServerIP);
      }
      if (aPort != kHttpPort)
      {
        iClient->print(":");
        iClient->print(aPort);
      }
    }
    iClient->print(aURLPath);
    iClient->println(" HTTP/1.1");
    // The host header, if required
    if (aServerName)
    {
        iClient->print("Host: ");
        iClient->print(aServerName);
        if (aPort != kHttpPort)
        {
          iClient->print(":");
          iClient->print(aPort);
        }
        iClient->println();
    }
    // And user-agent string
    iClient->print("User-Agent: ");
    if (aUserAgent)
    {
        iClient->println(aUserAgent);
    }
    else
    {
        iClient->println(kUserAgent);
    }

    // Everything has gone well
    iState = eRequestStarted;
    return HTTP_SUCCESS;
}

void DigiFi::sendHeader(const char* aHeader)
{
    iClient->println(aHeader);
}

void DigiFi::sendHeader(const char* aHeaderName, const char* aHeaderValue)
{
    iClient->print(aHeaderName);
    iClient->print(": ");
    iClient->println(aHeaderValue);
}

void DigiFi::sendHeader(const char* aHeaderName, const int aHeaderValue)
{
    iClient->print(aHeaderName);
    iClient->print(": ");
    iClient->println(aHeaderValue);
}

void DigiFi::sendBasicAuth(const char* aUser, const char* aPassword)
{
    // Send the initial part of this header line
    iClient->print("Authorization: Basic ");
    // Now Base64 encode "aUser:aPassword" and send that
    // This seems trickier than it should be but it's mostly to avoid either
    // (a) some arbitrarily sized buffer which hopes to be big enough, or
    // (b) allocating and freeing memory
    // ...so we'll loop through 3 bytes at a time, outputting the results as we
    // go.
    // In Base64, each 3 bytes of unencoded data become 4 bytes of encoded data
    unsigned char input[3];
    unsigned char output[5]; // Leave space for a '\0' terminator so we can easily print
    int userLen = strlen(aUser);
    int passwordLen = strlen(aPassword);
    int inputOffset = 0;
    for (int i = 0; i < (userLen+1+passwordLen); i++)
    {
        // Copy the relevant input byte into the input
        if (i < userLen)
        {
            input[inputOffset++] = aUser[i];
        }
        else if (i == userLen)
        {
            input[inputOffset++] = ':';
        }
        else
        {
            input[inputOffset++] = aPassword[i-(userLen+1)];
        }
        // See if we've got a chunk to encode
        if ( (inputOffset == 3) || (i == userLen+passwordLen) )
        {
            // We've either got to a 3-byte boundary, or we've reached then end
            b64_encode(input, inputOffset, output, 4);
            // NUL-terminate the output string
            output[4] = '\0';
            // And write it out
            iClient->print((char*)output);
// FIXME We might want to fill output with '=' characters if b64_encode doesn't
// FIXME do it for us when we're encoding the final chunk
            inputOffset = 0;
        }
    }
    // And end the header we've sent
    iClient->println();
}

void DigiFi::finishHeaders()
{
    iClient->println();
    iState = eRequestSent;
}

void DigiFi::endRequest()
{
    if (iState < eRequestSent)
    {
        // We still need to finish off the headers
        finishHeaders();
    }
    // else the end of headers has already been sent, so nothing to do here
}

int DigiFi::responseStatusCode()
{
    if (iState < eRequestSent)
    {
        return HTTP_ERROR_API;
    }
    // The first line will be of the form Status-Line:
    //   HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    // Where HTTP-Version is of the form:
    //   HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT

    char c = '\0';
    do
    {
        // Make sure the status code is reset, and likewise the state.  This
        // lets us easily cope with 1xx informational responses by just
        // ignoring them really, and reading the next line for a proper response
        iStatusCode = 0;
        iState = eRequestSent;

        unsigned long timeoutStart = millis();
        // Psuedo-regexp we're expecting before the status-code
        const char* statusPrefix = "HTTP/*.* ";
        const char* statusPtr = statusPrefix;
        // Whilst we haven't timed out & haven't reached the end of the headers
        while ((c != '\n') && 
               ( (millis() - timeoutStart) < iHttpResponseTimeout ))
        {
            if (available())
            {
                c = read();
                if (c != -1)
                {
                    switch(iState)
                    {
                    case eRequestSent:
                        // We haven't reached the status code yet
                        if ( (*statusPtr == '*') || (*statusPtr == c) )
                        {
                            // This character matches, just move along
                            statusPtr++;
                            if (*statusPtr == '\0')
                            {
                                // We've reached the end of the prefix
                                iState = eReadingStatusCode;
                            }
                        }
                        else
                        {
                            return HTTP_ERROR_INVALID_RESPONSE;
                        }
                        break;
                    case eReadingStatusCode:
                        if (isdigit(c))
                        {
                            // This assumes we won't get more than the 3 digits we
                            // want
                            iStatusCode = iStatusCode*10 + (c - '0');
                        }
                        else
                        {
                            // We've reached the end of the status code
                            // We could sanity check it here or double-check for ' '
                            // rather than anything else, but let's be lenient
                            iState = eStatusCodeRead;
                        }
                        break;
                    case eStatusCodeRead:
                        // We're just waiting for the end of the line now
                        break;
                    };
                    // We read something, reset the timeout counter
                    timeoutStart = millis();
                }
            }
            else
            {
                // We haven't got any data, so let's pause to allow some to
                // arrive
                delay(kHttpWaitForDataDelay);
            }
        }
        if ( (c == '\n') && (iStatusCode < 200) )
        {
            // We've reached the end of an informational status line
            c = '\0'; // Clear c so we'll go back into the data reading loop
        }
    }
    // If we've read a status code successfully but it's informational (1xx)
    // loop back to the start
    while ( (iState == eStatusCodeRead) && (iStatusCode < 200) );

    if ( (c == '\n') && (iState == eStatusCodeRead) )
    {
        // We've read the status-line successfully
        return iStatusCode;
    }
    else if (c != '\n')
    {
        // We must've timed out before we reached the end of the line
        return HTTP_ERROR_TIMED_OUT;
    }
    else
    {
        // This wasn't a properly formed status line, or at least not one we
        // could understand
        return HTTP_ERROR_INVALID_RESPONSE;
    }
}

int DigiFi::skipResponseHeaders()
{
    // Just keep reading until we finish reading the headers or time out
    unsigned long timeoutStart = millis();
    // Whilst we haven't timed out & haven't reached the end of the headers
    while ((!endOfHeadersReached()) && 
           ( (millis() - timeoutStart) < iHttpResponseTimeout ))
    {
        if (available())
        {
            (void)readHeader();
            // We read something, reset the timeout counter
            timeoutStart = millis();
        }
        else
        {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(kHttpWaitForDataDelay);
        }
    }
    if (endOfHeadersReached())
    {
        // Success
        return HTTP_SUCCESS;
    }
    else
    {
        // We must've timed out
        return HTTP_ERROR_TIMED_OUT;
    }
}

bool DigiFi::endOfBodyReached()
{
    if (endOfHeadersReached() && (contentLength() != kNoContentLengthHeader))
    {
        // We've got to the body and we know how long it will be
        return (iBodyLengthConsumed >= contentLength());
    }
    return false;
}

int DigiFi::read()
{
#if 0 // Fails on WiFi because multi-byte read seems to be broken
    uint8_t b[1];
    int ret = read(b, 1);
    if (ret == 1)
    {
        return b[0];
    }
    else
    {
        return -1;
    }
#else
    int ret = iClient->read();
    if (ret >= 0)
    {
        if (endOfHeadersReached() && iContentLength > 0)
	{
            // We're outputting the body now and we've seen a Content-Length header
            // So keep track of how many bytes are left
            iBodyLengthConsumed++;
	}
    }
    return ret;
#endif
}

int DigiFi::read(uint8_t *buf, size_t size)
{
    int ret =iClient->read(buf, size);
    if (endOfHeadersReached() && iContentLength > 0)
    {
        // We're outputting the body now and we've seen a Content-Length header
        // So keep track of how many bytes are left
        if (ret >= 0)
	{
            iBodyLengthConsumed += ret;
	}
    }
    return ret;
}

int DigiFi::readHeader()
{
    char c = read();

    if (endOfHeadersReached())
    {
        // We've passed the headers, but rather than return an error, we'll just
        // act as a slightly less efficient version of read()
        return c;
    }

    // Whilst reading out the headers to whoever wants them, we'll keep an
    // eye out for the "Content-Length" header
    switch(iState)
    {
    case eStatusCodeRead:
        // We're at the start of a line, or somewhere in the middle of reading
        // the Content-Length prefix
        if (*iContentLengthPtr == c)
        {
            // This character matches, just move along
            iContentLengthPtr++;
            if (*iContentLengthPtr == '\0')
            {
                // We've reached the end of the prefix
                iState = eReadingContentLength;
                // Just in case we get multiple Content-Length headers, this
                // will ensure we just get the value of the last one
                iContentLength = 0;
            }
        }
        else if ((iContentLengthPtr == kContentLengthPrefix) && (c == '\r'))
        {
            // We've found a '\r' at the start of a line, so this is probably
            // the end of the headers
            iState = eLineStartingCRFound;
        }
        else
        {
            // This isn't the Content-Length header, skip to the end of the line
            iState = eSkipToEndOfHeader;
        }
        break;
    case eReadingContentLength:
        if (isdigit(c))
        {
            iContentLength = iContentLength*10 + (c - '0');
        }
        else
        {
            // We've reached the end of the content length
            // We could sanity check it here or double-check for "\r\n"
            // rather than anything else, but let's be lenient
            iState = eSkipToEndOfHeader;
        }
        break;
    case eLineStartingCRFound:
        if (c == '\n')
        {
            iState = eReadingBody;
        }
        break;
    default:
        // We're just waiting for the end of the line now
        break;
    };

    if ( (c == '\n') && !endOfHeadersReached() )
    {
        // We've got to the end of this line, start processing again
        iState = eStatusCodeRead;
        iContentLengthPtr = kContentLengthPrefix;
    }
    // And return the character read to whoever wants it
    return c;
}


*/