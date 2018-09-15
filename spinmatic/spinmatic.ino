
// Display
#include <ESP_SSD1306.h>    // Modification of Adafruit_SSD1306 for ESP8266 compatibility
#include <Adafruit_GFX.h>   // Needs a little change in original Adafruit library (See README.txt file)
#include <SPI.h>            // For SPI comm (needed for not getting compile error)
#include <Wire.h>           // For I2C comm, but needed for not getting compile error
// Servo
#include <Servo.h> 
// Add Encoder Support
#include <Encoder.h> 

// PIN Definitions 
// =======================================
#define OLED_RESET  16  // Pin 15 -RESET digital signal
#define BEEPER_PIN D4 
#define SERVO_PIN D8
// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
#define ENCODER_PINA  D5
#define ENCODER_PINB  D6
#define ENCODER_PINBUTTON D3

 
Servo myservo;  // create servo object to control a servo 
ESP_SSD1306 display(OLED_RESET); // FOR I2C Display
Encoder myEnc(ENCODER_PINA, ENCODER_PINB);


// Timer
// https://playground.arduino.cc/Main/CountDownTimer
unsigned long Watch, _micro, timeMicros = micros();
unsigned int Clock = 0, R_clock;
boolean Reset = false, Stop = false, Paused = false;
volatile boolean timeFlag = false;
boolean rotated=false, directionR=false;

int interval =10;  // rotate every X seconds
int INITIAL_DURATION =30; // duration of development in seconds
int Twists =2;
int TIMER_INCREMENT = 15; // seconds

// Encoder Values
long encoderPrevValue  = 0;
long encoderValue = 0;
bool buttonConfirm = false;
bool SetupTimerMode = true;

void setup() 
{ 
  // Start Serial
  Serial.begin(115200);

  // SSD1306 Init
  display.begin(SSD1306_SWITCHCAPVCC);  // Switch OLED

  StartUpDisplay();

 // myservo.attach(SERVO_PIN);  // attaches the servo on D8 to the servo object 

  StartUpSound();

  // Timer
 
  InitTimer();
} 

 
void loop() 
{ 
    CheckButtonPress();
    
    if (SetupTimerMode)
    {
      int duration = SetupCountdownTimer(INITIAL_DURATION, TIMER_INCREMENT);
      SetTimer(duration); 
    }
    else
    {
      CountDownTimer(); // Run Timer
    }
    
     
    int seconds = ShowSeconds();
    int minutes = ShowMinutes();
 
    if (!Stop)
    {
        if (TimeHasChanged()) 
        {
            UpdateDisplay(minutes, seconds, true);
        }

        // Rotate Servo
        DoIntervalTask(minutes, seconds);
        
        if (minutes==0 && seconds==59)
        {
          LastminuteSound();
        }
    }
    else // Finished
    {   
      if (!buttonConfirm)
      {
          MiddleTextDisplay("Finished!");
          BeepSound();
          myservo.detach(); 
      }
    }  
}


// ENCODER
// ==========================
int GetEncoderValue()
{
  long _newPosition = myEnc.read();
  if (_newPosition != encoderPrevValue) {
    encoderPrevValue = _newPosition;
    Serial.println(_newPosition);
    encoderValue=_newPosition/4;
    encoderValue=abs(encoderValue);
  }
  return encoderValue;
}

// BUTTON
// ==========================
void CheckButtonPress()
{
  int buttonState = digitalRead(ENCODER_PINBUTTON);
  if (buttonState == LOW) {
    DoButtonLogic();
  }
}

void DoButtonLogic()
{
   if (SetupTimerMode)
   {
        BeepSound();
        myservo.attach(SERVO_PIN); 
        SetupTimerMode=false; // Start timer;
   }
   else
   {
        // Running Mode
        if (Stop)
        {
          // Finished
          ResetTimer();
          buttonConfirm=true;
          SetupTimerMode =true;
        }
        else
        {
          // Pause / Play
          Paused = !Paused;
          if (Paused) 
          {
            BeepSound();
            
          }
          delay(500);
        }
   }
}

int SetupCountdownTimer(int initDuration, int increment)
{
  int totalSeconds = GetEncoderValue()*15;
  int mins = (totalSeconds / 60) % 60;
  int secs=  totalSeconds - mins*60;
  UpdateDisplay(mins, secs, false);
  return totalSeconds;
}

// BEEPER SOUNDS
// ==========================
// Play tune on start up
void StartUpSound()
{
    tone(BEEPER_PIN, 2000, 400);
    delay(600);
    tone(BEEPER_PIN, 800, 400);
    delay(600);
    tone(BEEPER_PIN, 2000, 400);
}
void LastminuteSound()
{
   tone(D4,3000, 200);
}

// Default Beep
void BeepSound()
{
  tone(D4, 4200, 100);
}

// DISPLAY
// ==========================
void StartUpDisplay()
{
    // Show image buffer on the display hardware.
    // Since the buffer is intialized with an Adafruit splashscreen
    // internally, this will display the splashscreen.
    display.display();
    // Clear the buffer.
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Spinmatic V1.1");
    display.setTextSize(2);
    display.println("blog.glyons.at");
    display.display();
}

