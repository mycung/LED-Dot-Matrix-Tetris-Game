//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Global Definitions Begin++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include <SoftwareSerial.h>    
#include "LedControl.h"
#include <binary.h>     // Bibliothek einbinden, um Binärwerte ausschreiben zu können
#include <LiquidCrystal.h>
#include "pitches.h"

LiquidCrystal lcdisplay(8, 9, 5, 4, 3, 2);    // LCD-Display für die Punktestandausgabe definieren
const int pin_key = 7;    //Digitale Eingang für den Joystick- Schalter
const int pin_x = 0;      //Analoger Eingang A0
const int pin_y = 1;      //Analoger Eingang A1

LedControl lc = LedControl(12,10,11,4);       // Definition der Input-Pins ( DIN, CLK, CS )und Anzahl der MAX7219-Module. Da diese gebrückt werden, brauchen diese nur jeweils einen Anschluss am Arduino. 
                                              // Über lc können nun die Funktionen der LED-Control Bibliothek abgerufen werden 
                                              // Variable vom Typ LedControl, um die Funktionen der Bibliothek aufrufen zu können
 
//Spielfeldgrößendefinition            
#define  FIELD_WIDTH       8    //Konstante Feldbreite, Breite LED-Matrix MAX7219
#define  FIELD_HEIGHT      24   //Konstante Feldhöhe, Spielfeld geht über die unteren 3 LED-Matrizen --> 8x3 = 24 LEDs / Reihen hoch
#define  ORIENTATION_HORIZONTAL 
#define  NUM_PIXELS    FIELD_WIDTH*FIELD_HEIGHT   // Anzahl der Spielfeld-LEDs

//Richtungsoptionen definieren
#define  DIR_UP    1
#define  DIR_DOWN  2
#define  DIR_LEFT  3
#define  DIR_RIGHT 4
#define  DIR_PUSH  5  // Drücken des Joysticks als „Direction im Spiel( vielleicht Pausieren? )

//Verfügbare Richtungen / Eingabemöglichkeiten des Joysticks
#define  BTN_NONE  0
#define  BTN_UP    1
#define  BTN_DOWN  2
#define  BTN_LEFT  3
#define  BTN_RIGHT  4
#define  BTN_PUSH  5

uint8_t curControl = BTN_NONE;


#define  MAX_BRICK_SIZE    4
#define  BRICKOFFSET       -1       //Y-Offset für neue Bausteine, wenn diese oben platziert werden

#define  INIT_SPEED        1000     //Zeit nachdem das neue Bauteil erscheint
#define  SPEED_STEP        200      //Geschwindigkeitserhöhung pro Level 
#define  LEVELUP           2        //Anzahl der Reihen für ein LVL-Up

//Struktur für das Spielfeld
 struct Field{
    uint8_t pix[FIELD_WIDTH][FIELD_HEIGHT+1];//Make field one larger so that collision detection with bottom of field can be done in a uniform way
 };
  
//Globale Variable Spielfeld 
Field field;


int notenr = 0; 
int tetrismelody[] = {
  NOTE_E7,NOTE_B6,NOTE_C7,NOTE_D7,NOTE_C7,NOTE_B6,NOTE_A6,
  NOTE_A6,NOTE_C7,NOTE_E7,NOTE_D7,NOTE_C7,NOTE_B6,
  NOTE_C7,NOTE_D7,NOTE_E7,NOTE_C7,NOTE_A6,NOTE_A6,NOTE_A6,NOTE_B6,NOTE_C7,
  NOTE_D7,NOTE_F7,NOTE_A7,NOTE_G7,NOTE_F7,NOTE_E7,
  NOTE_C7,NOTE_E7,NOTE_D7,NOTE_C7,NOTE_B7,
  NOTE_B7,NOTE_C7,NOTE_D7,NOTE_E7,NOTE_C7,NOTE_A6,NOTE_A6,NOTE_A6
};

int melody []{
  NOTE_A6,NOTE_B6,NOTE_C7,NOTE_D7,NOTE_E7,NOTE_F7,NOTE_G7,NOTE_A7
};

int melodyduration[] = {
  4,8,8,4,8,8,4,
  8,8,4,8,8,4,
  8,4,4,4,4,8,8,8,8,
  4,8,4,8,8,4,
  8,4,8,8,4,
  8,8,4,4,4,4,4,2
};


