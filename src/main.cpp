#include <Arduino.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <FS.h>  
#include <String>
#include <iostream>
#include <sstream>
#include <vector>
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
//#include <ArduinoHttpClient.h>

using namespace std;

WiFiManager wifiManager;
String NTPApi = "http://quan.suning.com/getSysTime.do";
String WeatherApi = "http://api.seniverse.com/v3/weather/daily.json?key=SHyKztGdAomYp5BNL&location=ip&language=en&unit=c&start=0&days=5";
char json_o[6000] =  {0};
StaticJsonDocument<2000> doc;
HTTPClient httpclient;
WiFiClient wifi;
int httpclientCode;

String Split(String data, const char* separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==*separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
void UARTSendPKG(uint8_t* ptag,uint8_t* b1,uint8_t* b2,uint8_t* b3,uint8_t* b4,uint8_t* b5,uint8_t* b6)
{
	uint8_t pkg[10];
	uint8_t sum;
	uint8_t UARTHD=0xff;
	uint8_t UARTED=0xfe;
	uint8_t UARTEP=0x00;
	// Serial.write((uint8_t*)&UARTHD,1);
	// Serial.write(ptag,1);
	// Serial.write(b1,1);
	// Serial.write(b2,1);
	// Serial.write(b3,1);
	// Serial.write(b4,1);
	// Serial.write(b5,1);
	// Serial.write(b6,1);
	// sum=*b1+*b2+*b3+*b4+*b5+*b6;
	// Serial.write((uint8_t*)&sum,1);
	// Serial.write((uint8_t*)&UARTED,1);

	pkg[0]=UARTHD;
	pkg[1]=*ptag;
	pkg[2]=*b1;
	pkg[3]=*b2;
	pkg[4]=*b3;
	pkg[5]=*b4;
	pkg[6]=*b5;
	pkg[7]=*b6;
	pkg[8]=(*b1+*b2+*b3+*b4+*b5+*b6)&0xff;
	pkg[9]=UARTED;
	Serial.write(pkg,10);
}
inline uint8_t DectoBCD(uint8_t Dec)
{
	return ((Dec/10)<<4)|((Dec%10)&0x0f);
}
struct timedate
{
  /* data */
  uint8_t sec;
  uint8_t min;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint8_t year;
};
struct timedate NTP;
struct weather
{
  /* data */
  uint8_t code_day;
  uint8_t code_night;
  uint8_t high;
  uint8_t low;
  uint8_t rainfall;
  uint8_t wind_scale;
  uint8_t humidity;
};
struct weather WT[3];

uint8_t UARTRX;
uint8_t tag;
static uint8_t checkok[]={0x55,0x66,0x3f,0x66,0x55,0x00};
static uint8_t checkerr[]={0x55,0x66,0x5f,0x66,0x55,0x00};

void setup() {
	//wifiManager.setTimeout(10);
	wifiManager.autoConnect("AwsCubeAP");
	Serial.println("connected...yeey :)");
	httpclient.begin(wifi,NTPApi);
	httpclient.GET();
	httpclient.end();
	Serial.begin(115200);
}

void loop() {
    UARTRX=0xff;
    Serial.readBytes(&UARTRX,1);
    switch(UARTRX)
    {
    case 0:
    	tag=0x00;
		httpclient.begin(wifi,NTPApi);
		//Serial.println("connected...yeey :)");
		httpclientCode=httpclient.GET();
		if(httpclientCode==HTTP_CODE_OK)
		{
			UARTSendPKG(&tag,checkok,checkok+1,checkok+2,checkok+3,checkok+4,checkok+5);
		}
		else
		{
			UARTSendPKG(&tag,checkerr,checkerr+1,checkerr+2,checkerr+3,checkerr+4,checkerr+5);
		}
		httpclient.end();
		break;
	case 1:
		tag=0x01;
		httpclient.begin(wifi,NTPApi);
		httpclientCode=httpclient.GET();
		if(httpclientCode==HTTP_CODE_OK)
		{
			String RX=httpclient.getString();
			for (unsigned int i = 0; i < RX.length(); i++) 
			{
				json_o[i]=RX[i];
			}
			deserializeJson(doc, json_o);
			String tim2=doc["sysTime2"];
			NTP.sec=DectoBCD(atoi(Split(Split(tim2," ",1),":",2).c_str()));
			NTP.min=DectoBCD(atoi(Split(Split(tim2," ",1),":",1).c_str()));
			NTP.hour=DectoBCD(atoi(Split(Split(tim2," ",1),":",0).c_str()));
			NTP.day=DectoBCD(atoi(Split(Split(tim2," ",0),"-",2).c_str()));
			NTP.month=DectoBCD(atoi(Split(Split(tim2," ",0),"-",1).c_str()));
			NTP.year=DectoBCD(atoi(Split(Split(tim2," ",0),"-",0).c_str())-2000);
			//Serial.print(tim2+"\r\n");
			UARTSendPKG(&tag,&NTP.year,&NTP.month,&NTP.day,&NTP.hour,&NTP.min,&NTP.sec);
		}
		else
		{
			UARTSendPKG(&tag,checkerr,checkerr+1,checkerr+2,checkerr+3,checkerr+4,checkerr+5);
		}
		httpclient.end();
		break;
	case 2:
		tag=0x02;
		httpclient.begin(wifi,WeatherApi);
		httpclientCode=httpclient.GET();
		if(httpclientCode==HTTP_CODE_OK)
		{
			String RX=httpclient.getString();
			for (int i = 0; i < RX.length(); i++) //string类型转char[]类型
			{
				json_o[i]=RX[i];
			}
			deserializeJson(doc, json_o);
			WT[0].code_day=atoi(doc["results"][0]["daily"][0]["code_day"]);
			WT[0].code_night=atoi(doc["results"][0]["daily"][0]["code_night"]);
			WT[0].high=atoi(doc["results"][0]["daily"][0]["high"]);
			WT[0].low=atoi(doc["results"][0]["daily"][0]["low"]);
			WT[0].humidity=atoi(doc["results"][0]["daily"][0]["humidity"]);
			WT[0].wind_scale=atoi(doc["results"][0]["daily"][0]["wind_scale"]);
			UARTSendPKG(&tag,&WT[0].code_day,&WT[0].code_night,&WT[0].high,&WT[0].low,&WT[0].humidity,&WT[0].wind_scale);
		}
		else
		{
			UARTSendPKG(&tag,checkerr,checkerr+1,checkerr+2,checkerr+3,checkerr+4,checkerr+5);
		}
		httpclient.end();
		break;
	case 3:
		tag=0x03;
		httpclient.begin(wifi,WeatherApi);
		httpclientCode=httpclient.GET();
		if(httpclientCode==HTTP_CODE_OK)
		{
			String RX=httpclient.getString();
			for (int i = 0; i < RX.length(); i++) //string类型转char[]类型
			{
				json_o[i]=RX[i];
			}
			deserializeJson(doc, json_o);
			WT[1].code_day=atoi(doc["results"][0]["daily"][1]["code_day"]);
			WT[1].code_night=atoi(doc["results"][0]["daily"][1]["code_night"]);
			WT[1].high=atoi(doc["results"][0]["daily"][1]["high"]);
			WT[1].low=atoi(doc["results"][0]["daily"][1]["low"]);
			WT[1].humidity=atoi(doc["results"][0]["daily"][1]["humidity"]);
			WT[1].wind_scale=atoi(doc["results"][0]["daily"][1]["wind_scale"]);
			UARTSendPKG(&tag,&WT[1].code_day,&WT[1].code_night,&WT[1].high,&WT[1].low,&WT[1].humidity,&WT[1].wind_scale);
		}
		else
		{
			UARTSendPKG(&tag,checkerr,checkerr+1,checkerr+2,checkerr+3,checkerr+4,checkerr+5);
		}
		httpclient.end();
		break;
	case 4:
		tag=0x04;
		httpclient.begin(wifi,WeatherApi);
		httpclientCode=httpclient.GET();
		if(httpclientCode==HTTP_CODE_OK)
		{
			String RX=httpclient.getString();
			for (int i = 0; i < RX.length(); i++) //string类型转char[]类型
			{
				json_o[i]=RX[i];
			}
			deserializeJson(doc, json_o);
			WT[2].code_day=atoi(doc["results"][0]["daily"][2]["code_day"]);
			WT[2].code_night=atoi(doc["results"][0]["daily"][2]["code_night"]);
			WT[2].high=atoi(doc["results"][0]["daily"][2]["high"]);
			WT[2].low=atoi(doc["results"][0]["daily"][2]["low"]);
			WT[2].humidity=atoi(doc["results"][0]["daily"][2]["humidity"]);
			WT[2].wind_scale=atoi(doc["results"][0]["daily"][2]["wind_scale"]);
			UARTSendPKG(&tag,&WT[2].code_day,&WT[2].code_night,&WT[2].high,&WT[2].low,&WT[2].humidity,&WT[2].wind_scale);
		}
		else
		{
			UARTSendPKG(&tag,checkerr,checkerr+1,checkerr+2,checkerr+3,checkerr+4,checkerr+5);
		}
		httpclient.end();
		break;
	case 0xff:
		break;
    }
}