#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"
#include "TrackingAction.hh"

#include "DetectorConstruction.hh"
#include "G4RunManager.hh"
#include "MyStackingAction.hh"


namespace Test02
{

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void ActionInitialization::BuildForMaster() const
{
  auto runAction = new RunAction;
  SetUserAction(runAction);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void ActionInitialization::Build() const
{
  
  auto detConstruction = static_cast<const DetectorConstruction*>(G4RunManager::GetRunManager()->GetUserDetectorConstruction());
  
  SetUserAction(new PrimaryGeneratorAction(const_cast<DetectorConstruction*>(detConstruction)));
  MyStackingAction* stacking = new MyStackingAction();
  stacking->SetEnergyThreshold(15.0*MeV);
  SetUserAction(stacking);
  auto runAction = new RunAction;
  SetUserAction(runAction);

  auto eventAction = new EventAction(runAction);
  SetUserAction(eventAction);
  

  SetUserAction(new SteppingAction);
  SetUserAction(new TrackingAction);



}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

}
