// DigiX WiFi module example - released by Digistump LLC/Erik Kettenburg under CC-BY-SA 3.0

#include "DigiFi.h"
#include "DigiFiRingBuffer.h"
#include "DigiFiUSARTClass.h"
#define DEBUG

DigiFiRingBuffer digifi_rx_buffer;
DigiFiUSARTClass DigiFiSerial(USART0, USART0_IRQn, ID_USART0, &digifi_rx_buffer);


void USART0_Handler(void)
{
  DigiFiSerial.IrqHandler();
}

DigiFi::DigiFi()
{

}

/* Stream Implementation */
int DigiFi::available( void )
{
    return DigiFiSerial.available();
}
int DigiFi::peek( void )
{
    return DigiFiSerial.peek();
}
int DigiFi::read( void )
{
    return DigiFiSerial.read();
}
void DigiFi::flush( void )
{
    return DigiFiSerial.flush();
}
void DigiFi::setFlowControl( boolean en )
{
    return DigiFiSerial.setFlowControl(en);
}
size_t DigiFi::write( const uint8_t c )
{
    return DigiFiSerial.write(c);
}

void DigiFi::begin(int aBaud)
{
    setFlowControl(true);
    DigiFiSerial.begin(aBaud);
    
    /** /
    //Enable USART HW Flow Control
    USART0->US_MR |= US_MR_USART_MODE_HW_HANDSHAKING;
    
    //Disable PIO Control of URTS pin
    PIOB->PIO_ABSR |= (0u << 25);
    PIOB->PIO_PDR |= PIO_PB25A_RTS0;
    
    //Disable PIO Control of UCTS pin
    PIOB->PIO_ABSR |= (0u << 26);
    PIOB->PIO_PDR |= PIO_PB26A_CTS0;
    
    //Disable PIO Control of WRTS pin
    PIOC->PIO_ABSR |= (0u << 27);
    PIOC->PIO_PDR |= (1u << 27);
    
    //Disable PIO Control of WCTS pin
    PIOC->PIO_ABSR |= (0u << 20);
    PIOC->PIO_PDR |= (1u << 20);
    /**/
    while(DigiFiSerial.available()){DigiFiSerial.read();} 
}

void DigiFi::startATMode()
{
    //silly init sequence for wifi module
    while(DigiFiSerial.available()){DigiFiSerial.read();} 
    debug("start at mode");
    debug("next");
    DigiFiSerial.write("+++");
    debug("wait for a");
    while(!DigiFiSerial.available()){delay(1);}
    debug("clear buffer");
    while(DigiFiSerial.available()){DigiFiSerial.read();}
    DigiFiSerial.print("A"); 
    debug(readResponse(0));

    debug("echo off");
    DigiFiSerial.print("AT+E\r");
    debug(readResponse(0));
}

void DigiFi::endATMode()
{
    //back to trasparent mode
    DigiFiSerial.print("AT+E\r");
    debug(readResponse(0)); 
    DigiFiSerial.print("AT+ENTM\r");
    debug(readResponse(0));
    debug("exit at mode");
}
 
bool DigiFi::ready(){
    startATMode();
    //debug("send cmd");
    //+ok=<ret><CR>< LF ><CR>< LF >
    //”Disconnected”, if no WiFi connection;
    //”AP’ SSID（AP’s MAC” ）, if WiFi connection available;
    //”RF Off”, if WiFi OFF;
    debug("Check Link");
    String ret = STALinkStatus();
    debug("OUT");
    debug(ret);
    endATMode();
    debug(ret);
    //change this to report the AP it is connected to
    if(ret.substring(0,3) == "+ok" && ret != "+ok=RF Off" && ret != "+ok=Disconnected")
        return 1;
    else
        return 0;
}

