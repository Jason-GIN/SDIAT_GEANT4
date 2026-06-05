#ifndef Test02TrackerSD_h
#define Test02TrackerSD_h 1

#include "G4VSensitiveDetector.hh"

#include "TrackerHit.hh"
#include "TrackingAction.hh"
#include "MyRun.hh"

#include <vector>
#include <string>


class G4Step;
class G4HCofThisEvent;

namespace Test02
{

/// Tracker sensitive detector class
///
/// The hits are accounted in hits in ProcessHits() function which is called
/// by Geant4 kernel at each step. A hit is created with each step with non zero
/// energy deposit.

class TrackerSD : public G4VSensitiveDetector
{
  public:
    TrackerSD(const G4String& name,
                const G4String& hitsCollectionName);
    ~TrackerSD() override = default;

    // methods from base class
    void   Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
    void   EndOfEvent(G4HCofThisEvent* hitCollection) override;
    
    void SetParticleData(ParticleData p){localParticles.push_back(p); };
    std::vector<ParticleData> GetParticleData(){return localParticles; }

  private:
    TrackerHitsCollection* fHitsCollection = nullptr;
    G4Track* fTrack = nullptr;
    
    G4String filename="dataout.txt";
    G4String filename1;
    G4String filename2;
    std::string SDetectorName;

    std::vector<ParticleData> localParticles;

        
};

}

#endif
