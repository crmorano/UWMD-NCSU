//**************************************************************//
//  Name	:                         	 
//  Author  :  
//  Date	: 04 Nov, 2013    
//  Modified:                            	 
//  Version : 2.0                                        	 
//  Notes   :       	//
//      	:                      	 
//****************************************************************
#include <EEPROM.h>

#define NUMBEROFREADS 200 //About 20ms; About 100us per read
#define OUTPUTBUFFERSIZE 4 //
#define SENSORHIGH 1023 //
#define SENSITIVITY_ADDR 0 //Sensitivity Value
#define CAL_ADDR0 1 //Calibration ValueNote, 2 bytes total
#define CAL_ADDR1 2 //High Byte

void setSignalStrength(void);
void ledDisplayUpdate(void);
void checkButtons(void);
void clearButtons(void);
int calibrate(void);
int sendPulse(void);

boolean calibrated = false;
boolean motorOn = true;
int motorIntensity = 255; //0-255
boolean motorState = false;
int motorCount = 0;
int motorRunOnce = 1;
int motor_on_state = 0;

int sensitivity = 20; //formerly sensitivity; Can be manipulated to effect display sensitivity; # levels = numberOfLevels
int numberOfLevels = 5; //Sensitivity Levels
int sampleRange;
int averageValue; //average of ouputBuffer array

int numberOfLEDs = 5; //number of led groups in signal array
int led[5]; //Size = numberOfLEDs

int g_drive = 3; //coil output
int g_input = A0; //coil input

int mode_led = 4;//mode output
int mode_sw = A4;//mode input
int mode_state = 0;
int mode_led_state = 0;
char mode_button_hit = 0;

int up_sw = A3;  //up button in
char up_button_hit = 0;
int down_sw = A5;//down button in
char down_button_hit = 0;

int batt_led = 5;// battery display
int batt_led_state = 0;
//int battLevel = 1;
int calValue = 950; //no metal
int outputValue = 0;

int motor_port = 11;
boolean buttons_run_once = false;


//int readBuffer[NUMBEROFREADS];
int outputBuffer[OUTPUTBUFFERSIZE];
int outBuffIndex = 0;



//------------------------------------------SET UP-------------------------------------------------------------------------------
void setup() 
{ 
  pinMode(motor_port, OUTPUT);            //motor state
  digitalWrite(motor_port, LOW);

  pinMode(mode_led, OUTPUT);            //configure additional state LEDs for Mode transitions &
  digitalWrite(mode_led, LOW);
  pinMode(batt_led, OUTPUT);            //battery state
  digitalWrite(batt_led, LOW);
  

  //set buttons as inputs
  pinMode(mode_sw, INPUT_PULLUP);              //A4
  pinMode(down_sw, INPUT_PULLUP);              //A5
  pinMode(up_sw, INPUT_PULLUP);                //A3

  
  pinMode(g_input,INPUT);               //setup coil reading pin
  pinMode(g_drive,OUTPUT);              //setup driving interface for coil
  digitalWrite(g_drive, LOW);           //intialize as low
  analogReference(INTERNAL);            //reference voltage between 1.0v - 1.2v

  // initialize serial communication at 9600 bits per second:     ******FOR TESTING*******
  Serial.begin(9600);
  Serial.println("Begin Serial");

  for (int i = 0; i<numberOfLEDs ; i++)  //configure LED pins to output; LED array
  {
    led[i] = 10-i;                    //Starts at DIGITAL PORT 4
    pinMode(led[i], OUTPUT);
    digitalWrite(led[i], LOW);
  }

  for(int i = 0; i<OUTPUTBUFFERSIZE; i++) //initailize output buffer to all 0's
  {
    outputBuffer[i] = 0;
  }





  //Calibration Setup
  if(!digitalRead(mode_sw)){
    calValue = calibrate();
    Serial.println(calValue);
  } 
  else { //Calibrated, read value from EEPROM
    int cal_temp = EEPROM.read(CAL_ADDR1); //Load high bit
    cal_temp <<= 8; //Shift 8 over.
    cal_temp += EEPROM.read(CAL_ADDR0); //Load low bit
    if(cal_temp < 1023 && cal_temp > 600){
      calibrated = true;
      calValue = cal_temp;
      Serial.println(calValue);
    } 
    else {
      calValue = 950;
      calibrated = false;
    }
  }

  //
  int temp_sense = 0;
  temp_sense += EEPROM.read(SENSITIVITY_ADDR);
  if(temp_sense < 50 && temp_sense > 0){
    sensitivity = temp_sense;
    Serial.print("Sense ");
    Serial.println(temp_sense);
  } 
  else {
    sensitivity = 20;
    EEPROM.write(SENSITIVITY_ADDR, (byte)(sensitivity&0xFF));
    Serial.println("SenseReset");
  }
}


