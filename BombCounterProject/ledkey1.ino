#define PRESSED LOW
#define NOT_PRESSED HIGH
#define COUNT 8   // number of display's segments
#define DIR_CW 1 //direction clockwise
#define DIR_CCW -1 //direction counter clockwise
#define DIR_NONE 0 //direction none


const int TMSTROBE = 7;  //pins
const int clock = 5;
const int TMDISPLAY = 4;
const int ENCODERBUTTON = 11;
const int ENCODERB = 3;
const int ENCODERA = 2;


const int LONGPRESS = 500; //variables to distinguish if it was a short or long press
const int SHORTPRESS = 100;


typedef struct Buttons { //structure for encoder button
  const byte pin = ENCODERBUTTON;
  const int debounce = 10;
  unsigned long counter = 0;
  bool prevState = NOT_PRESSED;
  bool currentState;
} Button;


unsigned long MAX = 99999999; //max number which can be displayed on module - right now its designed for 8 segments, should be updated accordingly
long codeHolder; // helper variable which is usefull when dealing with countdown to ensure that we  make it on time (e.g. going from MAX value to 0 in 60 seconds);
Button button;
unsigned long CODE; // variabe to store number on display as a whole
int numbersOnDisplay[COUNT]; //array to store numbers on segments
int speedOption = 1; // value which speeds up or slow down countdown
int currentRotaryPosition = 0; //current position of rotation - usefull for deciding which segment should we blink etc.


unsigned long previousMillisBlink = 0; //helper variable storing previous time - usefull when dealing with blinking
unsigned long currentProcessingTime; //helper variable storing current time - usefull when dealing with countdown to ensure we make it on time
unsigned long processingTime; // helper variable storing inital time - usefull when dealing with countdown to ensure we make it on time
const int intervalBlink = 200; //frequency of blinking


boolean rotating = false; // helper flag used to debounce rotary encoder
boolean A_set = false; // helper flag used to debounce rotary encoder
boolean B_set = false; // helper flag used to debounce rotary encoder
boolean isLighting = true; // // helper flag used for blinking purposes
boolean changingPositionMode = false; // when true, activates mode in which user can change position on display, blinking also occur here
boolean changingNumberMode = false; // // when true, activates mode in which user can change number on specific segment, blinking also occur here
boolean showingDisplayMode = true; // when true, activates mode where whole number is displayed on display without blinking
boolean countingMode = false; // when true, activates countdown mode where countdown occurs and blinking when finished


void reset() // resetting
{
  sendCommand(0x40); // set auto increment mode
  digitalWrite(TMSTROBE, LOW);
  shiftOut(TMDISPLAY, clock, LSBFIRST, 0xc0);   // set starting address to 0
  for (uint8_t i = 0; i < 16; i++)
  {
    shiftOut(TMDISPLAY, clock, LSBFIRST, 0x00);
  }
  digitalWrite(TMSTROBE, HIGH);
}

void startMode() { //starting, from right to left 0 appears
  sendCommand(0x8f);  // activate and set brightness to max
  for (int i = COUNT - 1; i >= 0; i--) {
    lightDisplay(i, 0);
    delay(100);
  }
}
void sendCommand(uint8_t value)
{
  digitalWrite(TMSTROBE, LOW);
  shiftOut(TMDISPLAY, clock, LSBFIRST, value);
  digitalWrite(TMSTROBE, HIGH);
}

void setup()
{
  pinMode(TMSTROBE, OUTPUT);
  pinMode(clock, OUTPUT);
  pinMode(TMDISPLAY, OUTPUT);
  pinMode(ENCODERA, INPUT);
  pinMode(ENCODERB, INPUT);
  pinMode(ENCODERBUTTON, INPUT);
  attachInterrupt(0, goingClockwise, CHANGE);
  attachInterrupt(1, goingCounterClockwise, CHANGE);
  reset();
  startMode();

}

void goingClockwise() { //handling rotation when going clockwise
  if (rotating) delay(1);
  if (digitalRead(ENCODERA) != A_set) {
    A_set = !A_set;
    if (A_set & !B_set) {
      if (changingPositionMode)
        rotaryPosition(DIR_CW);
      else if (changingNumberMode)
        rotaryNumber(DIR_CW);
      else if (countingMode)
        rotarySpeed(DIR_CW);
    }
  }

  rotating = false;
}
void goingCounterClockwise() { //handling rotation when going counter clockwise
  if (rotating) delay(1);
  if (digitalRead(ENCODERB) != B_set) {
    B_set = !B_set;
    if (B_set & !A_set) {
      if (changingPositionMode)
        rotaryPosition(DIR_CCW);
      else if (changingNumberMode)
        rotaryNumber(DIR_CCW);
      else if (countingMode)
        rotarySpeed(DIR_CCW);
    }
  }
  rotating = false;
}
void lightDisplay(int position, int number) { // light specific number on specific position on display, in dectoHexNumbers last position is 0 (10=0)
  int decToHexNumbers[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66,
                           0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x00
                          };
  digitalWrite(TMSTROBE, LOW);
  shiftOut(TMDISPLAY, clock, LSBFIRST, 0xc0 + position * 2);
  shiftOut(TMDISPLAY, clock, LSBFIRST, decToHexNumbers[number]);
  digitalWrite(TMSTROBE, HIGH);

}
void lightWholeDisplay(boolean on = true) { // light whole display with current numbersOnDisplay, if on = false light whole display with 0
  if (on) {
    for (int i = 0; i <= COUNT - 1; i++) {
      lightDisplay(i, numbersOnDisplay[i]);
    }
  }
  else {
    for (int i = 0; i <= COUNT - 1; i++) {
      lightDisplay(i, 10);
    }
  }

}


