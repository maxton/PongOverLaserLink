#define sclk 13  // SainSmart: SCL
#define mosi 11  // SainSmart: SDA
#define cs   10  // SainSmart: CS
#define dc   9   // SainSmart: RS/DC
#define rst  8   // SainSmart: RES
#define CONNECTION_LED 2

#define MASTER

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

struct GameState {
  byte leftY;
  byte rightY;
  byte ballX;
  byte ballY;
  signed char ballVX;
  signed char ballVY;
  byte scoreLeft;
  byte scoreRight;
};

const int PADDLE_HEIGHT = 32;
const int PADDLE_MID = 16;
const int PADDLE_WIDTH = 8;
const int BALL_SIZE = 8;
const int BALL_RADIUS = 4;
const int LEFTMOST = 8;
const int RIGHTMOST = 248;

const int BEEP_PIN = 3;



GameState state;
GameState last_state;
int beep_timer;

Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

void setup() {
  Serial.begin(3600);
  state.leftY = 100;
  state.rightY = 100;
  state.ballX = 128;
  state.ballY = 128;
  state.ballVX = 2;
  state.ballVY = 0;
  state.scoreLeft = 0;
  state.scoreRight = 0;
  pinMode(3, OUTPUT);
  pinMode(CONNECTION_LED, OUTPUT);
  tft.initR(INITR_GREENTAB);
  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF);
  tft.setCursor(16,140);
  tft.print("0");
  tft.setCursor(100, 140);
  tft.print("0");
}


void checkForMasterPacket() {
  if(Serial.available() > 0) {
    while(Serial.available() >= 11) {
      if(Serial.peek() == 'a') readMasterPacket();
      else Serial.read();
    }
  } else {
  }
}

void checkForSlavePacket() {
  if(Serial.available() > 0) {
    while(Serial.available() >= 4) {
      if(Serial.peek() == 'a') readSlavePacket();
      else Serial.read();
    }
  } else {
  }
}

void flushBuffer() {
  while(Serial.available() > 0) {
    Serial.read();
  }
}

void readMasterPacket() {
  digitalWrite(CONNECTION_LED, HIGH);
  byte packet[8];
  char start = Serial.read();
  byte checksum = 0;
  if(start == 'a') {
    for(int i = 0; i < 8; i++) {
      packet[i] = Serial.read();
      checksum += packet[i];
    }
    if(checksum == Serial.read()) {
      if('z' == Serial.read()) {
        state.leftY = packet[0];
        state.rightY = packet[1];
        state.ballX = packet[2];
        state.ballY = packet[3];
        state.ballVX = packet[4];
        state.ballVY = packet[5];
        state.scoreLeft = packet[6];
        state.scoreRight = packet[7];
      }
    }
  } else {
  }
  flushBuffer();
}

void readSlavePacket() {
  digitalWrite(CONNECTION_LED, HIGH);
  if(Serial.read() == 'a') {
    byte newRight = Serial.read();
    if(newRight == Serial.read()) {
      state.rightY = newRight;
    }
  }
  flushBuffer();
}

void checkInput() {
#ifdef MASTER
  state.leftY = analogRead(0) / 4;
#endif
#ifndef MASTER
  state.rightY = analogRead(0) / 4;
#endif
}

void checkBeep() {
  if(beep_timer-- > 0){
    digitalWrite(BEEP_PIN, HIGH);
  } else {
    digitalWrite(BEEP_PIN, LOW);
  }
}

void updateState() {
  state.ballX += state.ballVX;
  state.ballY += state.ballVY;
  
  if(state.ballX <= LEFTMOST){
    int tmp_max = PADDLE_HEIGHT + state.leftY;
    int paddle_ctr = PADDLE_MID + state.leftY;
    int ball_ctr = state.ballY + BALL_RADIUS;
    if(ball_ctr > state.leftY && ball_ctr < tmp_max) {
      state.ballVX = -state.ballVX;
      state.ballVY = (ball_ctr - paddle_ctr) / 2;
    }
    else {
      state.scoreRight++;
      state.ballX = 128;
      state.ballY = 128;
      state.ballVY = 0;
      state.ballVX = -2;
    }
  }
  else if(state.ballX >= RIGHTMOST){
    int tmp_max = PADDLE_HEIGHT + state.rightY;
    int paddle_ctr = PADDLE_MID + state.rightY;
    int ball_ctr = state.ballY + BALL_RADIUS;
    if(ball_ctr > state.rightY && ball_ctr < tmp_max) {
      state.ballVX = -state.ballVX;
      state.ballVY = (ball_ctr - paddle_ctr) / 2;
    } else {
      state.scoreLeft++;
      state.ballX = 128;
      state.ballY = 128;
      state.ballVY = 0;
      state.ballVX = 2;
    }
  }
  if(state.ballY < BALL_RADIUS) {
    state.ballVY = 2;
  } else if(state.ballY > 250) {
    state.ballVY = -2;
  }
}

void sendMasterPacket() {
  byte checksum 
    = state.leftY
    + state.rightY
    + state.ballX
    + state.ballY
    + state.ballVX
    + state.ballVY
    + state.scoreLeft
    + state.scoreRight;
    
  Serial.write('a');
  Serial.write(state.leftY);
  Serial.write(state.rightY);
  Serial.write(state.ballX);
  Serial.write(state.ballY);
  Serial.write(state.ballVX);
  Serial.write(state.ballVY);
  Serial.write(state.scoreLeft);
  Serial.write(state.scoreRight);
  Serial.write(checksum);
  Serial.write('z');
}

void sendSlavePacket() {
  Serial.write('a');
  Serial.write(state.rightY); 
  Serial.write(state.rightY); //checksum
  Serial.write('z');
  for(int i = 0; i < 7; i++){
    Serial.write(0);
  }
}

const int SCREEN_W = 160;
const int SCREEN_H = 128;
void overWrite() {
  tft.fillRect(0, last_state.leftY / 2, 4, 16, 0x0000);
  tft.fillRect(124, last_state.rightY / 2, 4, 16, 0x0000);
  tft.fillRect(last_state.ballX / 2, last_state.ballY / 2, 4, 4, 0x0000);
}
void draw() {
  tft.fillRect(0, state.leftY / 2, 4, 16, 0xFFFF);
  tft.fillRect(124, state.rightY / 2, 4, 16, 0xFFFF);
  tft.fillRect(state.ballX / 2, state.ballY / 2, 4, 4, 0xFFFF);
  if(last_state.scoreLeft != state.scoreLeft){
    tft.fillRect(16, 140, 50, 30, 0x0000);
    char buff[3];
    tft.setCursor(16,140);
    tft.print(itoa(state.scoreLeft, buff, 10));
  }
  if(last_state.scoreRight != state.scoreRight){
    tft.fillRect(100, 140, 50, 30, 0x0000);
    char buff[3];
    tft.setCursor(100, 140);
    tft.print(itoa(state.scoreRight, buff, 10));
  }
}


void loop() {
  digitalWrite(CONNECTION_LED, LOW);
#ifndef MASTER
  checkForMasterPacket();
#endif
#ifdef MASTER
  checkForSlavePacket();
#endif
  checkInput();
  checkBeep();
  updateState();
#ifndef MASTER
  sendSlavePacket();
#endif
#ifdef MASTER
  sendMasterPacket();
#endif
  overWrite();
  draw();
  last_state = state;
}
