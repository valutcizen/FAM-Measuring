#include <SoftwareSerial.h>
#include <Adafruit_MAX31865.h> // https://github.com/adafruit/Adafruit_MAX31865

#define DEBUG
//#define PROGRAM_BLUETOOTH

#define BluetoothEnablePin 3
#define BluetoothRxPin 4
#define BluetoothTxPin 5
#define BluetoothStatePin 6
#define TemperatureConventerDataReadyPin 9
#define TemperatureConventerCsPin 10
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

struct Mes {
  uint8_t fault = 0;
  float average = 0;
  float variance = 0;
} MesResult;

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
#ifdef PROGRAM_BLUETOOTH
#endif    

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
  pinMode(BluetoothEnablePin, OUTPUT);
  pinMode(BluetoothTxPin, OUTPUT);
  pinMode(BluetoothRxPin, INPUT);
  pinMode(BluetoothStatePin, INPUT);
#ifdef PROGRAM_BLUETOOTH
  BluetoothSerial.begin(38400);
#else
  BluetoothSerial.begin(9600);
#endif
  BluetoothSerial.listen();
  digitalWrite(BluetoothEnablePin, HIGH);
}

inline void TemperatureConventerSetup()
{
  TemperatureConventer.begin(MAX31865_4WIRE);
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

inline void ResendAtCommands()
{
  while (Serial.available())
  {
    char r = Serial.read();
    Serial.write(r);
    BluetoothSerial.write(r);
  }
  while (BluetoothSerial.available())
    Serial.write(BluetoothSerial.read());
}
    
inline void SendBluetoothFault(uint8_t& fault)
{
  MesResult.fault = fault;
  MesResult.average = 0;
  MesResult.variance = 0;
  BluetoothSerialWrite();
}
    
inline void SendBluetoothFault(float& average, float& variance)
{
  MesResult.fault = 0;
  MesResult.average = 0;
  MesResult.variance = 0;
  BluetoothSerialWrite();
}

inline void BluetoothSerialWrite()
{
  BluetoothSerial.write(MesResult.fault);
  BluetoothSerial.write(MesResult.average);
  BluetoothSerial.write(MesResult.variance);
}

