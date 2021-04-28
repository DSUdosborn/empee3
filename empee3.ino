/*************************************************** 
  This is an example for the Adafruit VS1053 Codec Breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout 
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution

  2021 Feb - Mar  - Modification by D.Osborn as DES4990 SP2021 project

  *  Add Oled display
  *  Add Elapsed time tracking
  *  Random Play order
  *  Filename Discovery replaces hardcoded Track001
  *  utilize isMP3File()
  *  added serial console input for testing  (S)top (P)ause toggle (+/-) volume
  *  Directory swapping
  *  
  
 ****************************************************/

// include SPI, MP3 and SdFat libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SdFat.h>

// include Oled
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Define OLED display object
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(U8G2_R0); 



// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

//Declare musicPlayer object
Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

//  Timing defines
long day = 86400000; // 86400000 milliseconds in a day
long hour = 3600000; // 3600000 milliseconds in an hour
long minute = 60000; // 60000 milliseconds in a minute
long second =  1000; // 1000 milliseconds in a second
unsigned long startTime = 0;
unsigned long timeNow = 0;
unsigned long elapsed = 0;

// Volume start   lower numbers are louder
int CurrentVolume = 25;
int hzOff = 0;

// Track Info

  // Note these buffer may be desired to exist globably.
  // but do take much space if only needed temporarily, hence they are here.
  char title[31]; // buffer to contain the extract the Title from the current filehandles
  char artist[31]; // buffer to contain the extract the artist name from the current filehandles
  char album[31]; // buffer to contain the extract the album name from the current filehandles
  
  char* oledAlbum_1 = NULL;
  char* oledAlbum_2 = NULL;

  char* oledArtist_1 = NULL;
  char* oledArtist_2 = NULL;

/*
  Serial Event example
  NOTE: The serialEvent() feature is not available on the Leonardo, Micro, or
  other ATmega32U4 based boards.
  http://www.arduino.cc/en/Tutorial/SerialEvent
*/

String inputString = "";                            // a String to hold incoming data
bool stringComplete = false;                        // whether the string is complete  
 

//  Setup  Oled  RandomSeed  musicPlayer serialConsole

void setup() {

// serial port
  Serial.begin(9600);
  inputString.reserve(20);                        // reserve 200 bytes for the inputString:  
  Serial.println();
  Serial.println("dzed VS1053 eMPee3");
  
 
// SD card
  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);                                          // don't do anything more
  }
  Serial.println("SD OK!");

 // walkRoot(SD.open("/"), 0) ;

  
//oled
  display.begin();

  // Advert
  display.clearBuffer();                      // clear the internal memory
  display.setFont(u8g2_font_profont22_tf);   // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
  display.drawStr(6,14,"eMPee3dzed");       // write something to the internal memory
  display.sendBuffer();                       // transfer internal memory to the display

  delay(3000);

// rando's
  randomSeed(analogRead(0));
  

// initialise the music player
  if (! musicPlayer.begin()) {                  // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));

  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

  // This uses a pin interrupt. No timers required! But DREQ
  // must be on an interrupt pin. For Uno/Duemilanove/Diecimilla
  // that's Digital #2 or #3
  // See http://arduino.cc/en/Reference/attachInterrupt for other pins
  // *** This method is preferred
  if (! musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT))
    Serial.println(F("DREQ pin is not an interrupt pin"));

//  display.clearBuffer();         
  display.setFont(u8g2_font_helvB08_te );  
  display.drawStr(25,30,"VS1053 Booted");  
  display.sendBuffer();         
 
  delay(2000);
  
// Set volume for left, right channels. 
//  lower numbers == louder volume!
  musicPlayer.setVolume(CurrentVolume,CurrentVolume);

// Make a tone to indicate VS1053 is working
  musicPlayer.sineTest(0x44, 250);    
 

  
// spew the tunes
 
//  playDirectory(SD.open("/MAMA"));
//  playDirectory(SD.open("/DS"));
//  playDirectory(SD.open("/DRJ"));
//  playDirectory(SD.open("/BDY"));
//  playDirectory(SD.open("/BF"));
//  playDirectory(SD.open("/BNB"));
//  playDirectory(SD.open("/COV"));
//  playDirectory(SD.open("/FOG"));
//  playDirectory(SD.open("/JM"));
//  playDirectory(SD.open("/FQ"));
  playDirectory(SD.open("/"));  

}

void loop() {  
  if (stringComplete) {                 // SerialEvent() has input ready:
    
   Serial.println(inputString);
    
   inputString = "";                   // clear the string:
   stringComplete = false;             // reset ready flag
  }
}