//Structure to represent active brick on screen
struct Brick{
    boolean enabled;//Brick is disabled when it has landed
    int xpos,ypos;
    int yOffset;//Y-offset to use when placing brick at top of field
    uint8_t siz;
    uint8_t pix[MAX_BRICK_SIZE][MAX_BRICK_SIZE];
  };

  //globale Vaiable, aktiver Baustein, versch. Funktionen müssen auf diesen zugreifen können, sodass dieser global definiert werden muss. 
  Brick activeBrick;
  
  //Struct to contain the different choices of blocks
  struct AbstractBrick{      //Definition der Struktur der Bausteine (allgemeine Größen)
    int yOffset;             //Y-Offset den der Baustein hat, wenn er oben platziert wird
    uint8_t siz;             //Größe des Bausteins, (Wichtig für die Rotation des Bausteins) abhängig davon wird es in einem anderen Punkt rotiert
    uint8_t pix[MAX_BRICK_SIZE][MAX_BRICK_SIZE];        //Definition des Bausteins
  };

  //Brick "library"
  AbstractBrick brickLib[7] = {   //die sieben tetrominos 
    {
        1,//yoffset when adding brick to field
        4,
        { {0,0,0,0},
          {0,1,1,0},
          {0,1,1,0},
          {0,0,0,0}
        }
    },
    {
        0,
        4,
        { {0,1,0,0},
          {0,1,0,0},
          {0,1,0,0},
          {0,1,0,0}
        }
    },
    {
        1,
        3,
        { {0,0,0,0},
          {1,1,1,0},
          {0,0,1,0},
          {0,0,0,0}
        }
    },
    {
        1,
        3,
        { {0,0,1,0},
          {1,1,1,0},
          {0,0,0,0},
          {0,0,0,0}
        }
    },
    {
        1,
        3,
        { {0,0,0,0},
          {1,1,1,0},
          {0,1,0,0},
          {0,0,0,0}
        }
    },
    {
        1,
        3,
        { {0,1,1,0},
          {1,1,0,0},
          {0,0,0,0},
          {0,0,0,0}
        }
    },
    {
        1,
        3,
        { {1,1,0,0},
          {0,1,1,0},
          {0,0,0,0},
          {0,0,0,0}
        }
    }
  };

void readInput(){     //Dieser Teil muss auf den Joystick angepasst werde
  curControl = BTN_NONE;
  int direction_x = analogRead(pin_x);
  int direction_y = analogRead(pin_y);        //hier wird kein Switch Case verwendet, um eine gewisse Sensibilität beim Richtungswechsel herzustellen 
                                              //nach einer gewissen Anzahl an Schaltspielen, könnte es sein, dass der vom Joystick übergebene Analogwert sich in einer kleineren Spanne als 0-1023 befindet. 
  if (direction_x > 900){
    curControl = BTN_RIGHT;
  }
  else if (direction_x < 200){
    curControl = BTN_LEFT;
  }
  else if (direction_y > 900){
    curControl = BTN_DOWN;
  }
  else if (direction_y < 200){
    curControl = BTN_UP;
  }
  else{
    //BTN_NONE bleibt
  }

  if (digitalRead(pin_key) == 0){
     curControl = BTN_PUSH;
  } 
}

int setwhichLedcol(int y){        // die Spalten des Spielfelds müssen auf die entsprechenden Matrizen umgeschrieben werden. Da diese nur jeweils Spaltenbezeichnungen von 0-7 zulassen, muss hier die aktive Reihe des Spielfeldes auf die entsprechened Spalte der
  switch (y){                     // Matrix umgeschrieben werden. Durch den entsprechenden Aufbau des 4-er MAX7219-Aufbau, dienen die Spalten der MAX7219 als Reihen des Spielfelds und anders rum. 
    case 0:
      return 7;
      break;   

    case 1:
      return 6;
      break;   

    case 2:
      return 5;
      break;   

    case 3:
      return 4;
      break;   

    case 4:
      return 3;
      break;   

    case 5:
      return 2;
      break;   

    case 6:
      return 1;
      break;   

    case 7:
      return 0;
      break;   

    case 8:
      return 7;
      break;   

    case 9:
      return 6;
      break;   

    case 10:
      return 5;
      break;   

    case 11:
      return 4;
      break;   

    case 12:
      return 3;
      break;   

    case 13:
      return 2;
      break;   

    case 14:
      return 1;
      break;   

    case 15:
      return 0;
      break;   

    case 16:
      return 7;
      break;   

    case 17:
      return 6;
      break;   

    case 18:
      return 5;
      break;   

    case 19:
      return 4;
      break;   

    case 20:
      return 3;
      break;   

    case 21:
      return 2;
      break;   

    case 22:
      return 1;
      break;   

    case 23:
      return 0;
      break;   
      
  }
}

