#ifndef TPOLARIMETER_HPP
#define TPOLARIMETER_HPP 1

#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include <TCanvas.h>
#include <TH2.h>

#include "TAsymmetry.hpp"
#include "TWaveRecord.hpp"

class BeamData_t
{
 public:
  std::vector<short> in;
  std::vector<short> out1;
  std::vector<short> out2;
  std::vector<short> beam;
};

class TPolarimeter
{
 public:
  TPolarimeter();
  TPolarimeter(uint16_t link);
  ~TPolarimeter();

  void SetParameter(PolPar_t par) { fDigitizer->LoadParameters(par); };
  void SetShortGate(uint16_t val) { fShortGate = val; };
  void SetLongGate(uint16_t val) { fLongGate = val; };
  void SetThreshold(uint16_t val) { fThreshold = val; };
  void SetCFDThreshold(uint16_t val) { fCFDThreshold = val; };
  void SetTimeInterval(uint16_t val) { fTimeInterval = val; };

  void StartAcquisition();
  void StopAcquisition();
  void DummyRun();

 private:
  std::unique_ptr<TWaveRecord> fDigitizer;
  uint16_t fShortGate;
  uint16_t fLongGate;
  uint16_t fThreshold;
  uint16_t fCFDThreshold;
  time_t fTimeInterval;
  time_t fLastTime;

  std::unique_ptr<TCanvas> fCanvas;

  std::unique_ptr<TH2D> fHisIn;
  std::unique_ptr<TH2D> fHisOut1;
  std::unique_ptr<TH2D> fHisOut2;

  std::unique_ptr<TAsymmetry> fInPlane;
  std::unique_ptr<TAsymmetry> fOutPlane1;
  std::unique_ptr<TAsymmetry> fOutPlane2;

  int kbhit();
  void FetchDummyData();
  void FillHists();
  void TimeCheck();
  void Analysis();
  void PlotHists();
  void UploadResults();

  std::deque<BeamData_t> fQueue;
  std::mutex fMutex;
  bool fAcqFlag;
};

#endif
