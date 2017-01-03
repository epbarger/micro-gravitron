#include <Arduboy2.h>
#include <ArduboyTones.h>

#define ENEMY_BUFFER_SIZE 30

#define MENU 0
#define PLAYING 1

struct Player {
  public:
    float x;
    float y;
    bool gravity;
};

struct Enemy {
  public:
    float x;
    byte y;
    bool dead;
    bool dir; // 0 from left, 1 from right
    byte arrowIndex;
    byte spindex;
};

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);
byte gameState = MENU;
Player player;
unsigned long startTime = 0;
unsigned long currentTime = 0;
unsigned long recordTime = 0;
char timeStr[8];
char recordStr[8];
Enemy enemies[ENEMY_BUFFER_SIZE];
byte enemySpawnY[6] = {13, 13 + 2 + 7, 13 + (2*2) + (7*2), 13 + (2*3) + (7*3), 13 + (2*4) + (7*4), 13 + (2*5) + (7*5)};
bool reset = false;
bool menuString = false;
byte timeUntilLeftSpawn = 0;
byte timeUntilRightSpawn = 0;
bool arrows[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte menuIndex = 0;
bool menuLock = false;
bool menuButtonLock = false;
bool spawnSide = 0;
int spawnSideCounter = 0;

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);

  byte eepromValid = 0;
  EEPROM.get(300, eepromValid);
  if (eepromValid != 111){
    EEPROM.put(300, 111);
    EEPROM.put(320, 0);
    EEPROM.put(321, 0);
    EEPROM.put(322, 0);
    EEPROM.put(323, 0);
  } else {
    EEPROM.get(320, recordTime);
  }
  
  player.x = 64;
  player.y = 32;
  player.gravity = 1;

  nullOutEnemies();

  millisToString(recordTime, recordStr, 8);

  arduboy.initRandomSeed();
}

void loop() {
  if (!(arduboy.nextFrame())) return;
  updateState();
  drawState();
}

//////////////////////////////////

void updateState(){
  switch(gameState){
    case MENU:
      updatePlayerY();
      menuHandler();
      break;
    case PLAYING:
      updatePlayer();
      updateTime();
      updateEnemies();
      updateArrows();
      break;
  }
}

void drawState(){
  arduboy.clear();
  switch(gameState){
    case MENU:
      drawMenu();
      drawField();
      drawPlayer();
      break;
    case PLAYING:
      drawHud();
      drawField();
      drawEnemies();
      drawArrows();
      drawPlayer();
      break;
  }
//  arduboy.setCursor(100,55);
//  arduboy.print(arduboy.cpuLoad());
  arduboy.display();
}

void drawArrows(){
  for(int i = 0; i < 6; i++){
    if (arrows[i]){
      arduboy.fillTriangle(0, enemySpawnY[i] - 2, 2, enemySpawnY[i], 0, enemySpawnY[i] + 2, WHITE);
    }
  }

  for(int i = 6; i < 12; i++){
    if (arrows[i]){
      arduboy.fillTriangle(127, enemySpawnY[abs(6-i)] - 2, 127-2, enemySpawnY[abs(6-i)], 127, enemySpawnY[abs(6-i)] + 2, WHITE);
    }  
  }
}

void updateArrows(){
  for (int i = 0; i < 12; i++){
    arrows[i] = false;
  }
  for(int i = 0; i < ENEMY_BUFFER_SIZE; i++){
    if (enemies[i].dead == 0){
      if (((enemies[i].x < 0) && (enemies[i].x >= -70) && enemies[i].dir == 0) || 
          ((enemies[i].x > 127) && (enemies[i].x <= 127+70) && enemies[i].dir == 1)) {
        arrows[enemies[i].arrowIndex] = true;
      }
    }
  }
}

void drawHud(){
  arduboy.setCursor(0,0);
  arduboy.print(timeStr);
  arduboy.setCursor(126 - 7*(strlen(recordStr)-1),0);
  arduboy.print(recordStr);
}

