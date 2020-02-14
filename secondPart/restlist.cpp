#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <SPI.h>
#include <SD.h>

#include "coordinates.h"
#include "jcursor.h"
#include "restlist.h"
#include "sorting.h"

int8_t selectedRest;
int8_t pageNum;
restaurant currentRest;

void displayText(int index) {
	// 15 is the vertical span of a size-2 character
	// (including the one pixel of padding below)
	tft.setCursor(0, 15*(index % NUM_LINES));

  // highlights selected restaurant
	if (index == selectedRest) { 
    tft.setTextColor(TFT_BLACK, TFT_WHITE); 
  } else {
		tft.setTextColor(TFT_WHITE, TFT_BLACK);
	}
  // loads restaurants name and prints
  getRestaurantFast(rest_dist[index].index, &currentRest);
	tft.println(currentRest.name);
}

void pageUpdate() {
  tft.fillScreen(TFT_BLACK);
	tft.setTextSize(2);
	tft.setTextWrap(false);
  for (int i = selectedRest; i < (pageNum + 1) * NUM_LINES; i++) {
    Serial.println(i);
    displayText(i);
  }
}

// lets the user select a restaurant
void joySelect(int prevRest, int len) {
  int aVal = analogRead(JOY_VERT);
	if (aVal > POS_BUFFER || aVal < NEG_BUFFER) {
  	prevRest = selectedRest;
    // save the previous restaurant to draw it normally
    if (aVal > POS_BUFFER) {
      selectedRest++;
      if (selectedRest > (pageNum + 1) * NUM_LINES) {
        pageNum++;
        pageUpdate();
        return;
      }
		}
  	else {
      selectedRest--;
      if (selectedRest < 0) {
        selectedRest = 0;
        return;
      }
  	}
    // draw the old highlighted string normally
    displayText(prevRest);
    // highlight the new string
    displayText(selectedRest);

		delay(50);
	}
}

// adjust coordinates to centre the restaurant
// also considering boundary conditions
void adjustCoordinates(int posX, int posY) {
  bool normalXBound = posX >= CENTRE_X && posX <= MAX_X; 
  bool normalYBound = posY >= CENTRE_Y && posY <= MAX_Y; 
  currentPatchX = constrain(posX - CENTRE_X, 0, MAP_MAXX);
  currentPatchY = constrain(posY - CENTRE_Y, 0, MAP_MAXY);
  // places the cursor in the middle if bounds are normal
  // otherwise simply places it on the restaurant
  if (normalXBound) {cursorX = CENTRE_X;} 
  else {cursorX = abs(posX - currentPatchX);}
  if (normalYBound) {cursorY = CENTRE_Y;} 
  else {cursorY = abs(posY - currentPatchY);}
}

// places cursor on the selected restaurant
void goToResto() {
  getRestaurantFast(rest_dist[selectedRest].index, &currentRest);
  // converts from longitude and latitude
  int positionX = map(currentRest.lon, LON_WEST, LON_EAST, 0, YEG_SIZE);
  int positionY = map(currentRest.lat, LAT_NORTH, LAT_SOUTH, 0, YEG_SIZE);
  // constrains the position inside the map for out of bounds restaurants
  positionX = constrain(positionX, CUR_RAD, YEG_SIZE - CUR_RAD - 1);
  positionY = constrain(positionY, CUR_RAD, YEG_SIZE - CUR_RAD - 1);
  adjustCoordinates(positionX, positionY);
}

void restList() {
  int len, prevRest;
  switch (sortSetting) {
  case quick:
    len = runSort(quick);
    break;
  case insert:
    len = runSort(insert);
    break;
  case both:
    runSort(quick);
    len = runSort(insert);
    break;
  default:
    break;
  }
  selectedRest = 0; pageNum = 0;
  pageUpdate();
  while (digitalRead(JOY_SEL) == HIGH) {
    joySelect(prevRest, len);
  }
  goToResto();
}
