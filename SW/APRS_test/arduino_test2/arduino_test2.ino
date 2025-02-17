#define ARDUINO
#include "Arduino.h"
#include <SPI.h>
#include <Si446x.h>
#include "radio_config_Si4461.h"

#define CS_PIN 10
#define SDN_PIN 6
#define NIRQ_PIN 8

bool checkCTS(void);
bool setProperties(uint16_t startProperty, uint8_t length , uint8_t* paraBuf);
bool setFrequency(uint32_t freq);
bool setCommand(uint8_t length, uint8_t command, uint8_t* paraBuf);
void setModulation(void);
void enterTxMode(void);
void fifoReset(void);
void fifoWrite(uint8_t length, uint8_t* data);
void setConfig(uint8_t *parameters, uint16_t paraLen);
bool filter_coeffs(void);
bool setTxPower(uint8_t power);

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing Si4461...");
  Si446x_init();
  SPI.begin();
  si446x_info_t partInfo;

  Si446x_getInfo(&partInfo);
  Serial.print("PART ");
  Serial.println(partInfo.part, HEX);
  Serial.print("PART BUILD ");
  Serial.println(partInfo.partBuild, HEX);
  Serial.print("REV ");
  Serial.println(partInfo.chipRev, HEX);
  Serial.print("FUNC ");
  Serial.println(partInfo.func, HEX);
  Serial.print("ID ");
  Serial.println(partInfo.id, HEX);
  Serial.print("PATCH ");
  Serial.println(partInfo.patch, HEX);
  Serial.print("ROMID ");
  Serial.println(partInfo.romId, HEX);
  Serial.print("---------------");
  //49°13'19.3"N 16°35'44.5"E
  uint8_t test_packet[64] = "OK4VP-0>APRS,WIDE1-1,WIDE2-1:!4913.19N/01635.44W-Test message";
  uint8_t buf[] = {RF_POWER_UP};
  const uint8_t CONFIGURATION_DATA[] = RADIO_CONFIGURATION_DATA_ARRAY;
  
  pinMode(SDN_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  pinMode(NIRQ_PIN, INPUT);

  //RESET
  digitalWrite(SDN_PIN, HIGH);
  delay(100);
  digitalWrite(SDN_PIN, LOW);
  delay(20);

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(sizeof(buf), buf);
  digitalWrite(CS_PIN, HIGH);

  delay(200);

  //SET CONFIG
  setConfig(CONFIGURATION_DATA, sizeof(CONFIGURATION_DATA));
  fifoReset();
  fifoWrite(sizeof(test_packet),test_packet);
  //filter coefs?
  filter_coeffs();
  setTxPower(127);
  enterTxMode();
}



void loop() {
  // put your main code here, to run repeatedly:

}

bool setFrequency(uint32_t freq)
{
  uint8_t outdiv = 4;
  uint8_t band = 0;
  if (freq < 850000000UL) {
    outdiv = 4;
    band = 1;
  };
  if (freq < 420000000UL) {
    outdiv = 8;
    band = 2;
  };
  if (freq < 284000000UL) {
    outdiv = 12;
    band = 3;
  };
  if (freq < 142000000UL) {
    outdiv = 16;
    band = 4;
  };
  if (freq < 177000000UL) {
    outdiv = 24;
    band = 5;
  };

  uint32_t f_pfd = 2 * RADIO_CONFIGURATION_DATA_RADIO_XO_FREQ / outdiv;
  uint32_t n = ((uint32_t)(freq / f_pfd)) - 1;
  float ratio = (float)freq / (float)f_pfd;
  float rest = ratio - (float)n;
  uint32_t m = (uint32_t)(rest * 524288UL);
  uint32_t m2 = m / 0x10000;
  uint32_t m1 = (m - m2 * 0x10000) / 0x100;
  uint32_t m0 = (m - m2 * 0x10000 - m1 * 0x100);

  uint8_t buf[1] = {0b1000 + band};
  setProperties(0x2051, sizeof(buf), buf);

  uint8_t buf2[4] = {n, m2, m1, m0};
  return setProperties(0x4000, sizeof(buf2), buf2);

}

bool setProperties(uint16_t startProperty, uint8_t length , uint8_t* paraBuf)
{
  uint8_t buf[4];

  if (!checkCTS())
    return false;

  buf[0] = 0x11;
  buf[1] = startProperty >> 8;   		// GROUP
  buf[2] = length;                	// NUM_PROPS
  buf[3] = startProperty & 0xff; 		// START_PROP

  
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(buf, 4);
  SPI.transfer(paraBuf, length);
  digitalWrite(CS_PIN, HIGH);

  return true;
}

bool checkCTS(void)
{
  uint16_t timeOutCnt;
  timeOutCnt = 2500;
  while (timeOutCnt--)				// cts counter
  {
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(0x44);	// send READ_CMD_BUFF command
    if (SPI.transfer(0) == 0xff)	// read CTS
    {
      digitalWrite(CS_PIN, HIGH);
      return true;
    }
    digitalWrite(CS_PIN, HIGH);
  }
  return false;
}

bool setCommand(uint8_t length, uint8_t command, uint8_t* paraBuf)
{
  if (!checkCTS())
    return false;

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(command);				// send command
  SPI.transfer(paraBuf, length);	// send parameters
  digitalWrite(CS_PIN, HIGH);

  return true;
}

void setModulation(void) {
  uint8_t buf[] = { 0x20, 0x01, 0x00, 0x92};
  setCommand(4, 0x11, buf);
}

void enterTxMode(void)
{
  uint8_t buf[] = {0x00, 0x34, 0x00, 0x00};
  buf[0] = 0x00;
  setCommand(4, 0x31 , buf);
}

void fifoReset(void)
{
  uint8_t data = 0x03;
  setCommand(sizeof(data), 0x15, &data);
}

void fifoWrite(uint8_t length, uint8_t* data)
{
  setCommand(length, 0x66, data);
}

void setConfig(uint8_t *parameters, uint16_t paraLen)
{
  uint8_t cmdLen;
  uint8_t command;
  uint16_t pos;
  uint8_t buf[30];

  paraLen = paraLen - 1;
  cmdLen = parameters[0];
  pos = cmdLen + 1;

  while(pos < paraLen)
  {
    cmdLen = parameters[pos++] - 1;		// get command lend
    command = parameters[pos++];		// get command
    memcpy(buf, parameters + pos, cmdLen);		// get parameters

    setCommand(cmdLen, command, buf);
    pos = pos + cmdLen;
  }
}

bool filter_coeffs(void)
{ 
  uint8_t buf[9] = {0xAD, 0x75, 0xFB, 0x9A, 0x8E, 0xC8, 0x07, 0x21, 0x19}; // UBSEDS (FIR python)
  return setProperties(0x200f, sizeof(buf), buf);
}

bool setTxPower(uint8_t power)
{
  if (power > 127)	// max is 127
    return false;

  uint8_t buf[4] = {0x20, power, 0x00, 0x5d};

  return setProperties(0x2200, sizeof(buf), buf);
}

bool setReadyState(void)
{
  if (!checkCTS())
    return false;
  

}

uint8_t getState(void)
{
  
}