void drawMenu(){
  if (arduboy.everyXFrames(1)){
    arduboy.setCursor(50,27);
    arduboy.print(F("MICRO"));
    arduboy.setCursor(37,36);
    arduboy.setTextSize(1);
    arduboy.print(F("GRAVITRON"));
    arduboy.setTextSize(1);
  }

  arduboy.setCursor(126 - 7*(strlen(recordStr)-1),0);
  arduboy.print(recordStr);

  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);
  if (menuIndex == 0){
    arduboy.fillRect(0,0,6*4+1,9,WHITE); 
    arduboy.setTextColor(BLACK);
    arduboy.setTextBackground(WHITE);
  } 
  arduboy.setCursor(1,1);
  arduboy.print(F("PLAY"));

  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);
  if (menuIndex == 1){
    arduboy.fillRect(6*4+2,0,6*5+1,9,WHITE); 
    arduboy.setTextColor(BLACK);
    arduboy.setTextBackground(WHITE);
  }
  arduboy.setCursor(6*4+3, 1);
  arduboy.print(F("SOUND"));

  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);
}

void menuHandler(){
  if (arduboy.audio.enabled){
    arduboy.setCursor(0,32);
    arduboy.print("enabled");
  }
  if (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)){
    if (!menuButtonLock){
      if (menuIndex == 0){
        gameState = PLAYING;
        // arduboy.audio.saveOnOff();
        resetGame(false);
      } else if (menuIndex == 1){
        arduboy.setCursor(0,32);
        arduboy.print("PRESSED");
        if (arduboy.audio.enabled){
          arduboy.audio.off();
        } else {
          arduboy.audio.on();
        }
      }
      menuButtonLock = true;
    }
  } else {
    menuButtonLock = false;
  }

  if (arduboy.pressed(LEFT_BUTTON) || arduboy.pressed(RIGHT_BUTTON)){
    if (!menuLock){
      menuLock = true;
      if (arduboy.pressed(LEFT_BUTTON)){
        menuIndex = (menuIndex - 1) % 2;
      } else {
        menuIndex = (menuIndex + 1) % 2;
      }
    }
  } else {
    menuLock = false;
  }
}

void drawField(){
  arduboy.drawFastHLine(0,8,128,WHITE);
  arduboy.drawFastHLine(0,63,128,WHITE);

  if ((player.y - 1) <= 10){
    arduboy.drawFastHLine(0,8,128,BLACK);
  } else if ((player.y + 1) >= 63){
    arduboy.drawFastHLine(0,63,128,BLACK);
  }
}

void drawPlayer(){
  arduboy.fillRect(player.x - 1, player.y - 2, 3, 5, WHITE);
  if (player.gravity){
    arduboy.drawPixel(player.x, player.y + 2, BLACK);
  } else {
    arduboy.drawPixel(player.x, player.y - 2, BLACK);
  }
}

void updateTime(){
  currentTime = millis();
  millisToString(currentTime - startTime, timeStr, 8);
}

void resetGame(bool resetPosition){
  nullOutEnemies();
  for (int i = 0; i < 12; i++){ arrows[i] = false; }
  startTime = millis();
  if (resetPosition){
    player.x = 64;
    player.y = 32;
    player.gravity = 1;
  }
  arduboy.initRandomSeed();
  timeUntilLeftSpawn = random(30, 80);
  timeUntilRightSpawn = random(30, 80);
}

void updateEnemy(Enemy* enemy){
  if (enemy->dir){
    enemy->x -= 1.5;
  } else {
    enemy->x += 1.5;
  }

  enemy->spindex = (enemy->spindex + 1) % 120;
  
  if (((enemy->dir == 0) && (enemy->x > 127)) || ((enemy->dir == 1) && (enemy->x < 0))){
    enemy->dead = true;
  }
}

void drawEnemies(){
  for(int i = 0; i < ENEMY_BUFFER_SIZE; i++){
    if ((enemies[i].dead == false)){
      if ((enemies[i].spindex < 30) || ((enemies[i].spindex > 60) && (enemies[i].spindex < 90))){
        arduboy.drawRect(enemies[i].x-2, enemies[i].y-2, 5, 5, WHITE);
      } else {
        arduboy.drawLine(enemies[i].x-3, enemies[i].y, enemies[i].x, enemies[i].y-3, WHITE); // top left
        arduboy.drawLine(enemies[i].x, enemies[i].y-3, enemies[i].x + 3, enemies[i].y, WHITE); // top right
        arduboy.drawLine(enemies[i].x-3, enemies[i].y, enemies[i].x, enemies[i].y+3, WHITE); // bottom left
        arduboy.drawLine(enemies[i].x, enemies[i].y+3, enemies[i].x + 3, enemies[i].y, WHITE); // bottom right
      }
    }
  }
}