bool DigiFi::connect(char *aHost){
    debug("Connect");
    startATMode();
    debug("send client settings");
    setTCPConn("off");
    //assuming port 80 for now
    String conn=getNetParams();
    conn=conn.substring(4,conn.length()-4);
    if(conn != lastHost)
        setNetParams("TCP","CLIENT",80,aHost);
    
    lastHost = conn;

    setTCPConn("On");
    
    debug("Checking for link build up");
    String status=getTCPLnk();
    while(status.substring(0,6)!="+ok=on"){
        debug("Re-checking for link build up");
        status=getTCPLnk();
        debug(status);
    }

    endATMode();
    
    return 1;
}

String DigiFi::body(){
    return aBody;
}

String DigiFi::header(){
    return aHeader;
}

void DigiFi::debug(String output){
    #ifdef DEBUG
        Serial.println(output);
    #endif
}

void DigiFi::debugWrite(char output){
    #ifdef DEBUG
        Serial.write(output);
    #endif
}

bool DigiFi::get(char *aHost, char *aPath){
    if(connect(aHost) == 1){
        //delay(500);
        DigiFiSerial.print("GET ");
        DigiFiSerial.print(aPath);
        DigiFiSerial.print(" HTTP/1.1\r\nHost: ");
        DigiFiSerial.print(aHost);
        DigiFiSerial.print("\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n");
        DigiFiSerial.flush();

        //don't block while awating reply
        debug("wait for response...");
        bool success = true;
        int i=0;
        int st = millis();
        while(!DigiFiSerial.available()){
            if(millis() - st > requestTimeout * 1000) {
                success = false; 
                break;
            } 
            if(((millis() - st) % 1000) == 1)
                debugWrite('.');
            i++; 
        }
        debug("get header");
        if(success == false)
            return 0;
        aHeader = readResponse(0);
        debug(aHeader);

        String contentLength = aHeader.substring(aHeader.lastIndexOf("Content-Length: "));
        contentLength = contentLength.substring(16,contentLength.indexOf("\n"));
        debug(contentLength);

         debug("get body");
        aBody = readResponse(contentLength.toInt());

        return 1;
    }
    else
        return 0;

    //To do:
    /*
    User agent!
    Better handle timeouts/other errors
    Actually look at returned header for status
    Efficiency!
    */

}

String DigiFi::URLEncode(char *msg)
{
    //const char *msg = *smsg;//smsg.c_str();
    const char *hex = "0123456789abcdef";
    String encodedMsg = "";

    while (*msg!='\0'){
        if( ('a' <= *msg && *msg <= 'z')
                || ('A' <= *msg && *msg <= 'Z')
                || ('0' <= *msg && *msg <= '9') ) {
            encodedMsg += *msg;
        } else {
            encodedMsg += '%';
            encodedMsg += hex[*msg >> 4];
            encodedMsg += hex[*msg & 15];
        }
        msg++;
    }
    return encodedMsg;
}

bool DigiFi::post(char *aHost, char *aPath, String postData){
    if(connect(aHost) == 1){



        DigiFiSerial.print("POST ");
        DigiFiSerial.print(aPath);
        DigiFiSerial.print(" HTTP/1.1\r\nHost: ");
        DigiFiSerial.print(aHost);
        DigiFiSerial.print("\r\nCache-Control: no-cache\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n");
        DigiFiSerial.print("Content-Length: ");
        DigiFiSerial.print(postData.length());
        DigiFiSerial.print("\r\n\r\n");
        DigiFiSerial.print(postData);
        DigiFiSerial.print("\r\n\r\n");
        DigiFiSerial.flush();


        debug("wait for response...");
        bool success = true;
        int i=0;
        int st = millis();
        while(!DigiFiSerial.available()){
            if(millis() - st > requestTimeout * 1000) {
                success = false; 
                break;
            } 
            if(((millis() - st) % 1000) == 1)
                debugWrite('.');
            i++; 
        }
        
        if(success == false)
            return 0;

        debug("get header");
        aHeader = readResponse(0);
        debug(aHeader);


        String contentLength = aHeader.substring(aHeader.lastIndexOf("Content-Length: "));
        contentLength = contentLength.substring(16,contentLength.indexOf("\n"));
        debug(contentLength);

        debug("get body");
        aBody = readResponse(contentLength.toInt());

        return 1;
    }
    else
        return 0;

    //To do:
    /*
    User agent!
    accept post data as array or array or string, etc
    Better handle timeouts/other errors
    Actually look at returned header for status
    Efficiency!
    */

}