// Display the Countdown Time
void UpdateDisplay(int minutes, int seconds, bool showArrows)
{
    // Debug
    Serial.print(ShowHours());
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(":");
    Serial.print(seconds);
    Serial.print(":");
    Serial.print(ShowMilliSeconds());
    Serial.print(":");
    Serial.println(ShowMicroSeconds());

    int cursorY=3;
    if (!showArrows)
    {
      cursorY=20;
    }
    
    display.clearDisplay();
    if (minutes==0)
    {
      display.setCursor(30,cursorY);
      display.setTextSize(4);
      display.print(seconds);
      display.println("s");
    }
    else
    {
      display.setCursor(10,cursorY);
      display.setTextSize(3);
      display.print(minutes);
      display.setTextSize(2);
      display.print("m ");
      display.setTextSize(3);
      display.print(seconds);
      display.setTextSize(2);
      display.println("s");
    }

    if (showArrows)
    { 
        if (!Paused)
        {
            RotationArrowDisplay(directionR);
        }
    }
    if (Paused)
    {
        MiddleTextDisplay("Reset ?");
    }
    display.display(); 
}

// Display Rotation Direction
void RotationArrowDisplay(bool rotationDirection)
{
    display.setTextSize(2);
    if (rotationDirection)
    {
      display.setCursor(90,45);
      display.println("-->");
    }
    else
    {
      display.setCursor(10,50);
      display.println("<--");
    }
}
void BottomTextDisplay(char* Text)
{
  display.setTextSize(2);
  display.setCursor(10,50);
  display.println(Text);
}

void MiddleTextDisplay(char* Text)
{
    display.clearDisplay();
    display.setCursor(10,20);
    display.setTextSize(2);
    display.println(Text);
    display.display(); 
}

// Tasks
// ==========================
// Change servo direction on every interval
void DoIntervalTask(int minutes, int seconds)
{
    if ((seconds % interval == 0)) 
    {
        if (!rotated)
        {
          for (int r=1; r<=Twists; r++)
          {
            TurnServo();
          }
        }
        rotated=true;
    }
    else
    {
      rotated = false;
    }
}

void TurnServo()
{
    if (directionR)
    {
      Serial.println("Rotate Left");
      myservo.write(180); 
      directionR=false;
    }
    else
    {
      Serial.println("Rotate Right");
      fwd();
      directionR=true;
    }
}


 void fwd()
 {
   int pos;
    for(pos = 90; pos>=0; pos-=1)     // goes from 180 degrees to 0 degrees 
    {                                
      myservo.write(pos);              // tell servo to go to position in variable 'pos' 
      delay(5);                       // waits 15ms for the servo to reach the position 
    } 
 }

 void res()
 {
   int pos;
    for(pos = 0; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
    {                                  // in steps of 1 degree 
      myservo.write(pos);              // tell servo to go to position in variable 'pos' 
      delay(5);                       // waits 15ms for the servo to reach the position 
    } 
 }


// Timer
// ==========================
boolean CountDownTimer()
{
  static unsigned long duration = 1000000; // 1 second
  timeFlag = false;

  if (!Stop && !Paused) // if not Stopped or Paused, run timer
  {
    // check the time difference and see if 1 second has elapsed
    if ((_micro = micros()) - timeMicros > duration ) 
    {
      Clock--;
      timeFlag = true;

      if (Clock == 0) // check to see if the clock is 0
        Stop = true; // If so, stop the timer

     // check to see if micros() has rolled over, if not,
     // then increment "timeMicros" by duration
      _micro < timeMicros ? timeMicros = _micro : timeMicros += duration; 
    }
  }
  return !Stop; // return the state of the timer
}
 void ResetTimer()
{
  SetTimer(R_clock);
  Stop = false;
}

void InitTimer()
{
  Watch = micros(); // get the initial microseconds at the start of the timer
  timeMicros = micros(); // hwd added so timer will reset if stopped and then started
  Stop = false;
  Paused = false;
}

void StopTimer()
{
  Stop = true;
}

void StopTimerAt(unsigned int hours, unsigned int minutes, unsigned int seconds)
{
  if (TimeCheck(hours, minutes, seconds) )
    Stop = true;
}

void PauseTimer()
{
  Paused = true;
}

void ResumeTimer() // You can resume the timer if you ever stop it.
{
  Paused = false;
}



void SetTimer(unsigned int seconds)
{
 // StartTimer(seconds / 3600, (seconds / 3600) / 60, seconds % 60);
 Clock = seconds;
 R_clock = Clock;
 Stop = false;
}

int ShowHours()
{
  return Clock / 3600;
}

int ShowMinutes()
{
  return (Clock / 60) % 60;
}

int ShowSeconds()
{
  return Clock % 60;
}

unsigned long ShowMilliSeconds()
{
  return (_micro - Watch)/ 1000.0;
}

unsigned long ShowMicroSeconds()
{
  return _micro - Watch;
}

boolean TimeHasChanged()
{
  return timeFlag;
}

// output true if timer equals requested time
boolean TimeCheck(unsigned int hours, unsigned int minutes, unsigned int seconds) 
{
  return (hours == ShowHours() && minutes == ShowMinutes() && seconds == ShowSeconds());
}


