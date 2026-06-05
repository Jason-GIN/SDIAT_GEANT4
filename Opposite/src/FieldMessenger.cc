// FieldMessenger.cc
#include "FieldMessenger.hh"
//#include "MyDipoleField.hh"
#include "MyMagneticField.hh"
#include "G4UIparameter.hh"
#include "G4UImanager.hh"
#include "G4UnitsTable.hh"
#include <sstream>

namespace Test02
{

FieldMessenger::FieldMessenger(MyMagneticField* field)
    : fField(field) {
    
    // 创建命令目录 /field/
    fFieldDir = new G4UIdirectory("/field/");
    fFieldDir->SetGuidance("Magnetic field configuration commands.");

    // 命令：设置场参数
    fSetDipoleFieldParametersCmd = new G4UIcmdWith3VectorAndUnit("/field/Geolocation", this);
    fSetDipoleFieldParametersCmd->SetParameterName("r", "lat", "lon", false);
    fSetDipoleFieldParametersCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
    fSetDipoleFieldParametersCmd->SetUnitCategory("Angle");
    fSetDipoleFieldParametersCmd->SetDefaultUnit("degree");

    fSetUniformFieldParametersCmd = new G4UIcmdWith3VectorAndUnit("/field/UniStrength", this);
    fSetUniformFieldParametersCmd->SetParameterName("Bx", "By", "Bz", false);
    fSetUniformFieldParametersCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
    fSetUniformFieldParametersCmd->SetUnitCategory("Magnetic flux density");
    fSetUniformFieldParametersCmd->SetDefaultUnit("gauss");
    
    // 命令：启用/禁用磁场
    fEnableFieldCmd = new G4UIcmdWithABool("/field/enable", this);
    fEnableFieldCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
    fEnableFieldCmd->SetGuidance("Enable or disable magnetic field");
    fEnableFieldCmd->SetParameterName("enable", false);
    
     
    // 命令：设置磁场类型
    fSetFieldTypeCmd = new G4UIcmdWithAString("/field/SetType", this);
    fSetFieldTypeCmd->SetGuidance("Set magnetic field type: dipole, globaldipole, quadrupole, uniform");
    fSetFieldTypeCmd->SetParameterName("type", false);
    fSetFieldTypeCmd->SetCandidates("dipole globaldipole quadrupole uniform");
    fSetFieldTypeCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
   
    /*
    // 命令：设置场强
    fSetFieldStrengthCmd = new G4UIcmdWithADoubleAndUnit("/field/setStrength", this);
    fSetFieldStrengthCmd->SetGuidance("Set magnetic field strength");
    fSetFieldStrengthCmd->SetParameterName("strength", false);
    fSetFieldStrengthCmd->SetUnitCategory("Magnetic flux density");
    fSetFieldStrengthCmd->SetDefaultUnit("tesla");
    fSetFieldStrengthCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // 命令：设置磁场区域中心
    fSetFieldRegionCenterCmd = new G4UIcmdWith3VectorAndUnit("/field/setRegionCenter", this);
    fSetFieldRegionCenterCmd->SetGuidance("Set the center of magnetic field region");
    fSetFieldRegionCenterCmd->SetParameterName("X", "Y", "Z", false);
    fSetFieldRegionCenterCmd->SetUnitCategory("Length");
    fSetFieldRegionCenterCmd->SetDefaultUnit("m");
    fSetFieldRegionCenterCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    // 命令：设置磁场区域半径
    fSetFieldRegionRadiusCmd = new G4UIcmdWithADoubleAndUnit("/field/setRegionRadius", this);
    fSetFieldRegionRadiusCmd->SetGuidance("Set the radius of magnetic field region");
    fSetFieldRegionRadiusCmd->SetParameterName("radius", false);
    fSetFieldRegionRadiusCmd->SetUnitCategory("Length");
    fSetFieldRegionRadiusCmd->SetDefaultUnit("m");
    fSetFieldRegionRadiusCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
    
    // 命令：打印磁场状态
    fPrintFieldStatusCmd = new G4UIcmdWithoutParameter("/field/printStatus", this);
    fPrintFieldStatusCmd->SetGuidance("Print current field configuration");
    fPrintFieldStatusCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
    */
}

FieldMessenger::~FieldMessenger() {
    
    delete fFieldDir;
    delete fSetDipoleFieldParametersCmd;
    delete fSetUniformFieldParametersCmd;
    delete fEnableFieldCmd;
    delete fSetFieldTypeCmd;
    
    //delete fSetFieldStrengthCmd;
    //delete fSetFieldRegionCenterCmd;
    //delete fSetFieldRegionRadiusCmd;
    //delete fPrintFieldStatusCmd;
    
}

void FieldMessenger::SetNewValue(G4UIcommand* command, G4String newValue) {
    if (command == fEnableFieldCmd) {
        G4bool enable = fEnableFieldCmd->GetNewBoolValue(newValue);
        fField->EnableField(enable);;
        G4cout << "enable: " << enable << G4endl;
        G4cout << "Magnetic field " << (enable ? "enabled(ture)" : "disabled(false)") << G4endl;
        if (!enable) {
            G4cout << "Magnetic field disabled. Subsequent field settings "
                   << "will be stored but not applied." << G4endl;

        }      
    }else if (command == fSetFieldTypeCmd) {
        if (!fField->IsFieldEnabled()) {
            G4cout << "Warning: Setting field strength while field is disabled. "
                   << "Value will be stored but not used." << G4endl;

        }
        fField->SetFieldType(newValue);
        G4cout << "Magnetic field type set to: " << newValue << G4endl;
    
    }else if (command == fSetDipoleFieldParametersCmd) {
        if((fField->GetFieldType())!="dipole"){
            G4cout << "Warning: Setting diple parameters while field is not dipole. "
                   << "Value will be stored but not used." << G4endl;
        };
        
        G4ThreeVector location = fSetDipoleFieldParametersCmd->GetNew3VectorValue(newValue);
        fField->SetDipoleFieldParameters(location);
    
    }else if (command == fSetDipoleFieldParametersCmd) {
        if((fField->GetFieldType())!="globaldipole"){
            G4cout << "Warning: Setting diple parameters while field is not dipole. "
                   << "Value will be stored but not used." << G4endl;
        };
        
        G4ThreeVector location = fSetDipoleFieldParametersCmd->GetNew3VectorValue(newValue);
        fField->SetDipoleFieldParameters(location);
    
    }else if (command == fSetUniformFieldParametersCmd) {
        if((fField->GetFieldType())!="uniform"){
            G4cout << "Warning: Setting uniform parameters while field is not uniform. "
                   << "Value will be stored but not used." << G4endl;
        };
        
        G4ThreeVector strength  = fSetUniformFieldParametersCmd->GetNew3VectorValue(newValue);
        fField->SetDipoleFieldParameters(strength);
        
    }

}

}