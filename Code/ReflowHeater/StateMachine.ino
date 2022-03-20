////////////////////////////States////////////////////////////States////////////////////////////States////////////////////////////States////////////////////////////
/*
This 

*/

//////////////////////////// Menu //////////////////////////// Menu ////////////////////////////
void menu(){ //simple menu for moving to selection or preheat states
  if(machine.executeOnce){ //executes on first cycle in menu
    state="Menu";
    display.clearDisplay();
    drawStatusBar(); //only draws full status bar first time in loop
    heaterEnable = false;//turns heater off
    tempSet=0;
  }else if(newTemp){ //updates status bar if there's a new temp
    updateStatusBarTemp();    
  }              
  drawBlink(" Preheat      ",20,25,(selection==1));
  drawBlink(" Select Reflow",20,40,(selection==2));

  if((selection==1)&&downPressed){     //change menu selection w/ up and down
    selection++;
  }else if (upPressed){
    selection--;
  }
}
bool transitionMenuPreheat(){ //holding right -> preheat state
  //Serial.print("SITT preheat?? ");  //Serial.println(((selection==1)&&rightHeld));
  return(((selection==1)&&rightHeld));  
}
bool transitionMenuSelect(){ //holding right -> select state
  return(((selection==2)&&rightHeld));  
}

//////////////////////////// Select //////////////////////////// Select ////////////////////////////
void select(){  //displays loaded reflow curves and allows selection among them
  if(machine.executeOnce){//executes on first cycle in select state
    state="Select";
    heaterEnable=false;
    tempSet=0;    
    display.clearDisplay();
    drawStatusBar(); 
    drawSelection(); //draws details (temp, time, curve) for the current selection
  }else if(newTemp){
    updateStatusBarTemp();
  }
  if(downPressed&&(curveSelection<maxCurves-1)){ //moves to different selection based on button presses
    curveSelection++;
    drawSelection();    
  }else if(upPressed&&(curveSelection>0)){
    curveSelection--;
    drawSelection();
  }
  for(int i=0; i<maxCurves; i++){ //draws all curves in list on screen, current selection blinking
    drawBlink(curveNames[i],2,(10*i+15),(i==curveSelection)); 
  }
}
bool transitionSelectMenu(){ //holding left -> menu state
  return(leftHeld);  
}
bool transitionSelectReflow(){ //holding right -> reflow state
  return(rightHeld);
}

//////////////////////////// Reflow //////////////////////////// Reflow ////////////////////////////
void reflow(){ //executes the selected reflow curve when user holds right
  if(machine.executeOnce){ //draws selected reflow graph and title 
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
  }else if(rightHeld){ //starts reflow curve
    heaterEnable=!heaterEnable;
    if(heaterEnable){ //redraws graph on starting reflow to eliminate old history values if there are any
      drawGraph(graphXLeft, graphXRight, graphYBot, graphYTop);
      startMillis=millis();
    }
  }else if(newTemp){
    updateStatusBarTemp();
  }
  if(heaterEnable){ //if currently reflowing
    tempSet=Interpolation::Linear(curveSeconds[curveSelection], curveCelsius[curveSelection], numValues, (millis()-startMillis)/1000, true);//sets temp based on curve and time
    //Serial.println(tempSet);
    if((millis()-startMillis)/1000>curveSeconds[curveSelection][numValues-1]){ //stops reflow once curve time reached
      heaterEnable=false;
    }
    histIndex=int((millis()-startMillis)/1000/curveSeconds[curveSelection][numValues-1]*histSize);//indexes into history based on elapsed time such that each even durations are captures by each pixel
    //Serial.print(histIndex);
    if(history[histIndex]==0){//if there's no history, records a value
      history[histIndex]=temp;
      //Serial.println("YAY");
      display.drawPixel(graphXLeft+histIndex,graphYBot-int(temp*(graphYBot-graphYTop)/(curveCelsius[curveSelection][numValues-2])),SSD1306_WHITE); //draws current temp on graph
    }
  }else{//if not currently reflowing, sets desired temp to 0
    tempSet=0;
  }


  //calculate & set temp required
  //drawGraph()
}
bool transitionReflowSelect(){ //holding left -> select state
  return(leftHeld);  

}

//////////////////////////// Preheat //////////////////////////// Preheat ////////////////////////////
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
bool transitionPreheatMenu(){ //holding left -> menu state
  //Serial.print("should I transition to menu?? ");//Serial.println(leftHeld);
  return(leftHeld);
}


/*
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
*/
