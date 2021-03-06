/*
* created by seldon on RC
* 
* Automated RODI Filling Station
*
* 55 Gallon Drum Level Monitoring Using Ultrasonic Sensor HC-SR04
*
*/

#include <Time.h>
#include <TimeLib.h>

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

int trigPin;            //Pin on Arduino that will be used to trigger the HC-SR04
int echoPin;            //Pin on Arduino that will be used to echo the HC-SR04
int PressureSwitchPin;  //Pin on Arduino that will be used to monitor the tank full pressure switch
int RODIManualStartPin; //Pin on Arduino that will be used to manually start the RODI Cycle

int FeedSolenoidPin;  
int FlushSolenoidPin;
int TankSolenoidPin;
int BoosterPumpPin;

int RODIInitialFlushTime = 120;
int RODIFlushTime = 30;
int RODIRunTime = 3600;

float TankHeight = 870.0;  // Height of the tank in mm from the bottom to the sensor

time_t InitialTime;

enum TankStatus
{
    TANKSTATUS_LOWLOW,  //0
    TANKSTATUS_LOW,     //1
    TANKSTATUS_NORMAL,  //2
    TANKSTATUS_HIGH,    //3
    TANKSTATUS_HIGHHIGH //4
};

enum RODIStatus
{
    RODISTATUS_OFF,           //0
    RODISTATUS_INITIALFLUSH,  //1
    RODISTATUS_FLUSHING,      //2
    RODISTATUS_RUNNING        //3
};

RODIStatus initialStatus;
RODIStatus status;

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

TankStatus TankLevelStatus(TankStatus level, float percent, float distToWater);

float HCSR04Read(int trigPin, int echoPin);

float WaterLevelPercent (float TankHeight, float distToWater);

bool PressureSwitchRead(int PressureSwitchPin);
bool RODIManualStartSwitchRead(int RODIManualStartPin);

RODIStatus RODIOperationalStatus(RODIStatus status, TankStatus TankLevel, bool PressureSwitch, bool RODIManualStart, time_t InitialTime, time_t CurrentTime);

void RODIOperation(RODIStatus status, int FeedSolenoidPin, int FlushSolenoidPin, int TankSolenoidPin, int BoosterPumpPin);

void LCDOutput(TankStatus TankLevel, float percent, RODIStatus status, time_t CurrentTime);


void setup() 
{

  //Serial Port begin
  Serial.begin (9600);

  lcd.begin();         // initialize the lcd for 20 chars 4 lines, turn on backlight
  
  // Define the IO Pins
  trigPin = 4;
  echoPin = 5;
  PressureSwitchPin = 6;
  FeedSolenoidPin = 7;
  FlushSolenoidPin = 8;
  TankSolenoidPin = 9;
  BoosterPumpPin = 10;
  RODIManualStartPin = 11;
  
  //Define inputs and outputs
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  pinMode(PressureSwitchPin, INPUT);
  pinMode(RODIManualStartPin, INPUT);
  
  pinMode(FeedSolenoidPin, OUTPUT);
  pinMode(FlushSolenoidPin, OUTPUT);
  pinMode(TankSolenoidPin, OUTPUT);
  pinMode(BoosterPumpPin, OUTPUT);

  initialStatus = RODISTATUS_OFF;
  status = RODISTATUS_OFF;

  RODIOperation(status,FeedSolenoidPin,FlushSolenoidPin,TankSolenoidPin,BoosterPumpPin);

  InitialTime = now();

  // ------- Quick 3 blinks of backlight  -------------
  for(int i = 0; i< 3; i++)
  {
    lcd.backlight();
    delay(250);
    lcd.noBacklight();
    delay(250);
  }
  lcd.backlight(); // finish with backlight on  

  //-------- Write characters on the display ------------------
  // NOTE: Cursor Position: Lines and Characters start at 0  
  lcd.setCursor(0,0); //Start at character 4 on line 0
  lcd.print("Tank Status: ");
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print("Percent Full: ");
  delay(1000);  
  lcd.setCursor(0,2);
  lcd.print("RODI Status: ");
  delay(1000);  
  lcd.setCursor(0,3);
  lcd.print("Current Time: ");
  delay(2000);
  
}

void loop() 
{

  bool PressureSwitch;
  PressureSwitch = PressureSwitchRead(PressureSwitchPin);
  
  //Serial.print("Pressure Switch: ");
  //Serial.print(PressureSwitch);
  //Serial.println();
  //Serial.println();
  
  bool RODIManualStart;
  RODIManualStart = RODIManualStartSwitchRead(RODIManualStartPin);

  float distToWater;  //Distance in mm
  distToWater = HCSR04Read(trigPin,echoPin);

  Serial.print("Distance to water (mm): ");
  Serial.print(distToWater);
  Serial.println();
  Serial.println();
  
  float percent;
  percent = WaterLevelPercent (TankHeight, distToWater);

  //Serial.print("Percent Full: ");
  //Serial.print(percent);
  //Serial.println();
  //Serial.println();
  
  TankStatus TankLevel;
  TankLevel = TankLevelStatus(TankLevel, percent, distToWater);

  //Serial.print("Tank Level Status: ");
  //Serial.print(TankLevel);
  //Serial.println();
  //Serial.println();

  if(initialStatus != status)
  {
    InitialTime = now();
    initialStatus = status;
    //Serial.print("Initial Time Reset");
    //Serial.println();
    //Serial.println();
  } 

  time_t CurrentTime;
  CurrentTime = now() - InitialTime;

  status = RODIOperationalStatus(status, TankLevel, PressureSwitch, RODIManualStart, InitialTime, CurrentTime);

  LCDOutput(TankLevel, percent, status, CurrentTime);

  delay(4000);

}
