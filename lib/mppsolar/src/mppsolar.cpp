/*
  mppsolar.cpp - Library for Interacting with MPP Solar.
  Focusing specifically on model LV5048 to start.
  Created by Tom Tijerina Jul 29, 2020.
  Released into the public domain.
*/

#include "mppsolar.h"
#include "SoftwareSerial.h"

// ELM327::ELM327(HardwareSerial &serial) : _HardSerial(serial)

mppsolar::mppsolar(SoftwareSerial &InverterSerial) : _InverterSerial(InverterSerial)
{
  _InverterSerial.begin(2400);
}
// Are we waiting on a command to finish?
bool mppsolar::WaitingForCommand()
{
  if (_pendingCommand == "")
  {
    return false;
  }
  else
  {
    return true;
  }
}
//Set the limit on the automatic retry.
void mppsolar::SetAutomaticRetry(int new_Limit)
{
  if (new_Limit >= 0)
    _maximumRetries = new_Limit;
}

void mppsolar::Loop()
{
  _BufferResponse();

  if (_firstRun)
  {
    _commandBuffer.push("QID");
    _firstRun = false;
  }
  if (_commandBuffer.isEmpty()) //No point in filling it up if we havn't finished what was previously requested.
  {
    // _commandBuffer.push("QPIGS2");
    // _commandBuffer.push("QPIGS");
    if (millis() - _prev_millis > _internalDelay) //This may not be needed.
    {
      _commandBuffer.push("QPIRI");
      _commandBuffer.push("QPIWS");
      _commandBuffer.push("QPIGS2");
      _commandBuffer.push("QPIGS");
      _prev_millis = millis();
    }
  }

  // If we are not waiting on a command to finish, run the next one. Preference given to the users commands.
  if (_pendingCommand.isEmpty())
  {

    if (_userCommand.length() > 0)
    {
      ResponseReady = false;
      _userCommandRunning = true;
      _SendCommand(_userCommand);
      _userCommand = "";
    }

    else if (_commandBuffer.isEmpty() == false)
    {
      _SendCommand(_commandBuffer.shift());
    }
  }
}

int mppsolar::_RetryCommand()
{
  _bufferedCharsIndex = 0;
  _bufferAvailable = true;
  if (_retries < _maximumRetries)
  {
    _retries++;
    _SendCommand(_pendingCommand);
    return _retries;
  }
  else
  {
    _retries = 0;
    _pendingCommand = "";
    return -1;
  }
}
bool mppsolar::_strtob(char input)
{
  if (input == '1')
  {
    return true;
  }
  return false;
}

void mppsolar::_ValidateResponse()
{
  //Check CRC, if valid proceed.
  uint16_t newCrc;
  uint16_t oldCrc = (((_bufferedChars[_bufferedCharsIndex - 2]) << 8) & 0xff00) | (_bufferedChars[_bufferedCharsIndex - 1] & 0xff);

  uint8_t *txArray = (uint8_t *)&_bufferedChars;

  newCrc = _Calculate_CRC(txArray, _bufferedCharsIndex - 2);

  int i = _bufferedCharsIndex - 2;
  _bufferedChars[i] = 0; // Terminate the string in the input buffer, overwriting the crc - so it can  easily be printed out
  if (newCrc == oldCrc)  // Good crc
  {
    if ((_bufferedChars[1] == 'N') && (_bufferedChars[2] == 'A') && (_bufferedChars[3] == 'K'))
    {
      _RetryCommand();
      return;
    }
    //TODO right here process the data using the command string and a lookup table or soemthing. Maybe?

    //Copy only the wanted parts of the response string. Removing the starting bit and the CRC.

    ResponseLength = _bufferedCharsIndex - 3;
    memcpy(Response, _bufferedChars + 1, ResponseLength);

    Response[ResponseLength] = 0; //End string termination.

    if (_userCommandRunning)
    {
      ResponseReady = true;
    }
    else
    {
      _ParseResponse(_pendingCommand);
      //Check data against known types, etc.
    }

    //Reset so we can do it again!

    _userCommandRunning = false;
    _pendingCommand = "";
    _bufferedCharsIndex = 0; // Zero the pointer ready for the next packet
    _bufferAvailable = true;
    _retries = 0;

    //The previous command has finished. We can accept another.

    return;
  }
  else
  {
    _RetryCommand();
  }
}
bool mppsolar::Send(String command)
{
  if (_userCommand.length())
  {
    return false;
  }
  _userCommand = command;
  return true;
}
void mppsolar::_SendCommand(String command)
{
  int length = command.length();
  if (!length)
    return;
  uint8_t txArray[length];
  memcpy(&txArray, &command, length);
  int crc = _Calculate_CRC(txArray, length);

  _InverterSerial.write(txArray, length);
  _InverterSerial.write((crc >> 8) & 0xff);
  _InverterSerial.write(crc & 0xff);
  _InverterSerial.write(0x0d);
  _pendingCommand = command;
  _CommandTimestamp = millis();
}

