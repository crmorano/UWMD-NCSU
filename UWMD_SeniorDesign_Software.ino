//**************************************************************//
//  Name    :                              
//  Author  :  
//  Date    : 04 Nov, 2013    
//  Modified:                                 
//  Version : 2.0                                             
//  Notes   :           //
//          :                           
//****************************************************************
#define NUMBEROFREADS 200 //About 20ms
void checkButtons(void);
void clearButtons(void);
void LEDsOut(void);

int sensorHigh = 1023;
int sensitivity = 0;
int sampleRange;
const int numberOfLEDs = 5;
int g_drive = 3;
//int led[numberOfLEDs]; //Size = numberOfLEDs + 1
int led_0 = 4;
int led_1 = 5;
int led_2 = 6;
int led_3 = 7;
int led_4 = 8;
int mode_led = 9;
int batt_led = 10;
int up_sw = A5;
int down_sw = A4;
int mode_sw = A3;
char mode_button = 0;
char up_button = 0;
char down_button = 0;
char buttons_run_once = 0;
//int analogSig = 0; //Analog 
//int battLevel = 1;
int mode_state = 0;
int mode_led_state = 0;
int batt_led_state = 1;
int led_state = 3;
int readBuffer[NUMBEROFREADS];
int outputBuffer[4] = {0,0,0,0};
int outBuffLoc = 0;
int sum = 0;
float sum2 = 0;
void setup() {
 
  //set pins to output; LED outputs will be 0 - number of LEDs
  pinMode(led_0, OUTPUT);
  digitalWrite(led_0, LOW);
  pinMode(led_1, OUTPUT);
  digitalWrite(led_1, LOW);
  pinMode(led_2, OUTPUT);
  digitalWrite(led_2, LOW);
  pinMode(led_3, OUTPUT);
  digitalWrite(led_3, LOW);
  pinMode(led_4, OUTPUT);
  digitalWrite(led_4, LOW);
  pinMode(up_sw, INPUT);
  pinMode(down_sw, INPUT);
  pinMode(mode_sw, INPUT);
  pinMode(mode_led, OUTPUT);
  pinMode(batt_led, OUTPUT);
  pinMode(g_drive,OUTPUT);
  digitalWrite(g_drive, LOW);  
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
 
  analogReference(INTERNAL);
 
}

void loop() {
  checkButtons();
  if(millis()%100 < 10){ //Drive Pulse, start roughly every 100ms
    digitalWrite(g_drive, HIGH);
    delayMicroseconds(25);
    digitalWrite(g_drive,LOW);
    delayMicroseconds(50); //it might be worth removing this completely part
    int temp_min = 1000;
    int temp_max = 0;
    for(int i = 0; i < NUMBEROFREADS; i++){ //About 100us per read
      readBuffer[i] = analogRead(A0);
      temp_min = min(temp_min, readBuffer[i]);
      temp_max = max(temp_max, readBuffer[i]);
    }
    
    outputBuffer[outBuffLoc] = temp_max-temp_min;
    outBuffLoc++;
    outBuffLoc &= 3; //Cap at 4
    sum = 0;
    for(int i = 0; i < 4; i++){
      sum += outputBuffer[i];
    }
    sum /= 4;
    Serial.println(sum);

    
    //Set the sampleRange (Signal Strength)
  
  
  }
  
  
  if((millis()+50)%500 == 0){ //Check Buttons and set LEDs
    if(buttons_run_once){
      if(up_button){
        led_state++;
        if(led_state > 5){
          led_state = 5;
        }
      }
      if(down_button){
        led_state--;
        if(led_state < 0){
          led_state = 0;
        }  
      }
      if(mode_button){
        mode_led_state ^= 1;
        batt_led_state ^= 1;
      }
   
      LEDsOut();
      clearButtons();
      buttons_run_once = 0;
    }
  }
  if((millis()+70)%100 == 0){ //clean up script
    buttons_run_once = 1;  
  }
  
}

void checkButtons(void){
  if(!digitalRead(mode_sw)){
    mode_button = 1;
  }
  if(!digitalRead(up_sw)){
    up_button = 1;
  }
  if(!digitalRead(down_sw)){
    down_button = 1;
  }
}

void clearButtons(void){
  mode_button = 0;  
  up_button = 0;
  down_button = 0;
}

void LEDsOut(void){
  switch(led_state){
    case 0:
      digitalWrite(led_0, LOW);
      digitalWrite(led_1, LOW);
      digitalWrite(led_2, LOW);
      digitalWrite(led_3, LOW);
      digitalWrite(led_4, LOW);
      break;
    case 1:
      digitalWrite(led_0, HIGH);
      digitalWrite(led_1, LOW);
      digitalWrite(led_2, LOW);
      digitalWrite(led_3, LOW);
      digitalWrite(led_4, LOW);
      break;
    case 2:
      digitalWrite(led_0, HIGH);
      digitalWrite(led_1, HIGH);
      digitalWrite(led_2, LOW);
      digitalWrite(led_3, LOW);
      digitalWrite(led_4, LOW);
      break;
    case 3:
      digitalWrite(led_0, HIGH);
      digitalWrite(led_1, HIGH);
      digitalWrite(led_2, HIGH);
      digitalWrite(led_3, LOW);
      digitalWrite(led_4, LOW);
      break;
    case 4:
      digitalWrite(led_0, HIGH);
      digitalWrite(led_1, HIGH);
      digitalWrite(led_2, HIGH);
      digitalWrite(led_3, HIGH);
      digitalWrite(led_4, LOW);
      break;      
    case 5:
      digitalWrite(led_0, HIGH);
      digitalWrite(led_1, HIGH);
      digitalWrite(led_2, HIGH);
      digitalWrite(led_3, HIGH);
      digitalWrite(led_4, HIGH);
      break;    
    default:
      digitalWrite(led_0, LOW);
      digitalWrite(led_1, LOW);
      digitalWrite(led_2, LOW);
      digitalWrite(led_3, LOW);
      digitalWrite(led_4, LOW);
      break;       
  }
  
  digitalWrite(mode_led, mode_led_state);
  digitalWrite(batt_led, batt_led_state);
}
