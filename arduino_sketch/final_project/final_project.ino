#include <coap.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <SoftwareSerial.h>
SoftwareSerial ESP8266(10, 11); // RX | TX
#define tempPin A0
#define IP  "192.168.1.122"
#define DEBUG false         

const char* ssid = "";
const char* password = "";
float val;

void setup(){
   Serial.begin(9600);
   ESP8266.begin(115200);
   pinMode(3,OUTPUT);
   digitalWrite(3,LOW);

  //sendCommand("AT+RST\r\n",2000,false); // reset module
  sendCommand("AT+CWMODE=3\r\n",1000,DEBUG); // configure as access point
  sendCommand("AT+CWJAP=\"sheep\",\"123454321\"\r\n",3000,DEBUG);
  delay(10000);
  sendCommand("AT+CIFSR\r\n",1000,DEBUG); // get ip address
  sendCommand("AT+CIPMUX=1\r\n",1000,DEBUG); // configure for multiple connections
  sendCommand("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // turn on server on port 80
  
  Serial.println("Server Ready");
}

void loop(){
//ESP8266_test();
temperature_sensor();
delay(1000);

if(ESP8266.available()) // check if the esp is sending a message 
  {    
    if(ESP8266.find("+IPD,"))
    {
     delay(1000); // wait for the serial buffer to fill up (read all the serial data)
     // get the connection id so that we can then disconnect
     int connectionId = ESP8266.read()-48; // subtract 48 because the read() function returns 
                                           // the ASCII decimal value and 0 (the first decimal number) starts at 48
          
     ESP8266.find("pin="); // advance cursor to "pin="
     int pinNumber = (ESP8266.read()-48); // get first number i.e. if the pin 13 then the 1st number is 1
     int secondNumber = (ESP8266.read()-48);
     if(secondNumber>=0 && secondNumber<=9)
     {
      pinNumber*=10;
      pinNumber +=secondNumber; // get second number, i.e. if the pin number is 13 then the 2nd number is 3, then add to the first number
     }
     digitalWrite(pinNumber, !digitalRead(pinNumber)); // toggle pin    
     
     // build string that is send back to device that is requesting pin toggle
     String content;
     content += " LED is ";
     if(digitalRead(pinNumber))
     {
       content += "ON";
     }
     else
     {
       content += "OFF";
     }
     sendHTTPResponse(connectionId,content);
     
     // make close command
     //String closeCommand = "AT+CIPCLOSE="; 
     //closeCommand+=connectionId; // append connection id
     //closeCommand+="\r\n";
     //sendCommand(closeCommand,1000,DEBUG); // close connection
    }
  }
}

void ESP8266_test(){
// Keep reading from ESP-01s and send to Arduino Serial Monitor
if (ESP8266.available()){
     Serial.write(ESP8266.read());
}
// Keep reading from Arduino Serial Monitor and send to ESP-01s
if (Serial.available()){
  Serial.print("IP Address: ");
      ESP8266.write(Serial.read());
}  
}

void temperature_sensor(){
   val = analogRead(tempPin);
   val += analogRead(tempPin); 
   val += analogRead(tempPin);
   val += analogRead(tempPin);
   val += analogRead(tempPin);
   val = val/ 5;
   float temp = ( 5.0* val* 100.0)/ 1024.0; 
   //Serial.print("TEMPRATURE = ");
   Serial.print(temp);
   //Serial.print("C");
   Serial.println();
   //datatoserver(String(temp));
   delay(10000);
}

//Actuator
String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    int dataSize = command.length();
    char data[dataSize];
    command.toCharArray(data,dataSize);
    ESP8266.write(data,dataSize); // send the read character to the esp8266
    if(debug)
    {
      Serial.println("\r\n====== HTTP Response From Arduino ======");
      Serial.write(data,dataSize);
      Serial.println("\r\n========================================");
    }
    
    long int time = millis();
    while( (time+timeout) > millis())
    {
      while(ESP8266.available())
      {
        // The esp has data so display its output to the serial window 
        char c = ESP8266.read(); // read the next character.
        response+=c;
      }  
    }
    if(debug)
    {
      Serial.print(response);
    }
    return response;
}

void sendHTTPResponse(int connectionId, String content)
{
     // build HTTP response
     String httpResponse;
     String httpHeader;
     // HTTP Header
     httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n"; 
     httpHeader += "Content-Length: ";
     httpHeader += content.length();
     httpHeader += "\r\n";
     httpHeader +="Connection: close\r\n\r\n";
     httpResponse = httpHeader + content + " "; // There is a bug in this code: the last character of "content" is not sent, I cheated by adding this extra space
     sendCIPData(connectionId,httpResponse);
}
 
void sendCIPData(int connectionId, String data)
{
   String cipSend = "AT+CIPSEND=";
   cipSend += connectionId;
   cipSend += ",";
   cipSend +=data.length();
   cipSend +="\r\n";
   sendCommand(cipSend,1000,DEBUG);
   sendData(data,1000,DEBUG);
}

String sendCommand(String command, const int timeout, boolean debug)
{
    String response = ""; 
    ESP8266.print(command); // send the read character to the esp8266    
    long int time = millis();
    while( (time+timeout) > millis())
    {
      while(ESP8266.available())
      {    
        // The esp has data so display its output to the serial window 
        char c = ESP8266.read(); // read the next character.
        response+=c;
      }  
    }
    if(debug)
    {
      Serial.print(response);
    }
    return response;
}
/*
void datatoserver(String t)
{
      // 設定 ESP8266 作為 Client 端
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += IP;
    cmd += "\",8000";
    Serial.print("SEND: ");
    Serial.println(cmd);
    ESP8266.println(cmd);
    if( ESP8266.find( "Error" ) )
    {
        Serial.print( "RECEIVED: Error\nExit1" );
        return;
    }
    cmd = "GET /temp=" + t + "\r\n";
    ESP8266.print( "AT+CIPSEND=" );
    ESP8266.println( cmd.length() );
    if(ESP8266.find( ">" ) )
    {
        Serial.print(">");
        Serial.print(cmd);
        ESP8266.print(cmd);
    }
    else
    {
        ESP8266.print( "AT+CIPCLOSE" );
    }
    if( ESP8266.find("OK") )
    {
        Serial.println( "RECEIVED: OK" );
    }
    else
    {
        Serial.println( "RECEIVED: Error\nExit2" );
    }
}
*/

