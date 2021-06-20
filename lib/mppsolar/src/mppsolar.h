/*
  mppsolar.h - Library for Interacting with MPP Solar.
  Focusing specifically on model LV5048 to start.
  Created by Tom Tijerina Jul 29, 2020.
  Released into the public domain.
*/
#ifndef mppsolar_h
#define mppsolar_h
#include "Arduino.h"
#include "SoftwareSerial.h"

// Include Circularbuffer, and limit it to 255.
#define CIRCULAR_BUFFER_XS
#include <CircularBuffer.h>

#define MAXIMUM_BUFFER_SIZE 155


class mppsolar
{
public:
  mppsolar(SoftwareSerial &InverterSerial);
  bool Send(String command);             //Send a command to the inverter. Returns false if command is rejected.
  bool Ready();                          //Ready to execute another command.
  void Loop();                           //Execute the loop
  bool WaitingForCommand();              //Are we waiting for feedback on the last sent command?
  char Response[MAXIMUM_BUFFER_SIZE];    // an array to store the Response.
  bool ResponseReady;                    //Response is ready from last command.
  int ResponseLength;                    //Total length of response.
  void SetAutomaticRetry(int new_Limit); //Automatically retry on failures.

  // From ???
  String serialNumber = "";

  //For QPIRI
  unsigned long QPIRI_millis; //Last time it was updated.
  float Settings_ACInputVoltage;
  float Settings_ACInputCurrent;
  float Settings_ACOutputVoltage;
  float Settings_ACOutputFrequency;
  float Settings_ACOutputCurrent;
  int Settings_ACOutputApparentPower;
  int Settings_ACOutputActivePower;
  float Settings_BatteryVoltage;
  float Settings_BatteryRechargeVoltage;
  float Settings_BatteryUnderVoltage;
  float Settings_BatteryBulkChargeVoltage;
  float Settings_BatteryFloatChargeVoltage;

  int Settings_BatteryType; //option
  int Settings_MaxACChargingCurrent;
  int Settings_MaxChargingCurrent;
  int Settings_InputVoltageRange;     //option
  int Settings_OutputSourcePriority;  //option
  int Settings_ChargerSourcePriority; //option
  int Settings_MaxParallelUnits;
  int Settings_MachineType; //option
  int Settings_Topology;    //option
  int Settings_OutputMode;  //option
  float Settings_BatteryRedischargeVoltage;
  int Settings_PVOKCondition;  //option
  int Settings_PVPowerBalance; //option
  int Settings_MaxChargingTimeatCVStage;
  // From Qpigs
  unsigned long QPIGS_millis; //Last time it was updated.

  float acInputVoltage;
  float acInputFrequency;
  float acOutputVoltage;
  float acOutputFrequency;
  int acOutputApparentPower;
  int acOutputActivePower;
  int acOutputLoad;
  int busVoltage;
  float batteryVoltage;
  int batteryChargingCurrent;
  int batteryCapacity;
  int inverterHeatSinkTempature;
  float pvInputCurrent;
  float pvInputVoltage;
  float batteryVoltageFromscc;
  int batteryDischargeCurrent;
  bool issbuPriorityVersionAdded;
  bool isConfigurationChanged;
  bool issccFirmwareUpdated;
  bool loadOn;
  bool isBatteryVoltageToSteadyWhileCharging;
  bool isCharginOn;
  bool issccChargingOn;
  bool isacChargingOn;
  int RSV1;
  int RSV2;
  int pvInputPower;
  bool isChargingTofloat;
  bool isSwitchedON;
  bool isReserved;

  //From QPIGS2
  unsigned long QPIGS2_millis; //Last time it was updated.

  float L2acInputVoltage;
  float L2acInputFrequency;
  float L2acOutputVoltage;
  float L2acOutputFrequency;
  int L2acOutputApparentPower;
  int L2acOutputActivePower;
  int L2acOutputLoad;
  int pv2batteryChargingCurrent;
  int pv2InputCurrent;
  float L2batteryVoltage;
  bool isL2sccOk;
  bool isL2ACChargingOn;
  bool isL2SccChargingOn;
  bool isL2LineNotOk;
  bool isL2LoadOn;

  //From QPIWS

  unsigned long QPIWS_millis; //Last time it was updated.

  bool InverterFault = false;
  bool BusOverFault = false;
  bool BusUnderFault = false;
  bool BusSoftFailFault = false;
  bool LineFailWarning = false;
  bool OPVshortWarning = false;
  bool InverterVoltageTooLowFault = false;
  bool InverterVoltageTooHighFault = false;
  bool OverTemperatureFault = false;
  bool fanLockedFault = false;
  bool BatteryVoltageToHighFault = false;
  bool BatteryLowAlarmWarning = false;
  bool BatteryUnderShutdownWarning = false;
  bool OverloadFault = false;
  bool EEPROMFault = false;
  bool InverterOverCurrentFault = false;
  bool InverterSoftFailFault = false;
  bool SelfTestFailFault = false;
  bool OPDCvoltageOverFault = false;
  bool BatOpenFault = false;
  bool CurrentSensorFailFault = false;
  bool BatteryShortFault = false;
  bool PowerLimitWarning = false;
  bool PVVoltageHighWarning = false;
  bool MPPTOverloadFault = false;
  bool MPPTOverloadWarning = false;
  bool BatteryTooLowToChargeWarning = false;

private:
  SoftwareSerial &_InverterSerial;
  bool _firstRun = true;
  int _retries = 0;                         //How many times command has been retried.
  int _maximumRetries = 5;                  //How many times to try again on failure.
  CircularBuffer<String, 5> _commandBuffer; // buffer capacity is 5
  String _userCommand = "";                 //Used to store the users command.
  bool _userCommandRunning;
  void _SendCommand(String command);    //Send a command to the inverter.
  unsigned long _CommandTimestamp = 0;  //When the command was first sent.
  unsigned long _CommandTimeout = 1000; //How long before we assume we need to resend the command.

  unsigned long _prev_millis = 0;
  unsigned int _internalDelay = 2000; //Refresh every second or so.

  char _bufferedChars[MAXIMUM_BUFFER_SIZE]; // an array to store the received data
  int _bufferedCharsIndex;
  bool _bufferAvailable = true;        //Receive buffer is accepting incoming data.
  bool _receivedAvailable = false;     //Lets us know when we have data ready to process.
  void _ParseResponse(String command); //Parse response

  String _pendingCommand = ""; //Last command sent, or none.
  bool _strtob(char input);    //Used to convert a char of either "1" or "0" into boolean values.
  uint16_t _Calculate_CRC(uint8_t *pin, uint8_t len);
  void _BufferResponse();
  void _ValidateResponse();
  int _RetryCommand();
};





#endif