void updateEnemies(){
//  if (arduboy.everyXFrames(160)){
//    // generateDiamond(random(0,2));
//    generateGates(random(0,2));
//  }
//  
  for(int i = 0; i < ENEMY_BUFFER_SIZE; i++){
    if (enemies[i].dead == false){
      updateEnemy(&enemies[i]);
    }
  }

  if (spawnSide == 0 && timeUntilLeftSpawn <= 0){
    timeUntilLeftSpawn = spawnRandomEnemy(0);
  }
  timeUntilLeftSpawn--;

  if (spawnSide == 1 && timeUntilRightSpawn <= 0){
    timeUntilRightSpawn = spawnRandomEnemy(1);
  }
  timeUntilRightSpawn--;

  if (spawnSideCounter == 0){
    spawnSideCounter = 300;
    spawnSide = !spawnSide;
    timeUntilLeftSpawn = 30;
    timeUntilRightSpawn = 30;
  } else {
    spawnSideCounter--;
  }
}

byte spawnRandomEnemy(bool side){
  switch(random(0, 6)){
    case 0:
      generateGates(side);
      return 180;
    case 1:
      generateDiamond(side);
      return 120;
    case 2:
      generateSixSingleUp(side);
      return 250;
    case 3:
      generateSixSingleDown(side);
      return 250;
    case 4:
      middleSnake(side);
      return 120;
    case 5:
      generateTopBottomGates(side);
      return 180;
  }
}

void updatePlayerY(){
  if (player.gravity){
    player.y += 1.5;
  } else {
    player.y -= 1.5;
  }

  if (player.y >= (63)){
    player.gravity = 0;
    sound.tone(620, 100);
  } else if (player.y <= (8 + 2)){
    player.gravity = 1;
    sound.tone(620, 100);
  }
}

void updatePlayer(){
  if (reset == true){
    if ((currentTime - startTime) > recordTime){
      recordTime = currentTime - startTime;
      EEPROM.put(320, recordTime);
      millisToString(recordTime, recordStr, 8);
    }
    reset = false;
    sound.tone(200,250);
    delay(1000);
    resetGame(true);
  }
  
  updatePlayerY();

  if (arduboy.pressed(LEFT_BUTTON)){
    player.x -= 1.5;
    if (player.x < 0) {
      player.x = 127 - player.x;
    }
  } else if (arduboy.pressed(RIGHT_BUTTON)){
    player.x += 1.5;
    if (player.x > 127){
      player.x = 0 + player.x - 127;
    }
  }

  for(int i = 0; i < ENEMY_BUFFER_SIZE; i++){
    if (enemies[i].dead == false){
      if ((pow(player.x - enemies[i].x, 2) + pow(player.y - enemies[i].y, 2)) <= 25){
        reset = true;
      }
    }
  }

//  if (arduboy.pressed(A_BUTTON)){
//    reset = true;
//  }
}

void millisToString(unsigned long m, char* output, byte outputSize){
  m = m / 10;
  byte mills = m % 100;
  byte seconds = (m / 100) % 60;
  byte minutes = ((m / 100) / 60) % 60;

  char sbuf[outputSize];
  if (minutes < 1){
    sprintf(sbuf, "%d:%02d", seconds, mills);
  } else {
    sprintf(sbuf, "%d:%02d:%02d", minutes, seconds, mills);
  }
  strncpy(output, sbuf, outputSize);
}

void initEnemy(float x, byte spawn, bool direct){
  Enemy newEnemy;
  newEnemy.x = x;
  newEnemy.y = enemySpawnY[spawn]; // y;
  newEnemy.arrowIndex = spawn + (6*direct);
  newEnemy.dead = false;
  newEnemy.dir = direct;
  newEnemy.spindex = 0;
  for(int i = 0; i < ENEMY_BUFFER_SIZE; i++){
    if (enemies[i].dead == true) {
      enemies[i] = newEnemy;
      break;
    }
  }
}

