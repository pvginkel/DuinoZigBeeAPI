//***********************************************
// ZigBee-Home Automation Communications            
// Author: Professor Mayhem,
// Yoyodyne Monkey Works, Inc.
//
//
// 2/15/2017
//
// Revision 1.0
//
// ZigBee-Home Automation Communications is an
// Arduino port from a version of the same name
// written in the Propeller Spin language
// by John.Rucker(at)Solar-Current.com
//***********************************************
#include <ZigBeeAPI.h>
#define _DEBUG 0

/*
Hardware requirements:
Arduino, xBee shield, and Digi Xbee. Tested on Arduino Uno, Duemilanove, Leonardo, and Mega 2560 using Seeed Studio Xbee Shield.
Tested both with Xbee Pro ZB SMT (Surface Mount) Digi part number XBP24CZ7PIS-004 and with the Xbee Pro S2C XB24CZ7WIT-004
Configure the above xBee with the following settings: ZS = 2, NJ = 0x5a, EE = Enable, EO = 1,
KY = 0X5A6967426565416C6C69616E63653039, BD = 9600, D6 = None, AP = true, AO = 3

This demo allows you to control a LED attached to the LEDPin from a public ZigBee Home Automation network.
The demo is designed to work with a SmartThings hub available at SmartThings.com.
A custom SmartThings device type is available at:
https://github.com/JohnRucker/Nuts-Volts/blob/master/devicetypes/johnrucker/parallax-propeller-demo.src/parallax-propeller-demo.groovy
Install the above device driver in the SmartThings developer section for your hub.
Once installed SmartThings will recognize this device and will add a tile to their smart phone app when this device joins their Hub's network.
This new tile will allow you to send on / off commands to this device turning the LED on and off.

Conversation overview of the initial connection to the SmartThings home automation network.
  1) At power up the xBee will look for a valid ZigBee Home Automation coordinator or router to join, if the network is allowing new devices to
     join it will join. An Association number will be displayed on the Serial Monitor until the network is joined. To join a ZigBee Home Automation
     network the xBee must be configured with the proper security key and security settings outlined above in the hardware configuration.
  2) Once joined to a valid ZigBee Home Automation network a device announce packet (ZDO Cluster 0x00013) will be sent to the coordinator.
  3) The coordinator should respond back with a request for active end points. This request will be received and a response report will be
     sent back (ZDO Cluster 0x8005) to the coordinator telling it what end points are supported by this application.
  4) Next the coordinator will request a description of each end point's configuration. This application will respond to this request with
     a report of supported device types and clusters for each end point (ZDO Cluster 0x8004).
  5) At this point the home automation network knows what this device is and how to talk to it.
     loop() will continue to listen to ZigBee packets and will respond to packets that send the on or off command (Cluster 0x0006).
*/
//**************************************
// Start ZigBee API Communications 
//***************************************

  // SoftwareSerial Limitations:
  // 
  // On Mega and Mega 2560, only the following pins can be used for RX:
  // 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69).
  //
  // On Leonardo and Micro, only the following pins can be used for RX:
  // 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI).
  // On Arduino or Genuino 101 the current maximum RX speed is 57600bps
  // On Arduino or Genuino 101 RX doesn't work on Pin 13
  //
  // SoftwareSerial is not needed on Leonardo, or Mega. Attach the xBee to pins 0 & 1 of the Leonardo, or pins 19, & 18 of the Mega.
  // No other changes are required.
  
  // Async config
  int xBeeTx=2;     // This is the Arduino pin connected to the xBee's transmit pin. Not needed for Leonardo or Mega
  int xBeeRx=3;     // This is the Arduino pin connected to the xBee's receive pin. Not needed for Leonardo or Mega
  int xBeeBaud=9600;// The baud rate of the xBee must match this number (set with the xBee's BD command)
  int xBeeReset=12; // This is the Arduino pin connected to the xBee's reset pin, not needed if using soft reset (AT FR)
  #if !defined (__AVR_ATmega32U4__) && !defined (__MK20DX128__)
    SoftwareSerial Serial1(xBeeTx, xBeeRx);
  #endif                  
  ZigBeeAPI zb(Serial1);

  byte AppVersion = 1;               // Application version
  byte HardwareVersion = 1;          // Hardware version

  char Model[] = "Arduino xBee";     // ZigBee Basic Device ModelIdentifier 0 to 32 byte char string P79 of ZCL
  char Manufacturer[] = "Arduino";   // ZigBee Basic Device ManufacturerName 0 to 32 bytes
  int LEDPin=LED_BUILTIN;            // This is the pin that is connected to an LED's anode (positive side)
                                     // Make sure to use a current limiting resistor in series with the LED
                                     // Connect the other end of the LED to ground  
  const int BufferSize = 75;
  byte Buffer[BufferSize];
  word LclNet=0;
  unsigned long LclIeeeLo = 0, LclIeeeHi =0, now=0;
  
