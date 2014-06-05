 /* 
RGB Remote Nightlight v1 - Ben Miller @vmFoo 6-1-2014
Some minor #define snippets borrowed from neopixel examples

*/

#include <EEPROM.h>
#include <PinChangeInt.h>
#include <Adafruit_NeoPixel.h>


#define STATE_COLORS 1
#define STATE_EFFECT 2

//#define SLEEPTIMER  7200000 // 7200000 = 2 hours.  set to 0 to not turn off after a while
#define SLEEPTIMER 0

#define FOBPINA 5
#define FOBPINB 4
#define FOBPINC 3
#define FOBPIND 2

#define LEDPIN 6
#define PIXELS 52
#define COLORPRESETS 7   //how many preset colors are there
#define BRIGHTLEVELS 7 //how many brightness levels there are (excluding off)
#define EFFECTLEVELS 7 //how many effect levels there are.  Controling slew and climb
#define MAXCOLORSLEW 10000 //how many milli-increments to slew the color each cycle
#define MAXCLIMBSPEED 1000 //how many millis between adding anothe pixel to the effect

#define STATEADDR 1
#define BRIGHTADDR 2
#define COLORADDR 3
#define EFFECTADDR 4


/* Initiate variables and set defaults */
volatile boolean onoff; //helps the on/off states

volatile int bright=0 ;  //used to represet the current brightness
volatile uint32_t color; //used to represet the current set color

int i=0;  //predeclare a counter integer
uint32_t sleeptimer; //if you are using a sleep timer that turns it off after a whlie

uint32_t colors[COLORPRESETS]; //declare the color preset array
int currentColorPreset;  //

int brightlevels[BRIGHTLEVELS];
int currentBrightLevel;

int currentEffectLevel;  //1 to EFFECTLEVELS
uint32_t speedColorSlew;  //how fast the colors move from one to the other milisecond
uint32_t speedColorClimb;  //how quickly the change moves to the next pixel miliseconds

uint32_t targetcolor;
volatile uint8_t latest_interrupted_pin;


//variable for managing toggle states
int foba=0;
int fobb=0;
int fobc=0;
int fobd=0;



//the almighty state variable for the state machine
int state;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

void setup()
{
  //setup the debugging if available]
  Serial.begin(19200);
  Serial.println("RGB Remote Nightlight V1.0: Ready");

  //setup the pin modes  
  pinMode(FOBPINA, INPUT); 
  pinMode(FOBPINB, INPUT); 
  pinMode(FOBPINC, INPUT); 
  pinMode(FOBPIND, INPUT); 

  //setup the interrupt on each of the pins
  PCintPort::attachInterrupt(FOBPINA, &fobHandler, RISING);
  PCintPort::attachInterrupt(FOBPINB, &fobHandler, RISING);
  PCintPort::attachInterrupt(FOBPINC, &fobHandler, RISING);
  PCintPort::attachInterrupt(FOBPIND, &fobHandler, RISING);

  //setup the preset colors the color button will cycle
  colors[0]=pixels.Color(255,0,0);  //
  colors[1]=pixels.Color(255,255,0);  //
  colors[2]=pixels.Color(255,0,255);  //
  colors[3]=pixels.Color(0,255,0);  //
  colors[4]=pixels.Color(0,255,255);  //
  colors[5]=pixels.Color(0,0,255);  //
  colors[6]=pixels.Color(255,255,255);  //

  //setup the brightness levesl.  Evenly split between 1 and 255
  for(int x=1; x <= BRIGHTLEVELS; x++) {
      brightlevels[x-1]=x*(255/BRIGHTLEVELS);
  }
    
  //set the sleeptimer
  sleeptimer=millis()+SLEEPTIMER;

  //read the variables from EPROM and setup system
  EEPROM.write(STATEADDR, state);
  EEPROM.write(BRIGHTADDR, currentBrightLevel);
  EEPROM.write(COLORADDR, currentColorPreset);
  EEPROM.write(EFFECTADDR, currentEffectLevel);
  //set initial variables if 0 from eeprom
   if(state == 0) { 
    state=STATE_COLORS;
    currentBrightLevel=BRIGHTLEVELS/2; //halfway up
    currentColorPreset=6; //why not start at white ;-)
    currentEffectLevel=1; //start slow
   }

  //setup the sleep timer
  sleeptimer = millis()+SLEEPTIMER;  //after this time, turn off the lights
  
  //engage the neopixel subsystem and set it to off
  pixels.begin();
  pixels.setBrightness(0);   //off
  pixels.show();
  Serial.print("State: ");
  Serial.println(state);
  Serial.println("Setup complete.  In off mode.");

}

void loop(){
//    Only do stuff here for the effect.  Everything else is handled in interrups
//    Oh, but also check the sleep timer and do nothing if we are "off".

  if(latest_interrupted_pin) changeHandler();  //if an interrupt occured since the last loop

  if(onoff == true) {
    //check the sleeptimer
    if(millis() > sleeptimer && SLEEPTIMER ) { //if SLEEPTIMER is non 0
      turnOff();
    }
  }
  
  
}


