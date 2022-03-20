////////////////////////////Display////////////////////////////Display////////////////////////////Display////////////////////////////Display///////////////
void drawSpacedRight(int a){ //draws int a at 3 digit width, padding with spaces on left
  if(a<100){ //pads w/ one space if <3 digits
    display.print(F(" "));
    if(a<10){ //pads w/ two spaces if <3 digits
      display.print(F(" "));      
    }
  }
  display.print(F(a));
}
void drawSpacedLeft(int a){ //draws int a at 3 digit width, padding with spaces on right
  display.print(F(a));
  if(a<100){//pads w/ one space if <3 digits
    display.print(F(" "));
    if(a<10){//pads w/ two spaces if <3 digits
      display.print(F(" "));      
    }
  }
}
void drawStatusBar(){ //draws status bar on top of screen w/ state, temp, temp set, and 
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
void updateStatusBarTemp(){ //update status bar with new temperature (call when new temp available)
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
void drawGraph(int xleft, int xright, int ybot, int ytop){//draws a reflow graph at the specified coordinates
  //float xval=0;
  display.fillRect(xleft,ytop,xright,ybot,SSD1306_BLACK); //clears area used in graph
  display.drawLine(xleft, ybot, xleft, ytop, SSD1306_WHITE); //draws y axis
  display.drawLine(xleft, ybot, xright, ybot, SSD1306_WHITE); //draws x axis
  float xValue=0; //initializes intermediate values used in calculation
  float interpolatedY=0;
  float scaledY=0;
  for (int x=xleft; x<xright; x++){ //for every pixel width in graph
    xValue=float(x-xleft)/(xright-xleft)*(curveSeconds[curveSelection][numValues-1]); // calculates 'seconds' corresponding to current pixel
    interpolatedY=Interpolation::Linear(curveSeconds[curveSelection], curveCelsius[curveSelection], numValues, xValue, true); //calculates set temp for given 'seconds'
    scaledY=ybot-int(interpolatedY*(ybot-ytop)/(curveCelsius[curveSelection][numValues-2])); //scales set temp to display y values
    display.drawPixel(x,scaledY, SSD1306_WHITE); //draws pixel
  }
  //calculate setpoints
  //if !history draw dots
  //if history draw line from dots to history
  //draw vertical dashed line on current time & horizontal dashed line on current temp
}
void drawInputs(){//debug function that prints out currently pressed buttons
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
void drawBlink(String text, int xpos, int ypos, bool blinking){  //draws a blinking text object if blinking = true
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
void drawSelection(){//draws details and graph for selected reflow curve
  display.setTextSize(1);  
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(64,15);
  display.print(F(int(curveSeconds[curveSelection][numValues-1])));
  display.print(F("s, "));
  display.print(F(int(curveCelsius[curveSelection][numValues-2])));
  display.print(F("C"));

  drawGraph(64, 125, 62, 25);

}