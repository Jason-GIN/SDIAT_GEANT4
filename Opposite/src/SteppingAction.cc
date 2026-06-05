#include "SteppingAction.hh"

#include "DetectorConstruction.hh"

#include "G4SystemOfUnits.hh"
#include "G4SteppingManager.hh"
#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"

#include "G4TrackStatus.hh"
#include "G4StepStatus.hh"

namespace Test02
{

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SteppingAction::SteppingAction()
: G4UserSteppingAction()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SteppingAction::UserSteppingAction(const G4Step* step)
{
  if (!fScoringVolume) {
    const auto detConstruction = static_cast<const DetectorConstruction*>(
      G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    fScoringVolume = detConstruction->GetScoringVolume();
  }

  // get volume of the current step
  G4double flytime = step->GetPostStepPoint()->GetProperTime();
  G4String process = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
  G4double dE = (step->GetTotalEnergyDeposit())/(step->GetTrack()->GetKineticEnergy());
  
  if (process=="Transportation"&& dE< 1.0e+11){
    n++;
    if ((flytime/ns)>(1e+13)&& n>100) {step->GetTrack()->SetTrackStatus(fStopAndKill);};
  }else{
    n=0;
  };
  
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

}
