#ifndef DetectorMessenger_h
#define DetectorMessenger_h 1

#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcommand.hh"
#include "globals.hh"



namespace Test02
{
    class DetectorConstruction;
    class DetectorMessenger : public G4UImessenger
    {
    public:
        DetectorMessenger(DetectorConstruction *det);
        ~DetectorMessenger() override;

        void SetNewValue(G4UIcommand *command, G4String newValue) override;

    private:
        DetectorConstruction *fDetector = nullptr;
        G4UIdirectory* detDir = nullptr;
        G4UIcmdWithAString *fSetCmdDet = nullptr;    // /det/setMaterial
        G4UIcmdWithADoubleAndUnit *fSetCmdLat = nullptr; // /det/setSize
        G4UIcmdWithADoubleAndUnit *fSetCmdLon = nullptr; // /det/setSize
    };
}
#endif