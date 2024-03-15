// Based on the software provided by Waveshare: https://www.waveshare.com/wiki/1.3inch_OLED_Module_(C)

#include "OLED_Driver.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"
#include "Debug.h"
#include "ImageData.h"

UBYTE *BlackImage;

#include <Wire.h>

// DISTANCE SENSOR SETTINGS
#define TRIG_PIN  10 // Connect Arduino pin D10 to HC-SR04 pin Trig 
#define ECHO_PIN  11 // Connect Arduino pin D11 to HC-SR04 pin Echo

const int AVERAGE_SAMPLE_NUMBER = 10;

// Distance sensor Variables
volatile long duration;         // Sound wave travel duration
volatile int distance;          // Distance measurement
volatile int average_distance;  // Average distance measurement
volatile char cstr[8];

void setup() {
  System_Init();
  Serial.print(F("OLED_Init()...\r\n"));
  OLED_Init();
  Driver_Delay_ms(500); 
  //0.Create a new image cachegImage_1in3_c
  
  UWORD Imagesize = ((OLED_WIDTH % 8 == 0)? (OLED_WIDTH / 8 ): (OLED_WIDTH / 8 + 1)) * OLED_HEIGHT;
  if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
      Serial.print(F("Failed to apply for black memory...\r\n"));
      return -1;
  }
  Paint_NewImage(BlackImage, OLED_WIDTH, OLED_HEIGHT, 180, BLACK);  

  //1.Select Image
  Paint_SelectImage(BlackImage);
  Paint_Clear(BLACK);
  Driver_Delay_ms(500); 

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  average_distance = get_distance();

  Serial.print("ave dist: ");
  Serial.println(average_distance);

  Serial.print(F("Drawing:page 2\r\n"));  
  Paint_DrawString_EN(10, 5, "Average Distance", &Font12, WHITE, WHITE);
  Paint_DrawNum(10, 50, itoa(average_distance,cstr,10), &Font12, 2, WHITE, WHITE);    
  OLED_Display(BlackImage);
  Driver_Delay_ms(200);  
  Paint_Clear(BLACK);

  send_distance(average_distance);
}

int get_distance() {
  distance = 0;
  
  for (byte i = 0; i < AVERAGE_SAMPLE_NUMBER; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    
     // Sets TRIG_PIN HIGH (ACTIVE) for 10 microseconds and then LOW (INACTIVE)
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
    duration = pulseIn(ECHO_PIN, HIGH);
    
    // Calculating the distance
    distance += duration * 0.034 / 2;
  }

  return distance/AVERAGE_SAMPLE_NUMBER;
}

// function that executes whenever data is requested by the controller
// this function is registered as an event, see setup()
void send_distance(int average_distance) {
    uint8_t Addr = 0x4;
    Wire.beginTransmission(Addr);
    Wire.write(average_distance);
    Wire.endTransmission();
}