void generateRandomSingle(bool direct){
  if (direct){
    initEnemy(127 + 20.0, random(0, 6), direct);
  } else {
    initEnemy(-20.0, random(0, 6), direct); 
  }
}

void generateDiamond(bool direct){
  if (direct){
    initEnemy(127 + 79.0, 1, direct);
    initEnemy(127 + 70.0, 2, direct);
    initEnemy(127 + 70.0, 3, direct);
    initEnemy(127 + 79.0, 4, direct);
    initEnemy(127 + 88.0, 2, direct);
    initEnemy(127 + 88.0, 3, direct);  
  } else {
    initEnemy(-79.0, 1, direct);
    initEnemy(-70.0, 2, direct);
    initEnemy(-70.0, 3, direct);
    initEnemy(-79.0, 4, direct);
    initEnemy(-88.0, 2, direct);
    initEnemy(-88.0, 3, direct);
  }
}

void middleSnake(bool direct){
  if (direct){
    initEnemy(127 + 20 + 50, 2, direct);
    initEnemy(127 + 50+ 50, 2, direct);
    initEnemy(127 + 80+ 50, 3, direct);
    initEnemy(127 + 110+ 50, 3, direct);
  } else {
    initEnemy(-20-50, 2, direct);
    initEnemy(-50-50, 2, direct);
    initEnemy(-80-50, 3, direct);
    initEnemy(-110-50, 3, direct);
  }
}

void generateSixSingleUp(bool direct){
  if (direct) {
    for (int i = 0; i < 6; i++){
      initEnemy(127 + 70.0 + 50*i, 5-i, direct); 
    }
  } else {
    for (int i = 0; i < 6; i++){
      initEnemy(-70.0 - 50*i, 5-i, direct); 
    }  
  }
}

void generateSixSingleDown(bool direct){
  if (direct) {
    for (int i = 0; i < 6; i++){
      initEnemy(127 + 70.0 + 50*i, i, direct); 
    }
  } else {
    for (int i = 0; i < 6; i++){
      initEnemy(-70.0 - 50*i, i, direct); 
    }  
  }
}

void generateGates(bool direct){
  if (direct){
    initEnemy(127+20+50, 1, direct);
    initEnemy(127+20+50, 4, direct);
    initEnemy(127+70+50, 0, direct);
    initEnemy(127+70+50, 5, direct);
    initEnemy(127+120+50, 2, direct);
    initEnemy(127+120+50, 3, direct);
    initEnemy(127+170+50, 0, direct);
    initEnemy(127+175+50, 1, direct);
    initEnemy(127+190+50, 4, direct);
    initEnemy(127+195+50, 5, direct);
  } else {
    initEnemy(-20-50, 1, direct);
    initEnemy(-20-50, 4, direct);
    initEnemy(-70-50, 0, direct);
    initEnemy(-70-50, 5, direct);
    initEnemy(-120-50, 2, direct);
    initEnemy(-120-50, 3, direct);
    initEnemy(-170-50, 0, direct);
    initEnemy(-175-50, 1, direct);
    initEnemy(-190-50, 4, direct);
    initEnemy(-195-50, 5, direct);
  }
}


void generateTopBottomGates(bool direct){
  if (direct) {
    initEnemy(127+70,0,direct);
    initEnemy(127+70,1,direct);
    initEnemy(127+70,2,direct);
    initEnemy(127+120,3,direct);
    initEnemy(127+120,4,direct);
    initEnemy(127+120,5,direct);
  } else {
    initEnemy(-70,0,direct);
    initEnemy(-70,1,direct);
    initEnemy(-70,2,direct);
    initEnemy(-120,3,direct);
    initEnemy(-120,4,direct);
    initEnemy(-120,5,direct);
  }
}


void nullOutEnemies(){
  for(int i = 0; i < ENEMY_BUFFER_SIZE; i++){
    Enemy tmpEnemy;
    tmpEnemy.dead = true;
    enemies[i] = tmpEnemy;
  }
}