//-----------------------------------------LOOP------------------------------------------------------------------------------
void loop() 
{
  checkButtons();  
  if(millis()%100 < 10)  //Drive Pulse, start roughly every 100ms ???Timing
  { 
    outputBuffer[outBuffIndex] = sendPulse();//-temp_min; //output = difference between highest and lowest values
    outBuffIndex++;

    if(outBuffIndex > OUTPUTBUFFERSIZE-1)//If outOfBounds reset outBuffIndex to 0
    {
      outBuffIndex = 0; //Cap at OutputBufferSize (including location 0)
    }

    //find average of ouputBuffer 
    averageValue = 0;
    for(int i = 0; i < OUTPUTBUFFERSIZE; i++) 
    {
      averageValue += outputBuffer[i];
    }
    averageValue /= OUTPUTBUFFERSIZE;
    outputValue = calValue - averageValue;

    Serial.println(outputValue); // ***************FOR TESTING***********************
  }
  if((millis()+60)%100 == 0) {
    setSignalStrength();
    ledDisplayUpdate();
    motorUpdater();
  } else {
    motorRunOnce = 1;
  }
  
  if((millis()+50)%500 == 0) { //Check Buttons and set LEDs ????????Timing
    if(buttons_run_once)
    {
      int lastSensitivity = sensitivity;
      if(up_button_hit) //adjust sensitivity state
      {
        sensitivity++;
        if(sensitivity > 49){
          sensitivity = 49;
        }
      }

      if(down_button_hit) //adjust sensitivity state
      {
        sensitivity--;
        if(sensitivity < 1) //no lower than 0
        {
          sensitivity = 1;
        } 
      }
      if(sensitivity != lastSensitivity){
        EEPROM.write(SENSITIVITY_ADDR, (byte)(sensitivity&0xFF));
      }      
      if(mode_button_hit) //If mode was pressed
      {
        int buttonHoldCount = 0; 
         while(!digitalRead(mode_sw)) //While mode button is held
         {
          digitalWrite(mode_led,HIGH); //mode led on
          delay(100);
          buttonHoldCount+=100; //count time
         }

         if(buttonHoldCount>=3000) // If mode button was held for greated than 3 seconds
         {
           motorOn = !motorOn; //toggle motor on/off
           motorCount = 0;
         }
         
          digitalWrite(mode_led,LOW); //mode led off when button released
          
       }
      mode_led_state = 0;  //make 0
      clearButtons();
      buttons_run_once = false;
    }
  }

} 
//------------------------- END OF LOOP --------------------------------------------------------------------------------  

//------------------------- Calibrate -------------------------------------------------------------------------------- 
int calibrate(void)
{
  int calOut = 0;
  digitalWrite(mode_led, HIGH);
  Serial.println("calibrating"); // ***************FOR TESTING***********************
  while(!digitalRead(mode_sw)) //Keep going while the read button is held down
  {        
    calOut = sendPulse();
  }
  Serial.print("calValue = ");
  Serial.println(calOut); // ***************FOR TESTING***********************
  //Maybe add some error checking here.
  calValue = calOut;
  EEPROM.write(CAL_ADDR0, (byte)(calValue&0xFF));
  EEPROM.write(CAL_ADDR1, (byte)(calValue>>8));
  calibrated = true;
  digitalWrite(mode_led, LOW);
  return calOut;
}


