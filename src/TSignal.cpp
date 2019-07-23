#include <iostream>

#include <TH1F.h>

#include "TSignal.hpp"

TSignal::TSignal()
{
  fTrgTime = 0.;
  fShortCharge = 0.;
  fLongCharge = 0.;
  fRewind = 5;
}

TSignal::TSignal(std::vector<short> *signal, double th, int shortGate,
                 int longGate)
    : TSignal()
{
  fSignal = signal;
  fThreshold = th;
  fShortGate = shortGate;
  fLongGate = longGate;
}

TSignal::~TSignal() {}

void TSignal::Plot()
{
  if (!fCanvas) fCanvas.reset(new TCanvas);
  if (!fGraph) {
    fGraph.reset(new TGraph);
    fGraph->SetLineWidth(2);
  }
  if (!fShortBox) {
    fShortBox.reset(new TBox);
    fShortBox->SetFillColorAlpha(kMagenta, 0.7);
    fShortBox->SetFillStyle(3002);
  }
  if (!fLongBox) {
    fLongBox.reset(new TBox);
    fLongBox->SetFillColorAlpha(kCyan, 0.7);
    fLongBox->SetFillStyle(3002);
  }
  if (!fTriggerPos) {
    fTriggerPos.reset(new TLine);
    fTriggerPos->SetLineWidth(2);
    fTriggerPos->SetLineColor(kBlue);
    fTriggerPos->SetLineStyle(10);
  }
  if (!fBasePos) {
    fBasePos.reset(new TLine);
    fBasePos->SetLineWidth(2);
    fBasePos->SetLineColor(kRed);
    fBasePos->SetLineStyle(8);
  }

  fGraph->Clear();
  auto size = fSignal->size();
  for (unsigned int i = 0; i < size; i++) {
    fGraph->SetPoint(i, i, (*fSignal)[i]);
  }

  fCanvas->cd();
  fGraph->Draw("AL");
  if (fTrgTime > 0.) {
    auto min = fGraph->GetHistogram()->GetMinimum();
    auto max = fGraph->GetHistogram()->GetMaximum();
    auto start = int(fTrgTime) - 5;
    // std::cout << min << "\t" << max << std::endl;
    SetPosition(fShortBox, start, min, start + fShortGate, max);
    SetPosition(fLongBox, start, min, start + fLongGate, max);
    SetPosition(fTriggerPos, fTrgTime, min, fTrgTime, max);
    SetPosition(fBasePos, 0, fBaseLine, fSignal->size() - 1, fBaseLine);
    fLongBox->Draw("SAME");
    fShortBox->Draw("SAME");
    fTriggerPos->Draw("SAME");
    fBasePos->Draw("SAME");
  }
}

template <typename T>
void TSignal::SetPosition(T &obj, double x1, double y1, double x2, double y2)
{
  obj->SetX1(x1);
  obj->SetY1(y1);
  obj->SetX2(x2);
  obj->SetY2(y2);
}

void TSignal::ProcessSignal()
{
  CalBaseLine();
  CalTrgTime();
  CalShortCharge();
  CalLongCharge();
  CalPulseHeight();
}

void TSignal::CalBaseLine()
{
  constexpr auto nSamples = 40;
  fBaseLine = 0.;
  for (auto i = 0; i < nSamples; i++) {
    fBaseLine += (*fSignal)[i];
  }
  fBaseLine /= nSamples;

  // std::cout << fBaseLine << std::endl;
}

void TSignal::CalTrgTime()
{
  fTrgTime = 0.;

  const auto th = fBaseLine - fThreshold;
  const auto searchSize = fSignal->size() - 1;
  for (unsigned int i = 0; i < searchSize; i++) {
    // std::cout << i <<"\t"<< fSignal[i] <<"\t"<< th <<"\t"<< fSignal[i + 1] <<
    // std::endl;
    if ((*fSignal)[i] >= th && (*fSignal)[i + 1] <= th) {
      auto dx = 1.;
      auto dy = double((*fSignal)[i + 1] - (*fSignal)[i]);
      auto diff = double(th - (*fSignal)[i]);
      fTrgTime = i + diff * dx / dy;
      // std::cout << "hit\t" << fTrgTime << std::endl;
      break;
    }
  }
}

void TSignal::CalShortCharge()
{
  fShortCharge = 0.;

  if (fTrgTime > 0.) {
    auto start = int(fTrgTime) - fRewind;
    if (start < 0) start = 0;
    auto stop = start + fShortGate;
    if (stop > int(fSignal->size())) stop = fSignal->size();

    for (auto i = start; i < stop; i++) {
      fShortCharge += fBaseLine - double((*fSignal)[i]);
    }
    // std::cout << "short\t" << fShortCharge << std::endl;
  }
}

void TSignal::CalLongCharge()
{
  fLongCharge = 0.;

  if (fTrgTime > 0.) {
    auto start = int(fTrgTime) - fRewind;
    if (start < 0) start = 0;
    auto stop = start + fLongGate;
    if (stop > int(fSignal->size())) stop = fSignal->size();

    for (auto i = start; i < stop; i++) {
      fLongCharge += fBaseLine - double((*fSignal)[i]);
    }
    // std::cout << "long\t" << fLongCharge << std::endl;
  }
}

void TSignal::CalPulseHeight()
{
  fPulseHeight = 0.;
  if (fTrgTime > 0.) {
    double min = *std::min_element(fSignal->begin(), fSignal->end());
    fPulseHeight = fBaseLine - min;
  }
}
