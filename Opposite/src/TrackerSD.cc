#include "TrackerSD.hh"
#include "G4HCofThisEvent.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"

#include "G4TrackStatus.hh"
#include "G4StepStatus.hh"
#include <iostream>
#include <string>
#include <fstream>

#include "PrimaryGeneratorAction.hh"
#include "G4VProcess.hh"

namespace Test02
{

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  TrackerSD::TrackerSD(const G4String &name,
                       const G4String &hitsCollectionName)
      : G4VSensitiveDetector(name)
  {
    /*
    G4cout << "-------------GINSD---------------"<<G4endl;
    G4cout << "name:\t" << name << G4endl;
    G4cout << "hitsCollectionName:\t" << hitsCollectionName << G4endl;
    G4cout << "-------------GINSD---------------"<<G4endl;
    */
    SDetectorName = name;
    collectionName.insert(hitsCollectionName);
    G4cout << "TrackerSD SDetectorNamer: " << name << G4endl;
    G4cout << "TrackerSD constructor: " << hitsCollectionName << G4endl;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  void TrackerSD::Initialize(G4HCofThisEvent *hce)
  {
    /*//尝试输出数据
    std::ofstream fp;
    //string filename="dataout.txt";
    fp.open(filename, std::ios::out|std::ios::app);
    fp<<"Hits in PMTSD"<<G4endl<<"Output File Begins Here"<< G4endl;
    fp.close();
    */
    // Create hits collection

    fHitsCollection = new TrackerHitsCollection(SensitiveDetectorName, collectionName[0]);

    // Add this collection in hce
    // G4cout << "detName: " << SensitiveDetectorName << G4endl;
    // G4cout << "colNam: " << collectionName[0] << G4endl;
    G4int hcID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
    hce->AddHitsCollection(hcID, fHitsCollection);
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4bool TrackerSD::ProcessHits(G4Step *aStep,
                                G4TouchableHistory *)
  {
    G4double edep = aStep->GetTotalEnergyDeposit();
    auto newHit = new TrackerHit();

    G4int parentID = aStep->GetTrack()->GetParentID();

    newHit->SetTrackID(aStep->GetTrack()->GetTrackID());
    newHit->SetParentID(parentID);
    newHit->SetPID(aStep->GetTrack()->GetDefinition()->GetPDGEncoding());

    auto *localMap = TrackingAction::GetThreadLocalMap();

    // G4cout << "-------------GIN0---------------" << G4endl;
    // G4cout << "localMap: "  << localMap << G4endl;
    // G4cout << "parentID: " << parentID << G4endl;
    // G4cout << "-------------GIN0---------------" << G4endl;

    if (parentID > 0 && localMap)
    {
      // G4cout << "-------------GIN0.1---------------" << G4endl;
      auto it = localMap->find(parentID);
      if (it != localMap->end())
      {
        newHit->SetpPID(it->second.pdg);
        newHit->SetParentParticalName(it->second.name);
        // G4cout << "-------------GIN1---------------" << G4endl;
        // G4cout << "ParentParticalName: " << it->second.name << G4endl;
        // G4cout << "pPID: " << it->second.pdg << G4endl;
        // G4cout << "-------------GIN1---------------" << G4endl;
        //  使用父粒子信息...
      }
    }

    newHit->SetChamberNb(aStep->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber());
    newHit->SetEdep(edep);
    newHit->SetCharge(aStep->GetTrack()->GetDynamicParticle()->GetCharge());
    newHit->SetGetKineticEnergy(aStep->GetTrack()->GetKineticEnergy());
    newHit->SetPos(aStep->GetPostStepPoint()->GetPosition());
    newHit->SetMom(aStep->GetTrack()->GetMomentum());

    // newHit->SetTime(aStep->GetPostStepPoint()->GetGlobalTime());
    // newHit->SetlTime(aStep->GetPostStepPoint()->GetLocalTime());
    // newHit->SetpTime(aStep->GetPostStepPoint()->GetProperTime());

    newHit->SetParticalName(aStep->GetTrack()->GetDefinition()->GetParticleName());

    if (parentID > 0)
    {
      newHit->SetProcess(aStep->GetTrack()->GetCreatorProcess()->GetProcessName());
    };

    fHitsCollection->insert(newHit);

    // G4double Ed = newHit->GetEdep();//能量沉积
    G4double kE = newHit->GetKineticEnergy();
    G4double charge = newHit->GetCharge();
    G4int TrackId = newHit->GetTrackID(); // TrackID
    G4int ParentId = newHit->GetParentID();
    G4int PID = newHit->GetPID();
    G4int pPID = newHit->GetpPID();
    G4ThreeVector position = newHit->GetPos(); // 位置
    G4ThreeVector momentum = newHit->GetMom(); //
    G4double timeOfArr = newHit->GetTime();    // 到达PMT的时间ofstream myfile
    // G4double timeOfL = newHit->GetlTime();
    // G4double timeOfP = newHit->GetpTime();

    G4String ParticalName = newHit->GetParticalName();
    G4String ParentParticalName = newHit->GetParentParticalName();

    G4String Process = newHit->GetProcess();

    if (SDetectorName.substr(0, 10) == "Test02/SD2" /*&& PID == 22*/ && (kE >= 30. * MeV))
    {
      // newHit->Print();
      // G4cout << "-------------GINSD2---------------" << G4endl;
      // ParentId >= 1 &&
      G4String Detector = "SD2";
      auto pdata = G4AnalysisManager::Instance();
      // G4int SDnum=std::stoi(SDetectorName.substr(11,2));
      G4int SDnum = -1;
      G4cout << "-------------GINSD2---------------" << G4endl;
      G4cout << "SDnum" << SDnum << G4endl;
      G4cout << "-------------GINSD2---------------" << G4endl;
      pdata->FillNtupleIColumn(1, 0, SDnum);
      pdata->FillNtupleIColumn(1, 1, TrackId);
      pdata->FillNtupleDColumn(1, 2, kE / MeV);
      pdata->FillNtupleDColumn(1, 3, position.x() / km);
      pdata->FillNtupleDColumn(1, 4, position.y() / km);
      pdata->FillNtupleDColumn(1, 5, position.z() / km);
      pdata->FillNtupleDColumn(1, 6, momentum.x());
      pdata->FillNtupleDColumn(1, 7, momentum.y());
      pdata->FillNtupleDColumn(1, 8, momentum.z());
      pdata->FillNtupleDColumn(1, 9, charge);
      pdata->FillNtupleDColumn(1, 10, timeOfArr / ns);
      pdata->FillNtupleIColumn(1, 11, PID);
      pdata->FillNtupleIColumn(1, 12, ParentId);
      pdata->FillNtupleIColumn(1, 13, pPID);
      pdata->FillNtupleSColumn(1, 14, Detector);
      pdata->FillNtupleSColumn(1, 15, Process);

      pdata->AddNtupleRow(1);
    };

    if (SDetectorName == "Test02/SD2")
    {
      aStep->GetTrack()->SetTrackStatus(fStopAndKill);
    };

    if (SDetectorName.substr(0, 10) == "Test02/SD1" /*&& PID == 22*/ && (kE >= 30. * MeV))
    {
      // newHit->Print();
      // G4cout << "-------------GINSD2---------------" << G4endl;
      // ParentId >= 1 &&
      G4String Detector = "SD1";
      auto pdata = G4AnalysisManager::Instance();
      // G4int SDnum=std::stoi(SDetectorName.substr(11,2));
      G4int SDnum = -2;
      G4cout << "-------------GINSD1---------------" << G4endl;
      G4cout << "SDnum" << SDnum << G4endl;
      G4cout << "-------------GINSD1---------------" << G4endl;
      pdata->FillNtupleIColumn(2, 0, SDnum);
      pdata->FillNtupleIColumn(2, 1, TrackId);
      pdata->FillNtupleDColumn(2, 2, kE / MeV);
      pdata->FillNtupleDColumn(2, 3, position.x() / km);
      pdata->FillNtupleDColumn(2, 4, position.y() / km);
      pdata->FillNtupleDColumn(2, 5, position.z() / km);
      pdata->FillNtupleDColumn(2, 6, momentum.x());
      pdata->FillNtupleDColumn(2, 7, momentum.y());
      pdata->FillNtupleDColumn(2, 8, momentum.z());
      pdata->FillNtupleDColumn(2, 9, charge);
      pdata->FillNtupleDColumn(2, 10, timeOfArr / ns);
      pdata->FillNtupleIColumn(2, 11, PID);
      pdata->FillNtupleIColumn(2, 12, ParentId);
      pdata->FillNtupleIColumn(2, 13, pPID);
      pdata->FillNtupleSColumn(2, 14, Detector);
      pdata->FillNtupleSColumn(2, 15, Process);

      pdata->AddNtupleRow(2);

      if (PID != 12 && PID != 14 && PID != 16 && PID != -12 && PID != -14 && PID != -16)
      {
        ParticleData p;
        p.x = position.x() / km;
        p.y = position.y() / km;
        p.z = position.z() / km;
        p.dx = momentum.x();
        p.dy = momentum.y();
        p.dz = momentum.z();
        p.energy = kE / MeV;
        p.PID = static_cast<double>(PID);
        p.charge = charge;
        p.t = timeOfArr / ns;
        p.m = (aStep->GetTrack()->GetDynamicParticle()->GetMass()) ;
        SetParticleData(p);
      }
    };

    if (SDetectorName == "Test02/SD1")
    {
      aStep->GetTrack()->SetTrackStatus(fStopAndKill);
    };

    return true;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  void TrackerSD::EndOfEvent(G4HCofThisEvent *)
  {
    if (verboseLevel > 1)
    {
      G4int nofHits = fHitsCollection->entries();
      G4cout << G4endl
             << "-------->Hits Collection: in this event they are " << nofHits
             << " hits in the tracker chambers: " << G4endl;
      // for ( G4int i=0; i<nofHits; i++ ) (*fHitsCollection)[i]->Print();
    }

    auto run = static_cast<MyRun *>(
        G4RunManager::GetRunManager()->GetNonConstCurrentRun());
    run->AddParticles(localParticles);
    localParticles.clear();
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

}
