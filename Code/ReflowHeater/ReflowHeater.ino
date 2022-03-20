//#include ReflowHeater.h
/////////////////  library imports  //////////////////////////
#include "InterpolationLib.h"
#include <Adafruit_MAX31856.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <StateMachine.h>



/////////////////  Thermocouple library  //////////////////
#define DRDY_PIN 6
#define THERM_CS 7
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(THERM_CS);
bool newTemp=false;

/////////////////  OLED library  //////////////////
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels..
#define OLED_RESET     10 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_DC     9
#define OLED_CS     8
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

//////////////////  Interpolation & curve values  ////////////////
const int numValues = 6; 
const int maxCurves = 5;
double curveSeconds[maxCurves][numValues] = {{0,30,120,150,210,270},{0,90,180,210,240,300},{0,90,180,210,240,300},{0,90,180,210,240,300},{0,30,60,90,120,150}}; //preloaded reflow curve "x" values
double curveCelsius[maxCurves][numValues] = {{25,100,150,183,235,25},{25,90,130,138,165,25},{25,150,175,217,249,25},{25,75,90,100,125,25},{25,50,60,65,75,25}}; //preloaded reflow curve "y" values
const char* curveNames[maxCurves] = {"smd291ax","smdltlfp","smd291snl","user a","user b"}; 
int curveSelection = 0; //current selection index
const int graphYBot=59; //location & size of graph during reflow state
const int graphYTop=20;
const int graphXRight=123;
const int graphXLeft=5;
// const int histSpace=5;
// const int histDiam=5;
const int histSize=graphXRight-graphXLeft; //array for storing the acheived reflow values
int history[histSize]; 
int histIndex=0;  



const int blinkMillis=500;

////////////////  button press registration  //////////////
#define rightPin 2 //arduino pins for UI buttons
#define leftPin 3
#define upPin 4
#define downPin 18
long pressDuration=25; //millis durations for different button presses
long holdingDuration=500;
long heldDuration=holdingDuration;
long lastButtonMillis = 0; //time last button press started
long currentHoldMillis =0; //length of current button press

bool lastDown=false; //if down was pressed in last cycle   // down button debouncing and output bools
bool down=false; //if down is currently pressed
bool downHolding=false; //if long down is currently held
bool downPressed=false; //if short down press was released this cycle

bool lastUp=false; //up button 
bool up=false;
bool upHolding=false;
bool upPressed=false;

bool lastRight=false; //right button 
bool right=false;
bool rightHeld=false; //if long right press was just released
bool rightPressed=false;

bool lastLeft=false; //left button
bool left=false;
bool leftHeld=false;
bool leftPressed=false;

///////////////////  heater vars  /////////////////
bool heaterEnable = false;  // safety var for heater enabling / disabling
#define relay 20    //relay pin
float PID_kp=.015;  //proportional gain
// float PID_s=0;
// #define PID_ki=1;
// #define imax=.1;
//long last_millis =0; //vars for debuggging duration between temp readings
//long lastTemp=0;
float tempSet=0; //desired temp
float dc=0; //duty cycle for relay
float temp=0; //current temperature
int dc_duration=1000;

//////////////// state machine /////////////////////////////////
String state="Menu"; //text for current state, used in drawing status bar and updated on switching states, does not control state machine
StateMachine machine = StateMachine(); //initializes state machine

State* smenu = machine.addState(&menu);  //sets up functions used for main loop of each state in state machine
State* spreheat = machine.addState(&preheat);
State* sselect = machine.addState(&select);
//State* sedit = machine.addState(&edit);
State* sreflow = machine.addState(&reflow);

//////////////// menu state /////////////////////
int selection = 1;

//////////////// heater state /////////////////////
int preheatSet = 80; //default preheat setpoint, purposefully safe-ish temp

//////////////// edit curve state /////////////////////

//////////////// reflow state /////////////////////
long startMillis =0;

////////////////////////////Setup////////////////////////////Setup////////////////////////////Setup////////////////////////////Setup////////////////////////////
void setup() {
  // serial output //////
  Serial.begin(115200);
  while (!Serial) delay(10);

  // pin setup //////////
  pinMode(relay, OUTPUT);
  pinMode(DRDY_PIN, INPUT);
  pinMode(rightPin,INPUT_PULLUP);
  pinMode(leftPin,INPUT_PULLUP);
  pinMode(upPin,INPUT_PULLUP);
  pinMode(downPin,INPUT_PULLUP);

  // thermocouple setup //
  Serial.println("MAX31856 thermocouple test"); //this is all default code from example file
  if (!maxthermo.begin()) {
    Serial.println("Could not initialize thermocouple.");
    while (1) delay(10);
  }
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
  Serial.print("Thermocouple type: ");
  switch (maxthermo.getThermocoupleType() ) {
    case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
    case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
    case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
    case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
    case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
    case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
    case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
    case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
    case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
    case MAX31856_VMODE_G32: Serial.println("Voltage x8 Gain mode"); break;
    default: Serial.println("Unknown"); break;
  }
  maxthermo.setConversionMode(MAX31856_CONTINUOUS);

  // screen setup //
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {// set up screen SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  smenu->addTransition(&transitionMenuPreheat,spreheat); //adds transitions to relevant states
  spreheat->addTransition(&transitionPreheatMenu,smenu);  
  smenu->addTransition(&transitionMenuSelect,sselect);  
  sselect->addTransition(&transitionSelectMenu,smenu);  
  sselect->addTransition(&transitionSelectReflow,sreflow);  
  //sselect->addTransition(&transitionSelectEdit,sedit);  
  //sedit->addTransition(&transitionEditSelect,sselect);  
  sreflow->addTransition(&transitionReflowSelect,sselect);  
}

////////////////////////////Main Loop////////////////////////////Main Loop////////////////////////////Main Loop////////////////////////////Main Loop////////////////////////////
void loop() {
  updateButtons(); 
  readThermocouple();
  machine.run(); // checks for state machine transitions and then runs the function correlating to the current state 
  display.display();  
  drive(PID()); //drives heater module based on set temp and current temp
}


////////////////////////////Outputs////////////////////////////Outputs////////////////////////////Outputs////////////////////////////Outputs////////////////////////////
float PID(){//defines duty cycle based on set temp and current temp, currently only P, should add in integral term to eliminate steady state error
  // Serial.print(tempSet-temp);
  // Serial.print(", ");
  // Serial.println((.1*log(tempSet-temp+1)));

  if(temp>=tempSet){//in case function is behaving poorly, still cools if above value
    return 0;
  }
  return max((.1*log(tempSet-temp+1)),0);//logarythmic is better than linear (since high duty cycles are really bad) and PID was being finicky
  //.1=
}
void drive(float dc){//drives relay based on duty cycle
  if(((millis()%dc_duration) < (dc*dc_duration))&&heaterEnable){ //implements duty cycle and checks heaterEnable
    digitalWrite(relay, HIGH);   //enables heater
  }else{
    digitalWrite(relay, LOW);  //disables heater
  }
}