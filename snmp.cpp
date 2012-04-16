// SNMP manager (client) functions based on the udp client
// Author: Alexander Baxevanis (alex.baxevanis@gmail.com) 
// Copyright: GPL V2

#include "EtherCard.h"
#include "net.h"

#define gPB ether.buffer

// SNMP Packet Definitions

#define SNMP_AGENT_PORT  161

#define SNMP_PACKET_PREAMBLE_SIZE   7
#define SNMP_TOTAL_LENGTH           1
#define SNMP_COMMUNITY_LENGTH       6

byte snmpPacketPreamble[SNMP_PACKET_PREAMBLE_SIZE] PROGMEM = { 
  0x30,  // Type = Sequence
  0x00,  // Length = 0 (needs to be set later - offset = 1)
  0x02,  // Type = Integer
  0x01,  // Length = 1
  0x00,  // SNMP Version = 0 (SNMPv1) 
  0x04,  // Type = String
  0x00   // Length = 0 (needs to be set later - offset = 6)
}; 

#define SNMP_PDU_PREAMBLE_SIZE		17
#define SNMP_GETREQUEST_LENGTH       1
#define SNMP_VARLIST_LENGTH         12
#define SNMP_VARBIND_LENGTH         14
#define SNMP_OID_LENGTH             16

byte snmpPduPreamble[SNMP_PDU_PREAMBLE_SIZE] PROGMEM = {
  0xa0,  // Type=GetRequest
  0x00,  // Length = 0 (needs to be set later - offset = 1)
  0x02,  // Type = Integer
  0x01,  // Length = 1
  0x01,  // RequestID = 1
  0x02,  // Type = Integer
  0x01,  // Length = 1
  0x00,  // Error = 0 (noError)
  0x02,  // Type = Integer
  0x01,  // Length = 1
  0x00,  // Error Index = 0
  0x30,  // Type = Sequence (list of variable bindings)
  0x00,  // Length = 0 (needs to be set later - offset = 12)
  0x30,  // Type = Sequence (variable binding)
  0x00,  // Length = 0 (needs to be set later - offset = 14)
  0x06,  // Type = OID
  0x00   // Length = 0 (needs to be set later - offset = 16)
};

#define SNMP_FINAL_SIZE 2

byte snmpFinal[SNMP_FINAL_SIZE] PROGMEM = {
  0x05,  // Type = Null
  0x00   // Length = 0
}; 

char defaultCommunityString[] PROGMEM = "public";

static size_t encodeOID(uint16_t *oid, size_t oidLength, byte *encodedOID) 
{
  unsigned int encodedOIDlength = 0;
  byte val;
  val = oid[0] * 40 + oid[1];
  if(encodedOID) encodedOID[encodedOIDlength] = val; encodedOIDlength++;

  for (int i = 2; i < oidLength; i++) {
    int value = oid[i];
    int length = 0;
    if (value >= (268435456)) { // 2 ^ 28
      length = 5;
    } else if (value >= (2097152)) { // 2 ^ 21
      length = 4;
    } else if (value >= 16384) { // 2 ^ 14
      length = 3;
    } else if (value >= 128) { // 2 ^ 7
      length = 2;
    } else {  
      length = 1;
    }

    int j = 0;
    for (j = length - 1; j >= 0; j--) {
      if (j) {
        int p = ((value >> (7 * j)) & 0x7F) | 0x80;
        if(encodedOID) encodedOID[encodedOIDlength] = p; encodedOIDlength++;
      } else {
        int p = ((value >> (7 * j)) & 0x7F);
        if(encodedOID) encodedOID[encodedOIDlength] = p; encodedOIDlength++;
      }
    }
  }
  
  return encodedOIDlength;
}

