#include <SoftwareSerial.h>
#include <Adafruit_MAX31865.h> // https://github.com/adafruit/Adafruit_MAX31865

#define BluetoothRxPin 10
#define BluetoothTxPin 11
#define TemperatureConventerCsPin 3
#define TemperatureConventerDataReadyPin 9
#define RREF 430.0
#define RTEMP0 100.0
#define MesQuantity 20
#define MesPeriodMs 200

SoftwareSerial BluetoothSerial(BluetoothRxPin, BluetoothTxPin);
Adafruit_MAX31865 TemperatureConventer(TemperatureConventerCsPin);
float Temperatures[MesQuantity];
int MesCounter = 0;
bool MesGood = false;
unsigned long timestamp = 0;
unsigned long now = 0;

void setup()
{
  DebugSerialSetup();
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

inline void FaultService(uint8_t fault)
{
  PrintDebugFault(fault);
  SendBluetoothFault(fault);
  ResetMeasurements();
}

inline void MeasureService(float temp)
{
  PrintDebugTemp(temp);
  AddMeasure(temp);
  
  if (MesGood)
    PrintBluetoothTemp();
}

inline void WaitUntilMesPeriod()
{
  do
  {
    now = millis();
  } while (now - timestamp < MesPeriodMs);
  timestamp = now;
}

inline void DebugSerialSetup()
{
  Serial.begin(57600);
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
  timestamp = millis();
}

inline void PrintDebugFault(uint8_t fault)
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

inline void PrintDebugTemp(float temp)
{
  Serial.print("Measured: ");
  Serial.println(temp);
}

inline void ResetMeasurements()
{
  MesCounter = 0;
  MesGood = false;
}

inline void AddMeasure(float temp)
{
  Temperatures[MesCounter++] = temp;

  if (MesCounter >= MesQuantity)
  {
    MesCounter = 0;
    MesGood = true;
  }
}

inline void SendBluetoothFault(uint8_t fault)
{
  
}

inline void PrintBluetoothTemp()
{
  
}

