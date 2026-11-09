#include "MyStackingAction.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4Electron.hh"
#include "G4Gamma.hh"
#include "G4Positron.hh"


namespace Test02
{

MyStackingAction::MyStackingAction() 
    : fEnergyThreshold(5.0*MeV) {
}

MyStackingAction::~MyStackingAction() {
}

G4ClassificationOfNewTrack MyStackingAction::ClassifyNewTrack(const G4Track* track) {
    G4double energy = track->GetKineticEnergy();
    
    // 杀死所有低于阈值的粒子
    if (energy < fEnergyThreshold) {
        // 但保留主要粒子（从源头来的）
        if (track->GetParentID() == 0) {
            // 主要粒子即使能量低于阈值也保留
            return fUrgent;
        } else {
            // 次级粒子低于阈值则杀死,but keep nutrino
            G4int pdg = track->GetDefinition()->GetPDGEncoding();
            G4int apdg = std::abs(pdg);
            if (apdg == 12 || 
                apdg == 14 ||
                apdg == 16  ){
                    return fUrgent;
                }else{
                    return fKill;
                }
        }
    }
    
    return fUrgent;
}


}