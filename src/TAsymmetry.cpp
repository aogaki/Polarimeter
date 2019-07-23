#include <iostream>

#include <TROOT.h>
#include <TString.h>

#include "TAsymmetry.hpp"

TAsymmetry::TAsymmetry()
{
  fPulseShapeTh = 0.;
  fTimeTh = 0.;

  fIndex = 0;

  fTimeWindow = 4.;

  constexpr auto nPeaks = 1;
  fSpectrum.reset(new TSpectrum(nPeaks * 2));
  fSpectrumTh = 0.5;
}

TAsymmetry::TAsymmetry(TH2 *hist, int index) : TAsymmetry()
{
  fIndex = index;
  fHist.reset((TH2D *)hist->Clone(Form("Hist2D%02d", fIndex)));
  fHist->SetTitle("PS vs TOF");

  fHistTime.reset((TH1D *)hist->ProjectionX(Form("HistTime%02d", fIndex)));
  fHistTime->SetTitle("TOF");

  fHistPS.reset((TH1D *)hist->ProjectionY(Form("HistPS%02d", fIndex)));
  fHistPS->SetTitle("PS");
}

TAsymmetry::~TAsymmetry() {}

template <typename T>
void TAsymmetry::SetPosition(T &obj, double x1, double y1, double x2, double y2)
{
  obj->SetX1(x1);
  obj->SetY1(y1);
  obj->SetX2(x2);
  obj->SetY2(y2);
}

void TAsymmetry::Save(TString fileName)
{
  if (!fCanvas) Plot();
  if (!fileName.EndsWith(".pdf")) fileName = fileName + ".pdf";
  fCanvas->Print(fileName, "pdf");
}

void TAsymmetry::Plot()
{
  if (!fCanvas) {
    fCanvas.reset(
        new TCanvas(Form("glcanvas%02d", fIndex), "PS and TOF", 1400, 1000));
    fCanvas->Divide(2, 2);
  }
  if (!fVerLine) {
    fVerLine.reset(new TLine());
    fVerLine->SetLineWidth(2);
    fVerLine->SetLineColor(kRed);
    fVerLine->SetLineStyle(10);
  }
  if (!fHorLine) {
    fHorLine.reset(new TLine());
    fHorLine->SetLineWidth(2);
    fHorLine->SetLineColor(kBlue);
    fHorLine->SetLineStyle(10);
  }
  if (!fArea) {
    fArea.reset(new TBox);
    // fArea->SetFillStyle(3003);
    fArea->SetFillStyle(0);
    fArea->SetFillColorAlpha(kGreen, 0.5);
    fArea->SetLineWidth(2);
    fArea->SetLineColor(kGreen);
  }

  if (fHist) {
    fCanvas->cd(1);
    fHist->Draw("COLZ");
    fCanvas->cd(1)->SetLogz();

    auto y1 = fHist->GetYaxis()->GetBinCenter(1);
    auto lastBinY = fHist->GetYaxis()->GetNbins();
    auto y2 = fHist->GetYaxis()->GetBinCenter(lastBinY);
    auto x1 = fHist->GetXaxis()->GetBinCenter(1);
    auto lastBinX = fHist->GetXaxis()->GetNbins();
    auto x2 = fHist->GetXaxis()->GetBinCenter(lastBinX);
    SetPosition(fVerLine, fTimeTh, y1, fTimeTh, y2);
    SetPosition(fHorLine, x1, fPulseShapeTh, x2, fPulseShapeTh);
    fVerLine->Draw("SAME");
    fHorLine->Draw("SAME");

    auto peak = fFitFnc->GetParameter(1);
    auto sigma = fFitFnc->GetParameter(2);
    auto timePos = fSpectrum->GetPositionX()[0];
    SetPosition(fArea, timePos - fTimeWindow / 2., peak - 3 * sigma,
                timePos + fTimeWindow / 2., peak + 3 * sigma);
    fArea->Draw("SAME");
  }
  if (fHistPS) {
    fCanvas->cd(2);
    fHistPS->Draw();
  }
  if (fHistTime) {
    fCanvas->cd(3);
    fHistTime->Draw();
  }
  if (fHistResult) {
    fCanvas->cd(4);
    // fHistResult->Draw();
    fSpectrum->Search(fHistResult.get(), 2, "", fSpectrumTh);
    // fHistSlowComponent->Draw();
  }
}

