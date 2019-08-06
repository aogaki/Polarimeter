#include <fcntl.h>
#include <termios.h>

#include <fstream>
#include <iostream>
#include <thread>

#include <TBufferJSON.h>
#include <TFile.h>
#include <TGraph.h>
#include <TTree.h>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

#include "TAsymmetry.hpp"
#include "TBeamSignal.hpp"
#include "TPolarimeter.hpp"
#include "TSignal.hpp"

TPolarimeter::TPolarimeter()
    : fShortGate(30),
      fLongGate(300),
      fThreshold(500),
      fTimeInterval(10),
      fLastTime(0)
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

void TPolarimeter::StartAcquisition()
{
  fAcqFlag = true;

  fDigitizer->Initialize();
  fDigitizer->StartAcquisition();
}

void TPolarimeter::StopAcquisition()
{
  fAcqFlag = false;

  fDigitizer->StopAcquisition();
}

void TPolarimeter::FetchDummyData()
{
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

  BeamData_t data;

  const auto nEve = tree->GetEntries();
  for (auto iEve = 0; fAcqFlag; iEve++) {
    if (iEve >= nEve) iEve = 0;
    tree->GetEntry(iEve);

    data.in = *trace[0];
    data.out1 = *trace[1];
    data.out2 = *trace[1];
    data.beam = *trace[2];

    fMutex.lock();
    fQueue.push_back(data);
    fMutex.unlock();

    usleep(1);
  }

  file->Close();
}

void TPolarimeter::FillHists()
{
  std::unique_ptr<TSignal> inSignal(
      new TSignal(nullptr, fThreshold, fCFDThreshold, fShortGate, fLongGate));
  std::unique_ptr<TSignal> outSignal1(
      new TSignal(nullptr, fThreshold, fCFDThreshold, fShortGate, fLongGate));
  std::unique_ptr<TSignal> outSignal2(
      new TSignal(nullptr, fThreshold, fCFDThreshold, fShortGate, fLongGate));
  std::unique_ptr<TBeamSignal> beam(new TBeamSignal(nullptr));

  while (fAcqFlag) {
    while (!fQueue.empty()) {
      auto data = fQueue.front();

      inSignal->SetSignal(&(data.in));
      outSignal1->SetSignal(&(data.out1));
      outSignal2->SetSignal(&(data.out2));
      beam->SetSignal(&(data.beam));

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

      fMutex.lock();

      if (tof1 > 0.) fHisIn->Fill(tof1, ps1);
      if (tof2 > 0.) fHisOut1->Fill(tof2, ps2);
      if (tof3 > 0.) fHisOut2->Fill(tof3, ps3);

      fQueue.pop_front();

      fMutex.unlock();
    }

    usleep(1000);
  }
}

void TPolarimeter::DummyRun()
{
  std::thread fetchData(&TPolarimeter::FetchDummyData, this);
  std::thread fillHists(&TPolarimeter::FillHists, this);
  std::thread timeCheck(&TPolarimeter::TimeCheck, this);

  while (true) {
    if (kbhit()) {
      fAcqFlag = false;
      fetchData.join();
      fillHists.join();
      timeCheck.join();
      break;
    }

    usleep(1000);
  }
}

void TPolarimeter::TimeCheck()
{
  while (fAcqFlag) {
    if (fLastTime == 0) fLastTime = time(0);
    auto currentTime = time(0);
    if ((currentTime - fLastTime) >= fTimeInterval) {
      Analysis();
      PlotHists();
      UploadResults();
      fLastTime = currentTime;
    }
    usleep(1000);
  }
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

void TPolarimeter::PlotHists()
{
  if (!fCanvas) {
    fCanvas.reset(new TCanvas("Canvas", "test", 1000, 1000));
    fCanvas->Divide(2, 2);
  }

  fCanvas->cd(1);
  fHisIn->Draw("COL");

  fCanvas->cd(2);
  fHisOut1->Draw("COL");

  fCanvas->cd(3);
  fCanvas->cd(3)->SetLogz();
  fInPlane->DrawResult();

  fCanvas->cd(4);
  fCanvas->cd(4)->SetLogz();
  fOutPlane1->DrawResult();

  fCanvas->Modified();
  fCanvas->Update();
}

void TPolarimeter::UploadResults()
{
  std::cout << "result" << std::endl;
  auto result = TBufferJSON::ToJSON(fCanvas.get());
  result.ReplaceAll("$pair", "aogaki_pair");

  mongocxx::client conn{
      mongocxx::uri{"mongodb://daq:nim2camac@192.168.161.73/GBS"}};
  auto collection = conn["GBS"]["PolMeterResult"];
  bsoncxx::builder::stream::document buf{};
  buf << "in" << std::to_string(fInPlane->GetYield()) << "out1"
      << std::to_string(fOutPlane1->GetYield()) << "out2"
      << std::to_string(fOutPlane2->GetYield()) << "hists" << result.Data()
      << "time" << std::to_string(time(0));
  collection.insert_one(buf.view());
  buf.clear();
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