void mppsolar::_ParseResponse(String command)
{
  char receivedChars[MAXIMUM_BUFFER_SIZE];
  memcpy(receivedChars, Response, ResponseLength);

  if (command == "QPIRI")
  {
    QPIRI_millis = millis();
    char *val;
    char *ptr = NULL;

    val = strtok((char *)receivedChars, " "); // get the first value
    Settings_ACInputVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_ACInputCurrent = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_ACOutputVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_ACOutputFrequency = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_ACOutputCurrent = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_ACOutputApparentPower = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_ACOutputActivePower = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_BatteryVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_BatteryRechargeVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_BatteryUnderVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_BatteryBulkChargeVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_BatteryFloatChargeVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_BatteryType = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_MaxACChargingCurrent = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_MaxChargingCurrent = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_InputVoltageRange = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_OutputSourcePriority = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_ChargerSourcePriority = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_MaxParallelUnits = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_MachineType = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_Topology = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_OutputMode = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_BatteryRedischargeVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    Settings_PVOKCondition = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_PVPowerBalance = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    Settings_MaxChargingTimeatCVStage = strtod(val, &ptr);
  }
  if (command == "QID")
  {
    serialNumber = Response;
  }

  if (command == "QPIGS")
  {

    QPIGS_millis = millis();
    char *val;
    char *ptr = NULL;

    val = strtok((char *)receivedChars, " "); // get the first value
    acInputVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    acInputFrequency = atof(val);

    val = strtok(NULL, " "); // Get the next value
    acOutputVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    acOutputFrequency = atof(val);

    val = strtok(NULL, " "); // Get the next value
    acOutputApparentPower = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    acOutputActivePower = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    acOutputLoad = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    busVoltage = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    batteryVoltage = strtof(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    batteryChargingCurrent = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    batteryCapacity = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    inverterHeatSinkTempature = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    pvInputCurrent = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    batteryVoltageFromscc = strtof(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    batteryDischargeCurrent = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    issbuPriorityVersionAdded = _strtob(val[0]);
    isConfigurationChanged = _strtob(val[1]);
    issccFirmwareUpdated = _strtob(val[2]);
    loadOn = _strtob(val[3]);
    isBatteryVoltageToSteadyWhileCharging = _strtob(val[4]);
    isCharginOn = _strtob(val[5]);
    issccChargingOn = _strtob(val[6]);
    isacChargingOn = _strtob(val[7]);

    val = strtok(NULL, " "); // Get the next value
    RSV1 = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    RSV2 = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    pvInputPower = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    isChargingTofloat = _strtob(val[0]);
    isSwitchedON = _strtob(val[1]);
    isReserved = _strtob(val[2]);
    return;
  }

  if (command == "QPIGS2")
  {
    QPIGS2_millis = millis();

    char *val;
    char *ptr = NULL;

    val = strtok((char *)receivedChars, " "); // get the first value
    L2acInputVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    L2acInputFrequency = atof(val);

    val = strtok(NULL, " "); // Get the next value
    L2acOutputVoltage = atof(val);

    val = strtok(NULL, " "); // Get the next value
    L2acOutputFrequency = atof(val);

    val = strtok(NULL, " "); // Get the next value
    L2acOutputApparentPower = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    L2acOutputActivePower = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    L2acOutputLoad = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    pv2batteryChargingCurrent = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    pv2InputCurrent = strtod(val, &ptr);

    val = strtok(NULL, " "); // Get the next value
    L2batteryVoltage = strtof(val, &ptr);

    val = strtok(NULL, " "); // Get the next value\            char s[50];
    isL2sccOk = _strtob(val[0]);
    isL2ACChargingOn = _strtob(val[1]);
    isL2SccChargingOn = _strtob(val[2]);
    isL2LineNotOk = _strtob(val[4]);
    isL2LoadOn = _strtob(val[5]);

    return;
  }
  if (command == "QPIWS")
  {
    QPIWS_millis = millis();
    char *val;

    val = strtok((char *)receivedChars, " "); // get the first value
    InverterFault = _strtob(val[1]);
    BusOverFault = _strtob(val[2]);
    BusUnderFault = _strtob(val[3]);
    BusSoftFailFault = _strtob(val[4]);
    LineFailWarning = _strtob(val[5]);
    OPVshortWarning = _strtob(val[6]);
    InverterVoltageTooLowFault = _strtob(val[7]);
    InverterVoltageTooHighFault = _strtob(val[8]);
    OverTemperatureFault = _strtob(val[9]);
    fanLockedFault = _strtob(val[10]);
    BatteryVoltageToHighFault = _strtob(val[11]);
    BatteryLowAlarmWarning = _strtob(val[12]);
    BatteryUnderShutdownWarning = _strtob(val[14]);
    OverloadFault = _strtob(val[16]);
    EEPROMFault = _strtob(val[17]);
    InverterOverCurrentFault = _strtob(val[18]);
    InverterSoftFailFault = _strtob(val[19]);
    SelfTestFailFault = _strtob(val[20]);
    OPDCvoltageOverFault = _strtob(val[21]);
    BatOpenFault = _strtob(val[22]);
    CurrentSensorFailFault = _strtob(val[23]);
    BatteryShortFault = _strtob(val[24]);
    PowerLimitWarning = _strtob(val[25]);
    PVVoltageHighWarning = _strtob(val[26]);
    MPPTOverloadFault = _strtob(val[27]);
    MPPTOverloadWarning = _strtob(val[28]);
    BatteryTooLowToChargeWarning = _strtob(val[29]);
    return;
  }
}
// Buffers serial data in a non blocking way.
void mppsolar::_BufferResponse()
{
  char rc;
  while (_InverterSerial.available() > 0)
  {
    rc = _InverterSerial.read();
    if (_bufferedCharsIndex == 0 && (rc == '\r' || rc != '('))
      break;
    if (rc != '\r')
    {
      _bufferedChars[_bufferedCharsIndex] = rc;
      _bufferedCharsIndex++;
      if (_bufferedCharsIndex >= MAXIMUM_BUFFER_SIZE)
      {
        _bufferedCharsIndex = MAXIMUM_BUFFER_SIZE - 1; //Ensure we don't go over our maximum size.
      }
    }
    else
    {
      _bufferAvailable = false; //Not accepting new buffer data until finished processing data!

      //TODO This is needs to be a variable to check, and used in the main loop
      _ValidateResponse();
      return;
    }
    yield();
  }
  if (millis() - _CommandTimestamp > _CommandTimeout)
  {
    _RetryCommand();
  }
}

//_Calculate_CRC was provided from the manufacturer and also documented in a few obscure places.
uint16_t mppsolar::_Calculate_CRC(uint8_t *pin, uint8_t len)
{
  uint16_t crc;
  uint8_t da;
  uint8_t *ptr;
  uint8_t bCRCHign;
  uint8_t bCRCLow;

  uint16_t crc_ta[16] =
      {
          0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
          0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef};
  ptr = pin;
  crc = 0;

  while (len-- != 0)
  {
    da = ((uint8_t)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr >> 4)];
    da = ((uint8_t)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr & 0x0f)];
    ptr++;
  }
  bCRCLow = crc;
  bCRCHign = (uint8_t)(crc >> 8);

  if (bCRCLow == 0x28 || bCRCLow == 0x0d || bCRCLow == 0x0a)
  {
    bCRCLow++;
  }
  if (bCRCHign == 0x28 || bCRCHign == 0x0d || bCRCHign == 0x0a)
  {
    bCRCHign++;
  }
  crc = ((uint8_t)bCRCHign) << 8;
  crc += bCRCLow;
  return (crc);
}
