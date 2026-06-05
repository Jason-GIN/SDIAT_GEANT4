#ifndef MyStackingAction_h
#define MyStackingAction_h 1

#include "G4UserStackingAction.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4ClassificationOfNewTrack.hh"

namespace Test02
{
class MyStackingAction : public G4UserStackingAction {
public:
    MyStackingAction();
    virtual ~MyStackingAction();
    
    virtual G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track*);
    
    void SetEnergyThreshold(G4double val) { fEnergyThreshold = val; }
    
private:
    G4double fEnergyThreshold;
};
};

#endif