void setEffectLevel(int lvl){
   if (lvl > EFFECTLEVELS || lvl < 1) return;  //sanitize input

   //Level 1 shoudl be slowest setting
   currentEffectLevel=lvl;
   //the speedColorSlew represents how many milli-increments the color is skewed per milli
   //if the slew is 1000 then the color will be slewed 1 per second.  If it is 10,000 it will be slewed 10 per second
   //the higher the number the faster the slew
   speedColorSlew= (MAXCOLORSLEW / EFFECTLEVELS) * lvl;
     
   //the speedColorClimb represetns how many millis pass before it adds 1 more pixel to the set that are slewing
   //1000 means a pixel is added to the set that are slewing every second meaning the PIXELS pixel will start color slewing
   //after PIXELS seconds.
   //The LOWER the number the faster the climb
   
   speedColorClimb= (MAXCLIMBSPEED / EFFECTLEVELS) * (EFFECTLEVELS +1 -lvl );  //invert the multiple to be slow a 1 and fast at EFFECTSLEVELS
     
}

void turnOff(){
  //dim off effect
  for (int x=bright; x>0; x--){
      pixels.setBrightness(x);
      pixels.show();
      //delayMicroseconds(10000); //don't use delay and messup interrupts
  } 
  pixels.setBrightness(0); //handle brightness changes
  pixels.show();
  onoff=false;
  /*record settings to EPROM here
  STATEADDR contains state variable
  BRIGHTADDR contains currentBrightLevel
  COLORADDR contains currentColorPreset
  EFFECTADDR contains currentEffectLevel */
  EEPROM.write(STATEADDR, state);
  EEPROM.write(BRIGHTADDR, currentBrightLevel);
  EEPROM.write(COLORADDR, currentColorPreset);
  EEPROM.write(EFFECTADDR, currentEffectLevel);
  Serial.println("Turned off");
}

void turnOn(){
  bright=brightlevels[currentBrightLevel];
  color=colors[currentColorPreset];
  
  for (int y=0; y < bright; y++){
      pixels.setBrightness(y);
      for( int x=0 ; x< PIXELS ; x++)
         pixels.setPixelColor(x, color);
      pixels.show();
      //delayMicroseconds(10000); //don't use delay and messup interrupts
  }
  pixels.setBrightness(bright);
  pixels.show();
  
  onoff = true;
  sleeptimer = millis()+SLEEPTIMER;  //reset the sleeptimer 
  Serial.print("Turning on brightness: ");
  Serial.println(bright);
  Serial.print("Color preset:");
  Serial.println(currentColorPreset);
}

void changeHandler(){
  uint32_t target; //for use in blending to the right color
  
  //handle turning the pixels on and off
  if(latest_interrupted_pin == FOBPINA) {  //onoff state
  latest_interrupted_pin=0;
    if(onoff == true) {  //we are on,  turn off
      turnOff();
    } else{ //we are off, turn on
      turnOn();
    }
  }
  
  //switch to color mode and cycle colors
  if(latest_interrupted_pin == FOBPINB && onoff) { //quick easy change to color state and cycle
    latest_interrupted_pin=0;
    
    if(state != STATE_COLORS) {
      state = STATE_COLORS; //switch to this state and stop
      color=colors[currentColorPreset];
      Serial.println("Switching to color mode");
      for( int x =0 ; x< PIXELS ; x++)
        pixels.setPixelColor(x,color);
      pixels.show();
    }
    else {  //the button was pushed when we were already in static color state
            //cycle through the preset colors
      if(currentColorPreset < COLORPRESETS-1)
        currentColorPreset++;  //increment if not at max
      else {
        currentColorPreset=0;  //roll over
      }
      color=colors[currentColorPreset];
      for( int x =0 ; x< PIXELS ; x++)
          pixels.setPixelColor(x,color);
      pixels.show();

      Serial.print("Switched to color index:");
      Serial.print(currentColorPreset);
      Serial.print(" which is ");
      Serial.println(color);
    }
    for( int x =0 ; x< PIXELS ; x++)
      pixels.setPixelColor(x,color);
    pixels.show();
    return;  
  }
  
  
  //Cycle brightness levels does not affect the state
  if(latest_interrupted_pin == FOBPINC && onoff ) {  //quick easy change bright state but only if we're on
  latest_interrupted_pin=0;
    if(currentBrightLevel < BRIGHTLEVELS-1)
      currentBrightLevel++;  //increment if not at max
    else
      currentBrightLevel=0;  //roll over
    bright=brightlevels[currentBrightLevel];
    pixels.setBrightness(bright); //handle brightness changes
    pixels.show();
    Serial.print("Setting brightness to: ");
    Serial.println(bright);
  }
  
  //Put the machine into the effect state and the loop will handle updates
  //repeated presses will cycle the speeds for color slew and climb
  if(latest_interrupted_pin == FOBPIND && onoff) {
  latest_interrupted_pin=0;
    if(state != STATE_EFFECT) {
      Serial.println("Switched to effect state");
      state = STATE_EFFECT; //switch to this state and let the effect loop do the work
      //don't change anything else
    }
    else {  //the button was pushed when we were already in static color state
            //cycle through the preset colors
      Serial.println("Changing effect levels");
      if(currentEffectLevel < EFFECTLEVELS)
        setEffectLevel(currentEffectLevel+1);  //increment if not at max
      else {
        setEffectLevel(1);  //roll over
      }
    }
  }      
}


void fobHandler() {
  latest_interrupted_pin=PCintPort::arduinoPin;
  //set the variable and get out of the interrupt code
  //funky things can happen inside interrupt vectors
}
