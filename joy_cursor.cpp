


#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <SPI.h>
#include <SD.h>
#include "lcd_image.h"
#include "joy_cursor.h"

MCUFRIEND_kbv tft;
lcd_image_t yegImage = { "yeg-big.lcd", YEG_SIZE, YEG_SIZE };

// global variables for cursor and map positions 
int cursorX, cursorY, currentPatchX, currentPatchY;

// redraws the cursor on the given cursor position
void redrawCursor() {
  tft.fillRect(cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2,
               CURSOR_SIZE, CURSOR_SIZE, TFT_RED);
}

// draws an entire map patch on the screen
void drawMap() {
  lcd_image_draw(&yegImage, &tft, currentPatchX, currentPatchY,
                 0, 0, MAP_WIDTH, MAP_HEIGHT);
}

// setup for tft and map patch
void joySetup() {
  uint16_t ID = tft.readID();    // read ID from display
  if (ID == 0xD3D3) ID = 0x9481; // write-only shield  
  tft.begin(ID);                 // LCD gets ready to work
	if (!SD.begin(SD_CS)) {
		while (true) {}
	}
	tft.setRotation(1); tft.fillScreen(TFT_BLACK);
  // sets the current map patch to the middle of the Edmonton map
	currentPatchX = YEG_SIZE/2 - MAP_WIDTH/2;
	currentPatchY = YEG_SIZE/2 - MAP_HEIGHT/2;
  drawMap();
  // initial cursor position is the middle of the screen
  cursorX = MAP_WIDTH/2;
  cursorY = MAP_HEIGHT/2;
  redrawCursor();
}

// redraws map background on previous cursor position to remove black trail
void redrawMapBg(uint16_t tempX, uint16_t tempY) {
  int screenPatchX = tempX - CURSOR_SIZE/2;
  // prevents cursor from moving to black column
  screenPatchX = constrain(screenPatchX, 0, MAP_WIDTH - CURSOR_SIZE);
  int screenPatchY = tempY - CURSOR_SIZE/2;
  lcd_image_draw(&yegImage, &tft, currentPatchX + screenPatchX, 
                  currentPatchY + screenPatchY, screenPatchX, 
                  screenPatchY, CURSOR_SIZE, CURSOR_SIZE);
}
// processes analog data and changes the increment 
// depending on the read analog value (from +-1px to +-4px)
void processAnalog(int aVal, char dir) {
  int posBuffer = JOY_CENTER + JOY_DEADZONE;
  int negBuffer = JOY_CENTER - JOY_DEADZONE;
  int increment = 0;
  if (aVal > posBuffer) {
    increment += 1 + (aVal - posBuffer)/(JOY_DEADZONE * 2);
  } else if (aVal < negBuffer) {
    increment += -1 +(aVal - negBuffer)/(JOY_DEADZONE * 2);
  }
  // accounts for the inverted X increment
  if (dir == 'X') {
  	cursorX -= increment;
  } else {
  	cursorY += increment;	
  }
}
// moves map patch depending on which edge of the screen the cursor was touching
void moveMapPatch(bool xl, bool xr, bool yt, bool yb) {
  int crsB = CURSOR_SIZE/2 + 1;
  if (!xl) {currentPatchX -= MAP_WIDTH; cursorX = MAP_WIDTH - crsB;}
  if (!xr) {currentPatchX += MAP_WIDTH; cursorX = crsB;}
  if (!yt) {currentPatchY -= MAP_HEIGHT; cursorY = MAP_HEIGHT - crsB;}
  if (!yb) {currentPatchY += MAP_HEIGHT; cursorY = crsB;}
  currentPatchX = constrain(currentPatchX, 0, YEG_SIZE - MAP_WIDTH);
  currentPatchY = constrain(currentPatchY, 0, YEG_SIZE - MAP_HEIGHT);
  drawMap(); redrawCursor();
}
// checks if the edge of the Edmonton map is loaded
bool inMapBounds() {
  if (currentPatchX == 0 || currentPatchX == YEG_SIZE - MAP_WIDTH ||
      currentPatchY == 0 || currentPatchY == YEG_SIZE - MAP_HEIGHT) {
    return true;
  }
  return false;
} 

void processJoystick() {
  // temp variables to draw map on later
  int tempX = cursorX, tempY = cursorY;
  processAnalog(analogRead(JOY_HORIZ), 'X'); 
  processAnalog(analogRead(JOY_VERT), 'Y');
  int cRad = CURSOR_SIZE/2;
  // checks if the cursor is still in bounds of the screen
  bool xInLeftBnd = cursorX >= cRad; 
  bool xInRightBnd = cursorX <= MAP_WIDTH - 1 - cRad;
  bool yInTopBnd = cursorY >= cRad; 
  bool yInBotBnd = cursorY <= MAP_HEIGHT - 1 - cRad;

  if (!xInLeftBnd || !xInRightBnd || !yInTopBnd || !yInBottomBnd) {
    // if not on the edge of Edmonton map, moves the map patch
    // else clamps the cursor on the edge of the screen
    if (!inMapBounds()) {
      moveMapPatch(xInLeftBnd, xInRightBnd, yInTopBnd, yInBotBnd);
    }
    else {
      cursorX = constrain(cursorX, cRad, MAP_WIDTH - 1 - cRad);
      cursorY = constrain(cursorY, cRad, MAP_HEIGHT - 1 - cRad);
    }
  }
  // will only redraw map when the cursor moves to prevent the cursor from flickering
  if (tempX != cursorX || tempY != cursorY) {
    redrawMapBg(tempX, tempY);
    redrawCursor();
  }
   
}

