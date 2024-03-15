#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include "CytronMotorDriver.h"

//Steering
int servoPin = 5;
int servoDefaultValue = 88;
Servo servoSteering;

// Motor
CytronMD motor1(PWM_PWM, 7, 8);   // PWM 1A = Pin 7, PWM 1B = Pin 8.

//I2C (Controller Reader)
volatile int dist = 0;

//SPI (Peripheral)
byte data_spi = 0;
volatile char command = 0;

const int MIN_DISTANCE_CM = 30;

/* Initialization */

void setup() {
  Serial.begin(9600);   // debugging

  //Wire.begin();
  Wire.begin(4);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  
  spi_init();

  sssBegin();
    
  //motor_init();
  steer_init();
}

void spi_init()
{
  // turn on SPI in peripheral mode
  SPCR |= _BV(SPE);
  
  // have to send on controller in, *peripheral out*
  pinMode(MOSI, INPUT);
  pinMode(SCK, INPUT);
  pinMode(SS, INPUT);
  pinMode(MISO, OUTPUT);
  
  // now turn on interrupts
  SPI.attachInterrupt();
}

void steer_init()
{ 
  servoSteering.attach(servoPin);
  servoSteering.write(servoDefaultValue);
}

void loop() {
  //Serial.print("spi received: ");
  command = (char)data_spi;
  Serial.println(command);

  if (command == 'p' || command == 's') {
    set_radio(command);  
  } else {
    set_speed(command);  
  }
}

/* Methods */

int get_obstacle_distance()
{
  Wire.requestFrom(4, 1);  // request 1 byte from peripheral device #4

  while (Wire.available()) { 
    dist = Wire.read();
    Serial.print("i2c received: ");
    Serial.println(dist);       
  }

  return dist;
}

void receiveEvent(int howMany)
{
  while (Wire.available()) { 
    dist = Wire.read();
    Serial.print("i2c received: ");
    Serial.println(dist);       
  }
}

void set_radio(char command)
{
  if (command == 'p') {
    sssWrite(byte(1));
  }
  if (command == 's') {
    sssWrite(byte(0));
  }
}

void set_speed(char command)
{
  if (command == 'r'){ // Reverse
    motor1.setSpeed(-150);
  }
  if (command == 'v'){ // Left
    servoSteering.write(45);
  }
  if (command == 'h'){ // Right
    servoSteering.write(120);
  }
  if (command == 'b'){ // straight
    servoSteering.write(servoDefaultValue);
  }
  if (command == 'f'){ // Forward
    if (dist > MIN_DISTANCE_CM) {
      motor1.setSpeed(150);
    } else {
      motor1.setSpeed(0);
    }
  }
  if (command == 'x'){ // Stop
    motor1.setSpeed(0);
  }
}

/* Interrupts */
// SPI interrupt routine
ISR (SPI_STC_vect)
{
  data_spi = SPDR;

  if (dist > 255){
    SPDR = 255;
  } else {
    SPDR = dist;
  }
}
