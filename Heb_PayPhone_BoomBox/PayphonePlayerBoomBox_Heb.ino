/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                         //
//   a complete remake of fuzzywobble old payphone to boombox hack :                                                       //
//   https://blog.arduino.cc/2016/11/29/turn-an-old-payphone-into-a-boombox-for-90s-hits/                                  //
//                                                                                                                         //
//   ( different hardware , some concept changes , hebrew menus etc .. )                                                   //
//                                                                                                                         //
//   TOF sensor related code poorly ripped from                                                                            //
//              https://www.electroniclinic.com/tof10120-laser-rangefinder-arduino-display-interfacing-code/               //
//   ( the sensor I had in my drawer at the time :)   )                                                                    //
//                                                                                                                         //
//   hebrew supported LCD library taken from : https://github.com/Boris4K/LCD_1602_HEB                                     //
//   but was a bit wobbly : text manually reversed and positioned and some combos just won't work                          //
//                                                                                                                         //
//                                                                                                                         //
//    the inserted media should include a folder named "MP3" containing :                                                  //
//                                      - prefix 0001 to 0999 : songs to play                                              //
//                                      - prefix 9000 to 9009 : phone digits sound effects                                 //
//                                      - prefix 9010         : phone ring sound effect                                    //
//                                      - prefix 9011         : introduction to play when headset ia raised                //
//                                      - prefix 9012         : ending announcement to play when headset is placed back    //     
//                                                                                                                         //
//                                                                                                                         //
//    Feel free to use this code in any way you wish. would be nice to be mentioned though :-p                             //
//                                                                                                                         //
//    Have fun :)                                                                                                          //
//                                                                                                                         //
//    Tomer Yoselevich                                                                                                     //
//                                                                                                                         // 
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////   

#include <Keypad.h>
#include <SoftwareSerial.h> 
#include <DFPlayerMini_Fast.h>    
#include <LCD_1602_HEB.h>   

//////////////////////////////////////  Values for you to tweak are :   
byte SpkrVol = 50*30/100;                  //  Default external speaker Volume ( 0 to 30 ) 
byte PhonVol = 50*30/100;                  //  Default Phone handset Volume    ( 0 to 30 ) 
unsigned short CloseEnough=300;     //  Detection distance (0 to 2000 mm)
byte DistAvg=3;                     //  number of distance readings to average
//////////////////////////////////////

unsigned short length_val=0;  // Distance sensor variables  .. 
unsigned short Prv_length=0;  //
unsigned char i2c_rx_buf[16]; //
unsigned char dirsend_flag=0; //

SoftwareSerial mySerial(3,2);   // MP3 player setup
DFPlayerMini_Fast myMP3;        //

LCD_1602_HEB lcd(0x3F, 16, 2);   // LCD screen setup              

char hexaKeys[4][3]={{'1','2','3'},{'4','5','6'},{'7','8','9'},{'*','0','#'}}; // Keys setup 
byte colPins[3]={7,6,5}; byte rowPins[4]={8,9,10,11};                          //
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, 4, 3);    //

char ComboKeys[]="###"; int NN=1; int Mode=1;                  // Program global variables 
bool RandMode=false; bool RandTmp=false; bool Greeted=false;   //
bool SpkrTmp=false;  bool SpkrMode=true; bool RingRing=false;  //

void setup()                 //////////// Initial setup : //////////////////
{
  pinMode(4, OUTPUT); digitalWrite(4, HIGH);                                 // Speaker output - ready to ring ..  
  pinMode(A0, INPUT_PULLUP);                                                 // Speaker to headset switch 
  pinMode(12, INPUT_PULLUP);                                                 // Random button switch 
  pinMode(13, INPUT_PULLUP);                                                 // Headset in place switch 
  customKeypad.setDebounceTime(50);                                          // Read delay to prevent button noise 
  mySerial.begin(9600) ; myMP3.begin(mySerial); myMP3.pause(); delay(100);   // MP3 player initialization
  myMP3.volume(SpkrVol);
  lcd.init(); lcd.backlight(); 
  lcd.setCursor(9, 0) ; lcd.print(L"(: םולש");                               // Initial message - only show once 
  delay(2000);                                                               // Just some delay to show initial message  
  lcd.noBacklight(); 
}

