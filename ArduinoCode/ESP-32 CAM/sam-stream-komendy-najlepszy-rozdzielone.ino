
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"             // wyłącza wykrycie brownoutu
#include "soc/rtc_cntl_reg.h"    // wyłącza wykrycie brownoutu - typowe przy tym modelu
#include "esp_http_server.h"
#include "esp_camera.h"

const char *ssid = "Orange_Swiatlowod_095A";  //nazwa wi-fi
const char *password = "WWZYKLF6PR4V";        //hasło wi-fi

/*
// IP statyczne
IPAddress local_IP(192,168,1,28);
//http://192.168.1.27/stream 
IPAddress gateway(192,168,1,1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);
*/


String komenda = "";//zmienna do kumunikacji UART

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //wyłaczenie brownoutu
  Serial.begin(115200);//ustawienie szybkosci transmisji portu szeregowego w bit/s
  
  camera_ini();//obsługa kamery w drugim pliku
/*
  // Wi-Fi connection
if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
Serial.println("STA Failed to configure");
}//przypadek uzycia własnorecznie wybranego IP
*/
Serial.print("Łaczenie z  ");
Serial.println(ssid);
WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(",");
}

// wyswietlenie IP wybranego przez ruter
Serial.println("");
Serial.println("Wi-fi połaczone");
Serial.println("IP adres: ");
Serial.println(WiFi.localIP());

startCameraServer();

pinMode(4, OUTPUT);

digitalWrite(4, HIGH);
delay(500);
digitalWrite(4, LOW);
//zaświecenie dioda LED
//sygnalizacji udanej inicjalizacji programu
}

void loop() {

   if(Serial.available() > 0) {
    //jezeli port dostepny
    komenda = Serial.readStringUntil('\n');
    // zapis otrzymane polecenie w zmiennej
    
     //obsługa LEDU
     if (komenda.startsWith("on"))  
     digitalWrite(4, HIGH);
     if (komenda.startsWith("off"))  
     digitalWrite(4, LOW);

     //ustawienie efektu
     if (komenda.startsWith("Gray"))  
     {
     sensor_t * s = esp_camera_sensor_get();
     s->set_special_effect(s, 2);
     }
     if (komenda.startsWith("No Effect"))  
     {
     sensor_t * s = esp_camera_sensor_get();
     s->set_special_effect(s, 0);
     }
     
     //zmiana rozdzielczości
     if (komenda.startsWith("VGA"))  
     {
      sensor_t * s = esp_camera_sensor_get();
      s->set_framesize(s, FRAMESIZE_VGA);
     }
     if (komenda.startsWith("HVGA"))  
     {
      sensor_t * s = esp_camera_sensor_get();
      s->set_framesize(s, FRAMESIZE_HVGA);
     }

     //Zmiana jakości
     if (komenda.startsWith("quality")) 
     {
      sensor_t * s = esp_camera_sensor_get();
      komenda.replace("quality","");
      int set=komenda.toInt();
      s->set_quality(s, set);
     }
     
   }
   
   
}
