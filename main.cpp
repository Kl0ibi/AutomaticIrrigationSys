#include <WiFi.h>
#include "time.h"


// Replace with your network credentials 
const char* ssid = "WLAN1-D441TD11"; 
const char* password = "MamaistToll123"; 

//time server via NTP 
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;

uint8_t day=0;

void printLocalTime()
{
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void checkfornewDate()
{
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  if(timeinfo.tm_mday != day){
    day=timeinfo.tm_mday;
    Serial.println("newDay");
  }
}

// Set web server port number to 80 
WiFiServer server(80); 

// Variable to store the HTTP request String header;
String header;
// Declare the pins to which the LEDs are connected 
int greenled = 2;
int redled = 27; 

int greenstate = 0;// state of green LED
//char redstate[] = "off";// state of red LED



float humidity=0;
float temperature=0;
float voltage=0;
int hum_right_top=0;
int hum_right_bottom=0;
int hum_left_top=0;
int hum_left_bottom=0;
int rain=0;
int light=0;
float usedWater=0;

bool locked=false;

char sensor_keyword[]= "s";


char recievedChar;
char * strtokIndx;
char buf[40];

int readline(int readch, char *buffer, int len) {
  static int pos = 0;
  int rpos;
  if (readch > 0) {
    switch (readch) {
      default:
        if (pos < len - 1) {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
      case '\r': // Ignore CR
        break;
      case '\n': // Return on new-line
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;
    }
  }
  return 0;
}


void checkSerialInput() {
  strcpy(buf, "");
  while (Serial.available())  {
    readline(Serial.read(), buf, 80);
  }
  if ((buf[0] != NULL)) {

    strtokIndx  = strtok(buf, ",");
    
    if (strcmp(strtokIndx, sensor_keyword) == 0) {
      strtokIndx = strtok(NULL, ","); //parse same strtokIndx
      usedWater = atof(strtokIndx);
      strtokIndx = strtok(NULL, ","); //parse same strtokIndx
      humidity = atof(strtokIndx);
      strtokIndx = strtok(NULL, ","); //parse same strtokIndx
      temperature = atof(strtokIndx);
      strtokIndx = strtok(NULL, ","); //parse same strtokIndx
      voltage = atof(strtokIndx);
      strtokIndx = strtok(NULL, ","); //parse same strtokIndx
      greenstate = atof(strtokIndx);
      strtokIndx = strtok(NULL, ","); //parse same strtokIndx
      rain = atof(strtokIndx);
      strtokIndx = strtok(NULL, ","); //parse same strtokIndx
      light = atof(strtokIndx);
    }
    else {
      Serial.println("Unknown Command: " + String(strtokIndx));
    }
  }
}


void setup() {
  Serial.begin(115200);
 // Set the pinmode of the pins to which the LEDs are connected and turn them low to prevent flunctuations
  pinMode(greenled, OUTPUT);
  pinMode(redled, OUTPUT);
  digitalWrite(greenled, LOW);
  digitalWrite(redled, LOW);
  //connect to access point
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());// this will display the Ip address of the Pi which should be entered into your browser
  server.begin();

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //set the day
  day = timeinfo.tm_mday;

}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\"  http-equiv=\"refresh\" content=\"width=device-width, initial-scale=1, 5\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<meta http-equiv=\"refresh\" content=\"5\" >");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; background-color: #000000;}");
            client.println(".button { background-color: #CE1F1F; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 15px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #47CE1F;}</style></head>");
            


             

            // Web Page Heading
            client.println("<body><h1 style=color:white>Automatic Irregation System</h1>");
           // header.indexOf("GET /");
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.println("<p style=color:white>valve - State ");
            client.println(greenstate);
            client.println("</p>");
            // If the green LED is off, it displays the ON button       
            if (greenstate == 0) {
              client.println("<img src=\"https://i.imgur.com/zUZRBzE.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
              client.println("<p><a href=\"/valve/on\"><button class=\"button\">OFF</button></a></p>");
            } else if (greenstate == 1) {
              client.println("<img src=\"https://i.imgur.com/7tkgKxc.png\" alt=\"off button\" style=\"width:device-width;height:180px;\">");
              client.println("<p><a href=\"/valve/off\"><button class=\"button button2\">ON</button></a></p>");
            } else if (greenstate == 2){
               client.println("<img src=\"https://i.imgur.com/Ft4edIO.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
               header="";
            } else if (greenstate == 3){
               client.println("<img src=\"https://i.imgur.com/i8MuRIS.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
               header="";
            } else if (greenstate == 4){
               client.println("<img src=\"https://i.imgur.com/QN88syN.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
            } else if (greenstate == 5){
               client.println("<img src=\"https://i.imgur.com/dXy9L2Z.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
           } else if(greenstate == 6){
               client.println("<img src=\"https://i.imgur.com/Dhm4FPr.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
           } else if(greenstate == 7){
               client.println("<img src=\"https://i.imgur.com/cLD8znn.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
            } else if(greenstate == 8){
                client.println("<img src=\"https://i.imgur.com/GIQsZoL.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
              } else if(greenstate == 9){
                client.println("<img src=\"https://i.imgur.com/4VDFwnW.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
              } else if(greenstate == 10){
                client.println("<img src=\"https://i.imgur.com/m8Apk02.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
              } else if(greenstate == 11){
                client.println("<img src=\"https://i.imgur.com/zrGiwTU.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
              } else if(greenstate == 12){
                client.println("<img src=\"https://i.imgur.com/eWd6eW5.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
              } else if(greenstate == 13){
                client.println("<img src=\"https://i.imgur.com/OdvGGU6.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
              } else if(greenstate == 14){
                client.println("<img src=\"https://i.imgur.com/IzEcLVv.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
              } else if(greenstate == 15){
                client.println("<img src=\"https://i.imgur.com/No0H9QD.png\" alt=\"on button\" style=\"width:device-width;height:180px;\">");
                header="";
              }


                // turns the GPIOs on and off  
              if (header.indexOf("GET /valve/on") >= 0) {
                            Serial.println("valve on");
                            greenstate = 1;
                            digitalWrite(greenled, HIGH);
                          } else if (header.indexOf("GET /valve/off") >= 0) {
                            Serial.println("valve off");
                            greenstate = 0;
                            digitalWrite(greenled, LOW);                    
                          }
                            
            client.print("<p style=color:white;font-weight:bold>");
            client.print("used Water on ");
            client.print(&timeinfo, "%A, %B %d %Y");
            client.print(", ");
            client.print(usedWater);
            client.print(" L");
            client.print("</p>");   

            client.println("<p style=color:white;font-weight:bold>");
            client.println("Voltage: ");
            client.println(voltage);
            client.println(" V");
            client.println("</p>");  

            client.println("<p style=color:white;font-weight:bold>");
            client.println("Temperature: ");
            client.println(temperature);
            client.println(" C");
            client.println("</p>"); 

            client.println("<p style=color:white;font-weight:bold>");
            client.println("Humidity: ");
            client.println(humidity);
            client.println(" %");
            client.println("</p>"); 

            client.println("<p style=color:white;font-weight:bold>");
            client.println("Raining: ");
            if(rain == 1){
              client.println("Yes");
            } else {
              client.println("No");
            }
            client.println("</p>");

            client.println("<p style=color:white;font-weight:bold>");
            client.println("Light: ");
            client.println(light);
            client.println(" Lux");
            client.println("</p>");
            
    
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  checkSerialInput();
  checkfornewDate();
}