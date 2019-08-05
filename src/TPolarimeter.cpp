#include <fcntl.h>
#include <termios.h>

#include <iostream>

#include <TBufferJSON.h>
#include <TFile.h>
#include <TGraph.h>
#include <TTree.h>

#include "TAsymmetry.hpp"
#include "TBeamSignal.hpp"
#include "TPolarimeter.hpp"
#include "TSignal.hpp"

TPolarimeter::TPolarimeter() : fShortGate(30), fLongGate(300), fThreshold(500)
{
  fHisIn.reset(new TH2D("HisIn", "PS vs TOF", 1000, 0., 100., 1000, 0., 1.));
  fHisIn->SetDirectory(nullptr);
  fInPlane.reset(new TAsymmetry(fHisIn.get(), 0));

  fHisOut1.reset(
      new TH2D("HisOut1", "PS vs TOF", 1000, 0., 100., 1000, 0., 1.));
  fHisOut1->SetDirectory(nullptr);
  fOutPlane1.reset(new TAsymmetry(fHisOut1.get(), 1));

  fHisOut2.reset(
      new TH2D("HisOut2", "PS vs TOF", 1000, 0., 100., 1000, 0., 1.));
  fHisOut2->SetDirectory(nullptr);
  fOutPlane2.reset(new TAsymmetry(fHisOut2.get(), 2));
}

TPolarimeter::TPolarimeter(uint16_t link) : TPolarimeter()
{
  fDigitizer.reset(new TWaveRecord(CAEN_DGTZ_USB, link));
}

TPolarimeter::~TPolarimeter() {}

void TPolarimeter::Run()
{
  std::unique_ptr<TCanvas> fCanvas(new TCanvas("Canvas", "test", 700, 500));
  fCanvas->Divide(2, 2);
  std::unique_ptr<TGraph> grIn(new TGraph());
  grIn->SetMinimum(0);
  grIn->SetMaximum(18000);
  std::unique_ptr<TGraph> grOut1(new TGraph());
  grOut1->SetMinimum(0);
  grOut1->SetMaximum(18000);
  std::unique_ptr<TGraph> grOut2(new TGraph());
  grOut2->SetMinimum(0);
  grOut2->SetMaximum(18000);
  std::unique_ptr<TGraph> grBeam(new TGraph());
  grBeam->SetMinimum(0);
  grBeam->SetMaximum(18000);

  fDigitizer->Initialize();
  fDigitizer->StartAcquisition();
  for (int i = 0; true; i++) {
    // if (i > 10) break;
    if (kbhit()) break;
    // std::cout << i << std::endl;
    // for (int j = 0; j < 10; j++) fDigitizer->SendSWTrigger();
    fDigitizer->ReadEvents();

    const auto nHit = fDigitizer->GetNEvents();
    // std::cout << nHit << " hits" << std::endl;

    if (nHit > 0) {
      std::vector<HitData_t> &buf = fDigitizer->GetDataVec();
      auto hit = buf[buf.size() - 1];

      const auto recordLength = hit.inPlane.size();
      for (uint32_t iData = 0; iData < recordLength; iData++) {
        grIn->SetPoint(iData, iData * 2, hit.inPlane[iData]);
        grOut1->SetPoint(iData, iData * 2, hit.outPlane1[iData]);
        grOut2->SetPoint(iData, iData * 2, hit.outPlane2[iData]);
        grBeam->SetPoint(iData, iData * 2, hit.beamTrg[iData]);
      }

      fCanvas->cd(1);
      grIn->Draw("AL");
      fCanvas->cd(2);
      grBeam->Draw("AL");
      fCanvas->cd(3);
      grOut1->Draw("AL");
      fCanvas->cd(4);
      grOut2->Draw("AL");
      fCanvas->Modified();
      fCanvas->Update();
    }
    usleep(1000);
  }

  fDigitizer->StopAcquisition();

  std::cout << std::endl;
}

