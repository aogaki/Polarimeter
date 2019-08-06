
#include <iostream>
#include <memory>

#include <TApplication.h>

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

#include "TPolarimeter.hpp"
#include "TWaveRecord.hpp"

int main(int argc, char **argv)
{
  for (auto i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-h") {
      std::cout << "Something help" << std::endl;
      return 1;
    }
  }

  TApplication app("testApp", &argc, argv);

  auto link = 0;
  // std::unique_ptr<TWaveRecord> digi(new TWaveRecord(CAEN_DGTZ_USB, link));
  std::unique_ptr<TPolarimeter> polMeter(new TPolarimeter(link));

  mongocxx::client conn{
      mongocxx::uri{"mongodb://daq:nim2camac@192.168.161.73/GBS"}};
  auto collection = conn["GBS"]["PolMeterPar"];

  // Only taking last data
  auto opts = mongocxx::options::find{};
  opts.limit(1);
  auto order = document{} << "time" << -1 << finalize;
  opts.sort(order.view());
  auto cursor = collection.find({}, opts);
  auto doc = *(cursor.begin());
  std::cout << std::stod(doc["DCOffset"].get_utf8().value.to_string())
            << std::endl;

  PolPar_t par;
  par.DCOffset = std::stod(doc["DCOffset"].get_utf8().value.to_string());
  par.th = std::stoi(doc["threshold"].get_utf8().value.to_string());
  par.inCh = std::stoi(doc["inCh"].get_utf8().value.to_string());
  par.outCh1 = std::stoi(doc["outCh1"].get_utf8().value.to_string());
  par.outCh2 = std::stoi(doc["outCh2"].get_utf8().value.to_string());
  par.beamCh = std::stoi(doc["beamCh"].get_utf8().value.to_string());
  par.recordLength = 256;
  par.BLTEvents = 1024;
  par.polarity = CAEN_DGTZ_TriggerOnFallingEdge;
  par.postTriggerSize = 80;
  polMeter->SetParameter(par);

  auto shortGate = std::stoi(doc["shortGate"].get_utf8().value.to_string());
  polMeter->SetShortGate(shortGate);
  auto longGate = std::stoi(doc["longGate"].get_utf8().value.to_string());
  polMeter->SetLongGate(longGate);
  auto th = std::stoi(doc["threshold"].get_utf8().value.to_string());
  polMeter->SetThreshold(th);
  auto cfd = std::stoi(doc["CFDThreshold"].get_utf8().value.to_string());
  polMeter->SetCFDThreshold(cfd);

  polMeter->StartAcquisition();
  polMeter->DummyRun();
  polMeter->StopAcquisition();

  return 0;
}