uint16_t brickSpeed;                        //Variablendeklaration für Basisgrößen (Bausteingeschwindigkeit für das Runterfallen)
uint8_t nbRowsThisLevel;                    // Reihen die in diesem Level bereits aufgelöst wurden 
uint16_t nbRowsTotal;                       //aufgelöste Reihen
boolean tetrisGameOver;
boolean tetrisRunning = false;
int punktestand = 0;

//Tetrisanzeige 
//byte t[8]={B00000000,B00000111,B00000010,B00000010,B00000010,B00000010,B00000000,B00000000};
//byte et[8]={B00000000,B01101110,B01000100,B01100100,B01000100,B01100100,B00000000,B00000000};
//byte ri[8]={B00000000,B11101110,B10100100,B11000100,B10100100,B10101110,B00000000,B00000000};
//byte s[8]={B00000010,B11100101,B10000001,B11100010,B00100010,B11100000,B00000010,B00000000}; 

//Buchstaben für Play / Lose Anzeige
byte p[8] = {B00000000,B00000000,B01111111,B00001001,B00001001,B00001111,B00000000,B00000000};
byte l[8] = {B00000000,B00000000,B01111111,B01000000,B01000000,B01000000,B00000000,B00000000};
byte a[8] = {B00000000,B00000000,B01111111,B00001001,B00001001,B01111111,B00000000,B00000000};
byte y[8] = {B00000000,B00000000,B01001111,B01001000,B01001000,B01111111,B00000000,B00000000};

