#include <string.h>
#include <iostream>

#include "TWaveRecord.hpp"

TWaveRecord::TWaveRecord()
    : fpReadoutBuffer(nullptr),
      fpEventPtr(nullptr),
      fpEventStd(nullptr),
      fMaxBufferSize(0),
      fBufferSize(0),
      fEveCounter(0),
      fBLTEvents(0),
      fRecordLength(0),
      fVth(0),
      fTriggerMode(CAEN_DGTZ_TRGMODE_ACQ_ONLY),
      fPolarity(CAEN_DGTZ_TriggerOnRisingEdge),
      fPostTriggerSize(50),
      fTimeOffset(0),
      fPreviousTime(0)
{
}

TWaveRecord::TWaveRecord(CAEN_DGTZ_ConnectionType type, int link, int node,
                         uint32_t VMEadd)
    : TWaveRecord()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetBoardInfo();
  SetParameters();

  // fDataArray = new unsigned char[fBLTEvents * fOneHitSize * fNChs];

  fDataVec.resize(fBLTEvents * 2);
  for (auto &&data : fDataVec) {
    data.inPlane.reserve(fRecordLength);
    data.outPlane1.reserve(fRecordLength);
    data.outPlane2.reserve(fRecordLength);
    data.beamTrg.reserve(fRecordLength);
  }
  fDataVec.resize(0);
}

TWaveRecord::~TWaveRecord()
{
  Reset();
  Close();
  if (fpReadoutBuffer) {
    auto err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
    PrintError(err, "FreeReadoutBuffer");
  }

  delete[] fDataArray;
}

void TWaveRecord::SetParameters()
{
  fInCh = 0;
  fOutCh1 = 1;
  fOutCh2 = 2;
  fBeamCh = 3;

  fChMask =
      (0b1 << fInCh) | (0b1 << fOutCh1) | (0b1 << fOutCh2) | (0b1 << fBeamCh);
  fChTrgMask = (0b1 << fInCh) | (0b1 << fOutCh1) | (0b1 << fOutCh2);

  fRecordLength = 256;
  fBLTEvents = 1024;
  fPolarity = CAEN_DGTZ_TriggerOnFallingEdge;

  auto offset = 0.2;
  auto th = abs(500);
  if (fPolarity == CAEN_DGTZ_TriggerOnFallingEdge) {
    // Why offset and trigger use different data range (0xFFFF and 1 << 14)?
    fDCOffset = 0xFFFF * offset;
    fVth = ((1 << fNBits) * (1. - offset)) - th;
  } else if (fPolarity == CAEN_DGTZ_TriggerOnRisingEdge) {
    fDCOffset = 0xFFFF * (1.0 - offset);
    fVth = ((1 << fNBits) * offset) + th;
  }

  fPostTriggerSize = 80;
}

void TWaveRecord::LoadParameters(PolPar_t par)
{
  fInCh = par.inCh;
  fOutCh1 = par.outCh1;
  fOutCh2 = par.outCh2;
  fBeamCh = par.beamCh;

  fChMask =
      (0b1 << fInCh) | (0b1 << fOutCh1) | (0b1 << fOutCh2) | (0b1 << fBeamCh);
  fChTrgMask = (0b1 << fInCh) | (0b1 << fOutCh1) | (0b1 << fOutCh2);
  std::cout << fChMask << "\t" << fChTrgMask << std::endl;

  fRecordLength = par.recordLength;
  fBLTEvents = par.BLTEvents;
  fPolarity = par.polarity;

  auto offset = par.DCOffset;
  auto th = par.th;  // par.th is unsigned
  if (fPolarity == CAEN_DGTZ_TriggerOnFallingEdge) {
    // Why offset and trigger use different data range (0xFFFF and 1 << 14)?
    fDCOffset = 0xFFFF * offset;
    fVth = ((1 << fNBits) * (1. - offset)) - th;
  } else if (fPolarity == CAEN_DGTZ_TriggerOnRisingEdge) {
    fDCOffset = 0xFFFF * (1.0 - offset);
    fVth = ((1 << fNBits) * offset) + th;
  }

  fPostTriggerSize = par.postTriggerSize;
}

void TWaveRecord::Initialize()
{
  CAEN_DGTZ_ErrorCode err;

  Reset();
  AcquisitionConfig();
  TriggerConfig();

  err = CAEN_DGTZ_SetMaxNumEventsBLT(fHandler, fBLTEvents);
  PrintError(err, "SetMaxNEventsBLT");
  err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer,
                                      &fMaxBufferSize);
  PrintError(err, "MallocReadoutBuffer");

  BoardCalibration();
}