void TAsymmetry::DataAnalysis()
{
  PulseShapeCut();
  TimeCut();
  PulseShapeCutLast();
}

void TAsymmetry::TimeCut()
{
  if (!fHistResult) PulseShapeCut();

  constexpr auto thRatio = 0.01;
  const auto maxBin = fHistResult->GetMaximumBin();
  const auto max = fHistResult->GetBinContent(maxBin);
  const auto th = max * thRatio;
  const auto nBins = fHistResult->GetNbinsX();

  for (auto i = maxBin; i < nBins; i++) {
    auto binContent = fHistResult->GetBinContent(i);
    if (binContent < th) {
      fHistResult->GetXaxis()->SetRange(i, nBins);
      std::cout << i << std::endl;
      fTimeTh = fHistResult->GetBinCenter(i);
      break;
    }
  }
}

void TAsymmetry::PulseShapeCut()
{
  constexpr auto thRatio = 0.075;
  const auto maxBin = fHistPS->GetMaximumBin();
  const auto max = fHistPS->GetBinContent(maxBin);
  const auto th = max * thRatio;

  for (auto i = maxBin; i > 0; i--) {
    auto binContent = fHistPS->GetBinContent(i);
    if (binContent < th) {
      fHistResult.reset(
          fHist->ProjectionX(Form("HistResult%02d", fIndex), 1, i));
      // fHistResult->Rebin(4);
      // std::cout << i << std::endl;
      fPulseShapeTh = fHistPS->GetBinCenter(i);
      break;
    }
  }
}

void TAsymmetry::PulseShapeCutLast()
{
  if (fTimeTh == 0.) TimeCut();
  auto cut = fHist->GetXaxis()->FindBin(fTimeTh);
  fHistSlowComponent.reset(
      fHist->ProjectionY(Form("HistSlowComponent%02d", fIndex), cut));

  TSpectrum s(4);
  s.Search(fHistSlowComponent.get(), 3, "goff", 0.005);
  auto nPeaks = s.GetNPeaks();
  auto peak = fHistSlowComponent->GetBinCenter(fHistSlowComponent->GetNbinsX());
  auto sigma = 0.02;
  for (auto i = 0; i < nPeaks; i++) {
    auto pos = s.GetPositionX()[i];
    if (peak > pos) peak = pos;
    std::cout << pos << "\t";
  }
  std::cout << std::endl;

  fFitFnc.reset(
      new TF1(Form("f%02d", fIndex), "gaus", peak - sigma, peak + sigma));
  fHistSlowComponent->Fit(fFitFnc.get(), "RQ0");
  peak = fFitFnc->GetParameter(1);
  sigma = fFitFnc->GetParameter(2);
  fFitFnc->SetRange(peak - sigma, peak + sigma);
  fHistSlowComponent->Fit(fFitFnc.get(), "RQ0");

  peak = fFitFnc->GetParameter(1);
  sigma = fFitFnc->GetParameter(2);
  auto startBin = fHist->GetYaxis()->FindBin(peak - 3 * sigma);
  auto endBin = fHist->GetYaxis()->FindBin(peak + 3 * sigma);

  fHistResult.reset();  // I really can not understand why this line is needed.
  fHistResult.reset(
      fHist->ProjectionX(Form("HistResult%02d", fIndex), startBin, endBin));

  std::cout << fHistResult->FindBin(fTimeTh) << "\t" << fHistResult->GetNbinsX()
            << std::endl;
  fHistResult->GetXaxis()->SetRange(fHistResult->FindBin(fTimeTh),
                                    fHistResult->GetNbinsX());
  fPulseShapeTh = fHistPS->GetBinCenter(endBin);
}

double TAsymmetry::Integral()
{
  fSpectrum->Search(fHistResult.get(), 2, "goff", fSpectrumTh);
  const auto peak = fSpectrum->GetPositionX()[0];
  const auto startBin = fHistResult->FindBin(peak - fTimeWindow / 2.);
  const auto stopBin = fHistResult->FindBin(peak + fTimeWindow / 2.);

  return fHistResult->Integral(startBin, stopBin);
}
