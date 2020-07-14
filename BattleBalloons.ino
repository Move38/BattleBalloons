enum gameModes {SETUP, PLAY, RESET};
byte gameMode = SETUP;

enum fortifyStates {WAITING, GIVING, TAKING};
byte fortifyState[6] = {WAITING, WAITING, WAITING, WAITING, WAITING, WAITING};
bool isFortifying = false;

enum balloonSizes {SMALL, MEDIUM, LARGE};
byte balloonHues[3] = {80, 120, 15};
byte balloonSize = SMALL;
byte balloonHP = 6;

enum balloonTypes {STANDARD, WIN, LOSE};
byte balloonType = STANDARD;

enum celebrationStates {INERT, CROWN, TRAP, RESOLVE};
byte celebrationState = INERT;

void setup() {
  randomize();
}

void loop() {
  //loops
  switch (gameMode) {
    case SETUP:
      setupLoop();
      break;
    case PLAY:
      playLoop();
      fortifyLoop();
      break;
    case RESET:
      resetLoop();
      break;
  }

  //communication
  FOREACH_FACE(f) {
    byte sendData = (gameMode << 4) + (fortifyState[f] << 2) + (celebrationState);
    setValueSentOnFace(sendData, f);
  }

  //display
  switch (gameMode) {
    case SETUP:
    case RESET:
      setupDisplay();
      break;
    case PLAY:
      playDisplay();
      break;
  }

  //dump button presses
  buttonSingleClicked();
  buttonDoubleClicked();
  buttonMultiClicked();
  buttonLongPressed();
}

void setupLoop() {

  //listen for clicks to change our size
  if (buttonSingleClicked()) {
    balloonSize = (balloonSize + 1) % 3;//increment balloon size (0-1-2)
  }

  //listen for long-presses to make us special
  if (buttonLongPressed()) {
    balloonType = (balloonType + 1) % 3;//increment balloon type (0-1-2)
  }

  //begin game
  if (buttonMultiClicked()) {
    if (buttonClickCount() == 3) {
      gameMode = PLAY;
      beginGame();
    }
  }

  //look for neighbors pushing me into PLAY
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      if (getGameMode(getLastValueReceivedOnFace(f)) == PLAY) {
        gameMode = PLAY;
        beginGame();
      }
    }
  }
}

void beginGame() {
  switch (balloonSize) {
    case SMALL:
      balloonHP = 2 + random(1);
      break;
    case MEDIUM:
      balloonHP = 3 + random(2);
      break;
    case LARGE:
      balloonHP = 5 + random(1);
      break;
  }
}

void playLoop() {

  //take damage on click
  if (buttonSingleClicked()) {
    takeDamage();
  }

  //go into fortifying mode when you are alone
  if (isAlone) {
    isFortifying = true;
    fortifyState[0] = GIVING;
  }

  if (isFortifying) {
    fortifyLoop();
  }

  //end game manually
  if (buttonMultiClicked()) {
    if (buttonClickCount() == 3) {
      gameMode = RESET;
    }
  }

  //look for neighbors pushing me into RESET
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      if (getGameMode(getLastValueReceivedOnFace(f)) == RESET) {
        gameMode = RESET;
      }
    }
  }
}

void fortifyLoop() {

}

void takeDamage() {
  balloonHP--;
  if (balloonHP == 0) {
    //DEATH
  }
}

void resetLoop() {
  //check to make sure everyone got the messsage
  gameMode = SETUP;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      if (getGameMode(getLastValueReceivedOnFace(f)) == PLAY) {//uh oh, this person doesn't know we're reseting
        gameMode = RESET;
      }
    }
  }
}

//Signal Parsing
byte getGameMode(byte data) {
  return ((data >> 4) & 3);//returns bits [A] [B]
}


byte getFortifyState(byte data) {
  return ((data >> 2) & 3);//returns bits [C] [D]
}


byte getCelebrationState(byte data) {
  return (data & 3);//returns bits [E] [F]
}

#define FLASH_INTERVAL 1500

void setupDisplay() {

  //set the little flashy timing
  byte brightness = 255 - map(millis() % FLASH_INTERVAL, 0, FLASH_INTERVAL, 0, 255);

  //set background color
  switch (balloonType) {
    case STANDARD:
      setColor(makeColorHSB(balloonHues[balloonSize], 200, 50));//background color
      break;
    case CROWN:
      setColor(dim(YELLOW, brightness));//background color
      break;
    case TRAP:
      setColor(dim(MAGENTA, brightness));//background color
      break;
  }

  //set specific colors
  setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 0);
  if (balloonSize == MEDIUM) {
    setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 3);
  } else if (balloonSize == LARGE) {
    setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 2);
    setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 4);
  }
}

void playDisplay() {
  setColor(OFF);
  //display the balloon HP
  FOREACH_FACE(f) {
    if (f < balloonHP) {
      setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), f);
    }
  }
}