size_t EtherCard::snmpGetRequest(byte *snmpAgentIP, word srcPort, char *communityString, uint16_t *oid, size_t oidLength) {
  
  ether.udpPrepare(srcPort, snmpAgentIP, SNMP_AGENT_PORT);
  
  byte *pb = ether.buffer + UDP_DATA_P;
  
  // Note: this code assumes that all lengths will fit in a byte
  // Probably can't fit larger SNMP packets in our packet buffer anyway
  
  memcpy_P(pb, snmpPacketPreamble, SNMP_PACKET_PREAMBLE_SIZE);
  
  size_t communityStringLength;
  if(communityString != NULL) {
	communityStringLength = strlen(communityString);
  	memcpy(pb + SNMP_PACKET_PREAMBLE_SIZE, communityString, communityStringLength);
  } else {
	communityStringLength = strlen_P(defaultCommunityString);
  	memcpy_P(pb + SNMP_PACKET_PREAMBLE_SIZE, defaultCommunityString, communityStringLength);
  }
  pb[SNMP_COMMUNITY_LENGTH] = (byte) communityStringLength; 
  
  byte *pduStart = pb + SNMP_PACKET_PREAMBLE_SIZE + communityStringLength;
  memcpy_P(pduStart, snmpPduPreamble, SNMP_PDU_PREAMBLE_SIZE);
  
  size_t encodedOIDlength = encodeOID(oid, oidLength, pduStart + SNMP_PDU_PREAMBLE_SIZE);
  pduStart[SNMP_OID_LENGTH] = (byte) encodedOIDlength;
  pduStart[SNMP_VARBIND_LENGTH] = (byte) encodedOIDlength + 4;
  pduStart[SNMP_VARLIST_LENGTH] = (byte) encodedOIDlength + 6;
  pduStart[SNMP_GETREQUEST_LENGTH] = SNMP_PDU_PREAMBLE_SIZE + (byte) encodedOIDlength + SNMP_FINAL_SIZE - 2;
  
  pb[SNMP_TOTAL_LENGTH] = SNMP_PACKET_PREAMBLE_SIZE + communityStringLength + pduStart[SNMP_GETREQUEST_LENGTH];
  
  byte *finalStart =  pduStart + SNMP_PDU_PREAMBLE_SIZE + encodedOIDlength;
  memcpy_P(finalStart, snmpFinal, SNMP_FINAL_SIZE);
  
  size_t packetSize = pb[SNMP_TOTAL_LENGTH] + 2;
  ether.udpTransmit(packetSize);
  
  return finalStart-ether.buffer;
}

boolean EtherCard::snmpCheckResponse(word dstPort) {
  // Only checks if we have received a UDP packet on the expected port
  return ((gPB[IP_PROTO_P] == IP_PROTO_UDP_V) &&
     (gPB[UDP_DST_PORT_H_P] == (dstPort>>8)) &&
     (gPB[UDP_DST_PORT_L_P] == (dstPort & 0xFF)));
}

byte *EtherCard::findVariableInBuffer(uint16_t *oid, size_t oidLength) {
  size_t encodedOIDlength = encodeOID(oid, oidLength, NULL); 
  byte *encodedOID = (byte *)malloc(encodedOIDlength * sizeof(byte));
  encodeOID(oid, oidLength, encodedOID);
  
  byte *oidStart = (byte *) memmem(ether.buffer, ether.bufferSize, encodedOID, encodedOIDlength);
  free(encodedOID);
  return oidStart + encodedOIDlength;
}

char *EtherCard::snmpGetStringVariable(uint16_t *oid, size_t oidLength) {

   byte *varPtr = ether.findVariableInBuffer(oid, oidLength);

   if(varPtr == NULL) return 0;
  
   size_t varStart = varPtr - ether.buffer; 

   size_t stringLength = ether.buffer[varStart+1] + 1; // one exta char for NULL termination
   
   char *stringPtr = (char *) malloc(stringLength);
   memcpy(stringPtr, ether.buffer + varStart + 2, stringLength - 1);
   stringPtr[stringLength-1] = '\0';
   
   return stringPtr; // user responsible for calling free()
}

// This should work for Counter, Gauge, TimeTicks and positive Integers (up to 4 bytes long)
unsigned long EtherCard::snmpGetUnsignedIntegerVariable(uint16_t *oid, size_t oidLength) {

  byte *varPtr = ether.findVariableInBuffer(oid, oidLength);

  if(varPtr == NULL) return 0;
  
  size_t varStart = varPtr - ether.buffer; 
  
  unsigned long variable = 0;
  
  size_t length = ether.buffer[varStart+1];
  
  byte *bytesPtr = (byte *) &variable;
  
  // Network order to host order (little-endian)
  for(int i = 0; i < length; i++) {
    bytesPtr[i] = ether.buffer[varStart+length+1-i];
  } 

  return variable;
}