void DigiFi::close()
{
    //clear buffer
    while(DigiFiSerial.available()){DigiFiSerial.read();}
    DigiFiSerial.end();
}

String DigiFi::readResponse(int contentLength) //0 = cmd, 1 = header, 2=body
{
    String stringBuffer;
    char inByte;
    int rCount = 0;
    int nCount = 0;
    int curLength = 0;
    bool end = false;
    DigiFiSerial.flush();

    while (!end)
    {
        //look for this to be four bytes in a row
        if (DigiFiSerial.available())
        {
            inByte = DigiFiSerial.read();
            curLength++;
            debugWrite(inByte);

            if(contentLength == 0){
                if (inByte == '\n' && rCount == 2 && nCount == 1)
                {
                    end = true;
                    int strLength = stringBuffer.length()-3;
                    stringBuffer = stringBuffer.substring(0,strLength);
                }
                else if (inByte == '\r')
                    rCount++;
                else if (inByte == '\n')
                    nCount++;
                else{
                    rCount = 0;
                    nCount = 0;
                }
            }
            else if(curLength>=contentLength)
                end = true;
            
            stringBuffer += inByte;
        }
    }

    if(stringBuffer.substring(0,4) == "+ERR")
        lastErr = stringBuffer.substring(5,2).toInt();
    else
        lastErr = 0;
    return stringBuffer;
}

int DigiFi::lastError()
{
    return lastErr;
}

