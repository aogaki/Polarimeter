#ifndef TPOLARIMETER_HPP
#define TPOLARIMETER_HPP 1

#include <memory>

#include <TH2.h>

#include "TWaveRecord.hpp"

class TPolarimeter
{
 public:
  TPolarimeter();
  TPolarimeter(uint16_t link);
  ~TPolarimeter();

  void SetParameter(PolPar_t par) { fDigitizer->LoadParameters(par); };

  void Run();
  void DummyRun();

  void SetShortGate(uint16_t val) { fShortGate = val; };
  void SetLongGate(uint16_t val) { fLongGate = val; };
  void SetThreshold(uint16_t val) { fThreshold = val; };

 private:
  std::unique_ptr<TWaveRecord> fDigitizer;
  uint16_t fShortGate;
  uint16_t fLongGate;
  uint16_t fThreshold;

  // Think shared is really needed?
  std::shared_ptr<TH2D> fHisIn;
  std::shared_ptr<TH2D> fHisOut1;
  std::shared_ptr<TH2D> fHisOut2;

  int kbhit();
};

#endif