void Tx_Device_annce()                                                        // ZDO Cluster 0x0013 Device_annce
{
  //***************************************
  // Device_annce ZDO Cluster 0x0013 see page 109 of ZBSpec
  // This method announces that this device is now on the network and ready to receive packets
  // Called every time the Arduino reboots
  // Device announce is sent to the ZigBee Coordinator
  //***************************************
  memset(Buffer, 0 , BufferSize);
  Buffer[0] = 0x22;                                                           // Transaction seq number
  Buffer[1] = lowByte(LclNet);                                                // Network Address 2 bytes little endian Page 109 of ZBSpec
  Buffer[2] = highByte(LclNet);
  Buffer[3] = lowByte(LclIeeeLo);                                             // IEEE Address first 4 bytes little endian
  Buffer[4] = highByte(LclIeeeLo);
  Buffer[5] = lowByte(LclIeeeLo >> 16);
  Buffer[6] = highByte(LclIeeeLo >> 16);
  Buffer[7] = lowByte(LclIeeeHi);                                             // IEEE Address second 4 bytes little endian
  Buffer[8] = highByte(LclIeeeHi);
  Buffer[9] = lowByte(LclIeeeHi >> 16);
  Buffer[10] = highByte(LclIeeeHi >> 16);
  Buffer[11] = 0x8C;                                                          // 8C = MAC Capability Mask See page 83 of ZBSpec.
  	                                                                          // 0x8C = b10001100 = Mains powered device,
  	                                                                          // Receiver on when idle, address not self-assigned
  zb.TX(0x00000000, 0x0000FFFF, 0xFFFD, 0, 0, 0, 0x0013, Buffer, 12);         // $FFFD = Broadcast to all non sleeping devices
}

void ZCLpkt()
{
  //***************************************
  // ZigBee Cluster Library (ZCL) cluster numbers supported
  // ZCL are defined in the ZigBee Cluster Library Document 075123r04ZB
  // http://www.zigbee.org/wp-content/uploads/2014/11/docs-07-5123-04-zigbee-cluster-library-specification.pdf
  //***************************************
  switch (int(zb._PktCluster()))
  {
    case 0x0000:
      clstr_Basic();                                                          // Basic Cluster Page 78 of ZigBee Cluster Library
      break;
    case 0x0006:
      clstr_OnOff();                                                          // On/Off Cluser Page 125 of ZigBee Cluster Library
      break;
  }
}

void printByteData(uint8_t Byte)
{
  Serial.print((uint8_t)Byte >> 4, HEX);
  Serial.print((uint8_t)Byte & 0x0f, HEX);
}

