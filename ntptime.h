

#include <TimeLib.h> 
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// NTP Servers:
IPAddress timeServer1(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
IPAddress timeServer2(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
IPAddress timeServer3(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
IPAddress servers[3] = {timeServer1, timeServer2, timeServer3};

const int timeZone = -6;  // Central Standard Time (USA)


String printDigits(int digits)
{
    // utility for digital clock display: prints preceding colon and leading 0
    String output = ":";
    if(digits < 10)
        output += "0";
    output += String(digits);

    return output;
}

void get_time_as_string(String &currentTimeStr)
{
     currentTimeStr = String(hourFormat12()) + printDigits(minute()) + (isAM() ? " AM" : " PM");
}


int dstOffset (unsigned long unixTime)
{
  //Receives unix epoch time and returns seconds of offset for local DST
  //Valid thru 2099 for US only, Calculations from "http://www.webexhibits.org/daylightsaving/i.html"
  //Code idea from jm_wsb @ "http://forum.arduino.cc/index.php/topic,40286.0.html"
  //Get epoch times @ "http://www.epochconverter.com/" for testing
  //DST update wont be reflected until the next time sync
  time_t t = unixTime;
  int beginDSTDay = (14 - (1 + year(t) * 5 / 4) % 7);  
  int beginDSTMonth=3;
  int endDSTDay = (7 - (1 + year(t) * 5 / 4) % 7);
  int endDSTMonth=11;
  if (((month(t) > beginDSTMonth) && (month(t) < endDSTMonth))
    || ((month(t) == beginDSTMonth) && (day(t) > beginDSTDay))
    || ((month(t) == beginDSTMonth) && (day(t) == beginDSTDay) && (hour(t) >= 2))
    || ((month(t) == endDSTMonth) && (day(t) < endDSTDay))
    || ((month(t) == endDSTMonth) && (day(t) == endDSTDay) && (hour(t) < 1)))
    return (3600);  //Add back in one hours worth of seconds - DST in effect
  else
    return (0);  //NonDST
}

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets


const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
time_t getNtpTime()
{
    for (int server_num = 0; server_num < 3; server_num++)
    {
        while (Udp.parsePacket() > 0) ; // discard any previously received packets
        Serial.println("Transmit NTP Request server " + String(server_num));
        sendNTPpacket(servers[server_num]);
        uint32_t beginWait = millis();
        while (millis() - beginWait < 2000) {
            int size = Udp.parsePacket();
            if (size >= NTP_PACKET_SIZE) {
                Serial.println("Receive NTP Response");
                Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
                unsigned long secsSince1900;
                unsigned long epoch_time;
                // convert four bytes starting at location 40 to a long integer
                secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
                secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
                secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
                secsSince1900 |= (unsigned long)packetBuffer[43];
                epoch_time = secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
                return epoch_time + dstOffset(epoch_time);
            }
        }
        Serial.println("No NTP Response :-(");
    }
    return 0; // return 0 if unable to get the time
}

void start_ntptime()
{
    Udp.begin(localPort);
    Serial.println("UDP for NTP Time started on port: " + String(Udp.localPort()));
    Serial.println("waiting for sync");
    setSyncProvider(getNtpTime);
}