void TWaveRecord::ReadEvents()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_ReadData(fHandler, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                           fpReadoutBuffer, &fBufferSize);
  PrintError(err, "ReadData");

  uint32_t nEvents;
  err =
      CAEN_DGTZ_GetNumEvents(fHandler, fpReadoutBuffer, fBufferSize, &nEvents);
  PrintError(err, "GetNumEvents");
  // std::cout << nEvents << " Events" << std::endl;

  // fData->clear();

  fDataVec.resize(0);
  static HitData_t buf;
  buf.inPlane.resize(fRecordLength, 0);
  buf.outPlane1.resize(fRecordLength, 0);
  buf.outPlane2.resize(fRecordLength, 0);
  buf.beamTrg.resize(fRecordLength, 0);

  fEveCounter = 0;
  for (uint iEve = 0; iEve < nEvents; iEve++) {
    err = CAEN_DGTZ_GetEventInfo(fHandler, fpReadoutBuffer, fBufferSize, iEve,
                                 &fEventInfo, &fpEventPtr);
    PrintError(err, "GetEventInfo");
    // std::cout << "Event number:\t" << iEve << '\n'
    //           << "Event size:\t" << fEventInfo.EventSize << '\n'
    //           << "Board ID:\t" << fEventInfo.BoardId << '\n'
    //           << "Pattern:\t" << fEventInfo.Pattern << '\n'
    //           << "Ch mask:\t" << fEventInfo.ChannelMask << '\n'
    //           << "Event counter:\t" << fEventInfo.EventCounter << '\n'
    //           << "Trigger time tag:\t" << fEventInfo.TriggerTimeTag
    //           << std::endl;

    err = CAEN_DGTZ_DecodeEvent(fHandler, fpEventPtr, (void **)&fpEventStd);
    PrintError(err, "DecodeEvent");

    // It should be for each channel!
    uint64_t timeStamp = (fEventInfo.TriggerTimeTag + fTimeOffset) * fTSample;
    if (timeStamp < fPreviousTime) {
      constexpr uint64_t maxTime = 0xFFFFFFFF / 2;  // Check manual
      timeStamp += maxTime * fTSample;
      fTimeOffset += maxTime;
    }
    fPreviousTime = timeStamp;

    buf.mod = 0;
    buf.time = timeStamp;

    for (uint iCh = 0; iCh < fNChs; iCh++) {
      uint32_t ch = (0b1 << iCh);
      if ((ch & fChMask) == false) continue;
      const uint32_t chSize = fpEventStd->ChSize[iCh];
      if (chSize == 0) continue;

      for (uint32_t i = 0; i < chSize; i++) {
        if (iCh == fInCh)
          buf.inPlane[i] = fpEventStd->DataChannel[iCh][i];
        else if (iCh == fOutCh1)
          buf.outPlane1[i] = fpEventStd->DataChannel[iCh][i];
        else if (iCh == fOutCh2)
          buf.outPlane2[i] = fpEventStd->DataChannel[iCh][i];
        else if (iCh == fBeamCh)
          buf.beamTrg[i] = fpEventStd->DataChannel[iCh][i];
        else
          break;
      }
      // memcpy(&fDataArray[index], fpEventStd->DataChannel[iCh], waveSize);
    }
    fEveCounter++;
    fDataVec.push_back(buf);
  }
}

void TWaveRecord::AcquisitionConfig()
{
  CAEN_DGTZ_ErrorCode err;

  // Eanble all channels
  err = CAEN_DGTZ_SetChannelEnableMask(fHandler, fChMask);
  PrintError(err, "SetChannelEnableMask");

  // Set DC offset
  for (uint32_t iCh = 0; iCh < fNChs; iCh++)
    err = CAEN_DGTZ_SetChannelDCOffset(fHandler, iCh, fDCOffset);
  PrintError(err, "SetChannelDCOffset");

  // Set the acquisition mode
  err = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_SW_CONTROLLED);
  PrintError(err, "SetAcquisitionMode");

  // Set record length (length of waveform?);
  err = CAEN_DGTZ_SetRecordLength(fHandler, fRecordLength);
  PrintError(err, "SetRecordLength");
}

void TWaveRecord::TriggerConfig()
{
  CAEN_DGTZ_ErrorCode err;

  // Set the trigger threshold
  // The unit of its are V

  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    // Think about multiple channel setting
    err = CAEN_DGTZ_SetChannelTriggerThreshold(fHandler, iCh, fVth);
    PrintError(err, "SetChannelTriggerThreshold");
  }

  // Set the triggermode
  err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler, fTriggerMode, fChTrgMask);
  PrintError(err, "SetChannelSelfTrigger");
  err = CAEN_DGTZ_SetSWTriggerMode(fHandler, fTriggerMode);
  PrintError(err, "SetSWTriggerMode");

  // Set post trigger size
  err = CAEN_DGTZ_SetPostTriggerSize(fHandler, fPostTriggerSize);
  PrintError(err, "SetPostTriggerSize");

  // Set the triiger polarity
  for (uint32_t iCh = 0; iCh < fNChs; iCh++)
    CAEN_DGTZ_SetTriggerPolarity(fHandler, iCh, fPolarity);

  CAEN_DGTZ_TriggerPolarity_t pol;
  CAEN_DGTZ_GetTriggerPolarity(fHandler, 0, &pol);
  std::cout << "Polarity:\t" << pol << std::endl;
}

CAEN_DGTZ_ErrorCode TWaveRecord::StartAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");

  err = CAEN_DGTZ_AllocateEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "AllocateEvent");

  fTimeOffset = 0;
  fPreviousTime = 0;

  return err;
}

void TWaveRecord::StopAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "FreeEvent");
}
