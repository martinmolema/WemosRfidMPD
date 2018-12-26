/**
 * Cabling: RFID side (https://steve.fi/Hardware/d1-pins/)
 *  white    ==> SDA/SS ==> D8
 *  grey     ==> SCK    ==> D5
 *  purple   ==> MOSI   ==> D7
 *  blue     ==> MISO   ==> D6
 *  green    ==> IRQ    ==> <NONE>
 *  yellow   ==> GND    ==> GND 
 *  orange   ==> RST    ==> D4
 *  red      ==> 3.3V   ==> 3.3V

  Pin Out mapping from WEMOS to Arduino
  * static const uint8_t D0   = 16;
  * static const uint8_t D1   = 5;
  * static const uint8_t D2   = 4;
  * static const uint8_t D3   = 0;
  * static const uint8_t D4   = 2;
  * static const uint8_t D5   = 14;
  * static const uint8_t D6   = 12;
  * static const uint8_t D7   = 13;
  * static const uint8_t D8   = 15;
  * static const uint8_t D9   = 3;
  * static const uint8_t D10  = 1;

  * Pin  Function                          ESP-8266 Pin
  * TX   TXD                               TXD
  * RX   RXD                               RXD
  * A0   Analog input, max 3.3V input      A0 
  * D0   IO                                GPIO16
  * D1   IO, SCL                           GPIO5
  * D2   IO, SDA                           GPIO4
  * D3   IO, 10k Pull-up                   GPIO0
  * D4   IO, 10k Pull-up, BUILTIN_LED      GPIO2
  * D5   IO, SCK                           GPIO14
  * D6   IO, MISO                          GPIO12
  * D7   IO, MOSI                          GPIO13
  * D8   IO, 10k Pull-down, SS             GPIO15
  * G    Ground  GND
  * 5V   5V  -
  * 3V3  3.3V  3.3V
  * RST Reset RST
**/


#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <math.h>


#define SS_PIN          D8 // GPIO15
#define RST_PIN         D4 // GPIO2 == BUILD_IN_LED !!

MFRC522 mfrc522;

// current unique ID logged on
String currentUID =  "";

//Ethernet stuff
WiFiClient client;

//counter
int counter =0;

//SSID of your network
char ssid[] = "yourssid"; //SSID of your Wi-Fi router
char pass[] = "password"; //Password of your Wi-Fi router

// MPD Host
byte server[] = { 192,168,1,2 }; // mpd player IP-address (192.168.1.2 e.g.)

/*
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

String array_to_hex(byte *buffer, byte bufferSize){
  String result = "";
  char *tempResult = (char*)malloc(4);
  for (byte i = 0; i < bufferSize; i++) {
    itoa(buffer[i], tempResult, 16);
    result += tempResult;
  }
  free(tempResult);
  return result;
}//array_to_hex

void setup() {
  Serial.begin(19200);         // Initialize serial communications with the PC
  while (!Serial);            // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  pinMode(SS_PIN, OUTPUT);

  mfrc522 = MFRC522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
  
  SPI.begin();                // Init SPI bus
  mfrc522.PCD_Init();         // Init MFRC522 card
  Serial.println(F("Try an RFID chip"));

  // Connect to Wi-Fi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to...");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println(WiFi.localIP());
  Serial.println("Wi-Fi connected successfully");
    
  if (client.connect(server, 6600)){
    Serial.println("Connected to MPD server");
  }
  else{
    Serial.println("Could not connect to MPD server");
  }
}//setup()

void loop() {
    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()){
        delay(50);
        return;
    }

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()){
        delay(50);
        Serial.println("New card but serial read failed");
        return;
    }
    String uidHEX;

    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    uidHEX = array_to_hex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println(uidHEX);
    
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // MPD interfacing!
    if (currentUID == uidHEX) {  // same person? then toggle and save playlist
      togglePlayPause(uidHEX);
      savePlaylist(currentUID);
    }
    else{  // new person? Then save current list and retrieve list
      if (currentUID != "") savePlaylist(currentUID);
      currentUID = uidHEX;
      retrievePlayList(uidHEX);
      startPlay();
    }    

    Serial.println("Finished with MPD....");

	// wait for card to be "removed" (out of range)
    bool cardRemoved = false;
    int counter = 0;
    bool current, previous;
    previous = !mfrc522.PICC_IsNewCardPresent();
    
    while(!cardRemoved){
      current =!mfrc522.PICC_IsNewCardPresent();

      if (current && previous) counter++;

      previous = current;
      cardRemoved = (counter>2);      
      delay(50);
    }
}//loop()

String sendMPDCommand(String cmd){
  String result = "";
  int connected = client.connect(server, 6600);
  if (connected) {
    String command = "command_list_begin\n";
    command += cmd;
    command += "\ncommand_list_end\n";

    Serial.println("Sending command:");
    Serial.println(command);
    
    client.println(command);

    delay(50);

    result = readMPDResponse();
  }
  
  return result;
}//sendMPDCommand()

String readMPDResponse(){
  String result = String("");
  //client.connect(server, 6600);

  // wait for output (blocking!)
  while (client.available() == 0) Serial.print(".");
  
  while (client.available() > 0){
    char c = client.read();
    result.concat(c);
  }
  return result;
}//readMPDResponse

void savePlaylist(String uid){
  String cmd = "save ";
  cmd += uid;
  String saveResult = sendMPDCommand(cmd);
  Serial.print("MPD::save ");
  Serial.println(uid); 
  Serial.println(saveResult); 
  
}//savePlaylist

void togglePlayPause(String uid){
  
  String pauseResult = sendMPDCommand("pause");
  Serial.println("MPD::pause");
  Serial.println(pauseResult);

}//togglePlayPause

void retrievePlayList(String uid){

  //clear the current playlist
  String clearResult = sendMPDCommand("clear");
  Serial.println("MPD::Clear Playlist");
  Serial.println(clearResult);

  String cmd = "load ";
  cmd += uid;
  String loadResult    = sendMPDCommand(cmd);
  Serial.println("MPD::load new list");
  Serial.println(loadResult);
  
}//retrievePlayList


void startPlay(){
  String playResult = sendMPDCommand("play");
  Serial.println("MPD::play");
  Serial.println(playResult);
}//startPlay
