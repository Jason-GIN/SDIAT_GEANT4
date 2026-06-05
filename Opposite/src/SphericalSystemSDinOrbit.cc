#include "SphericalSystemSDinOrbit.hh"
#include "G4HCofThisEvent.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"
#include "G4AnalysisManager.hh"

#include "G4TrackStatus.hh"
#include "G4StepStatus.hh"
#include <iostream>
#include <string>
#include <fstream>

#include "PrimaryGeneratorAction.hh"

#include "G4EventManager.hh"
#include "G4Event.hh"
#include "G4RunManager.hh" 

namespace Test02
{

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SphericalSystemSDinOrbit::SphericalSystemSDinOrbit(const G4String& name,
                                               const std::vector<G4double>& radii)
    : G4VSensitiveDetector(name), fRadii(radii)
{
    collectionName.insert("SystemHitsinOrbitCollection");
    
    for (size_t i = 0; i < radii.size() - 1; ++i) {
      fRadii.push_back(radii.at(i));
    }
    
    /*
    // 创建所有球壳层
    for (size_t i = 0; i < radii.size() - 1; ++i) {
        G4String layerName = name + "_Layer_" + std::to_string(i);
        fLayers.push_back(new SphericalLayerSD(layerName, i, 
                                              radii[i], radii[i]+0.5*cm));
    }
    */
    G4cout << "SphericalSystemSD SDetectorNamer: " << name << G4endl;
    G4cout << "SphericalSystemSD constructor: " << "SystemHitsinOrbitCollection" << G4endl;
}

void SphericalSystemSDinOrbit::Initialize(G4HCofThisEvent* hce)
{

  // Create hits collection
  
  G4EventManager* eventManager = G4EventManager::GetEventManager();
  G4Event* currentEvent = eventManager->GetNonconstCurrentEvent();
    
  G4int eventID = 0; // 默认值
  if (currentEvent) {
      eventID = currentEvent->GetEventID();
  } else {
      // 当前事件可能为空，特别是在模拟刚开始时
      G4cout << "Warning: Current event is null, using eventID = 0" << G4endl;
  };
  
  //G4cout << "=== TrackerSD::Initialize called for event: " << eventID << " ===" << G4endl;
  if (!hce) {
        G4cerr << "ERROR: G4HCofThisEvent is null!" << G4endl;
        return;
  };
  
  fSystemHitsinOrbitCollection
    = new SphericalHitsinOrbitCollection(SensitiveDetectorName, collectionName[0]);
  
  // Add this collection in hce
  //G4cout << "SdetName: " << SensitiveDetectorName << G4endl;
  //G4cout << "ScolNam: " << collectionName[0] << G4endl;
  G4int hcID
    = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
  hce->AddHitsCollection( hcID, fSystemHitsinOrbitCollection );
  //G4cout << "Spherical Hits collection created with ID: " << hcID << G4endl;
  
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4bool SphericalSystemSDinOrbit::ProcessHits(G4Step* aStep,
                                     G4TouchableHistory*)
{
  if (!fSystemHitsinOrbitCollection) {
    G4Exception("SphericalSystemSD::ProcessHits", "InvalidHitsCollection",
                FatalException, "Hits collection not initialized!");
    return false;
  }
  
  G4double edep = aStep->GetTotalEnergyDeposit();
  auto newHit = new SphericalHitinOrbit();
  
  G4int parentID = aStep->GetTrack()->GetParentID();
  
  newHit->SetTrackID  (aStep->GetTrack()->GetTrackID());
  newHit->SetParentID (parentID);
  newHit->SetPID (aStep->GetTrack()->GetDefinition()->GetPDGEncoding());
  
  
  auto* localMap = TrackingAction::GetThreadLocalMap();
  
  //G4cout << "-------------GIN0---------------" << G4endl;
  //G4cout << "localMap: "  << localMap << G4endl;
  //G4cout << "parentID: " << parentID << G4endl;
  //G4cout << "-------------GIN0---------------" << G4endl;
  
  if (parentID > 0 && localMap) {
     //G4cout << "-------------GIN0.1---------------" << G4endl;
     auto it = localMap->find(parentID);
     if (it != localMap->end()) {
        newHit->SetpPID (it->second.pdg);
        newHit->SetParentParticalName(it->second.name);
        //G4cout << "-------------GIN1---------------" << G4endl;
        //G4cout << "ParentParticalName: " << it->second.name << G4endl;
        //G4cout << "pPID: " << it->second.pdg << G4endl;
        //G4cout << "-------------GIN1---------------" << G4endl;
            // 使用父粒子信息...
     }
  }
  
  
  newHit->SetChamberNb(aStep->GetPreStepPoint()->GetTouchableHandle()
                                               ->GetCopyNumber());
  newHit->SetEdep(edep);
  newHit->SetCharge(aStep->GetTrack()->GetDynamicParticle()->GetCharge());
  newHit->SetGetKineticEnergy(aStep->GetTrack()->GetKineticEnergy());
  newHit->SetPos (aStep->GetPostStepPoint()->GetPosition());
  newHit->SetMom (aStep->GetTrack()->GetMomentum());
  
  //newHit->SetTime(aStep->GetPostStepPoint()->GetGlobalTime());
  //newHit->SetlTime(aStep->GetPostStepPoint()->GetLocalTime());
  //newHit->SetpTime(aStep->GetPostStepPoint()->GetProperTime());
  
  newHit->SetParticalName(aStep->GetTrack()->GetDefinition()->GetParticleName());
  
  if(parentID > 0){
    newHit->SetProcess(aStep->GetTrack()->GetCreatorProcess()->GetProcessName());
  };


  fSystemHitsinOrbitCollection->insert( newHit );
  
  
  //G4double Ed = newHit->GetEdep();//能量沉积
  G4double kE = newHit->GetKineticEnergy();
  G4double charge = newHit->GetCharge();
  G4int TrackId = newHit->GetTrackID();//TrackID
  G4int ParentId = newHit->GetParentID();
  G4int PID = newHit->GetPID();
  G4int pPID = newHit->GetpPID();
  G4ThreeVector position = newHit->GetPos();//位置
  G4ThreeVector momentum = newHit->GetMom();//
  G4double timeOfArr = newHit->GetTime();//到达PMT的时间ofstream myfile
  //G4double timeOfL = newHit->GetlTime();
  //G4double timeOfP = newHit->GetpTime();
  
  G4String ParticalName = newHit->GetParticalName();
  G4String ParentParticalName = newHit->GetParentParticalName();

  //You should consider the copy number is not always equal to the height, 
  //because of on Orbit started at 400 km.

  G4int SDnum =(newHit->GetChamberNb())+400;
  
  G4String Detector="Orbit";

  G4String Process = newHit->GetProcess();



  //G4cout << "-------------GIN3SSD"<< SDnum <<"---------------" << G4endl;
  //ParentId >= 1 &&
  if ( /*&& PID == 22*/ kE>=30.*MeV){
    //newHit->Print();
    auto pdata= G4AnalysisManager::Instance();
    //G4int SDnum=std::stoi(SDetectorName.substr(11,2));
    //G4cout << "-------------GIN4---------------" << G4endl;
    //G4cout << "SDnum" << SDnum << G4endl;
    //G4cout << "-------------GIN4---------------" << G4endl;
    pdata->FillNtupleIColumn(1, 0, SDnum);
    pdata->FillNtupleIColumn(1, 1, TrackId);
    pdata->FillNtupleDColumn(1, 2, kE/MeV);
    pdata->FillNtupleDColumn(1, 3, position.x()/km);
    pdata->FillNtupleDColumn(1, 4, position.y()/km);
    pdata->FillNtupleDColumn(1, 5, position.z()/km);
    pdata->FillNtupleDColumn(1, 6, momentum.x());
    pdata->FillNtupleDColumn(1, 7, momentum.y());
    pdata->FillNtupleDColumn(1, 8, momentum.z());
    pdata->FillNtupleDColumn(1, 9, charge);
    pdata->FillNtupleDColumn(1, 10, timeOfArr/ns);
    pdata->FillNtupleIColumn(1, 11, PID);
    pdata->FillNtupleIColumn(1, 12, ParentId);
    pdata->FillNtupleIColumn(1, 13, pPID);
    pdata->FillNtupleSColumn(1, 14, Detector);
    pdata->FillNtupleSColumn(1, 15, Process);

    pdata->AddNtupleRow(1);
  };
  
  return true;
}



//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void SphericalSystemSDinOrbit::EndOfEvent(G4HCofThisEvent*)
{
  if ( verboseLevel>1 ) {
     G4int nofHits = fSystemHitsinOrbitCollection->entries();
     G4cout << G4endl
            << "-------->Hits Collection: in this event they are " << nofHits
            << " hits in the tracker chambers: " << G4endl;
     //for ( G4int i=0; i<nofHits; i++ ) (*fHitsCollection)[i]->Print();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......


}