byte o[8] = {B00000000,B00000000,B01111111,B01000001,B01000001,B01111111,B00000000,B00000000};
byte s[8] = {B00000000,B00000000,B01001111,B01001001,B01001001,B01111001,B00000000,B00000000};
byte e[8] = {B00000000,B00000000,B01111111,B01001001,B01001001,B01000001,B00000000,B00000000};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Global Definitions End+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Setup Start++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void setup(){
  Serial.begin(115200);
  //Wait for serial port to connect
  //bluetooth.begin(BLUETOOTH_SPEED);     //nicht benötigt
  //Initialise display
  pinMode(pin_key, INPUT);
  digitalWrite(pin_key, HIGH);

  for(int index=0;index<lc.getDeviceCount();index++) {
      lc.shutdown(index,false);                                             //Power-Saving Modus aller LED-Matrizen beenden (Hochfahren der Matrizen einleiten) 
      lc.setIntensity(index, 2);                                            //Bildschirmhelligkeit einstellen, hier auf niedrig
  }
  
  //Init random number generator
  //randomSeed(millis());

  lcdisplay.begin(16, 2);                                                    //Das LCD-Display hat 16 Zeichen in 2 Zeilen
  lcdisplay.clear();                                                         //Display mit Punktestand löschen
  lcdisplay.setCursor(0,0);                                                  //in erste Zeile springen 
  lcdisplay.print("Punktestand:");                                           //Überschrift auf LCD-Display schreiben
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Setup End++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Loop Start++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void loop(){

  tetrisanzeigePlay();
  readInput();
  if (curControl == BTN_PUSH){                                                //Das Spiel wird erst gestartet bzw. fortgesetzt, wenn der Joystick-Schalter betätigt wurde 
     runTetris();                                                             //Spiel wird ausgeführt
     clearAllMatrix();                                                        //Spielfeld ablöschen    
  }
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Loop End++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Functions Start++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void tetrisanzeigePlay(){                             // Play-Anzeige (Startbild)  
  lc.setRow(3,0,y[0]);
  lc.setRow(3,1,y[1]);
  lc.setRow(3,2,y[2]);
  lc.setRow(3,3,y[3]);
  lc.setRow(3,4,y[4]);
  lc.setRow(3,5,y[5]);
  lc.setRow(3,6,y[6]);
  lc.setRow(3,7,y[7]);

  lc.setRow(2,0,a[0]);
  lc.setRow(2,1,a[1]);
  lc.setRow(2,2,a[2]);
  lc.setRow(2,3,a[3]);
  lc.setRow(2,4,a[4]);
  lc.setRow(2,5,a[5]);
  lc.setRow(2,6,a[6]);
  lc.setRow(2,7,a[7]);

  lc.setRow(1,0,l[0]);
  lc.setRow(1,1,l[1]);
  lc.setRow(1,2,l[2]);
  lc.setRow(1,3,l[3]);
  lc.setRow(1,4,l[4]);
  lc.setRow(1,5,l[5]);
  lc.setRow(1,6,l[6]);
  lc.setRow(1,7,l[7]);

  lc.setRow(0,0,p[0]);
  lc.setRow(0,1,p[1]);
  lc.setRow(0,2,p[2]);
  lc.setRow(0,3,p[3]);
  lc.setRow(0,4,p[4]);
  lc.setRow(0,5,p[5]);
  lc.setRow(0,6,p[6]);
  lc.setRow(0,7,p[7]);      
}

void tetrisanzeigeLose(){
   //Lose anzeigen, wenn das Spiel verloren wurde 
  lc.setRow(3,0,e[0]);
  lc.setRow(3,1,e[1]);
  lc.setRow(3,2,e[2]);
  lc.setRow(3,3,e[3]);
  lc.setRow(3,4,e[4]);
  lc.setRow(3,5,e[5]);
  lc.setRow(3,6,e[6]);
  lc.setRow(3,7,e[7]);

  lc.setRow(2,0,s[0]);
  lc.setRow(2,1,s[1]);
  lc.setRow(2,2,s[2]);
  lc.setRow(2,3,s[3]);
  lc.setRow(2,4,s[4]);
  lc.setRow(2,5,s[5]);
  lc.setRow(2,6,s[6]);
  lc.setRow(2,7,s[7]);

  lc.setRow(1,0,o[0]);
  lc.setRow(1,1,o[1]);
  lc.setRow(1,2,o[2]);
  lc.setRow(1,3,o[3]);
  lc.setRow(1,4,o[4]);
  lc.setRow(1,5,o[5]);
  lc.setRow(1,6,o[6]);
  lc.setRow(1,7,o[7]);

  lc.setRow(0,0,l[0]);
  lc.setRow(0,1,l[1]);
  lc.setRow(0,2,l[2]);
  lc.setRow(0,3,l[3]);
  lc.setRow(0,4,l[4]);
  lc.setRow(0,5,l[5]);
  lc.setRow(0,6,l[6]);
  lc.setRow(0,7,l[7]);      
}


void clearAllMatrix(){
  for (int n=0; n < lc.getDeviceCount(); n++){ 
    lc.clearDisplay(n);
  }
}

void runTetris(){   //vorher (void)
  delay(100);                                   //Verzögerung, um den Drückbefehl zu überbrücken, damit dieser nicht als Pausierbefehl erkannt wird.
  tetrisInit();
  unsigned long prevUpdateTime = 0;
  tetrisRunning = true;
  int nextupbrick = randombrick();        //nächsten Tetromino aus Bibliothek zufällig generieren
  nextup_brick(nextupbrick);              //nächsten Tetromino in der obersten Matrix anzeigen
  while(tetrisRunning){
    unsigned long curTime;
    do{
      readInput();
      if (curControl != BTN_NONE){
        playerControlActiveBrick();
        printField();
      }
      if (tetrisGameOver) {
        clearAllMatrix();
        clearField();                  //Spielfeld ablöschen, damit im nächsten Zyklus ein neues Spielfeld erzeugt werden kann!      
        tetrisanzeigeLose();
        delay(3000);
        break;
      }
        curTime = millis();
    } 
    while ((curTime - prevUpdateTime) < brickSpeed);  //Once enough time  has passed, proceed. The lower this number, the faster the game is
    prevUpdateTime = curTime;
      if (tetrisGameOver){
      char buf[4];
      int len = sprintf(buf, "%i", nbRowsTotal);
      punktestand = 0; 
      lcdisplay.setCursor(0,1); 
      lcdisplay.print(punktestand);
      //Spielschleife verlassen, beim nächsten Mal wird ein neues Spiel gestartet
      tetrisRunning = false;

      break;
    }
    
    //Solange der Baustein (kollisions-)"frei"  ist, wird er nach unten bewegt 
    if (activeBrick.enabled){
      shiftActiveBrick(DIR_DOWN);
    } 
    else {
      //Active brick has "crashed", check for full lines
      //and create new brick at top of field
      checkFullLines();
      newActiveBrick_in_game(nextupbrick);
      prevUpdateTime = millis();                             //Reset update time to avoid brick dropping two spaces
      nextupbrick = randombrick();                           //nächsten Tetromino aus Bibliothek zufällig generieren
      nextup_brick(nextupbrick);                             //nächsten Tetromino in der obersten Matrix anzeigen
    }
    printField();
  }
}



void checkFullLines(){
  int x,y;
  int minY = 0;
  for (y=(FIELD_HEIGHT-1); y>=minY; y--){
    uint8_t rowSum = 0;
    for (x=0; x<FIELD_WIDTH; x++){
      rowSum = rowSum + (field.pix[x][y]);
    }
    if (rowSum>=FIELD_WIDTH){
      //Volle Reihe gefunden
      for (x=0;x<FIELD_WIDTH; x++){
        field.pix[x][y] = 0;
        printField();
        delay(10);
        tone(13, melody[7], 125);
      }
      punktestand = punktestand + 50;                             //Punktestand für jede aufgelöste Reihe um 50 hochzählen
      lcdisplay.setCursor(0,1);                                   //Cursor in 2. Zeile
      lcdisplay.print(punktestand);                               //neuen Punktestand schreiben
      
      moveFieldDownOne(y);                                        //restliche Blöcke eins nach unten verschieben
      y++; minY++;
      printField();
      delay(50);
      nbRowsThisLevel++; nbRowsTotal++;
      if (nbRowsThisLevel >= LEVELUP){
        nbRowsThisLevel = 0;
        brickSpeed = brickSpeed - SPEED_STEP;
        if (brickSpeed<200){
          brickSpeed = 200;
        }
      }
    }
  }
}

void clearField(){
  uint8_t x,y;
  for (y=0;y<FIELD_HEIGHT;y++){
    for (x=0;x<FIELD_WIDTH;x++){
      field.pix[x][y] = 0;
    }
  }
  for (x=0;x<FIELD_WIDTH;x++){               
    field.pix[x][FIELD_HEIGHT] = 1;
  }
}


void clearAllplayingMatrix(){                       //Spielfeld-Matrizen löschen
  for (int n=1; n < lc.getDeviceCount(); n++){ 
    lc.clearDisplay(n);
  }
}

void clearnextup(){                                 //oberste Matrix mit Bauteilvorhersage löschen
    lc.clearDisplay(0);
}


int setwhichLedadr(int y){ //je nach Y-Position der Bausteine, muss eine andere Matrix leuchten                             
  if (y <= 7){
    return 1;                                       //Matrix 1 leuchtet (2. von oben)
  }
  else if (y > 7 && y < 16){                    
    return 2;                                   //Matrix 2 leuchtet (2. von unten)
  }
  else{
    return 3;                                   //Matrix 3 leuchtet (unterste)
  }
}

void printField(){                                    //das Spielfeld wird hier ausgegeben
int x,y;
  for (x=0;x<FIELD_WIDTH;x++){
    for (y=0;y<FIELD_HEIGHT;y++){
      uint8_t activeBrickPix = 0;
      if (activeBrick.enabled){                          //der Tetromino wird nur angezeigt wenn dieser aktiv ist, also noch nicht abgelegt wurde
        
        //Ist die Position des Bausteins innerhalb der Spielfeldgrenzen, wird dieser angezeigt
        if ((x>=activeBrick.xpos) && (x<(activeBrick.xpos+(activeBrick.siz)))
            && (y>=activeBrick.ypos) && (y<(activeBrick.ypos+(activeBrick.siz)))){
          activeBrickPix = (activeBrick.pix)[x-activeBrick.xpos][y-activeBrick.ypos];
        }
      }
      if (field.pix[x][y] == 1){
        lc.setLed(setwhichLedadr(y), x,setwhichLedcol(y), true );
      } 
      else if (activeBrickPix == 1){
        lc.setLed(setwhichLedadr(y), x,setwhichLedcol(y), true ); 
      } 
      else {
        lc.setLed(setwhichLedadr(y), x,setwhichLedcol(y), false );         
      }           
    }
  }
}

//Bewegung des aktiven Bausteins nach links/rechts /unten
void shiftActiveBrick(int dir){
  if (dir == DIR_LEFT){
    activeBrick.xpos--;
  } 
 else if (dir == DIR_RIGHT){
    activeBrick.xpos++;
  } 
 else if (dir == DIR_DOWN){
    activeBrick.ypos++;
  }
  
  //Anschließend muss geprüft werden, ob die neue Position gültig ist (Kollisionscheck):
  //Wenn der Baustein an den seitlichen bzw. der unteren Spielfeldgrenze ist und ein rechts/links-Befehl kommt, ist die if-Bedingung erfüllt
  //Wenn der Baustein auf einen anderen fixierten Baustein trifft, ist die if-Beingung ebenfalls erfüllt
  //ansonsten kann dieser weiterhin bewegt werden.
  if ((checkSidesCollision(&activeBrick)) || (checkFieldCollision(&activeBrick))){
    //Serial.println("coll");
    if (dir == DIR_LEFT){
      activeBrick.xpos++;                           //gehe zurück an vorherige Position, da vorher die xpos um 1 dekrementiert wurde, wird sie hier um 1 inkrementiert, um die vorherige Position zu erhalten.
    } else if (dir == DIR_RIGHT){
      activeBrick.xpos--;                           //gehe zurück an vorherige Position, da vorher die xpos um 1 inkrementiert wurde, wird sie hier um 1 dekrementiert, um die vorherige Position zu erhalten.
    } else if (dir == DIR_DOWN){
      activeBrick.ypos--;                           //gehe wieder eins hoch
      addActiveBrickToField();
      activeBrick.enabled = false;                  //Aktiver Baustein wird deaktiviert, kann nicht mehr bewegt werden  
    }
  }
}

Brick tmpBrick;   //globale Variable, genutzt als Zwischenspeicher für das Rotationsabbild der aktiven Blöcke 

void rotateActiveBrick(){
  //das rotierte Bauteil wird in tmpBrick abgespeichert und noch nicht gleich am aktiven Bauteil umgesetzt 
  //sind alle Bedingungen für eine Rotation erfüllt, wird activeBrick = tmpBrick gesetzt
  uint8_t x,y;
  for (y=0;y<MAX_BRICK_SIZE;y++){
    for (x=0;x<MAX_BRICK_SIZE;x++){
      tmpBrick.pix[x][y] = activeBrick.pix[x][y];
    }
  }
  tmpBrick.xpos = activeBrick.xpos;
  tmpBrick.ypos = activeBrick.ypos;
  tmpBrick.siz = activeBrick.siz;
  
  //Die Größe des Bausteins bestimmt, in welchem Punkt er gedreht wird 
  if (activeBrick.siz == 3){
    //Für die Größe 3, wird der Baustein in einem 3x3 Feld um das Mittelfeld rotiert
    tmpBrick.pix[0][0] = activeBrick.pix[0][2];                                                   
    tmpBrick.pix[0][1] = activeBrick.pix[1][2];                                                   
    tmpBrick.pix[0][2] = activeBrick.pix[2][2];                                                   
    tmpBrick.pix[1][0] = activeBrick.pix[0][1];
    tmpBrick.pix[1][1] = activeBrick.pix[1][1];
    tmpBrick.pix[1][2] = activeBrick.pix[2][1];
    tmpBrick.pix[2][0] = activeBrick.pix[0][0];
    tmpBrick.pix[2][1] = activeBrick.pix[1][0];
    tmpBrick.pix[2][2] = activeBrick.pix[2][0];
    
    //Alle Pixel außerhalb des oberen linken 3x3 Feldes bleiben leer
    tmpBrick.pix[0][3] = 0;
    tmpBrick.pix[1][3] = 0;
    tmpBrick.pix[2][3] = 0;
    tmpBrick.pix[3][3] = 0;
    tmpBrick.pix[3][2] = 0;
    tmpBrick.pix[3][1] = 0;
    tmpBrick.pix[3][0] = 0;
    
  } else if (activeBrick.siz == 4){
    //Bei Größe = 4 wird um das Mittlere Kreuz einer 4x4 Matrix rotiert 
    tmpBrick.pix[0][0] = activeBrick.pix[0][3];
    tmpBrick.pix[0][1] = activeBrick.pix[1][3];
    tmpBrick.pix[0][2] = activeBrick.pix[2][3];
    tmpBrick.pix[0][3] = activeBrick.pix[3][3];
    tmpBrick.pix[1][0] = activeBrick.pix[0][2];
    tmpBrick.pix[1][1] = activeBrick.pix[1][2];
    tmpBrick.pix[1][2] = activeBrick.pix[2][2];
    tmpBrick.pix[1][3] = activeBrick.pix[3][2];
    tmpBrick.pix[2][0] = activeBrick.pix[0][1];
    tmpBrick.pix[2][1] = activeBrick.pix[1][1];
    tmpBrick.pix[2][2] = activeBrick.pix[2][1];
    tmpBrick.pix[2][3] = activeBrick.pix[3][1];
    tmpBrick.pix[3][0] = activeBrick.pix[0][0];
    tmpBrick.pix[3][1] = activeBrick.pix[1][0];
    tmpBrick.pix[3][2] = activeBrick.pix[2][0];
    tmpBrick.pix[3][3] = activeBrick.pix[3][0];
  } else {
    //alle anderen Bausteingrößen sind nicht zulässig!
    Serial.println("Brick size error");
  }
  //Rotationsbedingung ist, dass keine Kollision mit den Spielfeldgrenzen oder anderen Bausteinen besteht
  if ((!checkSidesCollision(&tmpBrick)) && (!checkFieldCollision(&tmpBrick))){                                      
    for (y=0;y<MAX_BRICK_SIZE;y++){
      for (x=0;x<MAX_BRICK_SIZE;x++){
        activeBrick.pix[x][y] = tmpBrick.pix[x][y];                                                         //Rotation wird ausgeführt
      }
    }
  }
}

//je nachdem welcher Eingang in der Funktion readInput() erfasst wurde, wird eine bestimmte Bauteilsteuerung ausgelöst
void playerControlActiveBrick(){
  switch(curControl){
    case BTN_LEFT:      
      shiftActiveBrick(DIR_LEFT);                         //der Baustein wird nach links bewegt
      break;
   case BTN_RIGHT:
      shiftActiveBrick(DIR_RIGHT);                        //der Baustein wird nach RECHTS bewegt
      break;
    case BTN_DOWN:
      shiftActiveBrick(DIR_DOWN);                         //der Baustein wird nach unten beschleunigt 
      break;
    case BTN_UP:
      rotateActiveBrick();                                //der Baustein wird gedreht
      break;
    case BTN_PUSH:
      tetrisRunning = false;                              //das Spiel wird pausiert, der nächste Baustein wird jedoch zufällig generiert 
      break;
  }
}


void newActiveBrick(){                                  //ein neuer Tetromino wird zufällig aus den 7 Bausteinen in der oben definierten Brick-Library ausgewählt und dem Aktiven Tetromino zugewiesen. 
  uint8_t selectedBrick = random(7);
  int num = selectedBrick;

 // der Neue Brick wird nach Parametern der Bibliothek angepasst
  activeBrick.siz = brickLib[selectedBrick].siz;
  activeBrick.yOffset = brickLib[selectedBrick].yOffset;
  activeBrick.xpos = FIELD_WIDTH/2 - activeBrick.siz/2;
  activeBrick.ypos = BRICKOFFSET-activeBrick.yOffset;
  activeBrick.enabled = true;

  //Copy pix array of selected Brick
  uint8_t x,y;
  for (y=0;y<MAX_BRICK_SIZE;y++){
    for (x=0;x<MAX_BRICK_SIZE;x++){
      activeBrick.pix[x][y] = (brickLib[selectedBrick]).pix[x][y];
    }
  }
  //Check collision, if already, then game is over
  if (checkFieldCollision(&activeBrick)){
    tetrisGameOver = true;
  }
}

void tetrisInit(){
  clearAllplayingMatrix();   //Leds ablöschen 
  brickSpeed = INIT_SPEED;
  nbRowsThisLevel = 0;
  nbRowsTotal = 0;
  tetrisGameOver = false;
  newActiveBrick();     //neuen Tetromino erstellen 
}

int randombrick(){
  uint8_t selectedBrick = random(7);
  int num = selectedBrick;
  return num;
}


void nextup_brick(int numbrick){
  //Vorschau für den nächsten Tetromino, der kommt, anzeigen
  clearnextup();
  lc.setColumn(0,0,B11111111);
  lc.setColumn(0,7,B11111111);
  lc.setRow(0,0,B11111111);
  lc.setRow(0,7,B11111111);

  switch(numbrick){
    case 0: 
    //row, col 
    //O-Tetromino
      lc.setLed(0,3,3,true);
      lc.setLed(0,3,4,true);  
      lc.setLed(0,4,3,true);
      lc.setLed(0,4,4,true);
      break;
      
    case 1: 
      //I-Tetromino
      lc.setLed(0,2,3,true);
      lc.setLed(0,3,3,true);  
      lc.setLed(0,4,3,true);
      lc.setLed(0,5,3,true);
      break;
      
    case 2: 
      //J-Tetromino
      lc.setLed(0,3,5,true);
      lc.setLed(0,3,4,true);  
      lc.setLed(0,3,3,true);
      lc.setLed(0,4,3,true);
      break;
      
    case 3: 
      //L-Tetromino 
      lc.setLed(0,4,5,true);
      lc.setLed(0,4,4,true);  
      lc.setLed(0,4,3,true);
      lc.setLed(0,3,3,true);
      break;
      
    case 4: 
      //T-Tetromino
      lc.setLed(0,4,5,true);
      lc.setLed(0,4,4,true);  
      lc.setLed(0,4,3,true);
      lc.setLed(0,3,4,true);
      break;
      
    case 5: 
      //S-Tetromino
      lc.setLed(0,2,4,true);
      lc.setLed(0,3,4,true);  
      lc.setLed(0,3,3,true);
      lc.setLed(0,4,3,true);
      break;
      
     case 6: 
      //Z-Tetromino
      lc.setLed(0,2,3,true);
      lc.setLed(0,3,3,true);  
      lc.setLed(0,3,4,true);
      lc.setLed(0,4,4,true);
      break;
  }
}  

//Kollisionsprüfung zwischen den verschiedenen Bausteinen
boolean checkFieldCollision(struct Brick* brick){  
  uint8_t bx,by;
  uint8_t fx,fy;
  for (by=0;by<MAX_BRICK_SIZE;by++){
    for (bx=0;bx<MAX_BRICK_SIZE;bx++){
      fx = (*brick).xpos + bx;
      fy = (*brick).ypos + by;
      if (( (*brick).pix[bx][by] == 1) 
            && ( field.pix[fx][fy] == 1)){
       return true;
      }
    }
  }
  return false;
}

//Kollisionsüberwachung zwischen dem übergebenen Bautein und den Spielfeldgrenzen / Seiten
boolean checkSidesCollision(struct Brick* brick){   
  uint8_t bx,by;
  uint8_t fx,fy;
  for (by=0;by<MAX_BRICK_SIZE;by++){
    for (bx=0;bx<MAX_BRICK_SIZE;bx++){
      if ( (*brick).pix[bx][by] == 1){
        fx = (*brick).xpos + bx;        //Aktuelle Position des aktiven Bausteins ermitteln 
        fy = (*brick).ypos + by;
        if (fx<0 || fx>=FIELD_WIDTH){   //Prüfe ob der Baustein an den Seitenbegrenzungen angekommen ist  
          return true;
        }
        if (fy==FIELD_HEIGHT){          //Prüfe ob der Baustein ganz unten angekommen ist
          return true;                  //true wenn Kollision erkannt
        }
      }
    }
  }
  return false;
}
   
void newActiveBrick_in_game(int num){                                
  // der Neue Brick wird nach Parametern der Bibliothek angepasst
  activeBrick.siz = brickLib[num].siz;
  activeBrick.yOffset = brickLib[num].yOffset;
  activeBrick.xpos = FIELD_WIDTH/2 - activeBrick.siz/2;
  activeBrick.ypos = BRICKOFFSET-activeBrick.yOffset;
  activeBrick.enabled = true;
  
  //Den Bibliotheksbaustein auf den aktuellen kopieren 
  uint8_t x,y;
  for (y=0;y<MAX_BRICK_SIZE;y++){
    for (x=0;x<MAX_BRICK_SIZE;x++){
      activeBrick.pix[x][y] = (brickLib[num]).pix[x][y];
    }
  }
  //Check collision, if already, then game is over
  if (checkFieldCollision(&activeBrick)){
    tetrisGameOver = true;
  }
}

//Aktiven Baustein auf dem Feld projizieren 
void addActiveBrickToField(){
  uint8_t bx,by;
  uint8_t fx,fy;
  for (by=0;by<MAX_BRICK_SIZE;by++){
    for (bx=0;bx<MAX_BRICK_SIZE;bx++){
      fx = activeBrick.xpos + bx;
      fy = activeBrick.ypos + by;
      
      if (fx>=0 && fy>=0 && fx<FIELD_WIDTH && fy<FIELD_HEIGHT && activeBrick.pix[bx][by]){          //Baustein ist innerhalb des Feldes 
        field.pix[fx][fy] = field.pix[fx][fy] || activeBrick.pix[bx][by];
        field.pix[fx][fy] = activeBrick.pix[bx][by];
      }
    }
  }
}

//Alle Pixel im Feld überhalb der Startreihe ein Pixel nach unten bewegen
void moveFieldDownOne(uint8_t startRow){
  if (startRow == 0){                          
    return;
  }
  uint8_t x,y;
  for (y=startRow-1; y>0; y--){
    for (x=0;x<FIELD_WIDTH; x++){
      field.pix[x][y+1] = field.pix[x][y];
    }
  }
}
