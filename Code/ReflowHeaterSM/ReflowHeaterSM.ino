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
double curveSeconds[maxCurves][numValues] = {{0,30,120,150,210,270},{0,90,180,210,240,300},{0,90,180,210,240,300},{0,90,180,210,240,300},{0,30,60,90,120,150}};
double curveCelsius[maxCurves][numValues] = {{25,100,150,183,235,25},{25,90,130,138,165,25},{25,150,175,217,249,25},{25,75,90,100,125,25},{25,50,60,65,75,25}};
const char* curveNames[maxCurves] = {"smd291ax","smdltlfp","smd291snl","user a","user b"};
int curveSelection = 0;
const int graphYBot=59;
const int graphYTop=20;
const int graphXRight=123;
const int graphXLeft=5;
int histIndex=0;
// const int histSpace=5;
// const int histDiam=5;
const int histSize=graphXRight-graphXLeft;
int history[histSize];



//Interpolation::Linear(xValues, yValues, numValues, xValue, true);


const int blinkMillis=500;

////////////////  button press registration  //////////////
#define rightPin 2
#define leftPin 3
#define upPin 4
#define downPin 18
long pressDuration=25;
long holdingDuration=500;
long heldDuration=holdingDuration;
long lastButtonMillis = 0;
long currentHoldMillis =0;

//bool cancelAction = false;
bool lastDown=false;
bool down=false;
bool downHolding=false;
bool downPressed=false;

bool lastUp=false;
bool up=false;
bool upHolding=false;
bool upPressed=false;

bool lastRight=false;
bool right=false;
bool rightHeld=false;
bool rightPressed=false;

bool lastLeft=false;
bool left=false;
bool leftHeld=false;
bool leftPressed=false;

///////////////////  heater vars  /////////////////
bool heaterEnable = false;
#define relay 20
float PID_kp=.015;
float PID_s=0;
// #define PID_ki=1;
// #define imax=.1;
//long last_millis =0;
//long lastTemp=0;
float tempSet=0;
float dc=0;
float temp=0;
int dc_duration=1000;

//////////////// state machine /////////////////////////////////
String state="Menu";
StateMachine machine = StateMachine();

State* smenu = machine.addState(&menu); 
State* spreheat = machine.addState(&preheat);
State* sselect = machine.addState(&select);
State* sedit = machine.addState(&edit);
State* sreflow = machine.addState(&reflow);

//////////////// menu state /////////////////////
int selection = 1;

//////////////// heater state /////////////////////
int preheatSet = 80;



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
  Serial.println("MAX31856 thermocouple test");
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
  smenu->addTransition(&transitionMenuPreheat,spreheat);
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
  machine.run();
  display.display();  
  drive(PID());

}


////////////////////////////States////////////////////////////States////////////////////////////States////////////////////////////States////////////////////////////

