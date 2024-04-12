/*
==============================================================================================================
A&A Electronics 2024 by IU1LCU & IU8HPE
Voltage / Current remote monitor with 4 controlled relay
Wiring for Ethernet (w5500): D13>SCLK / D12>MISO / D11>MOSI / D10>SCS
Voltage readings A0>1 A1>2
Current readings A2>1 A3>2 A4>3 A5>4
Based on current sensor ACS712 for 5AMPS 185mv/1A
Relay pins works as LOW = ON / HIGH = OFF
For arduino relay modules
!!!!!!! ATTENTION !!!!!!!
Please pay attention to change:
IP Address, compatible with your local network
int maxvolt = X; is the maximun voltage of the voltage divider, example if you wanna measure from 0 to MAX 15v external,
you will need a 1/3 voltage divider like 3 1K resistor in series, to have on the last resistor 5v/GND, for max 20v a 1/4 divider etc.

int intV = X it the internal (theroically) 5v, need to be right for the current sensor calibration, just measure the circuit when powered up
if you dont need extremly precision you can just use 5 and the current will be aproximately

==============================================================================================================
*/
#include <SPI.h>
#include <Ethernet.h>
int maxvolt = 5; //The maximun voltage of the voltage divider
int intV = 4.65; //The internal 5v, need to be right for the current sensor calibration

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 10, 117);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

// Relay states and pins
String relayStates[] = {"Off", "Off", "Off", "Off"};
const int relayPins[] = {2, 3, 4, 5};

// Analog pins for voltage and current sensors
const int voltagePins[] = {A0, A1}; // Analog pins for voltage sensors
const int currentPins[] = {A2, A3, A4, A5}; // Analog pins for current sensors

// Client variables 
char linebuf[80];
int charcount=0;

void setup() { 
  // Set relay pins as outputs
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }
  
  // Open serial communication at a baud rate of 9600
  Serial.begin(9600);
  
  // Start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
}

// Display dashboard page with buttons to control relays
void dashboardPage(EthernetClient &client) {

 client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();

  client.println("<!DOCTYPE HTML><html><head>");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<style>h3 {font-size: 22px;} h4 {font-size: 16px;}</style>");
  client.println("</head><body>");       
  client.println("<h3>A&A PowerMeter 2024</h3>");                                                      
  client.println("<h3>Relay control</h3>");
  // Generate buttons to control the relays
  for (int i = 0; i < 4; i++) {
    client.print("<h4>Relay ");
    client.print(i + 1);
    client.print(" - State: ");
    client.print(relayStates[i]);
    client.print(" </h4><a href=\"/relay");
    client.print(i + 1);
    client.print("on\"><button>ON</button></a><a href=\"/relay");
    client.print(i + 1);
    client.print("off\"><button>OFF</button></a><br>");
  }
  client.println("<h3>Analog readings</h3>");
  // Print analog readings for voltage sensors
  client.println("<ul>");
  for (int i = 0; i < 2; i++) {
    int sensorValue = analogRead(voltagePins[i]);
    float voltage = (float(sensorValue) / 1023) * maxvolt;
    client.print("<li>Voltage sensor ");
    client.print(i + 1);
    client.print(": ");
    client.print(voltage);
    client.println("V</li>");
  }
  client.println("</ul>");
  // Print analog readings for current sensors
  client.println("<ul>");
  for (int i = 0; i < 4; i++) {
    int sensorValue = analogRead(currentPins[i]);
    float voltage = (float(sensorValue) / 1023) * intV;
    int current_mA = (voltage - (intV / 2)) / 0.185;    
    client.print("<li>Current sensor ");
    client.print(i + 1);
    client.print(": ");
    client.print(current_mA);
    client.println("mA</li>");
  }
  client.println("</ul>");
  client.println("</body></html>"); 
}

void loop() {
  // Listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("New client");
    memset(linebuf, 0, sizeof(linebuf));
    charcount = 0;
    // An HTTP request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // Read char by char HTTP request
        linebuf[charcount] = c;
        if (charcount < sizeof(linebuf) - 1) charcount++;
        // If you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          dashboardPage(client);
          break;
        }
        if (c == '\n') {
          // Check which relay control URL was requested
          for (int i = 0; i < 4; i++) {
            if (strstr(linebuf, ("GET /relay" + String(i + 1) + "off").c_str()) > 0) {
              digitalWrite(relayPins[i], HIGH);
              relayStates[i] = "Off";
            }
            else if (strstr(linebuf, ("GET /relay" + String(i + 1) + "on").c_str()) > 0) {
              digitalWrite(relayPins[i], LOW);
              relayStates[i] = "On";
            }
          }
          // You're starting a new line
          currentLineIsBlank = true;
          memset(linebuf, 0, sizeof(linebuf));
          charcount = 0;          
        } 
        else if (c != '\r') {
          // You've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // Give the web browser time to receive the data
    delay(1);
    // Close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}
