////////////////////////////Inputs////////////////////////////Inputs////////////////////////////Inputs////////////////////////////Inputs////////////////////////////
//Update functions for different sensors and inputs, typically ran every loop


void updateButtons(){ //reads the UI buttons and assigns 8 output booleans corresponding to wether each button was just pressed, is being held, or was held 
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
  else if((currentHoldMillis>holdingDuration)&&!(left||right)){  //if there's a long 'holding' event for up or down in progress, updates the relevant 'holding variable'
    upHolding = (up&&!down);
    downHolding = (down&&!up);
  }
  if((!(right||left||up||down))&&(lastRight||lastLeft||lastUp||lastDown)){  //if there's a release event for any of the button
    //Serial.println(currentHoldMillis);
    if(currentHoldMillis>heldDuration){//if holdmillis is long, update held events for left and right
      rightHeld=lastRight;
      leftHeld=lastLeft;
    }else if(currentHoldMillis>pressDuration){//if holdmillis is short, update pressed events for all buttons
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
void readThermocouple(){ //updates temp with current temp if there's a new one available from the amplifier
  //thermocouple reading////////////////
  newTemp=!digitalRead(DRDY_PIN); 
  if(newTemp){  // The DRDY output goes low when a new thermocouple temperature is available
    temp=maxthermo.readThermocoupleTemperature();
  }
}