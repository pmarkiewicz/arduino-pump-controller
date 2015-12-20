#include <Arduino.h>

#define LED_PIN 13
#define PUMP_PIN 52
#define PRESSURE_IN A0
#define NO_OF_SAMPLES 10
#define MAX_ON 40000
#define MIN_OFF 20000
#define MAX_INIT_PRESSURE 40

int max_pressure;
int min_pressure;
int offset = 18;

int getSensorValue()
{
  int sensorValue = 0;
  
  for (int i = 0; i < NO_OF_SAMPLES; ++i)
  { 
     sensorValue += analogRead(PRESSURE_IN);
     delay(2);
  }

    return (sensorValue / NO_OF_SAMPLES) - offset;
}

int getPressure()
{
  int sensorValue = getSensorValue();
  return map(sensorValue, 0, 1024, 0, 700);
}

int getMaxPressure()
{
  int pressure = 0;
  int new_pressure;
  
  digitalWrite(LED_PIN, 1);
  digitalWrite(PUMP_PIN, 0);

  for (int i = 0; i < 5; i++)
  {
    delay(1500);
    new_pressure = getPressure();
    
    if (new_pressure <= pressure)
    {
      pressure = 0;
      Serial.println("err");
      return 0;
    }
    
    pressure = new_pressure;
    
    if (pressure >= MAX_INIT_PRESSURE)
    {
      break;
    }
  }
  
  for (int i = 0; i < 20; i++)
  {
    delay(2000);
    pressure = getPressure();
    
    if (pressure >= MAX_INIT_PRESSURE)
    {
      break;
    }
  }

  digitalWrite(LED_PIN, 0);
  digitalWrite(PUMP_PIN, 1);
  
  return pressure -2;
}

void setup() {
  digitalWrite(PUMP_PIN, 1);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, 1);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);

  Serial.begin(57600);

  //delay(500);
  
  analogReference(DEFAULT);

  Serial.println("check max");
  max_pressure =  getMaxPressure();
  while (max_pressure <= 10)
  {
    delay(10000);
    max_pressure =  getMaxPressure();
    Serial.print("check max restarted: ");
    Serial.println(getPressure());
  }
  
  min_pressure = max_pressure - 10;
  
  Serial.print(min_pressure);
  Serial.print('\t');
  Serial.println(max_pressure);
}

long tm = 0;
boolean pump_on = false;
int cnt = 0;
int last_pressure = 0;
int test_pressure = 0;
char paramType = 0;
char paramValue = 0;
int paramInc = 0;

void changeParam() {
  switch(paramValue) {
    case '+':
      paramInc = 1;
      break;
    case '-':
      paramInc = -1;
      break;
    default:
      return;
  }
  
  if (paramType == 'D') {
    min_pressure += paramInc;
    Serial.print("New min limit: ");
    Serial.print(min_pressure);
    Serial.println();
  }
  else if (paramType == 'U') {
    max_pressure += paramInc;
    Serial.print("New max limit: ");
    Serial.print(max_pressure);
    Serial.println();
  }
}

void loop() {
int pressure = getPressure();
long dt = millis() - tm;
  
  if (Serial.available() > 0) {
      paramValue = Serial.read();
      switch(paramValue) {
        case 'd':
        case 'D':
          paramType = 'D';
          break;
        case 'u':
        case 'U':
          paramType = 'U';
          break;
        default:
          changeParam();
          paramValue = 0;
      }
  }
  
  if (cnt++ % 10 == 0)
  {
    cnt = 0;
    if ((pressure < last_pressure - 1) || (pressure > last_pressure + 1))
    {
      Serial.print(pressure);
      Serial.print("\t");
      Serial.println(dt/1000);
      last_pressure = pressure;
    }
  }
  
  if (pump_on)
  {
    if (dt > 2500 && test_pressure >= pressure)
    {
      pump_on = false;
      tm = millis();
      Serial.println("err");
    }
    
    if (dt > MAX_ON)
    {
      pump_on = false;
      tm = millis();
      Serial.println("timeout");
    }
    else if (pressure > max_pressure)
    {
      pump_on = false;
      Serial.print("max -> ");
      Serial.print(dt/1000);
      Serial.println("s");
    }
  }  
  else if (dt < MIN_OFF)
  {
    /*if (pressure < min_pressure)
    {
      Serial.println("min time");
    }*/
    return;
  }  
  else if (pressure < min_pressure)
  {
    pump_on = true;
    test_pressure = pressure;
    Serial.print("min -> ");
    Serial.print(dt/1000);
    Serial.println("s");
    tm = millis();
  }
 
  digitalWrite(LED_PIN, pump_on);
  digitalWrite(PUMP_PIN, !pump_on);
   
  delay(200);       
}
