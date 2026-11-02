#include <Arduino.h>
#include <spi.h>
#include <esp_now.h>
#include <WiFi.h>
#include "SSD1306Wire.h" // legacy: #include "SSD1306.h"

// Mac adress of the me: 3C:71:BF:5A:79:0C
#define BOARDID 1 // eerste ontvangreeks
#define BOOTBUTTON 0
#define S88_CLOCK 23
#define S88_LOAD 18
#define S88_DATAIN 19          // DATA line = This line will be used to read the requested data in sync with the CLOCK pulses when the LOAD line is triggered.
#define NUMBER_OF_S88MODULES 4 // The number of S88 units we want to simulate with the Arduino.
#define RXD2 16
#define TXD2 17
#define SDA 22
#define SCL 21
#define SCREEN_I2C_ADDRESS 0x3C
#define SCREEN_SDA_PIN 22
#define SCREEN_SCL_PIN 21
#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128
#define SCREEN_VERTICAL_FLIP true
// Parameters for the scrolling text
#define scroll_delay 40         // set the scroll speed
#define scroll_char_width 9.666 // approximate average character width
#define scroll_y 46
// 51             // row of text line
#define scroll_height 24 // character height
#define OFFSET1 1        // de S88 waar deze zich moet melden bij de ECOS

int scroll_pos = SCREEN_WIDTH;
unsigned int next_scroll = 0;
String scroll_string;
const char *scroll_div = "  -  ";
// REPLACE WITH YOUR RECEIVER MAC Address
// uint8_t broadcastAddress[] = {0x3C, 0x71, 0xBF, 0x5A, 0x79, 0x0C}; // 3C:71:BF:5A:79:0C
// uint8_t broadcastAddress[] = {0x24, 0x0A, 0xC4, 0x82, 0x9D, 0xBC}; //
// uint8_t broadcastAddress[] ={0x0C, 0xDC, 0x7E, 0x5E, 0x99, 0xD4} ;
uint8_t broadcastAddress[] = {0xA0, 0x20, 0xA6, 0x00, 0x75, 0xA3};

void ReadS88();
// Structure example to send data
// Must match the receiver structure
typedef struct struct_message
{
  u8_t ID;
  u8_t OFFSET;
  u8_t AANTAL;
  uint16_t s88Data[NUMBER_OF_S88MODULES];
} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t peerInfo;

SSD1306Wire display(0x3c, SDA, SCL);

int nIndex = 0;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  char macStr[18];
  Serial.print("Packet from: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.println();
}

uint16_t bitset(uint16_t welke, int bit)
{
  return welke | (1 << bit); //
}

uint16_t bitreset(uint16_t welke, int bit)
{
  return welke & ~(1U << bit);
}

bool getbit(uint16_t welke, int bit)
{
  // Serial.printf("Get Bit: from: %d, Bit: %d, Outcome:%d\r\n", welke, bit,welke & (1 << bit) );
  return welke & (1 << bit);
}

int sentdata()
{
  int nResult = 0;
  // Set values to send

  myData.ID = BOARDID;
  myData.OFFSET = OFFSET1;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
  if (result != ESP_OK)
  {
    Serial.println("Error sending the data");
    nResult = 1;
  }

  return nResult;
}

void setup()
{
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);

  myData.s88Data[0] = 0xa55a;
  myData.s88Data[1] = 0x55aa;
  myData.s88Data[2] = 0xa5a5;
  myData.s88Data[3] = 0x5a5a;

  // Do the reroute of the IRQ address preferably at the end of the setup, to prevent spontanious reboots..
  Serial.begin(115200);
  Serial.printf("\r\nReboot\r\n");

  for (int i = 0; i < 16; i++)
  {
    Serial.print(getbit(myData.s88Data[1], i));
  }
  Serial.println();
  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.clear();
  //  display.drawString(0, 0, "Hello world: " + String(millis()));
  //  display.display();
  // display.setFont(ArialMT_Plain_10);

  pinMode(BOOTBUTTON, INPUT_PULLUP);
  pinMode(S88_CLOCK, OUTPUT);
  pinMode(S88_LOAD, OUTPUT);
  pinMode(S88_DATAIN, INPUT);
  digitalWrite(S88_CLOCK, LOW);
  digitalWrite(S88_LOAD, LOW);

  // attachInterrupt(digitalPinToInterrupt(S88_CLOCK), CLOCK, RISING); // Activate CLOCK interrupt on EVERY rising pulse of the CLOCK line.
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Init Done");
}

void loop()
{
  // put your main code here, to run repeatedly:
  ReadS88();
  delay(5);
  if (!digitalRead(BOOTBUTTON))
  {
    display.clear();
    for (int j = 0; j < 60; j += 15)
    {
      int offset = -1;
      for (int i = 1; i <= 112; i += 7)
      {
        if (getbit(myData.s88Data[j / 15], i / 7))
          display.drawString(i + offset, j, "1");
        else
          display.drawString(i + offset, j, "0");

        if (i % 28 == 22)
          offset += 3;
        if (i == 50)
          offset += 3;
      }
    }
    sentdata();
    display.display();
    while (!digitalRead(BOOTBUTTON))
    {
      yield();
    }
  }
  else
  {
    delay(500);
    //   sentdata();
  }
}

// put function definitions here:
void ReadS88()
{ 
  digitalWrite(S88_CLOCK, LOW);
  digitalWrite(S88_LOAD, HIGH);
  delayMicroseconds(42);
  digitalWrite(S88_CLOCK, HIGH);
  delayMicroseconds(42);
  digitalWrite(S88_LOAD, LOW);
  delayMicroseconds(124);
  for (int j = 0; j < 4; j++)
  {
    for (int i = 0; i < 16; i++)
    {
      digitalWrite(S88_CLOCK, LOW);
      delayMicroseconds(10);
      if (digitalRead(S88_DATAIN))
      {
        myData.s88Data[j] = bitset(myData.s88Data[j], i); // set
      }
      else
      {
        myData.s88Data[j] = bitreset(myData.s88Data[j], i); // reset
      }
      delayMicroseconds(35);
      digitalWrite(S88_CLOCK, HIGH);
      delayMicroseconds(45);
      if (i == 7)
        delayMicroseconds(1200 - 45);
    }
    //  Serial.printf("\r\nByte: %X, index: %d\r\n", myData.s88Data[j], j);


  }
  return;
}