void rotaryPosition(char direction) { // change position which blinks on display
  if (direction == DIR_CW) {
    lightDisplay(currentRotaryPosition,
                 numbersOnDisplay[currentRotaryPosition]);

    if (currentRotaryPosition < COUNT - 1) {
      currentRotaryPosition++;
    }
  } else if (direction == DIR_CCW) {
    lightDisplay(currentRotaryPosition,
                 numbersOnDisplay[currentRotaryPosition]);

    if (currentRotaryPosition > 0) {
      currentRotaryPosition --;
    }
  }

}


unsigned long long subtractValue() {  // calculate how much should countdown subtract
  if (speedOption == 1) {
    return 1;
  }
  else {
    return ((MAX / 60) * (speedOption) / 30);
  }

}


void rotarySpeed(char direction) { // change the speed of the countdown
  if (direction == DIR_CW) {

    if (speedOption < 30) {
      speedOption++;
    }
  } else if (direction == DIR_CCW) {

    if (speedOption > 1) {
      speedOption --;
    }
  }
  count();
  processingTime = millis();


}


void blink(boolean wholeDisplay = false) {   // blink segment specified by currentRotaryPosition, if wholeDisplay = true, blink whole display

  unsigned long currentMillisBlink = millis();
  if (wholeDisplay) {
    if (currentMillisBlink - previousMillisBlink > intervalBlink) {
      previousMillisBlink = currentMillisBlink;
      if (isLighting == true) {
        lightWholeDisplay(false);
        isLighting = false;
      }
      else {
        lightWholeDisplay();
        isLighting = true;
      }
    }
  }
  else {
    if (currentMillisBlink - previousMillisBlink > intervalBlink) {
      previousMillisBlink = currentMillisBlink;
      if (isLighting == true) {
        lightDisplay(currentRotaryPosition, 10);
        isLighting = false;
      }
      else {
        lightDisplay(currentRotaryPosition,
                     numbersOnDisplay[currentRotaryPosition]);
        isLighting = true;
      }
    }
  }

}

int rotaryNumber(char direction) { // change number on segment specified by currentRotaryPosition

  if (direction == DIR_CW) {

    if (numbersOnDisplay[currentRotaryPosition] < 9) {
      numbersOnDisplay[currentRotaryPosition]++;
    }
  } else if (direction == DIR_CCW) {

    if (numbersOnDisplay[currentRotaryPosition] > 0) {
      numbersOnDisplay[currentRotaryPosition]--;
    }
  }
}

void count() {  // count whole number on display and assign it to CODE
  long temp = 1;
  CODE = 0;
  for (int i = COUNT - 1; i >= 0; i--) {
    CODE = (numbersOnDisplay[i] * temp) + CODE;
    temp = (10 * temp);
  }
}

void updateNumbersOnDisplay(long number) { //update numbersOnDisplay array with given number
  for (int i = COUNT - 1; i >= 0; i--) {
    numbersOnDisplay[i] = number % 10;
    number = number / 10;
  }
}

void countdown() {  // bomb countdown which first decide on value to subtract, then handles the subtraction, and updates display
  unsigned long long howMuchSubtract = subtractValue();
  currentProcessingTime = millis();
  codeHolder = CODE - (((currentProcessingTime
                         - processingTime) * howMuchSubtract) / 1000);
  if (codeHolder <= 0) {
    CODE = 0;
    codeHolder = 0;
  }
  updateNumbersOnDisplay(codeHolder);
  lightWholeDisplay();

}

void decideOnMode(unsigned long currentMillisButton) { //after button event in loop(), decide where are we, if it was short or long press and which mode should be on
  if ((currentMillisButton - button.counter >= LONGPRESS) && !changingPositionMode &&
      !changingNumberMode && !countingMode) {
    changingPositionMode = true;
    showingDisplayMode = false;

  }
  else if ((currentMillisButton - button.counter >= SHORTPRESS) &&
           !(currentMillisButton - button.counter >= LONGPRESS) && changingPositionMode) {
    changingPositionMode = false;
    changingNumberMode = true;
  }
  else if ((currentMillisButton - button.counter >= SHORTPRESS) &&
           !(currentMillisButton - button.counter >= LONGPRESS) && changingNumberMode) {
    changingPositionMode = true;
    changingNumberMode = false;
  }
  else if ((currentMillisButton - button.counter >= LONGPRESS) &&
           changingPositionMode && !showingDisplayMode) {
    changingPositionMode = false;
    changingNumberMode = false;
    showingDisplayMode = true;

  }
  else if ((currentMillisButton - button.counter >= SHORTPRESS) &&
           !(currentMillisButton - button.counter >= LONGPRESS) && showingDisplayMode
           && !countingMode) {
    showingDisplayMode = false;
    countingMode = true;
    processingTime = millis();



  }
  else if ((currentMillisButton - button.counter >= SHORTPRESS) &&
           !(currentMillisButton - button.counter >= LONGPRESS) && countingMode) {
    countingMode = false;
    showingDisplayMode = true;


  }
}

void loop() // main loop reading button being pressed and
{

  rotating = true;
  button.currentState = digitalRead(button.pin);
  if (button.currentState != button.prevState) {
    delay(button.debounce);
    button.currentState = digitalRead(button.pin);

    if (button.currentState == PRESSED) {
      button.counter = millis();

    }
    if (button.currentState == NOT_PRESSED) {
      unsigned long currentMillisButton = millis();
      decideOnMode(currentMillisButton);

    }
    button.prevState = button.currentState;
  }
  if (changingPositionMode || changingNumberMode) {
    blink();
  }
  if (showingDisplayMode) {
    lightWholeDisplay();
    count();
  }
  if (countingMode) {
    if (CODE > 0) {
      countdown();
    }
    else {
      blink(true);
    }

  }
}



