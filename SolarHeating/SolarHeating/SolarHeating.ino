/*
  Solar Heating Control

  Uses one nodeMCU board for listening to two DS180 temperature sensor
  and control a relay (that control the heating system pump) by the
  difference int the sensors located in the solar panels heat collectors 
  and in the hot water boiler. This program takes advantage of the onboard
  wifi capabilities to broadcast the sensors values.

  It also uses a LCD in debuging time to print some information.

  created 2018
  by Aldo Monteiro

  This example code is in the public domain.

  https://github.com/alduxx/alduxino
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
 
OneWire oneWire(D4);  
DallasTemperature sensors(&oneWire);

hd44780_I2Cexp lcd;

const char* ssid = "container_repeater";
const char* password = "isabella";

ESP8266WebServer server(80);
IPAddress ip(192,168,100,111); 
IPAddress gateway(192,168,100,1); 
IPAddress subnet(255,255,255,0);

const int led = 13;
const int rele = 14;

double sensorTemp1 = 0;
double sensorTemp2 = 0;

/*=========================================================================*/
/*=                        UPDATE TEMPERATURES                            =*/
/*=========================================================================*/
void atualizaTemperaturas() {
  sensors.requestTemperatures(); 
  yield();
  
  sensorTemp1 = sensors.getTempCByIndex(0);
  sensorTemp2 = sensors.getTempCByIndex(1);
}


/*=========================================================================*/
/*=                   PRINT TEMPERATURES ON LCD                           =*/
/*=========================================================================*/
void printTempsLCD(){
  static char temp1[8];
  static char temp2[8];
  static char saida[16];

  lcd.setCursor(0, 1);
  
  dtostrf(sensorTemp1, 4, 1, temp1);
  dtostrf(sensorTemp2, 4, 1, temp2);
  
  //Serial.print("Temp1: ");
  //Serial.println(temp1);
  lcd.print("["); 
  lcd.print(temp1);
  lcd.print("] "); 
   
   
  //Serial.print("Temp2: ");
  //Serial.println(temp2);  
  lcd.print("[");
  lcd.print(temp2);
  lcd.print("] ");  
}

/*=========================================================================*/
/*=                          CONTROLA RELE                                =*/
/*=========================================================================*/
void controlaRele(){
    double tempDiff = sensorTemp2 - sensorTemp1;

    if( tempDiff > 5){
      digitalWrite(rele, LOW); 
    } else {
      digitalWrite(rele, HIGH); 
    }
}


/*=========================================================================*/
/*=                           HANDLE ROOT                                 =*/
/*=========================================================================*/
void handleRoot() {
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  char txt1[32] = "";
  
  digitalWrite(led, 1);

  int temp1 = (int)(sensorTemp1);
  int temp2 = (int)(sensorTemp2);

  lcd.setCursor(0, 1);
  
  char html[2000]; 
  
  snprintf ( html, 2000,
"<!DOCTYPE html>\
<html lang=\"pt-br\">\
<head>\
<meta http-equiv='refresh' content='10'/>\
<meta charset=\"UTF-8\">\
    <link rel=\"stylesheet\" type=\"text/css\" href=\"https://cdn.jsdelivr.net/gh/alduxx/alduxino@d6e11cd42592755bfceaee80830f571d91c2fe58/SolarHeating/static/css/estilos.css\">\
    <script src=\"https://cdn.jsdelivr.net/gh/alduxx/alduxino@d6e11cd42592755bfceaee80830f571d91c2fe58/SolarHeating/static/justgage/raphael-2.1.4.min.js\"></script>\
    <script src=\"https://cdn.jsdelivr.net/gh/alduxx/alduxino@d6e11cd42592755bfceaee80830f571d91c2fe58/SolarHeating/static/justgage/justgage.js\"></script>\
    <title>Casa Container - Sensor Temperatura Aquecedor Solar</title>\
</head>\
<body>\
    <div class=\"cab\">Casa Container - Sensor Temperatura Aquecedor Solar</div>\
    <div class=\"box\"><div id=\"gauge\" class=\"200x160px\"></div></div>\
    <div class=\"box\"><div id=\"gauge2\" class=\"200x160px\"></div></div>\
    <div id=\"pump\" class=\"box off\"><h3>Bomba</h3><span>Desligada</span></div>\
    <div class=\"uptime\"><strong>Uptime: %02d:%02d:%02d</strong></div>\
</body>\
    <script type=\"text/javascript\">\
    var g = new JustGage({\
        id: \"gauge\",\
        value: %d,\
        min: 0,\
        max: 100,\
        title: \"Temp Paineis\",\
      });\
      var g = new JustGage({\
        id: \"gauge2\",\
        value: %d,\
        min: 0,\
        max: 100,\
        title: \"Temp Boiler\",\
      });\
    </script>\
</html>",
    hr, min % 60, sec % 60,
    temp1,
    temp2
  );
  
  server.send(200, "text/html", html);
  // Serial.println(html);
  
  digitalWrite(led, 0);
}

/*=========================================================================*/
/*=                          HANDLE NOT FOUND                             =*/
/*=========================================================================*/
void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}



/*=========================================================================*/
/*=                               SETUP                                   =*/
/*=========================================================================*/
void setup(void) {
  /********************************  LCD  ************************************/
  lcd.begin(16,2);
  //lcd.init();   // INICIALIZA O DISPLAY LCD
  lcd.backlight(); // HABILITA O BACKLIGHT (LUZ DE FUNDO) 
  lcd.setCursor(0, 1); //SETA A POSIÇÃO EM QUE O CURSOR INCIALIZA(LINHA 1)
  lcd.print("CONECTANDO..."); //ESCREVE O TEXTO NA PRIMEIRA LINHA DO DISPLAY LCD
  lcd.setCursor(0, 0); //SETA A POSIÇÃO EM QUE O CURSOR RECEBE O TEXTO A SER MOSTRADO(LINHA 2)      
  lcd.print("WIFI"); //ESCREVE O TEXTO NA SEGUNDA LINHA DO DISPLAY LCD

  /********************** SENSOR TEMPERATURA   *******************************/
  sensors.begin(); 

  /*****************************  RELE    ************************************/
  pinMode(rele, OUTPUT);
  digitalWrite(rele, HIGH); 

  /******************************  WIFI   ************************************/
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(9600);
  
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  
  Serial.println("Will wait for connection now..."); 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  lcd.setCursor(0, 0);
  lcd.print(WiFi.localIP()); 

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();

  atualizaTemperaturas(); // Atualiza variaveis com temperaturas do sensor
  controlaRele(); // Controla o Rele a partir da diff de temperaturas
  printTempsLCD(); // Imprime temperaturas no LCD
}
