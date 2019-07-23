#include <TH1.h>

#include "TBeamSignal.hpp"

TBeamSignal::TBeamSignal() : TSignal() {}

TBeamSignal::~TBeamSignal() {}

TBeamSignal::TBeamSignal(std::vector<short> *signal) : TSignal()
{
  SetSignal(signal);
}

void TBeamSignal::SetSignal(std::vector<short> *signal)
{
  fSignal = signal;
  // SetThreshold();
}

void TBeamSignal::SetThreshold()
{
  double min = *std::min_element(fSignal->begin(), fSignal->end());
  double max = *std::max_element(fSignal->begin(), fSignal->end());
  fThreshold = (min + max) / 2.;
}

void TBeamSignal::CalTrgTime()
{
  fTrgTime = 0.;
  SetThreshold();

  const auto searchSize = fSignal->size() - 1;
  for (unsigned int i = 0; i < searchSize; i++) {
    if ((*fSignal)[i] >= fThreshold && (*fSignal)[i + 1] <= fThreshold) {
      auto dx = 1.;
      auto dy = double((*fSignal)[i + 1] - (*fSignal)[i]);
      auto diff = double(fThreshold - (*fSignal)[i]);
      fTrgTime = i + diff * dx / dy;
      break;
    }
  }
}

void TBeamSignal::Plot()
{
  if (!fCanvas) fCanvas.reset(new TCanvas);
  if (!fGraph) {
    fGraph.reset(new TGraph);
    fGraph->SetLineWidth(2);
  }
  if (!fTriggerPos) {
    fTriggerPos.reset(new TLine);
    fTriggerPos->SetLineWidth(2);
    fTriggerPos->SetLineColor(kBlue);
    fTriggerPos->SetLineStyle(10);
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
    SetPosition(fTriggerPos, fTrgTime, min, fTrgTime, max);
    fTriggerPos->Draw("SAME");
  }
}