void playDirectory(File dir) {

//  sd.chdir(dir);

   while(true) {
    nextFile:    
     File entry =  dir.openNextFile();
     if (!entry) {
       break;         // no more files
     }

     if (isMP3File(entry.name())) {
      
        entry.seek((entry.size()-128) + TRACK_ALBUM);
        entry.read(album,30);

        entry.seek((entry.size()-128) + TRACK_ARTIST);
        entry.read(artist,30);
        
        entry.seek((entry.size()-128) + TRACK_TITLE);
        entry.read(title,8);
        entry.seek(0);

        Serial.println(); 
        Serial.print(F("Processing: "));
        Serial.println(entry.name());
      
        if(strlen(album) > 0) {
          
          oledAlbum_1 = strtok(album,"_");
          oledAlbum_2 = strtok(NULL,"_");

          Serial.print(oledAlbum_1);
          if (strlen(oledAlbum_2)){
            Serial.println();
            Serial.println(oledAlbum_2);
          } else{
            Serial.println();
          }
                   
        }
        
        if (strlen(artist) > 0) {
          oledArtist_1 = strtok(artist,"_");
          oledArtist_2 = strtok(NULL,"_");

          Serial.print(oledArtist_1);
          if (strlen(oledArtist_2)){
            Serial.println();
            Serial.println(oledArtist_2);
          } else{
            Serial.println();
          }         
        }


// Start playing a file, then we can do stuff while waiting for it to finish
        delay(2000);
        
        if (! musicPlayer.startPlayingFile(entry.name())) {
          Serial.println("Could not open file ");
          Serial.print(entry.name());
          entry.close();
          goto nextFile;
        }
                  
        startTime = millis();

// Now's a good time to do something else like handling LEDs or buttons :)
      
        while (musicPlayer.playingMusic || musicPlayer.paused()) { 

          if (elapsed < 2500) {
            display.clearBuffer(); 
            display.setFont(u8g2_font_helvR14_tf);         
            display.drawStr(14,22,strtok(entry.name(),"."));  
            display.sendBuffer();             
          }

          if (elapsed > 2500 && elapsed < 5000) {
            display.clearBuffer();                      
            display.setFont(u8g2_font_helvB10_te);
            hzOff = (128-(strlen(artist)*8))/2;
            if(strlen(oledArtist_1) < 14){
              display.setFont(u8g2_font_helvR14_tf);
              hzOff = (128-(strlen(oledArtist_1)*10))/2;                    
            }

            if(!strlen(oledArtist_2)){        
              display.drawStr(hzOff,22,oledArtist_1);  
              display.sendBuffer();
            } else {
              display.drawStr(hzOff,14,oledArtist_1);
              display.setFont(u8g2_font_helvB10_te);
              hzOff = (128-(strlen(oledArtist_2)*8))/2;
              if(strlen(oledArtist_2) < 14){
                display.setFont(u8g2_font_helvR14_tf);
                hzOff = (128-(strlen(oledArtist_2)*10))/2;                    
              }                 
              display.drawStr(hzOff,30,oledArtist_2);  
              display.sendBuffer();                           
            }                              
          }

          if (elapsed > 5000 && elapsed < 10000){
            display.clearBuffer();
            display.setFont(u8g2_font_helvB10_te);
            hzOff = (128-(strlen(oledAlbum_1)*8))/2;
            if(strlen(oledAlbum_1) < 14){
              display.setFont(u8g2_font_helvR14_tf);
              hzOff = (128-(strlen(oledAlbum_1)*10))/2;                    
            }
            if(!strlen(oledAlbum_2)){        
              display.drawStr(hzOff,22,oledAlbum_1);  
              display.sendBuffer();
            } else {
              display.drawStr(hzOff,14,oledAlbum_1);
              display.setFont(u8g2_font_helvB10_te);   
              hzOff = (128-(strlen(oledAlbum_2)*8))/2;
              if(strlen(oledAlbum_2) < 14){
                display.setFont(u8g2_font_helvR14_tf);
                hzOff = (128-(strlen(oledAlbum_2)*10))/2;                    
              } 
              display.drawStr(hzOff,30,oledAlbum_2);  
              display.sendBuffer();                           
            }
          }
          
          if (elapsed > 10000 && elapsed < 15000){
              display.clearBuffer(); 
              display.setFont(u8g2_font_helvR14_tf);
              display.drawStr(20,22,title);
              display.sendBuffer();
              
          }
          if (elapsed > 15000){
            display.clearBuffer(); 
            display.setFont(u8g2_font_helvB10_te);
            hzOff = (128-(strlen(oledAlbum_1)*8))/2;
            if(strlen(oledAlbum_1) < 14){
              display.setFont(u8g2_font_helvR14_tf);
              hzOff = (128-(strlen(oledAlbum_1)*10))/2;                    
            }
            if(!strlen(oledAlbum_2)){        
              display.drawStr(hzOff,22,oledAlbum_1);  
              display.sendBuffer();
            } else {
              display.drawStr(hzOff,14,oledAlbum_1);
              display.setFont(u8g2_font_helvB10_te);   
              hzOff = (128-(strlen(oledAlbum_2)*8))/2;
              if(strlen(oledAlbum_2) < 14){
                display.setFont(u8g2_font_helvR14_tf);
                hzOff = (128-(strlen(oledAlbum_2)*10))/2;                    
              } 
              display.drawStr(hzOff,30,oledAlbum_2);  
              display.sendBuffer();                           
            }
          }

          if (Serial.available()) {
            char c = Serial.read();
    
// if we get an 'n' on the serial console, stop this track... loop will start next
            if (c == 'n') {
              musicPlayer.stopPlaying();
              time();
              printElapsed();
              startTime = millis();
            }
    
// if we get an 'e' on the serial console, update the elapsed time
            if (c == 'e') {
              time();
              printElapsed();
            }
            
// if we get an '+' on the serial console....
  //   if "is playing" then volume UP
  //   if "not playing" scroll Playlist names
            if (c == '+') {
              if (CurrentVolume <= 1) {
                CurrentVolume = 1;
              } else {
                CurrentVolume -= 2;
              }
              musicPlayer.setVolume(CurrentVolume,CurrentVolume);
              Serial.print( "CurrentVolume setting : ");
              Serial.println( CurrentVolume );              
            }  

// if we get an '-' on the serial console....
  //   if "is playing" then volume DOWN
  //   if "not playing" scroll Playlist names
            if (c == '-') {
              if (CurrentVolume > 253) {
                CurrentVolume = 253;
              } else {
                CurrentVolume += 2;               
              }
              musicPlayer.setVolume(CurrentVolume,CurrentVolume);
              Serial.print( "CurrentVolume setting : ");
              Serial.println( CurrentVolume );
            }  
                 
// if we get an 'p' on the serial console, pause/unpause!
            if (c == 'p') {
              if (! musicPlayer.paused()) {
                Serial.print("Paused  ");
                musicPlayer.pausePlaying(true);
              } else { 
                Serial.print("Resumed  ");
                musicPlayer.pausePlaying(false);
              }
            }
          }
          delay(500);
          time();
          
        }
       entry.close();
       time();
       printElapsed();
       startTime = millis();
      }
    }
    dir.close();
    Serial.println("Done playing music");
}

