/* 
 *  by tlathi
 *  Last modified: 2025-01-15 10:24:47 UTC by tlathi1
 *  
 *  This code can receive IR signals automatically and playback them in every one seconds with serial output and Built-in-LED feedback
 */
#include <IRLibDecodeBase.h>  
#include <IRLibSendBase.h>   
#include <IRLib_P01_NEC.h>    
#include <IRLib_P02_Sony.h>   
#include <IRLib_P03_RC5.h>
#include <IRLib_P04_RC6.h>
#include <IRLib_P05_Panasonic_Old.h>
#include <IRLib_P08_Samsung36.h> 
#include <IRLib_HashRaw.h>    
#include <IRLibCombo.h>      
IRdecode myDecoder;
IRsend mySender;

// Include a receiver either this or IRLibRecvPCI or IRLibRecvLoop
#include <IRLibRecv.h>
IRrecv myReceiver(2); //pin number for the receiver

// Storage for the recorded code
uint8_t codeProtocol;  // The type of code
uint32_t codeValue;    // The data bits if type is not raw
uint8_t codeBits;      // The length of the code in bits

//These flags keep track of whether we received the first code 
//and if we have received a new different code from a previous one.
bool gotOne, gotNew;

// Define the pin for the LED
const int ledPin = 13;

void setup() {
  gotOne = false; gotNew = false;
  codeProtocol = UNKNOWN;
  codeValue = 0;
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  delay(2000); while (!Serial); //delay for Leonardo
  Serial.println(F("Send a code from your remote and we will record it."));
  myReceiver.enableIRIn(); // Start the receiver
}

// Stores the code for later playback
void storeCode(void) {
  gotNew = true; gotOne = true;
  codeProtocol = myDecoder.protocolNum;
  Serial.print(F("Received "));
  Serial.print(Pnames(codeProtocol));
  if (codeProtocol == UNKNOWN) {
    Serial.println(F(" saving raw data."));
    myDecoder.dumpResults();
    codeValue = myDecoder.value;
  }
  else {
    if (myDecoder.value == REPEAT_CODE) {
      // Don't record a NEC repeat value as that's useless.
      Serial.println(F("repeat; ignoring."));
    } else {
      codeValue = myDecoder.value;
      codeBits = myDecoder.bits;
    }
    Serial.print(F(" Value:0x"));
    Serial.println(codeValue, HEX);
  }
}

void sendCode(void) {
  digitalWrite(ledPin, HIGH); // Turn the LED on
  if (!gotNew) { //We've already sent this so handle toggle bits
    if (codeProtocol == RC5) {
      codeValue ^= 0x0800;
    }
    else if (codeProtocol == RC6) {
      switch (codeBits) {
        case 20: codeValue ^= 0x10000; break;
        case 24: codeValue ^= 0x100000; break;
        case 28: codeValue ^= 0x1000000; break;
        case 32: codeValue ^= 0x8000; break;
      }
    }
  }
  gotNew = false;
  if (codeProtocol == UNKNOWN) {
    //The raw time values start in decodeBuffer[1] because
    //the [0] entry is the gap between frames. The address
    //is passed to the raw send routine.
    codeValue = (uint32_t)&(recvGlobal.decodeBuffer[1]);
    //This isn't really number of bits. It's the number of entries
    //in the buffer.
    codeBits = recvGlobal.decodeLength - 1;
    Serial.println(F("Sent raw"));
  }
  mySender.send(codeProtocol, codeValue, codeBits);
  if (codeProtocol == UNKNOWN) return;
  Serial.print(F("Sent "));
  Serial.print(Pnames(codeProtocol));
  Serial.print(F(" Value:0x"));
  Serial.println(codeValue, HEX);
  digitalWrite(ledPin, LOW); // Turn the LED off
}

void loop() {
  if (myReceiver.getResults()) {
    myDecoder.decode();
    storeCode();
    myReceiver.enableIRIn(); // Re-enable receiver
  }
  
  if (gotOne) {
    sendCode();
    delay(1000); // Wait for 1 second before sending the code again
  }
}
