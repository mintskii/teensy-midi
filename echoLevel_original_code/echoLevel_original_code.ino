/*
'Knobber' - one knob/one button USB MIDI controller by Echolevel - 
http://echolevel.tumblr.com/post/49737964614/knobber-usb-midi-controller-by-echolevel
Feedback: http://twitter.com/echolevel

Special thanks to Philip Cunningham (aka unsymbol, aka 
Firebrand Boy - http://philipcunningham.org )

SysEx Config Message Structure:
0xF0 # SysEx message start byte
0x14 # Manufacturer ID; 0x14 is actually Fairlight, but I don't forsee too many conflicts here... 
0x01 # Knobber knob channel number
0x01 # Knobber button channel number
0x0E # Knobber knob CC number
0x0F # Knobber button CC number
0x01 # Knobber button behaviour (0 = momentary, 1 = toggle)
0xF7 # SysEx message end byte

On first run, your Teensy's EEPROM might contain values left over from a previous sketch so you 
should use Sysex Librarian, MIDI-OX or similar to transmit the default .syx file (available from
wherever you got this code). Thereafter, you can copy that default .syx and use a hex editor to 
adjust the values according to the structure above.
*/

#include <Bounce.h>
#include <EEPROM.h>

// Default settings - will be overwritten if EEPROM values are present.
int knobChan = 1; int buttonChan = 1; int knobCC = 14; int buttonCC = 15; 
int kPin = 45; int bPin = 4; int behaviour = 1;
int inputAnalog, ccValue, iAlag;
boolean toggled = false;
Bounce button0 = Bounce(0,5);

void setup() {
  //MIDI rate
  Serial.begin(31250);
  pinMode(bPin, INPUT_PULLUP);
  delay(5);
  knobChan =  EEPROM.read(1); 
  usbMIDI.sendControlChange(44, knobChan, 2);
  delay(5);
  buttonChan = EEPROM.read(2); 
  delay(5);
  knobCC = EEPROM.read(3);    
  delay(5);
  buttonCC = EEPROM.read(4);
  delay(5);
  behaviour = EEPROM.read(5);
}

void loop() {
  // Check for SysEx config message
  if(usbMIDI.read() && usbMIDI.getType() == 7) {                
     if (usbMIDI.getData1() > 1 && usbMIDI.getData1() < 9) {
        // unpack SysEx
        byte * sysbytes = usbMIDI.getSysExArray();
        if (sysbytes[0] == 0xf0 && sysbytes[7] == 0xf7) { // Good length; legit sysex.
          if(sysbytes[1] == 0x14) {  // It's either Knobber or a Fairlight CMI...
              // 2, 3, 4, 5 and 6 can now be written to EEPROM and to global vars
              EEPROM.write(1, sysbytes[2]);
              knobChan = sysbytes[2];
              EEPROM.write(2, sysbytes[3]);
              buttonChan = sysbytes[3];
              EEPROM.write(3, sysbytes[4]);
              knobCC = sysbytes[4];
              EEPROM.write(4, sysbytes[5]);
              buttonCC = sysbytes[5];
              EEPROM.write(5, sysbytes[6]);
              behaviour = sysbytes[6];

          }          
        }
     } 
  }
  
  
  if(behaviour > 0) {
      // Pushbutton - MOMENTARY behaviour
      button0.update();
      if (button0.fallingEdge()) {
          usbMIDI.sendControlChange(buttonCC, 127, buttonChan);
      }
      if (button0.risingEdge()) {
          usbMIDI.sendControlChange(buttonCC, 0, buttonChan);
      } 
  } else {      
      // Pushbutton - TOGGLE behaviour
      button0.update();
      if(button0.fallingEdge()) {
         if (toggled) {
             usbMIDI.sendControlChange(buttonCC, 0, buttonChan);
             toggled = false;
         } else {
             usbMIDI.sendControlChange(buttonCC, 127, buttonChan);
            toggled = true;
         } 
      }
  }
    
  inputAnalog = analogRead(kPin);  
  if(abs(inputAnalog - iAlag) > 7) {  
    // calc the CC value based on the raw value
    ccValue = inputAnalog/8;                                
    // Invert the pot value (because I soldered it backwards...)
    int inverted = map(ccValue, 127, 0, 0, 127);            
    // send the MIDI
    usbMIDI.sendControlChange(knobCC, inverted, knobChan);                                  
    iAlag = inputAnalog;
  }

  delay(5); // limits message frequency
}