void menu(){
  if(machine.executeOnce){
    state="Menu";
    display.clearDisplay();
    drawStatusBar();
    heaterEnable = false;
  }
  //Serial.println("menu");
  tempSet=0;            //turns heater off
   //ensures heater is off
  drawBlink(" Preheat      ",20,25,(selection==1));
  drawBlink(" Select Reflow",20,40,(selection==2));
  if(newTemp){
    updateStatusBarTemp();    
  }
  if((selection==1)&&downPressed){     
    selection++;
  }else if (upPressed){
    selection--;
  }

  // if(selection){
  //   drawChar(0x1A);
  // }
}
bool transitionMenuPreheat(){
  //Serial.print("SITT preheat?? ");  //Serial.println(((selection==1)&&rightHeld));
  return(((selection==1)&&rightHeld));  
}
bool transitionPreheatMenu(){
  //Serial.print("should I transition to menu?? ");//Serial.println(leftHeld);
  return(leftHeld);
}
bool transitionMenuSelect(){
  return(((selection==2)&&rightHeld));  
}
bool transitionSelectMenu(){
  return(leftHeld);  
}
bool transitionSelectReflow(){
  return(rightHeld);
}
bool transitionReflowSelect(){
  return(leftHeld);  

}
void preheat(){
  //Serial.println("preheat");
  if(machine.executeOnce){
    state="Preheat";
    display.clearDisplay();
    drawStatusBar();    
    display.setTextSize(2);
    display.setCursor(20,30);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    drawSpacedRight(temp);  
    display.setCursor(56,30);
    display.print(F("/"));
    display.setCursor(68,30);
    drawSpacedRight(preheatSet);  

  }else if(newTemp){ //update temps if new temps available
    //updateStatusbar();
    display.setTextSize(2);
    display.setCursor(20,30);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    drawSpacedRight(temp);  
    updateStatusBarTemp();    
  }
  if (upPressed||upHolding){ //adjusts temperature
    preheatSet++;
    display.setTextSize(2);
    display.setCursor(68,30);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    drawSpacedRight(preheatSet);     
  }else if(downPressed||downHolding){
    preheatSet--;
    display.setTextSize(2);
    display.setCursor(68,30);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    drawSpacedRight(preheatSet);        
  }else if(rightHeld){
    heaterEnable=!heaterEnable; 
  }
  if(heaterEnable){ //enables heater
    tempSet=preheatSet;
  }else{
    tempSet=0;
  }
}
void select(){
  if(machine.executeOnce){
    state="Select";
    heaterEnable=false;
    tempSet=0;    
    display.clearDisplay();
    drawStatusBar(); 
    drawSelection();
  }else if(newTemp){
    updateStatusBarTemp();
  }
  if(downPressed&&(curveSelection<maxCurves-1)){
    curveSelection++;
    drawSelection();    
    
  }else if(upPressed&&(curveSelection>0)){
    curveSelection--;
    drawSelection();
  }
  for(int i=0; i<maxCurves; i++){
    drawBlink(curveNames[i],2,(10*i+15),(i==curveSelection));
  }
}
void edit(){
  //left right -> change digit, up down -> increment digit
  //hold back exit -> right 

  if(leftHeld){
    //state="Select Curve";
  }else if(rightHeld&&!heaterEnable){
    //state="Reflow";
  }

  //display name, return setpoints 1 time 1, return setpoint 2 time 2, ...setpoint 5 time 5
  //
}
void reflow(){
  if(machine.executeOnce){
    state="Reflow";
    for(int i=0; i<histSize; i++){
      history[i]=0;
    }    display.clearDisplay();
    drawStatusBar();    
    drawGraph(graphXLeft, graphXRight, graphYBot, graphYTop);
    display.setTextSize(1);  
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(8,13);
    display.print(F(curveNames[curveSelection]));
    display.print(F(": "));
    display.print(F(int(curveCelsius[curveSelection][numValues-2])));
    display.print(F("C  "));
  }else if(rightHeld){
    heaterEnable=!heaterEnable;
    if(heaterEnable){
      drawGraph(graphXLeft, graphXRight, graphYBot, graphYTop);
      startMillis=millis();
    }
  }
  if(heaterEnable){
    tempSet=Interpolation::Linear(curveSeconds[curveSelection], curveCelsius[curveSelection], numValues, (millis()-startMillis)/1000, true);
    //Serial.println(tempSet);
    if((millis()-startMillis)/1000>curveSeconds[curveSelection][numValues-1]){
      heaterEnable=false;
    }
    histIndex=int((millis()-startMillis)/1000/curveSeconds[curveSelection][numValues-1]*histSize);
    //Serial.print(histIndex);
    if(history[histIndex]==0){
      history[histIndex]=temp;
      //Serial.println("YAY");
      display.drawPixel(graphXLeft+histIndex,graphYBot-int(temp*(graphYBot-graphYTop)/(curveCelsius[curveSelection][numValues-2])),SSD1306_WHITE);
    }
  }else{
    tempSet=0;
  }
  if(newTemp){
    updateStatusBarTemp();
  }

  //calculate & set temp required
  //drawGraph()
}

