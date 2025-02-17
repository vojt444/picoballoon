#include "Arduino.h"
#include <SPI.h>
#include <Si446x.h>

#define CS_PIN 10
#define SDN_PIN 6
#define NIRQ_PIN 8

#define SI446X_CSN			10
#define SI446X_SDN			6
#define SI446X_IRQ			8

void ax25_encode_callsign(char *callsign, uint8_t ssid, uint8_t *encoded);
uint16_t ax25_fcs(const uint8_t *data, size_t length);
void create_aprs_packet(uint8_t *packet, size_t *packet_len, char *src, uint8_t src_ssid, char *dst, uint8_t dst_ssid, char *digipeaters[][2], int digi_count, char *payload);
void print_packet_hex(const uint8_t *packet, size_t length);

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing Si4461...");
  Si446x_init();
  SPI.begin();
  si446x_info_t partInfo;
  si446x_state_t state;
  si446x_state_t state2;
  uint8_t err = 0;

  //uint8_t test_packet[64] = "OK4VP-0>APRS,WIDE1-1,WIDE2-1:!4913.19N/01635.44W-Test message";
  uint8_t aprs_packet[256];
  size_t packet_len;

  char *source = "OK4VP ";
  uint8_t source_ssid = 1;
  char *destination = "APRS  ";
  uint8_t destination_ssid = 0;
  
  char *digipeaters[][2] = {{"WIDE1", "1"}, {"WIDE2", "1"}};
  int digi_count = 2; // Number of digipeaters

  char *payload = ",TEST-MESSAGE";

  create_aprs_packet(aprs_packet, &packet_len, source, source_ssid, destination, destination_ssid, digipeaters, digi_count, payload);
  print_packet_hex(aprs_packet, packet_len);

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
  Serial.print("---------------\n");

  Si446x_setTxPower(SI446X_MAX_TX_POWER);

  state = Si446x_getState();
  Serial.println(state, HEX);

  err = Si446x_TX(aprs_packet, sizeof(aprs_packet), 0, SI446X_STATE_READY);
  if(err == 1)
    Serial.print("Packet sent succesfully\n");
  else
    Serial.print("Packet error\n");

  delay(1000);

  err = Si446x_sleep();
  if(err == 1)
    Serial.print("Sleep mode set succesfully\n");
  else
    Serial.print("Sleep mode set error\n");

  state2 = Si446x_getState();
  Serial.println(state2, HEX);

}

void loop() {
  // put your main code here, to run repeatedly:

}

void ax25_encode_callsign(char *callsign, uint8_t ssid, uint8_t *encoded) {
    memset(encoded, ' ' << 1 , 6);
    /*for(uint8_t i = 0; i < 6; i++)
    {
      Serial.print(encoded[i], HEX);
      Serial.print(' ');
    }
    Serial.print('\n');*/
    for(uint8_t i = 0; i < 6; i++) 
      encoded[i] = (callsign[i] << 1);

    encoded[6] = ((ssid & 0x0F) << 1);
    /*for(int i = 0; i < sizeof(encoded); i++)
      Serial.print(encoded[i], HEX);
    Serial.print("\n");*/
}

uint16_t ax25_fcs(const uint8_t *data, size_t length) 
{
    uint16_t crc = 0xFFFF;
    for(size_t i = 0; i < length; i++) 
    {
        crc ^= data[i];
        for(int j = 0; j < 8; j++) 
        {
            if(crc & 0x0001)
              crc = (crc >> 1) ^ 0x8408;
            else 
              crc >>= 1;
        }
    }
    return crc;
}

void create_aprs_packet(uint8_t *packet, size_t *packet_len, char *src, uint8_t src_ssid, char *dst, uint8_t dst_ssid, char *digipeaters[][2], int digi_count, char *payload) 
{
    uint8_t frame[256];
    size_t pos = 0;

    frame[pos++] = 0x7E;

    ax25_encode_callsign(dst, dst_ssid, &frame[pos]);
    pos += 7;

    ax25_encode_callsign(src, src_ssid, &frame[pos]);
    pos += 7;

    for(int i = 0; i < digi_count; i++) 
    {
        ax25_encode_callsign(digipeaters[i][0], atoi(digipeaters[i][1]), &frame[pos]);
        if (i == digi_count - 1) 
          frame[pos + 6] |= 0x01;

        pos += 7;
    }

    frame[pos++] = 0x03;
    frame[pos++] = 0xF0;

    strncpy((char *)&frame[pos], payload, strlen(payload));
    pos += strlen(payload);

    uint16_t crc = ax25_fcs(frame, pos);
    frame[pos++] = crc & 0xFF;
    frame[pos++] = (crc >> 8) & 0xFF;

    frame[pos++] = 0x7E;

    memcpy(packet, frame, pos);
    *packet_len = pos;
}

void print_packet_hex(const uint8_t *packet, size_t length) 
{
    Serial.print("Encoded AX.25 Packet (Hex): ");
    for(size_t i = 0; i < length; i++) 
    {
      Serial.print(packet[i], HEX);
      Serial.print(" ");
    }

    Serial.print("\n");
}

