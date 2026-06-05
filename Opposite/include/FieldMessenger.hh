#ifndef FIELDMESSENGER_HH
#define FIELDMESSENGER_HH

#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcommand.hh"
#include "globals.hh"

class MyMagneticField;

namespace Test02
{

class FieldMessenger : public G4UImessenger {
public:
    FieldMessenger(MyMagneticField* field);
    virtual ~FieldMessenger();

    virtual void SetNewValue(G4UIcommand* command, G4String newValue);

private:
    MyMagneticField* fField;

    // 命令目录
    G4UIdirectory* fFieldDir;
    // 各种命令
    G4UIcmdWith3VectorAndUnit* fSetDipoleFieldParametersCmd;
    G4UIcmdWith3VectorAndUnit* fSetUniformFieldParametersCmd;
    G4UIcmdWithABool* fEnableFieldCmd;
    G4UIcmdWithAString* fSetFieldTypeCmd;
    
    //G4UIcmdWithADoubleAndUnit* fSetFieldStrengthCmd;
    //G4UIcmdWith3VectorAndUnit* fSetFieldRegionCenterCmd;
    //G4UIcmdWithADoubleAndUnit* fSetFieldRegionRadiusCmd;
    //G4UIcmdWithABool* fEnableFieldCmd;
    //G4UIcmdWithoutParameter* fPrintFieldStatusCmd;
};

}

#endif
