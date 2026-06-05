#ifndef Test02SphericalSystemSDinOrbit_h
#define Test02SphericalSystemSDinOrbit_h 1

#include "G4VSensitiveDetector.hh"

#include "SphericalSystemHitinOrbit.hh"
#include "TrackingAction.hh"

#include <vector>
#include <string>

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

namespace Test02
{

class SphericalSystemSDinOrbit : public G4VSensitiveDetector {
  public:
    SphericalSystemSDinOrbit (const G4String& name, 
                           const std::vector<G4double>& radii);
    ~SphericalSystemSDinOrbit () override = default;
    
    virtual void Initialize(G4HCofThisEvent* hce);
    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history);
    void   EndOfEvent(G4HCofThisEvent* hitCollection);
    
    // 系统级分析方法
    //G4double GetTotalEnergyDeposit() const;
    //G4ThreeVector CalculateShowerCenter() const;
    //G4int FindInteractionLayer() const;
    
  private:
    //std::vector<SphericalLayerSD*> fLayers;
    SphericalHitsinOrbitCollection* fSystemHitsinOrbitCollection = nullptr;;
    
    // 球壳半径参数
    std::vector<G4double> fRadii;
};
}
#endif