void setup()
{
  // Set as Output
  pinMode(LEDPin, OUTPUT);

  //***************************************
  // Start Serial Monitor Communications
  //***************************************
  Serial.begin(9600);
  Serial1.begin(xBeeBaud);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println();
  Serial.println(F("On Line."));
  
  #if _DEBUG
    Serial.print("Compiled on: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
  #endif

  //***************************************
  // Un-Remark the next command to force xBee to leave Network and reset
  //***************************************
  //LeaveNetwork();

  //***************************************
  // Loop until xBee is joined to a ZigBee network
  // See page 246 of XBee/XBee-PRO S2C ZigBee User Guide for an explanation of AI Network Join Status numbers
  // http://www.digi.com/resources/documentation/digidocs/pdfs/90002002.pdf
  //***************************************
  Serial.println();
  Serial.print(F("Network Join Status: "));
  zb.AT("AI");
  if (byte(zb._PktData()[0]) == 0x00)
  {                                                                           // If xBee is not joined print the AI error codes to screeen for reference
    Serial.println(F("Successfully joined a network"));
  }
  else
  {
    Serial.println();
    //Association Indicator (AI) definition strings
    Serial.println(F("0x00 - Successfully formed or joined a network. (Coordinators form a network, routers and end devices join a network.)"));
    Serial.println(F("0x21 - Scan found no PANs"));
    Serial.println(F("0x22 - Scan found no valid PANs based on current SC and ID settings"));
    Serial.println(F("0x23 - Valid Coordinator or Routers found, but they are not allowing joining (NJ expired)"));
    Serial.println(F("0x24 - No joinable beacons were found"));
    Serial.println(F("0x25 - Unexpected state, node should not be attempting to join at this time"));
    Serial.println(F("0x27 - Node Joining attempt failed (typically due to incompatible security settings)"));
    Serial.println(F("0x2A - Coordinator Start attempt failed"));
    Serial.println(F("0x2B - Checking for an existing coordinator"));
    Serial.println(F("0x2C - Attempt to leave the network failed"));
    Serial.println(F("0xAB - Attempted to join a device that did not respond."));
    Serial.println(F("0xAC - Secure join error - network security key received unsecured"));
    Serial.println(F("0xAD - Secure join error - network security key not received"));
    Serial.println(F("0xAF - Secure join error - joining device does not have the right preconfigured link key"));
    Serial.println(F("0xFF - Scanning for a ZigBee network (routers and end devices)"));
    Serial.println();
    Serial.print(F("Looping until network is joined: "));
    now=millis();
    while (byte(zb._PktData()[0]) != 0)                                       // Loop until xBee joins a valid network
    {
      if ((millis() - now) > 1000)
      {
        now=millis();
        if(zb.AT("AI"))
        {
          Serial.print(" ");
          Serial.write(0x0D);
          printByteData(byte(zb._PktData()[0]));
          printByteData(byte(zb._PktData()[1]));
        }
      }
     	zb.RX(10);  // Continue checking for a reply while we are waiting...
    }
  }
  
  //***************************************
  // Read xBee's address settings and store in memory
  // The xBee's 64 bit IEEE address is in ROM on the xBee and will never changed (SH & SL = IEEE Address)
  // The xBee's 16 bit network address (MY address) is set by the ZigBee coordinator and may change at any time
  //**************************************
  Serial.println();
  if (zb.AT("MY"))
  {
    LclNet = byte(zb._PktData()[0]);                                          // Set Network Address
    LclNet = (LclNet << 8) + byte(zb._PktData()[1]);                          // Set Network Address
  }

  if (zb.AT("SH"))
  {
    LclIeeeHi = byte(zb._PktData()[0]);                                       // Set IEEE Address Hi
    LclIeeeHi = (LclIeeeHi << 8) + byte(zb._PktData()[1]);
    LclIeeeHi = (LclIeeeHi << 8) + byte(zb._PktData()[2]);
    LclIeeeHi = (LclIeeeHi << 8) + byte(zb._PktData()[3]);
  }
  if(zb.AT("SL"))
  {
    LclIeeeLo = byte(zb._PktData()[0]);                                       // Set IEEE Address Lo
    LclIeeeLo = (LclIeeeLo << 8) + byte(zb._PktData()[1]);
    LclIeeeLo = (LclIeeeLo << 8) + byte(zb._PktData()[2]);
    LclIeeeLo = (LclIeeeLo << 8) + byte(zb._PktData()[3]);
  }
  delay(1000);                                                                // wait 1 sec
  Tx_Device_annce();

  //***************************************
  // Populate string variables with address information
  //**************************************
  Serial.println();
  Serial.print(F("IEEE Add: "));
  Serial.print(LclIeeeHi,HEX);
  Serial.print("-");
  Serial.println(LclIeeeLo,HEX);
}

void loop()
{
  //***************************************
  // loop() waits for inbound data from the ZigBee network and responds based on the packet's profile number
  // If the profile number in the received packet is 0x0000 then the packet is a ZigBee Device Object (ZDO) packet
  // If the profile number is not 0x0000 then the packet is a ZigBee Cluster Library (ZCL) packet
  // The packet is also printed to the screen formatted so you can see addressing and payload details.
  //***************************************
  Serial.println();
  Serial.println(F("Waiting for packet"));
  if (zb.RX(0) == 1)
  {
    Serial.println(F("Good packet received:"));
    Serial.print(F("\tIEEE Add: "));
    Serial.print(long(zb._PktIEEEAddHi()),HEX);
    Serial.print(long(zb._PktIEEEAddLo()),HEX);
    Serial.println();

    Serial.print(F("\tNet Add: "));
    Serial.print(long(zb._PktNetAdd()),HEX);
    Serial.println();

    Serial.print(F("\tDst EP: "));
    Serial.print(byte(zb._PktDEP()),HEX);
    Serial.println();

    Serial.print(F("\tSrc EP: "));
    Serial.print(byte(zb._PktSEP()),HEX);
    Serial.println();

    Serial.print(F("\tProfile ID: "));
    Serial.print(word(zb._PktProfile()),HEX);
    Serial.println();

    Serial.print(F("\tCluster ID: "));
    Serial.print(word(zb._PktCluster()),HEX);
    Serial.println();

    Serial.print(F("\tPayload Hex: "));
    Serial.print("->");
    for (int x=0;  x <= zb._PktDataSize(); x++)
    {
      Serial.print(byte(zb._PktData()[x]),HEX);
      if (x != zb._PktDataSize())
      {
        Serial.print(" ");
      }
    }
    Serial.println(F("<-"));

    if (zb._PktProfile() != 0x00)
    {
      ZCLpkt();
    }
    else
    {
      ZDOpkt();
    }
  }
  else
  {
    Serial.print(F("Unknown packet ->"));
    int x=0;
    while(zb._ReadLog()[x] != '\0')
    {  
      Serial.print(char(zb._ReadLog()[x]));
      x++;
    }
    Serial.println(F("<-"));
    Serial.print(F("Pkt ->"));
    for (int i=0; i <= zb._PktDataSize(); i++)
    {
      printByteData(byte(zb._PktData()[i]));
      if (i != xBuffSize)
      {
        Serial.print(" ");
      }
    }
    Serial.println(F("<-"));
  }
}

void LeaveNetwork()
{
  //***************************************
  // Force ZigBee to leave network and start
  // looking for a network to join
  //***************************************
  Serial.println();
  Serial.print(F("Sending Leave PAN Command:"));
  Serial.print(zb.ATbyte("CB", 0x04));
  resetXB();
  delay(1000);
}

void resetXB()
{
  //***************************************
  // Do a hardware reset to xBee
  // Required afer a leave network command has been sent
  //***************************************
  Serial.println(zb.AT("FR"));
  pinMode(xBeeReset, OUTPUT);    // set xBeeReset pin to output
  digitalWrite(xBeeReset, LOW);  // drive xBeeReset pin low
  delay(1);                      // Reset requires 26 microseconds
  pinMode(xBeeReset, INPUT);     // set xBeeReset pin to input
}

//ZigBee Device Objects commands follow

void ZDOpkt()
{
  //***************************************
  // ZigBee Device Objects (ZDO) cluster numbers
  // ZDO are defined in the ZigBee Specification Document 053474r20
  // http://www.zigbee.org/wp-content/uploads/2014/11/docs-05-3474-20-0csg-zigbee-specification.pdf
  //***************************************
  switch (int(zb._PktCluster()))
  {
    case 0x0004:
      Simple_Desc_req();                                                      // Page 105 of ZBSpec
      break;
    case 0x0005:
      Active_EP_req();                                                        // Page 105 of ZBSpec
      break;
  }
}

void Simple_Desc_req()                                                        // ZDO Cluster 0x8004 Simple_Desc_rsp
{
  //***************************************
  // Simple_Desc_rsp Cluster 0x8004 on page 159 of ZBSpec
  // A Simple Description Request packet cluster 0x0004 is used to discover the configuration of an end point. The results of this command are
  // retuned in a Simple_Desc_res packet cluster 0x8004 that contains the ZigBee Proflie, ZigBee Device Type, and a list of inbound and
  // outbound clusters for the requested end point.  This command is sent once for each end point to discover all services available.
  //***************************************
  byte epToRpt = byte(zb._PktData()[3]);                                      // End Point to report Simple Desc on

  Serial.println();
  Serial.print(F("Reporting Simple Desc for End Point "));
  Serial.print(epToRpt,HEX);

  if (epToRpt == 0x38)                                                        // Report for end point 0x38
  {
    memset(Buffer, 0 , BufferSize);
    Buffer[0] = zb._PktData()[0];                                             // Set Transaction Seq number to match inbound packet's seq number
    Buffer[1] = 0x00;                                                         // Status 0x00 = success Table 2.93 on page 159 of ZBSpec
    Buffer[2] = zb._PktData()[1];                                             // Set Network address little endian order
    Buffer[3] = zb._PktData()[2];
    Buffer[4] = 0x0E;                                                         // Length in bytes of the Simple Descriptor to Follow

    Buffer[5] = 0x38;                                                         // Endpoint of the simple descriptor Table 2.38 on page 88 of ZBSpec

    Buffer[6] = 0x04;                                                         // Application Profile ID 2 Bytes Little endian. 0x0104 = Home Automation Profile
    Buffer[7] = 0x01;
    Buffer[8] = 0x02;                                                         // Device type 2 Bytes Little endian, 0x0002 = On/Off Output see page 42 of ZigBee Home Automation Profile
    Buffer[9] = 0x00;

    Buffer[10] = 0x00;                                                        // App Dev Version 4bits + reserved 4bits

    Buffer[11] = 0x02;                                                        // Input cluster count in this case we only have 0x02 input clusters

    Buffer[12] = 0x00;                                                        // Input cluster list 2 bytes each little endian. 0x0000 = Basic Cluster
    Buffer[13] = 0x00;

    Buffer[14] = 0x06;                                                        // Output cluster 2 bytes each little endian. 0x0006 = On / Off Cluster
    Buffer[15] = 0x00;

    Buffer[16] = 0x00;                                                        // Output cluster list. No output clusters

    zb.TX(zb._PktIEEEAddHi(), zb._PktIEEEAddLo(), zb._PktNetAdd(), 0, 0, 0, 0x8004, Buffer, 17);
  }
  else                                                                        // Respond with Error since no valid end points were found.
  {
    memset(Buffer, 0 , BufferSize);
    Buffer[0] = zb._PktData()[0];                                             // Set Transcation Seq number to match inbound packets seq number
    Buffer[1] = 0x82;                                                         // Status 0x82 = Invalid_EP page 212 of ZigBee Specification
    Buffer[2] = zb._PktData()[1];                                             // Set Network address little endian order
    Buffer[3] = zb._PktData()[2];
    Buffer[4] = 0x00;                                                         // Length in bytes of the Simple Descriptor to Follow

    zb.TX(zb._PktIEEEAddHi(), zb._PktIEEEAddLo(), zb._PktNetAdd(), 0, 0, 0, 0x8004, Buffer, 5);
  }
}

void Active_EP_req()                                                          // ZDO Cluster 0x8005 Active_EP_rsp
{
  //***************************************
  // Active_EP_rsp Cluster 0x8005 see page 161 of ZBSpec
  // Active EP Request ZDO cluster 0x0005 is used to discover the end points of a ZigBee device.
  // The results of this request are returned in ZDO Cluster 0x8005, a packet that list the end point count and each end point number.
  //***************************************
  Serial.print(F("Reporting Active End Points."));
  memset(Buffer, 0 , BufferSize);
  Buffer[0] = zb._PktData()[0];                                               // Set Transaction Seq number to match inbound packets seq number
  Buffer[1] = 0x00;                                                           // Status 0x00 = success Table 2.94 on page 161 of ZBSpec
  Buffer[2] = zb._PktData()[1];                                               // Set Network address little endian order
  Buffer[3] = zb._PktData()[2];
  Buffer[4] = 0x01;                                                           // Active EndPoint count only one in this case page 161 of ZBSpec
  Buffer[5] = 0x38;                                                           // EndPoint number
  zb.TX(zb._PktIEEEAddHi(), zb._PktIEEEAddLo(), zb._PktNetAdd(), 0, 0, 0, 0x8005, Buffer, 6);
}

//ZigBee Cluster Library commands follow

void clstr_Basic()                                                            // Cluster 0x0000 Basic
{
  //***************************************
  // ZCL Cluster 0x0000 Basic Cluster
  // Section 3.2 on page 78 of ZCL
  //***************************************
  int StrLen = 0;
  byte seqNum = 0;
  byte cmdID = 0;
  word attributeID = 0;

  seqNum = byte(zb._PktData()[1]);                                            // Transaction seq number can be any value used in return packet to match a response to a request
  cmdID = byte(zb._PktData()[2]);                                             // Command ID Byte P16 of ZCL
  attributeID = byte(zb._PktData()[4]);                                       // Attribute ID Word(little endian) P126 of ZCL
  attributeID = (attributeID << 8) + byte(zb._PktData()[3]);

  Serial.println();
  Serial.print(F("Basic Cluster read attribute ID "));
  Serial.print(attributeID,HEX);

  if (cmdID == 0x00 && attributeID == 0x0001)                                 // Read Attribute 0x0001 ApplicationVersion
  {
    memset(Buffer, 0 , BufferSize);                                           // Clear response buffer
    Buffer[0] = 0x18;                                                         // Frame Control 0x18 = direction is from server to client, disable default response P14 of ZCL
    Buffer[1] = seqNum;                                                       // Set the sequence number to match the seq number in requesting packet
    Buffer[2] = 0x01;                                                         // Command Identifer 0x01 = Read attribute response see Table 2.9 on page 16 of ZCL

    Buffer[3] = 0x01;                                                         // Attribute Identifier (2 bytes) field being reported see figure 2.24 on page 36 of ZCL
    Buffer[4] = 0x00;

    Buffer[5] = 0x00;                                                         // Status 00 = Success

    Buffer[6] = 0x20;                                                         // Attribute Data Type 0x20 = Unsigned 8-bit integer, see Table 2.16 on page 55 of ZCL

    Buffer[7] = AppVersion;                                                   // Single byte value

    Serial.println();
    Serial.print(F("Read Application Ver, sending result"));
    zb.TX(zb._PktIEEEAddHi(), zb._PktIEEEAddLo(), zb._PktNetAdd(), zb._PktDEP(), zb._PktSEP(), zb._PktProfile(), zb._PktCluster(), Buffer, 8);
  }
  if (cmdID == 0x00 && attributeID == 0x0003)                                 // Read Attribute 0x0003 HWVersion
  {
    memset(Buffer, 0 , BufferSize);                                           // Clear response buffer
    Buffer[0] = 0x18;                                                         // Frame Control 0x18 = direction is from server to client, disable default response P14 of ZCL
    Buffer[1] = seqNum;                                                       // Set the sequence number to match the seq number in requesting packet
    Buffer[2] = 0x01;                                                         // Command Identifer 0x01 = Read attribute response see Table 2.9 on page 16 of ZCL

    Buffer[3] = 0x03;                                                         // Attribute Identifier (2 bytes) field being reported see figure 2.24 on page 36 of ZCL
    Buffer[4] = 0x00;

    Buffer[5] = 0x00;                                                         // Status 00 = Success

    Buffer[6] = 0x20;                                                         // Attribute Data Type 0x20 = Unsigned 8-bit integer, see Table 2.16 on page 55 of ZCL

    Buffer[7] = HardwareVersion;                                              // Single byte value

    Serial.println();
    Serial.print(F("Read Hardware Ver, sending result"));
    zb.TX(zb._PktIEEEAddHi(), zb._PktIEEEAddLo(), zb._PktNetAdd(), zb._PktDEP(), zb._PktSEP(), zb._PktProfile(), zb._PktCluster(), Buffer, 8);
  }
  if (cmdID == 0x00 && attributeID == 0x0004)                                 // Read Attribute 0x0004 ManufacturerName
  {
    StrLen = sizeof(Manufacturer)-1;
    memset(Buffer, 0 , BufferSize);                                           // Clear response buffer
    Buffer[0] = 0x18;                                                         // Frame Control 0x18 = direction is from server to client, disable default response P14 of ZCL
    Buffer[1] = seqNum;                                                       // Set the sequence number to match the seq number in requesting packet
    Buffer[2] = 0x01;                                                         // Command Identifer 0x01 = Read attribute response see Table 2.9 on page 16 of ZCL

    Buffer[3] = 0x04;                                                         // Attribute Identifier (2 bytes) field being reported see figure 2.24 on page 36 of ZCL
    Buffer[4] = 0x00;

    Buffer[5] = 0x00;                                                         // Status 00 = Success

    Buffer[6] = 0x42;                                                         // Attribute Data Type 0x42 = Character string, see Table 2.16 on page 56 of ZCL

    Buffer[7] = StrLen;                                                       // Character string size

    memcpy(&Buffer[8], &Manufacturer, StrLen);                                // Copy byte string array into buffer

    Serial.println();
    Serial.print(F("Read Manufacturer request, sending result"));
    zb.TX(zb._PktIEEEAddHi(), zb._PktIEEEAddLo(), zb._PktNetAdd(), zb._PktDEP(), zb._PktSEP(), zb._PktProfile(), zb._PktCluster(), Buffer, 8 + StrLen);
  }
  if (cmdID == 0x00 && attributeID == 0x0005)                                 // Read Attribute 0x0005 ModelIdentifier
  {
    StrLen = sizeof(Model)-1;
    memset(Buffer, 0 , BufferSize);                                           // Clear response buffer
    Buffer[0] = 0x18;                                                         // Frame Control 0x18 = direction is from server to client, disable default response P14 of ZCL
    Buffer[1] = seqNum;                                                       // Set the sequence number to match the seq number in requesting packet
    Buffer[2] = 0x01;                                                         // Command Identifer 0x01 = Read attribute response see Table 2.9 on page 16 of ZCL

    Buffer[3] = 0x05;                                                         // Attribute Identifier (2 bytes) field being reported see figure 2.24 on page 36 of ZCL
    Buffer[4] = 0x00;

    Buffer[5] = 0x00;                                                         // Status 00 = Success

    Buffer[6] = 0x42;                                                         // Attribute Data Type 0x42 = Charcter string, see Table 2.16 on page 56 of ZCL

    Buffer[7] = StrLen;                                                       // Character string size

    memcpy(&Buffer[8], &Model, StrLen);                                       // Copy byte string array into buffer

    Serial.println();
    Serial.print(("Read Model request, sending result"));
    zb.TX(zb._PktIEEEAddHi(), zb._PktIEEEAddLo(), zb._PktNetAdd(), zb._PktDEP(), zb._PktSEP(), zb._PktProfile(), zb._PktCluster(), Buffer, 8 + StrLen);
  }
}

void clstr_OnOff()                                                            // Cluster 0x0006 On/Off
{
  //***************************************
  // ZCL Cluster 0x0006 On/Off Cluster
  // Section 3.8 on page 125 of ZCL
  //***************************************
  byte frmType, seqNum, cmdID;
  word attributeID;
  frmType = byte(zb._PktData()[0]);                                           // Frame Type is bit 0 and 1 of Byte 0 P14 of ZCL
  frmType = frmType & 3;                                                      // Bitwise AND (&) with a mask to make sure we are looking at first two bits
  seqNum = byte(zb._PktData()[1]);                                            // Transaction seq number can be any value used in return packet to match a response to a request
  cmdID = byte(zb._PktData()[2]);                                             // Command ID Byte P16 of ZCL
  attributeID = byte(zb._PktData()[4]);                                       // Attribute ID Word(little endin) P126 of ZCL
  attributeID = (attributeID << 8) + byte(zb._PktData()[3]);                                       

  if (frmType == 0x00 && cmdID == 0x00 && attributeID == 0x00)                // frmType = 0x00 (General Command Frame see Table 2.9 on P16 of ZCL)
  {
    memset(Buffer, 0 , BufferSize);                                           // Read attributes
    Buffer[0] = 0x18;                                                         // Frame Control 0x18 = direction is from server to client, disable default response P14 of ZCL
    Buffer[1] = seqNum;                                                       // Set the sequence number to match the seq number in requesting packet
    Buffer[2] = 0x01;                                                         // Command Identifer 0x01 = Read attribute response see Table 2.9 on page 16 of ZCL

    Buffer[3] = 0x00;                                                         // Attribute Identifier (2 bytes) field being reported see figure 2.24 on page 36 of ZCL
    Buffer[4] = 0x00;

    Buffer[5] = 0x00;                                                         // Status 00 = Success

    Buffer[6] = 0x10;                                                         // Attribute Data Type 0x10 = Boolean, see Table 2.16 on page 54 of ZCL


    Buffer[7] = digitalRead(LEDPin);                                          // Attribute Data Field in this case 0x00 = Off, see Table 3.40 on page 126 of ZCL. Set the on / off status based on pin
    Serial.println();
    Serial.print(F("Read attribute requst, sending result LED is "));
    if (digitalRead(LEDPin) == 1)
    {
      Serial.print(F("on."));
    }
    else
    {
      Serial.print(F("off."));
    }
    zb.TX(zb._PktIEEEAddHi(), zb._PktIEEEAddLo(), zb._PktNetAdd(), zb._PktDEP(), zb._PktSEP(), zb._PktProfile(), zb._PktCluster(), Buffer, 8);
  }

  if (frmType == 0x01 && cmdID == 0x00 && attributeID == 0x00)                // Set device Off -- P126 Section 3.8.2.3 of ZCL
  {
    digitalWrite(LEDPin, LOW);                                                // Turn LED off - 0V
    sendDefaultResponse(cmdID, 0x00, 0x38);                                   // Send default response back to originator of command
    Serial.println();
    Serial.print(F("LED Off (Pin "));
    Serial.print(LEDPin,DEC);
    Serial.print(F(" set to ground)"));
  }

  if (frmType == 0x01 && cmdID == 0x01 && attributeID == 0x00)                // Set device On
  {
    digitalWrite(LEDPin, HIGH);                                               // Set High
    sendDefaultResponse(cmdID, 0x00, 0x38);                                   // Send default response back to originator of command
    Serial.println();
    Serial.print(F("LED On (Pin "));
    Serial.print(LEDPin,DEC);
    Serial.print(F(" set to High)"));
  }

  if (frmType == 0x01 && cmdID == 0x02 && attributeID == 0x00)                // Toggle device
  {
    digitalWrite(LEDPin,!digitalRead(LEDPin));                                // Toggle Output
    sendDefaultResponse(cmdID, 0x00, 0x38);                                   // Send Default response back to originator of command
    Serial.println();
    Serial.print(F("LED Toggle"));
  }
}

void sendDefaultResponse(byte CmdID, byte Status, byte EndPoint)
{
  //***************************************
  // Send Default Response Page 39 of ZigBee Cluster Library
  // CmdID = Byte size number representing the received command to which this command is a response see Table 2.9 on page 16 of ZigBee Cluster Library
  // Status = Byte size value specifies either Success (0x00) or the nature of the error see Table 2.17 on page 67 of ZigBee Cluster Library
  // EndPoint = Byte size value of the EndPoint this response is for
  //***************************************
  memset(Buffer, 0 , BufferSize);                                             // Clear Buffer
  Buffer[0] = 0x18;                                                           // Frame Control 0x18 = direction is from server to client, disable default response P14 of ZCL
  Buffer[1] = zb._PktData()[1];                                               // Set the sequence number to match the seq number in requesting packet
  Buffer[2] = 0x0B;                                                           // Command Identifer 0x0B = Default response see Table 2.9 on page 16 of ZCL

  Buffer[3] = CmdID;                                                          // Command Identifer to report on

  Buffer[4] = Status;                                                         // Status see Table 2.17 on page 67 of ZigBee Cluster Library

  Serial.println();
  Serial.print(F("Sending Default Response"));
  zb.TX(zb._PktIEEEAddHi(), zb._PktIEEEAddLo(), zb._PktNetAdd(), EndPoint, zb._PktSEP(), zb._PktProfile(), zb._PktCluster(), Buffer, 5);
}

//// * * * * * * * ZigBee Packet formation Notes * * * * * * *
////
//// ZigBee data packet (payload) syntax
////                                 --------------------------------------------- Frame Control 0x18 = direction is from server to client, disable default response P14 of ZCL
////                                |     ---------------------------------------- Transaction seq num 0x66 = Should be the same as the transaction number requesting response
////                                |    |     ----------------------------------- Command Identifer 0x01 = Read attribute response see Table 2.9 on page 16 of ZCL
////                                |    |    |      ----------------------------- Direction field 0x00: = attribute are reported P33 of ZCL
////                                |    |    |     |    ------------------------- Attribute Identifier (2 bytes) field being reported see figure 2.24 on page 36 of ZCL
////                                |    |    |     |   |    |     --------------- Attribute Data Type 0x10 = Boolean, see Table 2.16 on page 54 of ZCL
////                                |    |    |     |   |    |    |     ---------- Attribute Data Field in this case 0x00 = Off, see Table 3.40 on page 126 of ZCL
////                                |    |    |     |   |    |    |    |
////       Resp_OnOFF      Byte   0x18 0x66 0x01  0x00 0x00 0x00 0x10 0x00

////                                 --------------------------------------------- Transaction seq number
////                                |     ---------------------------------------- Status 0x00 = success Table 2.94 on page 161 of ZBSpec
////                                |    |     ----------------------------------- Network address of interest 2 bytes little endian order page 161 of ZBSpec
////                                |    |    |    |     ------------------------- Active EndPoint count page 161 of ZBSpec
////                                |    |    |    |    |     -------------------- Attribute EndPoint List page 161 of ZBSpec
////                                |    |    |    |    |    |
////       Active_EP_rsp   Byte    0x22 0x00 0x63 0x6B 0x01 0x38

////                                 ---------------------------------------------------------------------- Transaction seq number
////                                |     ----------------------------------------------------------------- Status 0x00 = success Table 2.93 on page 159 of ZBSpec
////                                |    |     ------------------------------------------------------------ Network address of interest 2 bytes little endian order page 161 of ZBSpec
////                                |    |    |    |     -------------------------------------------------- Length in bytes of the Simple Descriptor to Follow
////                                |    |    |    |    |     --------------------------------------------- Endpoint of the simple descriptor Table 2.38 on page 88 of ZBSpec
////                                |    |    |    |    |    |     ---------------------------------------- Application Profile ID 2 Bytes Little endian. 0x0104 = Home Automation Profile
////                                |    |    |    |    |    |    |    |     ------------------------------ Application device identifier 2 Bytes Little endian, 0x0002 = On/Off Output See Table 5.1 on page 17 of ZigBee Home Automation Profile
////                                |    |    |    |    |    |    |    |    |    |     -------------------- App Dev Version 4bits + reserved 4bits
////                                |    |    |    |    |    |    |    |    |    |    |     --------------- Input cluster count in this case we only have 0x01 input clusters
////                                |    |    |    |    |    |    |    |    |    |    |    |     ---------- Input cluster list 2 bytes each little endian. 0x0006 = OnOff Cluster Page 125 of ZCL
////                                |    |    |    |    |    |    |    |    |    |    |    |    |    |    - Output cluster list. No output clusers are supported
////                                |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
////                                |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
////                                |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
////      Simple_Desc_rsp Byte    0x22 0x00 0x63 0x6B 0x0A 0x38 0x04 0x01 0x02 0x00 0x00 0x01 0x06 0x00 0x00

////                                 ---------------------------------------------------------------------- Transaction seq number
////                                |     ----------------------------------------------------------------- Network Address 2 bytes little endian Page 109 of ZBSpec
////                                |    |    |     ------------------------------------------------------- IEEE Address 8 bytes little endian
////                                |    |    |    |    |    |    |    |    |    |    |    ---------------- MAC Capability Mask See page 83 of ZBSpec
////                                |    |    |    |    |    |    |    |    |    |    |    |
////      Device_annce    Byte    0x22 0x00 0x63 0x5A 0xE1 0x70 0x40 0x00 0xA2 0x13 0x00 0x8C

