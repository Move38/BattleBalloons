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

Timer celebrationTimer;
#define CELEBRATION_TIME 1500
byte currentCelebrationType = STANDARD;

Timer popTimer;
#define POP_TIME 150

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
      if (balloonHP > 0 && balloonHP < 6) {
        fortifyLoop();
      }
      celebrateLoop();
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

Timer fortifyLagTimer;
#define FORTIFY_LAG_TIME 300
bool isLagging = false;

void fortifyLoop() {

  //go into fortifying mode when you are alone
  if (isAlone() && !isFortifying) {
    isFortifying = true;
    fortifyState[0] = GIVING;
  }

  if (isFortifying) {
    if (!isAlone()) {//uh-oh, I'm not alone anymore
      if (fortifyLagTimer.isExpired()) {//my lag timer is expired!
        if (isLagging) {//was I waiting for the timer to expire?
          isLagging = false;
          isFortifying = false;
          fortifyState[0] = WAITING;
        } else {//should I start lagging?
          fortifyLagTimer.set(FORTIFY_LAG_TIME);
          isLagging = true;
        }
      }
    }
  }

  //actual do the face stuff
  FOREACH_FACE(f) {
    if (fortifyState[f] == GIVING) {//listen for neighbors in TAKING
      if (!isValueReceivedOnFaceExpired(f)) {
        if (getFortifyState(getLastValueReceivedOnFace(f)) == TAKING) {//Give a health to that neighbor
          fortifyState[f] = WAITING;
          takeDamage();
          isFortifying = false;
        }
      }
    } else if (fortifyState[f] == WAITING) {//listen for neighbors in GIVING
      if (!isValueReceivedOnFaceExpired(f)) {
        if (getFortifyState(getLastValueReceivedOnFace(f)) == GIVING) {
          fortifyState[f] = TAKING;
        }
      }
    } else if (fortifyState[f] == TAKING) {//listen for neighbors in WAITING
      if (!isValueReceivedOnFaceExpired(f)) {
        if (getFortifyState(getLastValueReceivedOnFace(f)) == WAITING) {//take the HP
          fortifyState[f] = WAITING;
          gainHP();
        }
      }
    }
  }
}

void celebrateLoop() {
  if (celebrationState == INERT) {
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
        if (getCelebrationState(getLastValueReceivedOnFace(f)) == CROWN || getCelebrationState(getLastValueReceivedOnFace(f)) == TRAP) {
          celebrationState = getCelebrationState(getLastValueReceivedOnFace(f));
          beginCelebration(celebrationState);
        }
      }
    }
  } else if (celebrationState == RESOLVE) {
    bool foundUnawareNeighbor = false;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
        if (getCelebrationState(getLastValueReceivedOnFace(f)) == CROWN || getCelebrationState(getLastValueReceivedOnFace(f)) == TRAP) {
          foundUnawareNeighbor = true;
        }
      }
    }

    if (!foundUnawareNeighbor) {
      celebrationState = INERT;
    }

  } else {//this covers both CROWN and TRAP
    bool foundUnawareNeighbor = false;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
        if (getCelebrationState(getLastValueReceivedOnFace(f)) == INERT) {
          foundUnawareNeighbor = true;
        }
      }
    }

    if (!foundUnawareNeighbor) {
      celebrationState = RESOLVE;
    }
  }
}

void beginCelebration(byte type) {
  currentCelebrationType = type;
  celebrationTimer.set(CELEBRATION_TIME);
}

void takeDamage() {
  balloonHP--;
  if (balloonHP == 0) {
    popTimer.set(POP_TIME);
    if (balloonType == WIN) {
      celebrationState = CROWN;
      beginCelebration(CROWN);
    } else if (balloonType == LOSE) {
      celebrationState = TRAP;
      beginCelebration(TRAP);
    }
  }
}

void gainHP() {
  balloonHP++;
  if (balloonHP > 6) {
    balloonHP = 6;
  }
}

void resetLoop() {
  //check to make sure everyone got the messsage
  gameMode = SETUP;

  FOREACH_FACE(f) {
    fortifyState[f] = WAITING;//quickly reset that whole mess

    //check if it's safe to change mode fully
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
  if (balloonHP == 0) {
    switch (balloonType) {
      case STANDARD:
        setColor(makeColorHSB(balloonHues[balloonSize], 255, 100));
        break;
      case WIN:
        setColor(dim(YELLOW, 100));
        break;
      case LOSE:
        setColor(dim(MAGENTA, 100));
        break;
    }
  } else {
    FOREACH_FACE(f) {
      if (f < balloonHP) {
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), f);
      }
    }
  }

  if (isFortifying) {
    setColorOnFace(WHITE, 0);
  }

  if (!popTimer.isExpired()) {
    byte currentProgress = map(popTimer.getRemaining(), 0, POP_TIME, 0, 3);
    switch (currentProgress) {
      case 3:
        setColor(makeColorHSB(balloonHues[balloonSize], 255, 255));
        break;
      case 2:
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 0);
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 1);
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 5);
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 2);
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 4);
        break;
      case 1:
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 0);
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 1);
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 5);
        break;
      case 0:
        setColorOnFace(makeColorHSB(balloonHues[balloonSize], 255, 255), 0);
        break;
    }
  }

  if (!celebrationTimer.isExpired()) {//ooh, the celebration timer is running!
    byte randomPingFace = random(5);
    byte otherRandomPingFace = (randomPingFace + random(4)) % 6;

    byte pingBrightness = map(celebrationTimer.getRemaining(), 0, CELEBRATION_TIME, 100, 255);

    if (currentCelebrationType == CROWN) {
      setColorOnFace(dim(YELLOW, pingBrightness), randomPingFace);
      setColorOnFace(dim(YELLOW, pingBrightness), otherRandomPingFace);
    } else {
      setColorOnFace(dim(MAGENTA, pingBrightness), randomPingFace);
      setColorOnFace(dim(MAGENTA, pingBrightness), otherRandomPingFace);
    }

  }
}
