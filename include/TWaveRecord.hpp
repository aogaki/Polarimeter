#ifndef TWaveRecord_hpp
#define TWaveRecord_hpp 1

// For the standard fiemware digitizer
// This will be super class of other firmware and model family

#include <string>
#include <vector>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#include "TDigitizer.hpp"

struct HitData_t {
  uint16_t mod;
  uint64_t time;
  std::vector<uint16_t> inPlane;
  std::vector<uint16_t> outPlane1;
  std::vector<uint16_t> outPlane2;
  std::vector<uint16_t> beamTrg;
};

struct PolPar_t {
  double DCOffset;
  uint16_t th;

  uint16_t inCh;
  uint16_t outCh1;
  uint16_t outCh2;
  uint16_t beamCh;

  uint16_t recordLength;
  uint16_t BLTEvents;
  CAEN_DGTZ_TriggerPolarity_t polarity;

  uint16_t postTriggerSize;
};

class TWaveRecord : public TDigitizer
{
 public:
  TWaveRecord();
  TWaveRecord(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
              uint32_t VMEadd = 0);
  virtual ~TWaveRecord();

  void Initialize();

  void ReadEvents();

  CAEN_DGTZ_ErrorCode StartAcquisition();
  void StopAcquisition();

  uint32_t GetNEvents() { return fEveCounter; };

  void LoadParameters(PolPar_t par);
  std::vector<HitData_t> &GetDataVec() { return fDataVec; };

 protected:
  // For event readout
  char *fpReadoutBuffer;
  char *fpEventPtr;
  CAEN_DGTZ_EventInfo_t fEventInfo;
  CAEN_DGTZ_UINT16_EVENT_t *fpEventStd;  // for void **Eve
  uint32_t fMaxBufferSize;
  uint32_t fBufferSize;
  uint32_t fEveCounter;
  uint32_t fBLTEvents;
  uint32_t fRecordLength;
  uint32_t fChMask;
  uint32_t fChTrgMask;

  // For trigger setting
  uint16_t fVth;
  uint16_t fDCOffset;
  CAEN_DGTZ_TriggerMode_t fTriggerMode;
  CAEN_DGTZ_TriggerPolarity_t fPolarity;
  uint32_t fPostTriggerSize;

  // Data
  uint64_t fTimeOffset;
  uint64_t fPreviousTime;
  std::vector<HitData_t> fDataVec;

  uint16_t fInCh;
  uint16_t fOutCh1;
  uint16_t fOutCh2;
  uint16_t fBeamCh;

  void SetParameters();

  void AcquisitionConfig();
  void TriggerConfig();
};

#endif
