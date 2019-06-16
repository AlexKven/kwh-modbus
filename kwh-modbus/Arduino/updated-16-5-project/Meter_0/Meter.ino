#include <EEPROM.h>
#include <util/atomic.h>
#include "TimeManager.h"
#include "DenseShiftBuffer.hpp"

volatile unsigned long totalPulses;
unsigned long timeCountingPulses;
unsigned long pulsesLastMinute = 0;

//const bool CLEAR_EEPROM = true;
const double WATTHOURS_PER_PULSE = 0.0026944444465999994;
const double WATTHOURS_PER_UNIT = 0.0019073486;
const double UNITS_PER_PULSE = 1.412664914;
DenseShiftBuffer<uint32_t, 16> *cachedMinutes;

unsigned long getTimeMinutes()
{
  return mainTimeManager->getClock() / 60;
}

unsigned long getCachedMinute(int min)
{
  if (min >= cachedMinutes->getNumStored())
    return 0;
  return cachedMinutes->get(min);
}

int getNumMinutesStored()
{
  return cachedMinutes->getNumStored();
}

long safelyGetTotalPulses()
{
  long result;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    result = totalPulses;
  }
  return result;
}

void safelyIncrementTotalPulses()
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    totalPulses++;
  }
}

void safelyZeroTotalPulses()
{
  do
  {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      totalPulses = 0;
    }
  } while (safelyGetTotalPulses() != 0);
  timeCountingPulses = micros();
}

void pulse()
{
  safelyIncrementTotalPulses();
}

unsigned long getAndResetPulses()
{
  unsigned long pulses;
  pulses = safelyGetTotalPulses();
  Serial.println(pulses);
  unsigned long pulseDiff = pulses - pulsesLastMinute;
  pulsesLastMinute = pulses;
  return pulseDiff;
}

void onMinuteElapsedMeter()
{
  Serial.println(F("Minute passed."));
  double wattHours;
  unsigned long formattedWattHours;
  unsigned long pulses;
//  if (numMinutes == NUM_CACHED_HOURS * 60)
//      minuteStartIndex = (minuteStartIndex + 1) % (NUM_CACHED_HOURS * 60);
//    else
//      numMinutes++;
    pulses = safelyGetTotalPulses();
    unsigned long pulseDiff = pulses - pulsesLastMinute;
    //timeLastMinute = _millis;
//    wattHours = (double)pulseDiff * WATTHOURS_PER_PULSE;
    formattedWattHours = (double)pulseDiff * UNITS_PER_PULSE;
    cachedMinutes->push(formattedWattHours);
    
    Serial.print(F(" Pulses: "));
    Serial.print(pulseDiff);
    Serial.print(F(" Watthours: "));
//    Serial.println(wattHours);
//    Serial.print(F(" Formatted Watthours: "));
    Serial.println(formattedWattHours);
    Serial.print(F(" From RAM: "));
    Serial.println(getCachedMinute(0));
    pulsesLastMinute = pulses;
  
    for (int i = cachedMinutes->getNumStored() - 1; i >= 0 ; i--)
    {
      unsigned long time = getTimeMinutes() - i;
      formattedWattHours = cachedMinutes->get(i);
      wattHours = (double)formattedWattHours * WATTHOURS_PER_UNIT;
      Serial.print("Minute: ");
      Serial.print(time);
      Serial.print(" Energy: ");
      Serial.println(formattedWattHours);
    }
}
  

//bool isTimeCached(unsigned long time)
//{
//  int diff = (getCurrentTime() - time);
//  return diff < numMinutes && diff >= 0;
//}

void setupMeter()
{
//  cachedMinutes = new byte[60 * 3 * NUM_CACHED_HOURS];
//  for (int i = 0; i < 60 * 3 * NUM_CACHED_HOURS; i++)
//  {
//    cachedMinutes[i] = 0;
//  }
//  if (NUM_CACHED_HOURS == 2)
//  {
//    if (CLEAR_EEPROM)
//    {
//      clearEEPROM();
//      numMinutes = 0;
//      minuteStartIndex = 0;
//    }
//    else
//      readStateFromEEPROM();
//  }
  cachedMinutes = new DenseShiftBuffer<uint32_t, 16>(6);
  pulsesLastMinute = safelyGetTotalPulses();
  attachInterrupt(digitalPinToInterrupt(2), pulse, RISING);
  safelyZeroTotalPulses();
}

unsigned long lastUpdateTimeMS = 0;

void loopMeter()
{
//  unsigned long currentTimeMS = millis();
//  if (lastUpdateTimeMS == 0 || currentTimeMS - lastUpdateTimeMS >= 60000)
//  {
//    lastUpdateTimeMS = currentTimeMS;
//    onMinuteElapsedMeter();
//  }
  
  unsigned long _millis = millis();
  unsigned long _micros = micros();
  unsigned long pulses;
  if (_millis - lastUpdateTimeMS >= 60000 || (lastUpdateTimeMS == 0 && _millis > 0))
  {
    onMinuteElapsedMeter();
    lastUpdateTimeMS = _millis;
  }
  if (Serial.available())
  {
    while (Serial.available())
    {
      Serial.read();
    }
//    String content = "";
//    char character;
//
//    int len = 1;
//  
//    while(Serial.available()) {
//        character = Serial.read();
//        content.concat(character);
//        delay(2);
//        len++;
//    }
//    content.getBytes(curMessage, len);
//    messenger->enqueueMsg(255, curMessage);
    pulses = safelyGetTotalPulses();
    unsigned long t = _micros - timeCountingPulses;
    Serial.print(pulses);
    Serial.print(" pulses in ");
    Serial.print(t);
    Serial.println(" microseconds.");
  }
    
}