void loop()                   /////////// Main Program : ////////////////
{                                                                                                            
  bool OnHook=digitalRead(13); if(OnHook==HIGH)    {   //////// Headset is in place : //////////////////
    if (Mode==2) {                                               // Finishing previous play mode:     //
      lcd.backlight(); lcd.clear(); delay(100);                  //                                   //
      lcd.setCursor(6, 0); lcd.print(L"(: תוארתהלו הדות");       // Session completion message        //
      myMP3.pause(); delay(300); myMP3.playFromMP3Folder(9012);  // Clip 9012 in the mp3 folder       //
      delay (500);                                               //                                   //
      SpkrSwap(); if(!SpkrMode) SpkrSwap();                      // Change back to speaker output     //
      delay(2000);   myMP3.pause();  delay(300);                 // Delay and stop message if active  // 
      lcd.noBacklight();       RingRing=false;                   //                                   //
    }                                                                                                 //
    Mode=1;                                                                                           // 
    RandMode=false; RandTmp=false; Greeted=false;                                                     //
  } 
  else  {                                              //////// Headset is up : ////////////////////////
    if(Mode==1) Greeted=false;                                                                        //
    else {                                    // If in play mode:                                     //
      delay(50);                                                //                                    //
      bool RandBttn=digitalRead(12); if(RandBttn==LOW) {        // Random button pressed              //
        if(!RandTmp) {                                                                                //
          RandTmp=true;//latch                                                                        //
          RandMode=!RandMode;                                   //  Swap Random play flag             //
          if(RandMode) myMP3.pause();delay(200);                //  Stop playing running song         //
        } else RandTmp=false;                                                                         //
      }                                                                                               //
    }                                                                                                 // 
    Mode=2;                                                                                           //
  }
  bool SpkrBttn=digitalRead(A0); if(SpkrBttn==LOW) {       // Change output button pressed : 
     if(!SpkrTmp)   {  SpkrTmp=true;       SpkrSwap();  }  //
  }else SpkrTmp=false;                                     //
  
  switch (Mode)   {
    case 1:    {                      //////// Standby mode : //////////////////////////////////////////
      unsigned short Distance=GetAvgDist();                                                           //
      if(Distance <= CloseEnough)                              /////// Someone's near:                //
      { if(!RingRing){                                         // If ring not already set             //
          RingRing=true;                                       //                                     //
          myMP3.playFromMP3Folder(9010);                       // Clip 9010 in the mp3 folder         //
          lcd.backlight(); lcd.clear(); delay(100);            //                                     //
          lcd.setCursor(11, 0) ; lcd.print(L"? ולה");          // Ring message                        //
          lcd.setCursor(3, 1) ; lcd.print(L"(: ?ונעת ילוא");   //                                     // 
        }                                                                                             //
      }else {                                                  /////// No one's here:                 //
        myMP3.pause(); delay(200);                             // Stop ringing                        //
        lcd.noBacklight(); RingRing=false;                                                            //
      }                                                                                               //
    }    break;                                                                                       
    case 2:    {                      //////// Play mode : /////////////////////////////////////////////
                                                                                                      //
      if(!Greeted) {                                           //// First entry to play mode :        //
        myMP3.pause(); delay(200);                             // Stop playing                        //
        if(SpkrMode) SpkrSwap();                               // Switch to Headset                    //
        Greeted=true;                                          // run only once per session           //
        NN=1;                                                  // reset selection number              //
        ComboKeys[0]='#'; ComboKeys[1]='#'; ComboKeys[2]='#';  //                                     //
        lcd.backlight(); lcd.clear(); delay(100);              //                                     //
        lcd.setCursor(6, 0); lcd.print(L":ריש ורחב");          // Song selection message              //
        lcd.setCursor(0, 0); lcd.print(">>");                  //                                     //
        myMP3.playFromMP3Folder(9011);                         // Clip 9011 in the mp3 folder         //
        delay(500);                                            //                                     //
      }                                                                                               //
      delay(50);                                                                                      //
      char customKey = customKeypad.getKey();                  // Key read:                           //
      if(customKey=='#') {                                                 // Volume up               //
        if(SpkrMode) { SpkrVol=min(SpkrVol+1,30); myMP3.volume(SpkrVol); } //                         //
        else         { PhonVol=min(PhonVol+1,30); myMP3.volume(PhonVol); } //                         //
      }                                                                                               //
      else if (customKey=='*'){                                            // Volume down             //
        if(SpkrMode) { SpkrVol=max(SpkrVol-1,0) ; myMP3.volume(SpkrVol); } //                         //
        else         { PhonVol=max(PhonVol-1,0) ; myMP3.volume(PhonVol); } //                         //
      }                                                                                               //
      else { int nn=-1; // Invalid digit                                                              //
        if( customKey>='0' && customKey<='9' ) {                           // Numeric digit pressed   // 
          nn=customKey-'0' ; ComboKeys[NN-1]=customKey ; RandMode=false ;  //                         //
          myMP3.playFromMP3Folder(9000+nn) ;                               // Clip 9000+digit         //
          delay(600) ;                                                     //                         //
        }                                                                                             //
        if( RandMode && !myMP3.isPlaying() )   {               ///// In Random mode                   //  
          nn=random(0, 9)  ; ComboKeys[NN-1]=nn+'0'    ;       // Generate random digit               //
          }                                                    //                                     //                                                                                             //
        if(nn>=0){ // If valid digit                                                                  //   
          if(NN>=3)    {                                            ///// 3 input digits reached:     //
            lcd.setCursor(2, 0); lcd.print(ComboKeys);              // Display digits                 //
            delay(400);  myMP3.pause();  delay(600);                //                                //
            myMP3.playFromMP3Folder(atoi(ComboKeys));               // Play selected song             //
            delay(1000);                                            //                                //
            NN=1;                                                   //                                //
          } else {                                                                                    //      
            if(NN<3) ComboKeys[2]='#';  // clear previous selection for display                       //
            if(NN<2) ComboKeys[1]='#';  //                                                            //
            NN=NN+1;                                                                                  //
          }                                                                                           //
        }                                                                                             //
      }                                                                                               //
      int DisplayVol;                               // Calculate and display volume percentage        //
      if(SpkrMode) DisplayVol=float(SpkrVol)/3*10;  //                                                //
      else         DisplayVol=float(PhonVol)/3*10;  //                                                //
      lcd.setCursor(12, 1);                         //                                                //
      if(DisplayVol<100) lcd.print(" ");            //                                                //
      if(DisplayVol<10) lcd.print(" ");             //                                                //
      lcd.print(DisplayVol); lcd.print("%");        //                                                //
                                                                                                      //
      lcd.setCursor(2, 0); lcd.print(ComboKeys);    // Display digits                                 // 
      if(RandMode)  { lcd.setCursor(0, 1) ; lcd.print("Random"); }  // Random mode indicator          //
      else          { lcd.setCursor(0, 1) ; lcd.print("      "); }  //                                //
      }  break;                                                                                         
  }                                                                                                   
}                                                                                                     


                         ///////////  Supporting functions:  : ///////////