String DigiFi::AT(char *cmd, char *params)
{
    DigiFiSerial.print("AT+");
    DigiFiSerial.print(cmd);
    if(sizeof(*params) > 0)
    {
        DigiFiSerial.print("=");
        DigiFiSerial.print(params);
    }
    DigiFiSerial.print("\r");
    return readResponse(0);
}
void DigiFi::toggleEcho() //E
{
    DigiFiSerial.print("AT+E\r");
    readResponse(0);
}
String DigiFi::getWifiMode() //WMODE AP STA APSTA
{
    DigiFiSerial.print("AT+WMODE\r");
    return readResponse(0);
}
void DigiFi::setWifiMode(char *mode)
{
    DigiFiSerial.print("AT+WMODE=");
    DigiFiSerial.print(mode);
    DigiFiSerial.print("\r");
    readResponse(0);
}
void DigiFi::setTransparent() //ENTM
{
    DigiFiSerial.print("AT+ENTM\r");
    readResponse(0);
}
String DigiFi::getTMode() //TMODE throughput cmd
{
    DigiFiSerial.print("AT+TMODE\r");
    return readResponse(0);
}
void DigiFi::setTMode(char *mode)
{
    DigiFiSerial.print("AT+TMODE=");
    DigiFiSerial.print(mode);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getModId() //MID
{
    DigiFiSerial.print("AT+MID\r");
    return readResponse(0);
}
String DigiFi::version() //VER
{
    DigiFiSerial.print("AT+VER\r");
    return readResponse(0);
}
void DigiFi::factoryRestore() //RELD rebooting...
{
    DigiFiSerial.print("AT+RELD\r");
    readResponse(0);
}
void DigiFi::reset() //Z (No return)
{
    DigiFiSerial.print("AT+Z\r");
    //readResponse(0);
    lastErr=0; //This command doesnt return anything.
}
String DigiFi::help()//H
{
    DigiFiSerial.print("AT+H\r");
    return readResponse(0);
}
int DigiFi::readConfig(byte* buffer)//CFGRD
{
    DigiFiSerial.print("AT+CFGRD\r");
    DigiFiSerial.readBytes((char*)buffer,4);
    if((char*)buffer=="+ERR")
        return -1; //TODO Set lastErr here (Technically it shouldn't ever error here)
    DigiFiSerial.readBytes((char*)buffer,2);
    int len=(int)word(buffer[1],buffer[0]);
    DigiFiSerial.readBytes((char*)buffer,len);
    return len;
}
void DigiFi::writeConfig(byte* config, int len)//CFGWR
{
    DigiFiSerial.print("AT+CFGWR=");
    DigiFiSerial.write(highByte(len));
    DigiFiSerial.write(lowByte(len));
    DigiFiSerial.write(config,len);
    DigiFiSerial.print("\r");
    readResponse(0);
}
int DigiFi::readFactoryDef(byte* buffer)//CFGFR
{
    DigiFiSerial.print("AT+CFGFR\r");
    DigiFiSerial.readBytes((char*)buffer,4);
    if((char*)buffer=="+ERR")
        return -1; //TODO Set lastErr here (Technically it shouldn't ever error here)
    DigiFiSerial.readBytes((char*)buffer,2);
    int len=(int)word(buffer[1],buffer[0]);
    DigiFiSerial.readBytes((char*)buffer,len);
    return len;
}
void DigiFi::makeFactory() //CFGTF
{
    DigiFiSerial.print("AT+CFGTF\r");
    readResponse(0);
}
String DigiFi::getUart()//UART baudrate,data_bits,stop_bit,parity
{
    DigiFiSerial.print("AT+UART\r");
    return readResponse(0);
}
void DigiFi::setUart(int baudrate,int data_bits,int stop_bit,char *parity)
{
    DigiFiSerial.print("AT+UART=");
    DigiFiSerial.print(baudrate);
    DigiFiSerial.print(",");
    DigiFiSerial.print(data_bits);
    DigiFiSerial.print(",");
    DigiFiSerial.print(stop_bit);
    DigiFiSerial.print(",");
    DigiFiSerial.print(parity);
    DigiFiSerial.print("\r");
    readResponse(0);
}
/*
String getAutoFrame(); //UARTF
void setAutoFrame(char *para);
int getAutoFrmTrigTime(); //UARTFT
void setAutoFrmTrigTime(int ms);
int getAutoFrmTrigLength(); //UARTFL
void setAutoFrmTrigLength(int v);
*/
void DigiFi::sendData(int len, char *data)//SEND
{
    DigiFiSerial.print("AT+SEND=");
    DigiFiSerial.print(len);
    DigiFiSerial.print(",");
    DigiFiSerial.print(data);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::recvData(int len)//RECV len,data (+ok=0 if timeout (3sec))
{
    DigiFiSerial.print("AT+RECV=");
    DigiFiSerial.print(len);
    DigiFiSerial.print("\r");
    return readResponse(0);
}
String DigiFi::ping(char *ip)//PING Success Timeout Unknown host
{
    DigiFiSerial.print("AT+PING=");
    DigiFiSerial.print(ip);
    DigiFiSerial.print("\r");
    return readResponse(0);
}
String DigiFi::getNetParams()//NETP (TCP|UDP),(SERVER|CLIENT),port,IP 
{
    DigiFiSerial.print("AT+NETP\r");
    return readResponse(0);
}
void DigiFi::setNetParams(char *proto, char *cs, int port, char *ip)
{
    DigiFiSerial.print("AT+NETP=");
    DigiFiSerial.print(proto);
    DigiFiSerial.print(",");
    DigiFiSerial.print(cs);
    DigiFiSerial.print(",");
    DigiFiSerial.print(port);
    DigiFiSerial.print(",");
    DigiFiSerial.print(ip);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getTCPLnk()//TCPLK on|off 
{
    DigiFiSerial.print("AT+TCPLK\r");
    return readResponse(0);
}
int DigiFi::getTCPTimeout()//TCPTO 0 <= int <= 600 (Def 300)
{
    DigiFiSerial.print("AT+TCPTO\r");
    readResponse(0);
}
String DigiFi::getTCPConn()//TCPDIS On|off
{
    DigiFiSerial.print("AT+TCPDIS\r");
    return readResponse(0);
}
void DigiFi::setTCPConn(char *sta)
{
    DigiFiSerial.print("AT+TCPDIS=");
    DigiFiSerial.print(sta);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getWSSSID()//WSSSID
{
    DigiFiSerial.print("AT+WSSSID\r");
    return readResponse(0);
}
void DigiFi::setWSSSID(char *ssid)
{
    DigiFiSerial.print("AT+WSSSID=");
    DigiFiSerial.print(ssid);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getSTAKey()//WSKEY (OPEN|SHARED|WPAPSK|WPA2PSK),(NONE|WEP|TKIP|AES),key
{
    DigiFiSerial.print("AT+WSKEY\r");
    return readResponse(0);
}
void DigiFi::setSTAKey(char* auth,char *encry,char *key)
{
    DigiFiSerial.print("AT+WSKEY=");
    DigiFiSerial.print(auth);
    DigiFiSerial.print(",");
    DigiFiSerial.print(encry);
    DigiFiSerial.print(",");
    DigiFiSerial.print(key);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getSTANetwork()//WANN (static|DHCP),ip,subnet,gateway
{
    DigiFiSerial.print("AT+WANN\r");
    return readResponse(0);
}
void DigiFi::setSTANetwork(char *mode, char *ip, char *subnet, char *gateway)
{
    DigiFiSerial.print("AT+WANN=");
    DigiFiSerial.print(mode);
    DigiFiSerial.print(",");
    DigiFiSerial.print(ip);
    DigiFiSerial.print(",");
    DigiFiSerial.print(subnet);
    DigiFiSerial.print(",");
    DigiFiSerial.print(gateway);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getSTAMac()//WSMAC returns MAC
{
    DigiFiSerial.print("AT+WSMAC\r");
    return readResponse(0);
}
void DigiFi::setSTAMac(int code, char *mac)//Code default is 8888, no idea what its for
{
    DigiFiSerial.print("AT+WSSSID=");
    DigiFiSerial.print(code);
    DigiFiSerial.print(",");
    DigiFiSerial.print(mac);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::STALinkStatus()//WSLK (Disconnected|AP SSID (AP MAC)|RF Off)
{
    DigiFiSerial.print("AT+WSLK\r");
    return readResponse(0);
}
String DigiFi::STASignalStrength()//WSLQ (Disconnected|Value)
{
    DigiFiSerial.print("AT+WSLQ\r");
    return readResponse(0);
}
String DigiFi::scan()//WSCAN returns list
{
    DigiFiSerial.print("AT+WSCAN\r");
    return readResponse(0);
}
String DigiFi::getSTADNS()//WSDNS address
{
    DigiFiSerial.print("AT+WSDNS\r");
    return readResponse(0);
}
void DigiFi::setSTADNS(char *dns)
{
    DigiFiSerial.print("AT+WSDNS=");
    DigiFiSerial.print(dns);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getAPNetwork()//LANN ip,subnet
{
    DigiFiSerial.print("AT+LANN\r");
    return readResponse(0);
}
void DigiFi::setAPNetwork(char *ip, char *subnet)
{
    DigiFiSerial.print("AT+LANN=");
    DigiFiSerial.print(ip);
    DigiFiSerial.print(",");
    DigiFiSerial.print(subnet);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getAPParams()//WAP (11B|11BG|11BGN),SSID,(AUTO|C1...C11)
{
    DigiFiSerial.print("AT+WAP\r");
    return readResponse(0);
}
void DigiFi::setAPParams(char *mode, char *ssid, char *channel)
{
    DigiFiSerial.print("AT+WAP=");
    DigiFiSerial.print(mode);
    DigiFiSerial.print(",");
    DigiFiSerial.print(ssid);
    DigiFiSerial.print(",");
    DigiFiSerial.print(channel);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getAPKey()//WAKEY (OPEN|WPA2PSK),(NONE|AES),key
{
    DigiFiSerial.print("AT+WAKEY\r");
    return readResponse(0);
}
void DigiFi::setAPKey(char* auth,char *encry,char *key)
{
    DigiFiSerial.print("AT+WAKEY=");
    DigiFiSerial.print(auth);
    DigiFiSerial.print(",");
    DigiFiSerial.print(encry);
    DigiFiSerial.print(",");
    DigiFiSerial.print(key);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getAPMac()//WAMAC returns MAC
{
    DigiFiSerial.print("AT+WAMAC\r");
    return readResponse(0);
}
String DigiFi::getAPDHCP()//WADHCP (on|off)
{
    DigiFiSerial.print("AT+WADHCP\r");
    return readResponse(0);
}
void DigiFi::setAPDHCP(char *status)
{
    DigiFiSerial.print("AT+WADHCP=");
    DigiFiSerial.print(status);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getAPPageDomain()//WADMN domain
{
    DigiFiSerial.print("AT+WADM\r");
    return readResponse(0);
}
void DigiFi::setAPPageDomain(char *domain)
{
    DigiFiSerial.print("AT+WADMN=");
    DigiFiSerial.print(domain);
    DigiFiSerial.print("\r");
    readResponse(0);
}
void DigiFi::setPageDisplayMode(char *mode)//WEBSWITCH (iw|ew)
{
    DigiFiSerial.print("AT+WEBSWITCH=");
    DigiFiSerial.print(mode);
    DigiFiSerial.print("\r");
    readResponse(0);
}
void DigiFi::setPageLanguage(char *lang)//PLANG CN|EN
{
    DigiFiSerial.print("AT+PLANG=");
    DigiFiSerial.print(lang);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getUpgradeUrl()//UPURL url !!!DANGEROUS!!!
{
    DigiFiSerial.print("AT+UPURL\r");
    return readResponse(0);
}
void DigiFi::setUpgradeUrl(char *url)//url,filename (filename is optional, if provided upgrade is auto started)
{
    DigiFiSerial.print("AT+UPURL=");
    DigiFiSerial.print(url);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getUpgradeFile()//UPFILE filename !!!DANGEROUS!!!
{
    DigiFiSerial.print("AT+UPFILE\r");
    return readResponse(0);
}
void DigiFi::setUpgradeFile(char *filename)
{
    DigiFiSerial.print("AT+UPFILE=");
    DigiFiSerial.print(filename);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::startUpgrade()//UPST !!!DANGEROUS!!!
{
    DigiFiSerial.print("AT+UPST\r");
    return readResponse(0);
}
String DigiFi::getWebAuth()//WEBU user,pass
{
    DigiFiSerial.print("AT+WEBU\r");
    return readResponse(0);
}
void DigiFi::setWebAuth(char *user, char *pass)
{
    DigiFiSerial.print("AT+WEBU=");
    DigiFiSerial.print(user);
    DigiFiSerial.print(",");
    DigiFiSerial.print(pass);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getSleepMode()//MSLP normal|standby
{
    DigiFiSerial.print("AT+MSLP\r");
    return readResponse(0);
}
void DigiFi::setSleepMode(char *mode)
{
    DigiFiSerial.print("AT+MSLP=");
    DigiFiSerial.print(mode);
    DigiFiSerial.print("\r");
    readResponse(0);
}
void DigiFi::setModId(char *modid)//WRMID
{
    DigiFiSerial.print("AT+WRMID=");
    DigiFiSerial.print(modid);
    DigiFiSerial.print("\r");
    readResponse(0);
}
String DigiFi::getWifiCfgPassword()//ASWD aswd
{
    DigiFiSerial.print("AT+ASWD\r");
    return readResponse(0);
}
void DigiFi::setWifiCfgPassword(char *aswd)
{
    DigiFiSerial.print("AT+ASWD=");
    DigiFiSerial.print(aswd);
    DigiFiSerial.print("\r");
    readResponse(0);
}