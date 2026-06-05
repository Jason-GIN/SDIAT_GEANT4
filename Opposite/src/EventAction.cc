#include "EventAction.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "G4RunManager.hh"
#include "TrackingAction.hh"
#include "TrackerSD.hh"

#include "G4AnalysisManager.hh"
#include "MyRun.hh"

namespace Test02
{

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  EventAction::EventAction(RunAction *runAction)
      : fRunAction(runAction)
  {
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  void EventAction::BeginOfEventAction(const G4Event *event)
  {
    fEdep = 0.;
    // 重置当前事件的数据缓存
    fCurrentEventID = event->GetEventID();
    fPrimaryPID = event->GetPrimaryVertex(0)->GetPrimary(0)->GetPDGcode();
    fPrimaryEnergy = event->GetPrimaryVertex(0)->GetPrimary(0)->GetKineticEnergy();
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  void EventAction::EndOfEventAction(const G4Event *)
  {
    auto pdata = G4AnalysisManager::Instance();
    pdata->FillNtupleIColumn(0, 0, fCurrentEventID);
    pdata->FillNtupleIColumn(0, 1, fPrimaryPID);
    pdata->FillNtupleDColumn(0, 2, fPrimaryEnergy);
    pdata->AddNtupleRow(0);

    // accumulate statistics in run action
    // fRunAction->AddEdep(fEdep);
    auto *localMap = TrackingAction::GetThreadLocalMap();
    if (localMap)
    {
      localMap->clear();
    }
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

}