void TPolarimeter::DummyRun()
{
  std::unique_ptr<TCanvas> fCanvas(new TCanvas("Canvas", "test", 1000, 1000));
  fCanvas->Divide(2, 2);

  std::unique_ptr<TFile> file(new TFile("Data/wave11.root", "READ"));
  auto tree = (TTree *)file->Get("wave");
  tree->SetBranchStatus("*", kFALSE);

  std::vector<short> *trace[3]{nullptr};
  tree->SetBranchStatus("trace0", kTRUE);
  tree->SetBranchAddress("trace0", &trace[0]);
  tree->SetBranchStatus("trace1", kTRUE);
  tree->SetBranchAddress("trace1", &trace[1]);
  tree->SetBranchStatus("trace8", kTRUE);
  tree->SetBranchAddress("trace8", &trace[2]);

  std::unique_ptr<TSignal> inSignal(
      new TSignal(trace[0], fThreshold, fCFDThreshold, fShortGate, fLongGate));
  std::unique_ptr<TSignal> outSignal1(
      new TSignal(trace[1], fThreshold, fCFDThreshold, fShortGate, fLongGate));
  std::unique_ptr<TSignal> outSignal2(
      new TSignal(trace[1], fThreshold, fCFDThreshold, fShortGate, fLongGate));
  std::unique_ptr<TBeamSignal> beam(new TBeamSignal(trace[2]));

  const auto nEve = tree->GetEntries();
  for (auto iEve = 0; true; iEve++) {
    if (iEve >= nEve) iEve = 0;
    tree->GetEntry(iEve);
    inSignal->ProcessSignal();
    outSignal1->ProcessSignal();
    outSignal2->ProcessSignal();
    beam->ProcessSignal();

    auto beamTrg = beam->GetTrgTime();

    auto long1 = inSignal->GetLongCharge();
    auto short1 = inSignal->GetShortCharge();
    auto ps1 = short1 / long1;
    constexpr auto timeOffset1 = 0.;
    auto tof1 = inSignal->GetTrgTime() - beamTrg + timeOffset1;

    auto long2 = outSignal1->GetLongCharge();
    auto short2 = outSignal1->GetShortCharge();
    auto ps2 = short2 / long2;
    constexpr auto timeOffset2 = 10.14 + 1.04;  // Check Aogaki
    auto tof2 = outSignal1->GetTrgTime() - beamTrg + timeOffset2;

    auto long3 = outSignal2->GetLongCharge();
    auto short3 = outSignal2->GetShortCharge();
    auto ps3 = short3 / long3;
    constexpr auto timeOffset3 = 10.14 + 1.04;  // Check Aogaki
    auto tof3 = outSignal2->GetTrgTime() - beamTrg + timeOffset3;

    if (tof1 > 0.) fHisIn->Fill(tof1, ps1);
    if (tof2 > 0.) fHisOut1->Fill(tof2, ps2);
    if (tof3 > 0.) fHisOut2->Fill(tof3, ps3);

    if (((iEve + 1) % 10000) == 0) {
      fCanvas->cd(1);
      fHisIn->Draw("COL");
      fCanvas->cd(2);
      fHisOut1->Draw("COL");

      if (((iEve + 1) % 100000) == 0) {
        // if (0 == 0) {
        Analysis();

        fCanvas->cd(3);
        fCanvas->cd(3)->SetLogz();
        fInPlane->DrawResult();

        fCanvas->cd(4);
        fCanvas->cd(4)->SetLogz();
        fOutPlane1->DrawResult();
      }
      fCanvas->Modified();
      fCanvas->Update();
    }

    // usleep(10);
    if (kbhit()) break;
  }

  file->Close();
}

void TPolarimeter::Analysis()
{
  fInPlane->SetHist(fHisIn.get());
  fOutPlane1->SetHist(fHisOut1.get());
  fOutPlane2->SetHist(fHisOut2.get());

  fInPlane->DataAnalysis();
  auto yieldIn = fInPlane->GetYield();

  fOutPlane1->DataAnalysis();
  auto yieldOut1 = fOutPlane1->GetYield();

  fOutPlane2->DataAnalysis();
  auto yieldOut2 = fOutPlane2->GetYield();

  std::cout << yieldOut1 << "\t" << yieldIn << "\t"
            << fabs(yieldIn - yieldOut1) / (yieldIn + yieldOut1) << std::endl;
  std::cout << yieldOut2 << "\t" << yieldIn << "\t"
            << fabs(yieldIn - yieldOut2) / (yieldIn + yieldOut2) << std::endl;

  // fInPlane->DrawResult();
}

int TPolarimeter::kbhit()
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}
