/*
 Written by: Mujahed Altahle
 
This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/ 
or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

 */
/* A simple Project for Remote Controlling with nRF24L01+ radios. 
 We have 2 nodes, the transmitter (this one) and the receiver which is attached to the Car.
 The idea is to transmit  2 parameters , one is Direction (Backward, or Forward with the speed) the other is the Steering (Left, or Right with the degree of rotation).
 The nRF24L01 transmit values in type of "uint8_t" with maximum of 256 for each packet, so the values of direction is divided by (10) at the Tx side,
 then it is multiplied by 10 again at the Rx side.
 The Rx rules is to output the received parameters to port 3 and 6 where the Servo and the ESC are are attached
 a condition is required to prevent the controller from transmitting values that is not useful (like 1480-1530 for ESC and 88-92 for Servo) to save more power as much as we can
 */
 
#include <Servo.h> 
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
//
// Hardware configuration
//
#define CE_PIN 9
#define CSN_PIN 10

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

Servo servo; //steering servo declaration 
Servo esc; // Electronic Speed Contoller declaration

int Servo_X;  
int Servo_Y;  
byte ESC_speed;
int ESC_value;

// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

//
// Setup NRF24
//

uint8_t received_data[2];
uint8_t num_received_data =sizeof(received_data);

//
// Setup ESC
//
#define motA    3
#define motB    6
#define ServoPin    5

void setup(void)
{
  delay(1000); //wait until the esc starts in case of Arduino got power first
  servo.attach(ServoPin);  // attaches the servo on pin 3 to the servo object 
  //esc.attach(6);  // attaches the ESC on pin 6 to the ese object  
  pinMode(motA, OUTPUT);
  pinMode(motB, OUTPUT);
  
  servo.write(90);
  ESC_value=500;
 
  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();

  //
  // Setup and configure rf radio
  //

  radio.begin(); //Begin operation of the chip.
  // This simple sketch opens a single pipes for these two nodes to communicate
  // back and forth.  One listens on it, the other talks to it.
  radio.setDataRate(RF24_250KBPS); // Both endpoints must have this set the same
  radio.openReadingPipe(1,pipe);
  radio.startListening();
  //
  // Dump the configuration of the rf unit for debugging
  //
  radio.printDetails(); 
}


void loop(void)
{
  // if there is data ready
  if ( radio.available() )
  {
    bool done = false;

    while (!done)
    {
      // Fetch the payload, and see if this was the last one.

      done = radio.read( received_data, num_received_data );
      ESC_value=received_data[0]; //Multiplication by 10 because the ESC operates for vlues around 1500 and the nRF24L01 can transmit maximum of 255 per packet 
      Servo_X=map(ESC_value, 255, 0, 0, 1023); 
      Serial.print(" esc:\n\r"); 
      Serial.print(Servo_X);
// Start ESC bi direction , if joy in the middle set motA and B to low to avoid jogging 
  if (Servo_X >480 && Servo_X <520) {
    analogWrite(motA, 0);
    analogWrite(motB, 0); 
  }
  else if (Servo_X <489){            
    ESC_speed = map(Servo_X, 0,497,255, 0); //Reverse 0 is the fastest
    analogWrite(motA, ESC_speed);
    analogWrite(motB, 0); 
  }
  else if (Servo_X >524){
    ESC_speed = map(Servo_X, 524,1023,0, 255);  //Forward 1023 is the fastest
    analogWrite(motA, 0);
    analogWrite(motB, ESC_speed); 
  }
  else{                                // set MotA and B to low to avoid unwanted start
    analogWrite(motA, 0);
    analogWrite(motB, 0);  
  }

      //esc.writeMicroseconds(ESC_value);
    //  Serial.println(ESC_value);
      Servo_Y=received_data[1];
      Servo_Y=map(Servo_Y,255,0,128,68 );
      servo.write((Servo_Y));
      Serial.print(" servo\n\r"); 
      Serial.print(received_data[1]);
     // Serial.println(received_data[1]);
    }
  }
}

