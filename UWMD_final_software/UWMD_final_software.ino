//**************************************************************//
//  Name	:                         	 
//  Author  :  
//  Date	: 04 Nov, 2013    
//  Modified:                            	 
//  Version : 2.0                                        	 
//  Notes   :       	//
//      	:                      	 
//****************************************************************

#define NUMBEROFREADS 200 //About 20ms; About 100us per read
#define OUTPUTBUFFERSIZE 4 //
#define SENSORHIGH 1023 //

void setSignalStrength(void);
void ledDisplayUpdate(void);
void checkButtons(void);
void clearButtons(void);
void calibrate(void);

//boolean calibrated = false;

int sensorMax; //highest readable value at current sensitivity (clipping value)
int sensitivity = 3; //formerly sensitivity; Can be manipulated to effect display sensitivity; # levels = numberOfLevels
int numberOfLevels = 5; //Sensitivity Levels
int sampleRange;
int averageValue; //average of ouputBuffer array

int numberOfLEDs = 5; //number of led groups in signal array
int led[5]; //Size = numberOfLEDs

int g_drive = 3; //coil output
int g_input = A0; //coil input

int mode_led = 9;//mode output
int mode_sw = A3;//mode input
int mode_state = 0;
int mode_led_state = 0;
char mode_button_hit = 0;

int up_sw = A5;  //up button in
char up_button_hit = 0;
int down_sw = A4;//down button in
char down_button_hit = 0;

int batt_led = 10;// battery display
int batt_led_state = 1;
//int battLevel = 1;
int calValue = 950;
int outputValue = 0;

boolean buttons_run_once = false;


int readBuffer[NUMBEROFREADS];
int outputBuffer[OUTPUTBUFFERSIZE];
int outBuffLoc = 0;



//------------------------------------------SET UP-------------------------------------------------------------------------------
void setup() 
{ 
   //set buttons as inputs
    pinMode(mode_sw, INPUT);              //A3
    pinMode(down_sw, INPUT);              //A4
    pinMode(up_sw, INPUT);                //A5

    pinMode(mode_led, OUTPUT);            //configure additional state LEDs for Mode transitions &
    pinMode(batt_led, OUTPUT);            //battery state
  
    pinMode(g_input,INPUT);               //setup coil reading pin
    pinMode(g_drive,OUTPUT);              //setup driving interface for coil
    digitalWrite(g_drive, LOW);           //intialize as low
    analogReference(INTERNAL);            //reference voltage between 1.0v - 1.2v
    
    
    for (int i = 0; i<numberOfLEDs ; i++)  //configure LED pins to output; LED array
    {
	led[i] = i+4;                    //Starts at DIGITAL PORT 4
	pinMode(led[i], OUTPUT);
	digitalWrite(led[i], LOW);
    }
    
    for(int i = 0; i<OUTPUTBUFFERSIZE; i++) //initailize output buffer to all 0's
    {
      outputBuffer[i] = 0;
    }
    
    //sensorMax = (sensitivity*SENSORHIGH)/numberOfLevels;
    
    // initialize serial communication at 9600 bits per second:     ******FOR TESTING*******
    Serial.begin(9600);
}


//-----------------------------------------LOOP------------------------------------------------------------------------------
void loop() 
{
  checkButtons();
  if(millis()%100 < 10)  //Drive Pulse, start roughly every 100ms ???Timing
  { 
    digitalWrite(g_drive, HIGH);//pulse coil
    delayMicroseconds(25);
    digitalWrite(g_drive,LOW);
    delayMicroseconds(50); //it might be worth removing this part completely 
    int temp_min = 1050;
    int temp_max = 0;
    
    for(int i = 0; i < NUMBEROFREADS; i++) //About 100us per read
    { 
      readBuffer[i] = analogRead(g_input);
      temp_min = min(temp_min, readBuffer[i]); //if this is the lowest read, update
      temp_max = max(temp_max, readBuffer[i]); //if this is the highest read, update
    }
        
    outputBuffer[outBuffLoc] = temp_max;//-temp_min; //output = difference between highest and lowest values
    outBuffLoc++;
    outBuffLoc &= OUTPUTBUFFERSIZE-1; //Cap at OutputBufferSize (including location 0)
    
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
  if((millis()+60)%500 == 0)
  {
    setSignalStrength();
    ledDisplayUpdate(); 
  }     
  if((millis()+50)%500 == 0) //Check Buttons and set LEDs ????????Timing
  { 
    if(buttons_run_once)
    {
      if(up_button_hit) //adjust sensitivity state
      {
        sensitivity++;
        
        if(sensitivity > 5) //no higher than 5
        {
          sensitivity = 5;
        }
      sensorMax = (sensitivity*SENSORHIGH)/numberOfLevels; //update sensorMax
      }
      
      if(down_button_hit) //adjust sensitivity state
      {
        sensitivity--;
        if(sensitivity < 0) //no lower than 0
        {
          sensitivity = 0;
        }
      sensorMax = (sensitivity*SENSORHIGH)/numberOfLevels; //update sensorMax  
      }
      
      if(mode_button_hit)
      {
        calibrate();
        mode_led_state ^= 1; //if 1, make 0
        batt_led_state ^= 1; //if 1, make 0
      }
      
      clearButtons();
      buttons_run_once = false;
    }
  }
  
} 
//------------------------- END OF LOOP --------------------------------------------------------------------------------  
  
//------------------------- Calibrate -------------------------------------------------------------------------------- 
void calibrate(void)
{
  mode_led_state = 1;
  digitalWrite(mode_led, mode_led_state);
  while(true)
  {
    if(!digitalRead(mode_sw))   //exit calibrate routine if mode button is pressed; A3 
    {
      break;
    }
  }
  // calibrated = true;
}
 
 
//------------------------ SET SIGNAL STRENGTH ------------------------------------------------------------------------------- 
 void setSignalStrength(void) //SignalStrength based on analog input, sensorHigh, number of led groups in signal array
 {
   for(int i=0; i<=numberOfLEDs; i++) //for # LEDs
   {
      if(averageValue>=(i*sensorMax/numberOfLEDs)) //if sensor value is greater or equal to activation strength for that LED, update sampleRange; updates from lowest to highest range
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



