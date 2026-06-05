#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4UImanager.hh"
//#include "QBBC.hh" //这个是geant4的物理数据包，目前已知的还有FTFP_BERT，
#include "FTFP_BERT_HP.hh"

#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "G4AutoDelete.hh"

#include "MyMagneticField.hh"
#include "FieldMessenger.hh"
#include "MySession.hh"

#include "Randomize.hh"
#include "time.h"
#include <memory>


// #include ""

using namespace Test02;


int main(int argc ,char** argv)
{
  // Detect interactive mode (if no arguments) and define UI session
  //
  G4UIExecutive* ui = nullptr;
  
  CLHEP::HepRandom::setTheEngine(new CLHEP::RanecuEngine());
  //设定随机数种子
  G4long seed = time(NULL);
  CLHEP::HepRandom::setTheSeed(seed);
  
  if ( argc == 1 ) { ui = new G4UIExecutive(argc, argv); }

  // Optionally: choose a different Random engine...
  // G4Random::setTheEngine(new CLHEP::MTwistEngine);

  //use G4SteppingVerboseWithUnits
  G4int precision = 4;
  G4SteppingVerbose::UseBestUnit(precision);

  // Construct the default run manager
  //
  auto* runManager =
    G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // Set mandatory initialization classes
  //
  // Detector construction
  auto gMasterField = new MyMagneticField();
  auto gFieldMessenger = std::make_unique<FieldMessenger>(gMasterField);
  G4AutoDelete::Register(gMasterField);

  runManager->SetUserInitialization(new DetectorConstruction(gMasterField));

  // Physics list
  G4VModularPhysicsList* physicsList = new FTFP_BERT_HP;
  physicsList->SetVerboseLevel(1);
  runManager->SetUserInitialization(physicsList);

  // User action initialization
  runManager->SetUserInitialization(new ActionInitialization());


  // Initialize visualization
  //
  G4VisManager* visManager = new G4VisExecutive;
  // G4VisExecutive can take a verbosity argument - see /vis/verbose guidance.
  // G4VisManager* visManager = new G4VisExecutive("Quiet");
  visManager->Initialize();

  // Get the pointer to the User Interface manager
  G4UImanager* UImanager = G4UImanager::GetUIpointer();
  //UImanager->SetCoutDestination(new MySession());
  // Process macro or start UI session
  //
  if ( ! ui ) {
    // batch mode
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command+fileName);
  }
  else {
    // interactive mode
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }

  // Job termination
  // Free the store: user actions, physics_list and detector_description are
  // owned and deleted by the run manager, so they should not be deleted
  // in the main() program !

  delete visManager;
  delete runManager;
};
