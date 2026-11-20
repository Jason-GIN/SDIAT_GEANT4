#include "DetectorMessenger.hh"
#include "DetectorConstruction.hh"

#include "G4UIparameter.hh"
#include "G4UImanager.hh"
#include "G4UnitsTable.hh"
#include <sstream>

namespace Test02
{
    DetectorMessenger::DetectorMessenger(DetectorConstruction *det)
        : fDetector(det)
    {
        // 可选：先创建 /Geo/ 命令目录，便于在 help 中归类
        detDir = new G4UIdirectory("/Geo/");
        detDir->SetGuidance("Geometry Control");

        // 定义命令：设置大气中的探测器
        fSetCmdDet = new G4UIcmdWithAString("/Geo/SphereDetector", this);
        fSetCmdDet->SetGuidance("on means will save, off means no");
        fSetCmdDet->SetParameterName("type", false);
        fSetCmdDet->SetCandidates("on off");
        fSetCmdDet->AvailableForStates(G4State_PreInit, G4State_Idle);

        // 命令 2：地理信息（带单位）
        fSetCmdLat = new G4UIcmdWithADoubleAndUnit("/Geo/lat", this);
        fSetCmdLat->SetGuidance("Set latitude.");
        fSetCmdLat->SetParameterName("latitude", false);
        fSetCmdLat->SetDefaultUnit("degree");
        // 命令 3：地理信息（带单位）
        fSetCmdLon = new G4UIcmdWithADoubleAndUnit("/Geo/lon", this);
        fSetCmdLon->SetGuidance("Set longitude.");
        fSetCmdLon->SetParameterName("longitude", false);
        fSetCmdLon->SetDefaultUnit("degree");
    }

    DetectorMessenger::~DetectorMessenger()
    {
        delete fSetCmdDet;
        delete fSetCmdLat;
        delete fSetCmdLon;
        delete detDir;
        // 如果创建了 detDir，也需要 delete detDir;
    }

    void DetectorMessenger::SetNewValue(G4UIcommand *command, G4String newValue)
    {
        if (command == fSetCmdDet)
        {
            fDetector->SetDet(newValue); // 假设 DetectorConstruction 有此公有方法
        }
        else if (command == fSetCmdLat)
        {
            fDetector->SetfLat(fSetCmdLat->GetNewDoubleValue(newValue));
            G4cout<<"----------------GIN------------"<<G4endl;
            G4cout<< "Lat: " <<fSetCmdLat->GetNewDoubleValue(newValue)<<G4endl;
            G4cout<<"----------------GIN------------"<<G4endl;
        }
        else if (command == fSetCmdLon)
        {
            fDetector->SetfLon(fSetCmdLon->GetNewDoubleValue(newValue));
        }
    }
}