// Just checks to see if the name ends in ".mp3"
boolean isMP3File(const char *fileName) {
  return (strlen(fileName) > 4) &&
         !strcasecmp(fileName + strlen(fileName) - 4, ".mp3");
}

void time(){  
  timeNow = millis();
  elapsed =  timeNow - startTime;
}

void printElapsed() {

  if ( elapsed > 4000 ) {
  
  int days = elapsed / day ;                                //number of days
  int hours = (elapsed % day) / hour;                       //the remainder from days division (in milliseconds) divided by hours, this gives the full hours
  int minutes = ((elapsed % day) % hour) / minute ;         //and so on...
  int seconds = (((elapsed % day) % hour) % minute) / second;

// digital clock display of current time
  Serial.print(days,DEC); 
  printDigits(hours); 
  printDigits(minutes);
  printDigits(seconds);

  Serial.println();
  }
}

void printDigits(byte digits){
 // utility function for digital clock display: prints colon and leading 0
 Serial.print(":");
 if(digits < 10)
   Serial.print('0');
 Serial.print(digits,DEC);  
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial receive (RX). This
  routine is run between each time loop() runs, so using delay inside loop can delay response. 
  Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();                     // get the new byte:
    inputString += inChar;                                // add it to the inputString:
    if (inChar == '\n') {             // if  newline, set a flag so the main loop will process it:
      stringComplete = true;
    }
  }
}


/// File listing helper
void walkRoot(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
        
     if (entry.isDirectory()) {

       for (uint8_t i=0; i<numTabs; i++) {
          Serial.print('\t');
       }
       Serial.println(entry.name());
       walkRoot(entry, numTabs+1);
     } 
     
     // else {
       // files have sizes, directories do not
       //Serial.print("\t\t");
       //Serial.println(entry.size(), DEC);
     //}
     entry.close();
   }  
}
