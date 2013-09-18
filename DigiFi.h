// DigiX WiFi module example - released by Digistump LLC/Erik Kettenburg under CC-BY-SA 3.0


#ifndef DigiFi_h
#define DigiFi_h

#include <Arduino.h>
#include <string.h>
#ifdef __cplusplus

#endif

#define DIGIFI_RTS  105
#define DIGIFI_CTS  104

class DigiFi : Stream
{
    public:
        static const int requestTimeout = 15;
        DigiFi();
        void begin(int aBaud);
        bool ready();
        bool connect(char *aHost);
        bool get(char *aHost, char *aPath);
        bool post(char *aHost, char *aPath, String postData);
        void startATMode();
        void endATMode();
        void close();
        String header();
        String body();
        int lastError();
        void debug(String output);
        void debugWrite(char output);
        String URLEncode(char *smsg);
        void setFlowControl(boolean);
        
        /* Stream Implementation */
        int available( void ) ;
        int peek( void ) ;
        int read( void ) ;
        void flush( void ) ;
        size_t write( const uint8_t c ) ;
        using Print::write ; // pull in write(str) and write(buf, size) from Print
        
        /* AT Wrappers */
        String AT(char *cmd, char *params);
        void toggleEcho(); //E
        String getWifiMode(); //WMODE AP STA APSTA
        void setWifiMode(char *mode);
        void setTransparent(); //ENTM
        String getTMode(); //TMODE throughput cmd
        void setTMode(char *mode);
        String getModId(); //MID
        String version(); //VER
        void factoryRestore(); //RELD rebooting...
        void reset(); //Z (No return)
        String help();//H
        int readConfig(byte* buffer);//CFGRD
        void writeConfig(byte* config, int len);//CFGWR
        int readFactoryDef(byte* buffer);//CFGFR
        void makeFactory(); //CFGTF
        String getUart();//UART baudrate,data_bits,stop_bit,parity
        void setUart(int baudrate,int data_bits,int stop_bit,char *parity);
        /* These are commented out as I'm unsure how they should be named
        String getAutoFrame(); //UARTF
        void setAutoFrame(char *para);
        int getAutoFrmTrigTime(); //UARTFT
        void setAutoFrmTrigTime(int ms);
        int getAutoFrmTrigLength(); //UARTFL
        void setAutoFrmTrigLength(int v);
        */
        void sendData(int len, char *data);//SEND
        String recvData(int len);//RECV len,data (+ok=0 if timeout (3sec))
        String ping(char *ip);//PING Success Timeout Unknown host
        String getNetParams();//NETP (TCP|UDP),(SERVER|CLIENT),port,IP 
        void setNetParams(char *proto, char *cs, int port, char *ip);
        String getTCPLnk();//TCPLK on|off 
        int getTCPTimeout();//TCPTO 0 <= int <= 600 (Def 300)
        String getTCPConn();//TCPDIS On|off
        void setTCPConn(char *sta);
        String getWSSSID();//WSSSID
        void setWSSSID(char *ssid);
        String getSTAKey();//WSKEY (OPEN|SHARED|WPAPSK|WPA2PSK),(NONE|WEP|TKIP|AES),key
        void setSTAKey(char* auth,char *encry,char *key);
        String getSTANetwork();//WANN (static|DHCP),ip,subnet,gateway
        void setSTANetwork(char *mode, char *ip, char *subnet, char *gateway);
        String getSTAMac();//WSMAC returns MAC
        void setSTAMac(int code, char *mac);//Code default is 8888, no idea what its for
        String STALinkStatus();//WSLK (Disconnected|AP SSID (AP MAC)|RF Off)
        String STASignalStrength();//WSLQ (Disconnected|Value)
        String scan();//WSCAN returns list
        String getSTADNS();//WSDNS address
        void setSTADNS(char *dns);
        String getAPNetwork();//LANN ip,subnet
        void setAPNetwork(char *ip, char *subnet);
        String getAPParams();//WAP (11B|11BG|11BGN),SSID,(AUTO|C1...C11)
        void setAPParams(char *mode, char *ssid, char *channel);
        String getAPKey();//WAKEY (OPEN|WPA2PSK),(NONE|AES),key
        void setAPKey(char* auth,char *encry,char *key);
        String getAPMac();//WAMAC returns MAC
        String getAPDHCP();//WADHCP (on|off)
        void setAPDHCP(char *status);
        String getAPPageDomain();//WADMN domain
        void setAPPageDomain(char *domain);
        void setPageDisplayMode(char *mode);//WEBSWITCH (iw|ew)
        void setPageLanguage(char *lang);//PLANG CN|EN
        String getUpgradeUrl();//UPURL url !!!DANGEROUS!!!
        void setUpgradeUrl(char *url);//url,filename (filename is optional, if provided upgrade is auto started)
        String getUpgradeFile();//UPFILE filename !!!DANGEROUS!!!
        void setUpgradeFile(char *filename);
        String startUpgrade();//UPST !!!DANGEROUS!!!
        String getWebAuth();//WEBU user,pass
        void setWebAuth(char *user, char *pass);
        String getSleepMode();//MSLP normal|standby
        void setSleepMode(char *mode);
        void setModId(char *modid);//WRMID
        String getWifiCfgPassword();//ASWD aswd
        void setWifiCfgPassword(char *aswd);
    private:
        String readResponse(int contentLength);
        String aHeader;
        String aBody;
        String lastHost;
        int lastErr;
};

#endif