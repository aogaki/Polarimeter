#ifndef TBEAMSIGNAL_HPP
#define TBEAMSIGNAL_HPP 1

#include "TSignal.hpp"

class TBeamSignal : public TSignal
{
 public:
  TBeamSignal();
  TBeamSignal(std::vector<short> *signal);
  ~TBeamSignal();

  virtual void SetSignal(std::vector<short> *signal) override;

  virtual void Plot() override;

 private:
  void SetThreshold();

  virtual void CalBaseLine() override{};

  virtual void CalTrgTime() override;

  virtual void CalShortCharge() override{};
  virtual void CalLongCharge() override{};

  virtual void CalPulseHeight() override{};
};

#endif
