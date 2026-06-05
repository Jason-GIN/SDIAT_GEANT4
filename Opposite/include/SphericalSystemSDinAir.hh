#ifndef Test02SphericalSystemSDinAir_h
#define Test02SphericalSystemSDinAir_h 1

#include "G4VSensitiveDetector.hh"

#include "SphericalSystemHitinAir.hh"
#include "TrackingAction.hh"

#include <vector>
#include <string>

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

namespace Test02
{

class SphericalSystemSDinAir : public G4VSensitiveDetector {
  public:
    SphericalSystemSDinAir (const G4String& name, 
                           const std::vector<G4double>& radii);
    ~SphericalSystemSDinAir () override = default;
    
    virtual void Initialize(G4HCofThisEvent* hce);
    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history);
    void   EndOfEvent(G4HCofThisEvent* hitCollection);
    
    G4bool AltJudge(G4ThreeVector xyz,G4double steplength);
    // 系统级分析方法
    //G4double GetTotalEnergyDeposit() const;
    //G4ThreeVector CalculateShowerCenter() const;
    //G4int FindInteractionLayer() const;
    
  private:
    //std::vector<SphericalLayerSD*> fLayers;
    SphericalHitsinAirCollection* fSystemHitsinAirCollection = nullptr;;
    
    // 球壳半径参数
    std::vector<G4double> fRadii;
};
}
#endif