////////////////////////////Display////////////////////////////Display////////////////////////////Display////////////////////////////Display///////////////
void drawSpacedRight(int a){
  if(a<100){
    display.print(F(" "));
    if(a<10){
      display.print(F(" "));      
    }
  }
  display.print(F(a));
}
void drawSpacedLeft(int a){
  display.print(F(a));
  if(a<100){
    display.print(F(" "));
    if(a<10){
      display.print(F(" "));      
    }
  }
}
void drawStatusBar(){
  //draw state: curve selected
  //draw settemp
  //draw currenttemp
  //draw fire status
  //display.setCursor(10,5);
  display.setCursor(0,0);
  display.setTextSize(1);

  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.drawLine(0, 10, display.width()-1, 10, SSD1306_WHITE);
  display.print(F(state));
  display.setCursor(0,0);
  // if(state=="Select:"||state=="Reflow:"){
  //   display.setCursor(42,0);
  //   display.print(F(curveNames[i]));
  // }
  display.setCursor(46,0);  
  // if(temp<100){
  //   display.print(F(" "));
  // }
  display.print(F(int(temp)));
  display.setCursor(64,0);  
  display.print(F("/"));
  display.setCursor(70,0);
  drawSpacedRight(tempSet);  
  // display.println(F(int(tempSet)));
  // display.println(F(int(tempSet)));
  // display.println(F(int(tempSet)));
  // display.println(F(int(tempSet)));
  // display.println(F(int(tempSet)));
  // display.println(F(int(tempSet)));
  // display.println(F(int(tempSet)));
}
void updateStatusBarTemp(){
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(46,0);  
  drawSpacedRight(temp);  
  display.setCursor(70,0);
  if(tempSet<100){
    display.print(F(" "));
    if(tempSet<10){
      display.print(F(" "));      
    }
  }
  display.print(F(int(tempSet)));
  display.setCursor(98,0);  
  if(heaterEnable||temp>50){
    drawBlink("HOT!!",98,0,true);
  }else{
    display.print(F("     "));
  }
}
void drawGraph(int xleft, int xright, int ybot, int ytop){
  //float xval=0;
  display.fillRect(xleft,ytop,xright,ybot,SSD1306_BLACK);
  display.drawLine(xleft, ybot, xleft, ytop, SSD1306_WHITE);
  display.drawLine(xleft, ybot, xright, ybot, SSD1306_WHITE);
  float xValue=0;
  float interpolatedY=0;
  float scaledY=0;
  for (int x=xleft; x<xright; x++){
    //display.drawPixel(x,(ybot+ytop)/2, SSD1306_WHITE);
    //Serial.print(x-xleft);Serial.print(", ");
    xValue=float(x-xleft)/(xright-xleft)*(curveSeconds[curveSelection][numValues-1]);
    //Serial.print(xValue);Serial.print(", ");
    interpolatedY=Interpolation::Linear(curveSeconds[curveSelection], curveCelsius[curveSelection], numValues, xValue, true);
    //Serial.print(interpolatedY);Serial.print(", ");
    scaledY=ybot-int(interpolatedY*(ybot-ytop)/(curveCelsius[curveSelection][numValues-2]));
    //Serial.println(scaledY); 
    display.drawPixel(x,scaledY, SSD1306_WHITE);
    //display.drawPixel(x, ybot-int(Interpolation::Linear(curveSeconds[curveSelection], curveCelsius[curveSelection], numValues, float(x)/(xright-xleft)*(curveSeconds[curveSelection][numValues-1]), true)*(ytop-ybot)/(curveCelsius[curveSelection][numValues-1])), SSD1306_WHITE);
  }
  //calculate setpoints
  //if !history draw dots
  //if history draw line from dots to history
  //draw vertical dashed line on current time & horizontal dashed line on current temp
}
void drawInputs(){
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);  
  display.setTextColor(SSD1306_WHITE);
  //display.print(F("hi"));
  if(right){
    display.print(F("right"));
  }
  if(left){
    display.print(F("left"));
  }
  if(down){
    display.print(F("down"));
  }
  if(up){
    display.print(F("up"));
  }
  display.print(F(temp));
  display.display();
}
void drawBlink(String text, int xpos, int ypos, bool blinking){
  display.setTextSize(1);  
  display.setCursor(xpos,ypos);
  if(blinking && (millis()%(2*blinkMillis)>blinkMillis)){
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(text);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  }else{
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.print(text);
  }
}
void drawSelection(){
  display.setTextSize(1);  
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(64,15);
  display.print(F(int(curveSeconds[curveSelection][numValues-1])));
  display.print(F("s, "));
  display.print(F(int(curveCelsius[curveSelection][numValues-2])));
  display.print(F("C"));

  drawGraph(64, 125, 62, 25);

}