void SpkrSwap()
{
  SpkrMode=!SpkrMode;
  myMP3.volume(0);delay(100);
  digitalWrite(4, SpkrMode); delay(100);
  if(SpkrMode) myMP3.volume(SpkrVol) ;else myMP3.volume(PhonVol); delay(100); 
}
unsigned short GetAvgDist()
{
      unsigned int length_Avg=0;
      for(int i=0;i<DistAvg;i++) {
        SensorRead(0x00,i2c_rx_buf,2); length_val=i2c_rx_buf[0]; length_val=length_val<<8; length_val|=i2c_rx_buf[1];
        length_Avg=length_Avg+length_val;   delay(100);
        if(length_val>110) {    if(dirsend_flag==0) { dirsend_flag=1 ; SensorWrite(0x0e,&dirsend_flag,1) ; delay(100) ; } }
        else if(length_val<90){ if(dirsend_flag==1) { dirsend_flag=0 ; SensorWrite(0x0e,&dirsend_flag,1) ; delay(100) ; } }
        }
        return(length_Avg/DistAvg);
}
void SensorWrite(unsigned char addr,unsigned char* datbuf,unsigned char cnt) 
{
  unsigned short result=0;
  Wire.beginTransmission(82); // transmit to device #82 (0x52)
  Wire.write(byte(addr));      // sets distance data address (addr)
  delay(1);                   // datasheet suggests at least 30uS
  Wire.write(datbuf, cnt);    // write cnt bytes to slave device
  Wire.endTransmission();      // stop transmitting
}
void SensorRead(unsigned char addr,unsigned char* datbuf,unsigned char cnt) 
{
  unsigned short result=0;
  Wire.beginTransmission(82); // transmit to device #82 (0x52)
  Wire.write(byte(addr));      // sets distance data address (addr)
  Wire.endTransmission();      // stop transmitting
  delay(1);                   // datasheet suggests at least 30uS
  Wire.requestFrom(82, cnt);    // request cnt bytes from slave device #82 (0x52)
  if (cnt <= Wire.available()) { // if two bytes were received
    *datbuf++ = Wire.read();  // receive high byte (overwrites previous reading)
    *datbuf++ = Wire.read(); // receive low byte as lower 8 bits
  }
}
