#ifndef TSIGNAL_HPP
#define TSIGNAL_HPP 1

#include <memory>
#include <vector>

#include <TBox.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TLine.h>

class TSignal
{
 public:
  TSignal();
  TSignal(std::vector<short> *signal, double th, double cfd, int shortGate,
          int longGate);
  ~TSignal();

  void ProcessSignal();
  virtual void Plot();

  virtual void SetSignal(std::vector<short> *signal) { fSignal = signal; };
  void SetShortGate(double shortGate) { fShortGate = shortGate; };
  void SetLongGate(double longGate) { fLongGate = longGate; };
  void SetThreshold(double th) { fThreshold = th; };

  double GetTrgTime() { return fTrgTime; };
  double GetShortCharge() { return fShortCharge; };
  double GetLongCharge() { return fLongCharge; };
  double GetPulseHeight() { return fPulseHeight; };

 protected:
  std::vector<short> *fSignal;
  int fShortGate;
  int fLongGate;
  int fRewind;  // trigger - fRewind = start of integration
  double fThreshold;
  double fCFDThreshold;

  virtual void CalBaseLine();
  double fBaseLine;

  virtual void CalTrgTime();
  double fTrgTime;

  virtual void CalShortCharge();
  virtual void CalLongCharge();
  double fShortCharge;
  double fLongCharge;

  virtual void CalPulseHeight();
  double fPulseHeight;

  template <typename T>
  void SetPosition(T &obj, double x1, double y1, double x2, double y2);

  std::unique_ptr<TCanvas> fCanvas;
  std::unique_ptr<TGraph> fGraph;
  std::unique_ptr<TBox> fShortBox;
  std::unique_ptr<TBox> fLongBox;
  std::unique_ptr<TLine> fTriggerPos;
  std::unique_ptr<TLine> fBasePos;
};

#endif
