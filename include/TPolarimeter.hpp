#ifndef TPOLARIMETER_HPP
#define TPOLARIMETER_HPP 1

#include <memory>

#include <TH2.h>

#include "TAsymmetry.hpp"
#include "TWaveRecord.hpp"

class TPolarimeter
{
 public:
  TPolarimeter();
  TPolarimeter(uint16_t link);
  ~TPolarimeter();

  void Run();
  void DummyRun();

  void Analysis();

  void SetParameter(PolPar_t par) { fDigitizer->LoadParameters(par); };
  void SetShortGate(uint16_t val) { fShortGate = val; };
  void SetLongGate(uint16_t val) { fLongGate = val; };
  void SetThreshold(uint16_t val) { fThreshold = val; };

 private:
  std::unique_ptr<TWaveRecord> fDigitizer;
  uint16_t fShortGate;
  uint16_t fLongGate;
  uint16_t fThreshold;

  // Think shared is really needed?
  std::unique_ptr<TH2D> fHisIn;
  std::unique_ptr<TH2D> fHisOut1;
  std::unique_ptr<TH2D> fHisOut2;

  std::unique_ptr<TAsymmetry> fInPlane;
  std::unique_ptr<TAsymmetry> fOutPlane1;
  std::unique_ptr<TAsymmetry> fOutPlane2;

  int kbhit();
};

#endif