//------------------------ SET SIGNAL STRENGTH ------------------------------------------------------------------------------- 
void setSignalStrength(void) //SignalStrength based on analog input, sensorHigh, number of led groups in signal array
{
  sampleRange = 0;
  for(int i=1; i<=numberOfLEDs; i++) //for # LEDs
  {
    if(outputValue>=i*sensitivity) //if sensor value is greater or equal to activation strength for that LED, update sampleRange; updates from lowest to highest range
    {
      sampleRange = i;
    }
  }
  
}




//---------------------UPDATE LED ARRAY, MODE LED, & BATT LED--------------------------------------------------------------------------------
void ledDisplayUpdate(void) //Update LEDs based on SignalStrength
{
  for(int i=0; i<numberOfLEDs; i++)
  {
    if(sampleRange>i)
    {
      digitalWrite(led[i], HIGH);
    }
    else
    {
      digitalWrite(led[i], LOW);
    }
  }
  
  digitalWrite(mode_led, mode_led_state);
  digitalWrite(batt_led, batt_led_state);
  
  }

//---------------------Motor Updater // Controls timing and functionality of motor-------------------------------------------------------------------------------------------------------------
void motorUpdater(void)
{
 //digitalWrite(motor_port,LOW);
// if(((millis()+90)%100)){      // timing********
    if(motorOn){ //if motor is on
      if(!motorRunOnce){
        return; //Break if already run
      }
      motorRunOnce = 0;
      if(sampleRange>=numberOfLEDs/2+1){ //and if sampleRange above pulse threshold
        motorState = true;
      } else{
        motorState = false;
      }
      
      if(motorState) {
        if (motorCount < 1){
          //digitalWrite(motor_port,HIGH); 
          motor_on_state = 1;
          batt_led_state = 1;
        } else {
          //digitalWrite(motor_port,LOW);
          motor_on_state = 0;
          batt_led_state = 0;
        }
        motorCount++;
        if(motorCount > 9){
          motorCount = 0;
        }
      } else {
        motor_on_state = 0;
        batt_led_state = 0;
      }
   } else {
    //digitalWrite(motor_port,LOW);
    motor_on_state = 0;
    batt_led_state = 0;
    motorCount = 0;
  }
 //} else {
 //  motorRunOnce = 1;
 //}
 digitalWrite(motor_port, motor_on_state);
}

//---------------------CHECK BUTTONS-------------------------------------------------------------------------------------------------------------
void checkButtons(void) //checks the state of button inputs(A3 - A5) mode_button_hit_hitand updates button state as high for pressed buttons
{
  if(!digitalRead(mode_sw))   //A3 
  {
    mode_button_hit = 1;
  }
  if(!digitalRead(down_sw))  //A4
  {
    down_button_hit = 1;
  }
  if(!digitalRead(up_sw))    //A5
  {
    up_button_hit = 1;
  }
  if((millis()+70)%100 == 0)//????Timing
  { 
    buttons_run_once = true;  
  }
}




//---------------------CLEAR BUTTONS-----------------------------------------------------------------------------------------------------
void clearButtons(void) //resets buttons to an unpressed state
{
  mode_button_hit = 0;  
  up_button_hit = 0;
  down_button_hit = 0;
}
//---------------------SEND PULSE-----------------------------------------------------------------------------------------------------
int sendPulse(void){
  int output = 0;
  digitalWrite(g_drive, HIGH);//pulse coil
  delayMicroseconds(25);
  digitalWrite(g_drive,LOW);
  delayMicroseconds(50); //it might be worth removing this part completely 
  int temp_min = 1050;
  int temp_max = 0;
  int temp_value = 0;

  for(int i = 0; i < NUMBEROFREADS; i++) //About 100us per read
  { 
    //readBuffer[i] = analogRead(g_input); //Use this for DSP
    temp_value = analogRead(g_input); 
    temp_min = min(temp_min, temp_value); //if this is the lowest read, update
    temp_max = max(temp_max, temp_value); //if this is the highest read, update
  }

  output = temp_max;//-temp_min; //output = difference between highest and lowest values
  return output;



}


