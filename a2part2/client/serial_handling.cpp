#include "serial_handling.h"

extern shared_vars shared;

// prints request to Serial
void send_request(const lon_lat_32& start, const lon_lat_32& end) {
  // Request format: R slon slat elon elat
  char msg[40];
  sprintf(msg, "R %ld %ld %ld %ld", 
          start.lat, start.lon, end.lat, end.lon);
  Serial.println(msg); 
}

// given function for finite state machines
// copied from cmput 274 assignment 2
bool wait_on_serial(long timeout, uint8_t nbytes = 1) {
  unsigned long deadline = millis() + timeout; // wraparound not a problem
  while (Serial.available() < nbytes && (timeout < 0 || millis() < deadline)) {
    delay(1); // be nice, no busy loop
  }
  return Serial.available() >= nbytes;
}
// processes line from serial
// returns false if nothing was received
bool process_line(char *buff, uint32_t timeout) {
  int len = 0; char c;
  uint32_t deadline = millis() + timeout;
  // adapted from simpleclient.cpp to read with timeout
  while (millis() < deadline) {
    if (Serial.available()) {
      c = Serial.read();
      if (c == '\n' || c == '\r') {
        return true;
      } else {
        buff[len] = c;
        len++;
        buff[len] = 0; // null terminator
      }
    }
  }
  return false;
}

// get number of waypoints from serial
// returns true if successful
bool get_num() {
  char buffer[25];
  // 10 second timeout for number of waypoints
  if (process_line(buffer, 10000)) {
    if (buffer[0] == 'N') {
      // copy buffer to another char array
      // assumes 500 is the max waypoints
      char num[4]; num[3] = 0;
      for (int i = 0; i < 3; i++) {
        num[i] = buffer[i + 2];
      }
      shared.num_waypoints = atoi(num);
      return true;
    }
  }
  return false;
}


bool process_waypoint(int16_t index) {
  char buff[25], lon[10], lat[10];
  // 1 second timeout for every waypoint (1000 ms)
  if (process_line(buff, 1000)) {
    if (buff[0] == 'W') {
      // assumes that all numbers would be the same format:
      // 8 digits negative latitude 7 digits longitude 
      for (int i = 0; i < 10; i++) {
        lat[i] = buff[i + 2];
        lon[i] = buff[i + 10];
      }
      // add null terminators
      lon[9] = 0; lat[9] = 0;

      // store parsed coordinates to shared variable
      shared.waypoints[index].lon = atol(lon);
      shared.waypoints[index].lat = atol(lat);

      // send acknowledge to serial
      Serial.println("A");
      return true;
    }
  }
  return false;
}

uint8_t get_waypoints(const lon_lat_32& start, const lon_lat_32& end) {
  // Currently this does not communicate over the serial port.
  // It just stores a path length of 0. You should make it communicate with
  // the server over Serial using the protocol in the assignment description.

  // TODO: implement the communication protocol from the assignment
  // send request to serial
  send_request(start, end);
  status_message("Sent Request...");
  // return 0 at any point that the communication fails
  if (!get_num()) {return 0;}
  // send acknowledgement
  // let user choose a new route if no path was found 
  if (shared.num_waypoints == 0) {
    status_message("No path found!");
    delay(1000);
    return 1;
  }
  Serial.println("A");
  status_message ("Receiving path...");
  for (int i = 0; i < shared.num_waypoints; i++) {
    if (!process_waypoint(i)) {return 0;}
  }
  // 1 indicates a successful exchange, of course you should only output 1
  // in your final solution if the exchange was indeed successful
  // (otherwise, return 0 if the communication failed)
  return 1;
}
