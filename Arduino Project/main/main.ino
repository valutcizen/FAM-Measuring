#include <SoftwareSerial.h>
#include <Adafruit_MAX31865.h> // https://github.com/adafruit/Adafruit_MAX31865

#define DEBUG

#define BluetoothRxPin 10
#define BluetoothTxPin 11
#define TemperatureConventerCsPin 10
#define TemperatureConventerDataReadyPin 9
#define RREF 430.0
#define RTEMP0 100.0
#define MesQuantity 50
#define MesPeriodMs 40
#define StatisticsPeriod 200

SoftwareSerial BluetoothSerial(BluetoothRxPin, BluetoothTxPin);
Adafruit_MAX31865 TemperatureConventer(TemperatureConventerCsPin);
float Temperatures[MesQuantity];
int MesCounter = 0;
bool MesGood = false;
unsigned long MesTimestamp = 0;
unsigned long StatTimestamp = 0;
unsigned long now = 0;

void setup()
{
#ifdef DEBUG
  DebugSerialSetup();
#endif

  BluetoothSerialSetup();
  TemperatureConventerSetup();
  MesPeriodSetup();
}

void loop()
{
  float temp = TemperatureConventer.temperature(RTEMP0, RREF);
  uint8_t fault = TemperatureConventer.readFault();

  if (fault)
    FaultService(fault);
  else
    MeasureService(temp);

  WaitUntilMesPeriod();
}

inline void FaultService(uint8_t& fault)
{
#ifdef DEBUG
  PrintDebugFault(fault);
#endif

  SendBluetoothFault(fault);
  ResetMeasurements();
}

inline void MeasureService(float& temp)
{
  AddMeasure(temp);

  if (MesGood && (now - StatTimestamp > StatisticsPeriod))
    PrintBluetoothStatistics();
}

inline void WaitUntilMesPeriod()
{
  do
  {
    now = millis();
  } while (now - MesTimestamp < MesPeriodMs);
  MesTimestamp = now;
}

inline void DebugSerialSetup()
{
  Serial.begin(9600);
  Serial.println("Serial port initialized.");
}

inline void BluetoothSerialSetup()
{
  BluetoothSerial.begin(4800);
  Serial.println("Serial port for bluetooth initialized.");
}

inline void TemperatureConventerSetup()
{
  TemperatureConventer.begin(MAX31865_4WIRE);
  Serial.println("Temperature conventer initialized.");
}

inline void MesPeriodSetup()
{
  MesTimestamp = millis();
  StatTimestamp = MesTimestamp;
}

inline void PrintDebugFault(uint8_t& fault)
{
  Serial.print("Fault 0x"); Serial.println(fault, HEX);
  if (fault & MAX31865_FAULT_HIGHTHRESH) {
    Serial.println("RTD High Threshold"); 
  }
  if (fault & MAX31865_FAULT_LOWTHRESH) {
    Serial.println("RTD Low Threshold"); 
  }
  if (fault & MAX31865_FAULT_REFINLOW) {
    Serial.println("REFIN- > 0.85 x Bias"); 
  }
  if (fault & MAX31865_FAULT_REFINHIGH) {
    Serial.println("REFIN- < 0.85 x Bias - FORCE- open"); 
  }
  if (fault & MAX31865_FAULT_RTDINLOW) {
    Serial.println("RTDIN- < 0.85 x Bias - FORCE- open"); 
  }
  if (fault & MAX31865_FAULT_OVUV) {
    Serial.println("Under/Over voltage"); 
  }
}

inline void PrintBluetoothStatistics()
{
  StatTimestamp = now;
  float average = GetAverage();
  float variance = GetVariance(average);

#ifdef DEBUG
  Serial.print("A: "); Serial.print(average); Serial.print(" 100V: "); Serial.println(variance*100);
#endif
}

inline void ResetMeasurements()
{
  MesCounter = 0;
  MesGood = false;
}

inline void AddMeasure(float& temp)
{
  Temperatures[MesCounter++] = temp;

  if (MesCounter >= MesQuantity)
  {
    MesCounter = 0;
    MesGood = true;
  }
}

inline float GetAverage()
{
  float result = 0;

  for (int i = 0; i < MesQuantity; ++i)
    result += Temperatures[i];

  return result / MesQuantity;
}

inline float GetVariance(float& average)
{
  float result = 0;

  for (int i = 0; i < MesQuantity; ++i)
    result += pow(Temperatures[i] - average, 2);

  return result / MesQuantity;
}

inline void SendBluetoothFault(uint8_t& fault)
{
  
}