////////////////////////////Inputs////////////////////////////Inputs////////////////////////////Inputs////////////////////////////Inputs////////////////////////////
void updateButtons(){
  currentHoldMillis=millis()-lastButtonMillis; //current holding duration
  rightHeld=false;//reset output button statuses
  rightPressed=false;
  leftHeld=false;
  leftPressed=false;
  downHolding=false;    
  downPressed=false;
  upHolding=false;
  upPressed=false;

  lastRight=right;  //update prior button readings
  lastLeft=left;
  lastUp=up;
  lastDown=down;
  
  right=!digitalRead(rightPin); //read current values
  left=!digitalRead(leftPin);
  down=!digitalRead(downPin);
  up=!digitalRead(upPin);

  if((!(lastRight||lastLeft||lastUp||lastDown))&&(right||left||up||down)){  //update lastButtonMillis if new press 
    lastButtonMillis=millis();
  }
  else if((currentHoldMillis>holdingDuration)&&!(left||right)){  //if there's a long holding event in progress
    upHolding = (up&&!down);
    downHolding = (down&&!up);
  }
  if((!(right||left||up||down))&&(lastRight||lastLeft||lastUp||lastDown)){  //if there's a release event
    //Serial.println(currentHoldMillis);
    if(currentHoldMillis>heldDuration){//Held events if holdmillis is long
      rightHeld=lastRight;
      leftHeld=lastLeft;
      //Serial.println("WTF");
    }else if(currentHoldMillis>pressDuration){//press events if holdmillis is shortish
      rightPressed=lastRight;
      leftPressed=lastLeft;
      downPressed=lastDown;
      upPressed=lastUp;
    }
  }
  // if(upPressed){
  //   Serial.println("upPressed");
  // }
  // if(upHolding){
  //   Serial.println("upHolding");
  // }
  // if(rightPressed){
  //   Serial.println("rightPressed");
  // }
  // if(rightHeld){
  //   Serial.println("rightHeld");
  // }
}
void readThermocouple(){
  //thermocouple reading////////////////
  newTemp=!digitalRead(DRDY_PIN);
  if(newTemp){  // The DRDY output goes low when a new conversion result is available
    temp=maxthermo.readThermocoupleTemperature();
    // Serial.print(PID());  Serial.print(", ");
    //Serial.println(temp);
    //last_millis=millis();   
    //last_temp=temp;
  }
}
////////////////////////////Outputs////////////////////////////Outputs////////////////////////////Outputs////////////////////////////Outputs////////////////////////////
float PID(){
  // Serial.print(tempSet-temp);
  // Serial.print(", ");
  // Serial.println((.1*log(tempSet-temp+1)));

  if(temp>=tempSet){
    return 0;
  }
  return max((.1*log(tempSet-temp+1)),0);//+PID_s*temp
  //.1=
}
void drive(float dc){
  if(((millis()%dc_duration) < (dc*dc_duration))&&heaterEnable){
    digitalWrite(relay, HIGH);   //enables heater
  }else{
    digitalWrite(relay, LOW);  //disables heater
  }
}