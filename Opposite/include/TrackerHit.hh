#ifndef Test02TrackerHit_h
#define Test02TrackerHit_h 1

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "tls.hh"

namespace Test02
{

/// Tracker hit class
///
/// It defines data members to store the trackID, chamberNb, energy deposit,
/// and position of charged particles in a selected volume:
/// - fTrackID, fChamberNB, fEdep, fPos

class TrackerHit : public G4VHit
{
  public:
    TrackerHit() = default;
    TrackerHit(const TrackerHit&) = default;
    ~TrackerHit() override = default;

    // operators
    TrackerHit& operator=(const TrackerHit&) = default;
    G4bool operator==(const TrackerHit&) const;

    inline void* operator new(size_t);
    inline void  operator delete(void*);

    // methods from base class
    void Draw() override;
    void Print() override;

    // Set methods
    void SetTrackID  (G4int track)      { fTrackID = track; };
    void SetStepID  (G4int track)      { fStepID = track; };
    void SetParentID  (G4int track)      { fParentID = track; };
    void SetChamberNb(G4int chamb)      { fChamberNb = chamb; };
    void SetPID      (G4int pid)        { fPID = pid; };
    void SetpPID      (G4int ppid)        { fpPID = ppid; };
    void SetEdep     (G4double de)      { fEdep = de; };
    void SetCharge     (G4double Charge)      { fCharge = Charge; };
    void SetPos      (G4ThreeVector xyz){ fPos = xyz; };
    void SetMom      (G4ThreeVector mxyz){ fMom = mxyz; };
    void SetTime     (G4double time)    { fTime= time;};
    void SetlTime     (G4double time)    { flT= time;};
    void SetpTime     (G4double time)    { fpT= time;};
    void SetParticalName(G4String ParticalName) { fParticalName=ParticalName; };
    void SetParentParticalName(G4String ParentParticalName) { fParentParticalName=ParentParticalName; };
    void SetGetKineticEnergy(G4double ke){ fkE= ke; };
    void SetProcess(G4String ProcessName) { fProcessName=ProcessName; };

    // Get methods
    G4int GetTrackID() const     { return fTrackID; };
    G4int GetStepID() const     { return fStepID; };
    G4int GetParentID() const     { return fParentID; };
    G4int GetChamberNb() const   { return fChamberNb; };
    G4int GetPID() const         { return fPID; };
    G4int GetpPID() const         { return fpPID; };
    G4double GetEdep() const     { return fEdep; };
    G4double GetKineticEnergy() const     { return fkE; };
    G4double GetCharge() const     { return fCharge; };
    G4ThreeVector GetPos() const { return fPos; };
    G4ThreeVector GetMom() const { return fMom; };
    G4double GetTime() const     { return fTime;};
    G4double GetlTime() const     { return flT;};
    G4double GetpTime() const     { return fpT;};
    G4String GetParticalName() const{ return fParticalName;};
    G4String GetParentParticalName() const{ return fParentParticalName;};
    G4String GetProcess() const{ return fProcessName;};

  private:
    G4int         fTrackID = -1;
    G4int         fStepID = -1;
    G4int         fParentID = -1;
    G4int         fChamberNb = -1;
    G4int         fPID = -1;
    G4int         fpPID = -1;
    G4double      fEdep = 0.;
    G4double      fkE = 0.;
    G4double      fCharge = 0.;
    G4ThreeVector fPos;
    G4ThreeVector fMom;
    G4double      fTime;
    G4double      flT;
    G4double      fpT;
    G4String      fParticalName;
    G4String      fParentParticalName = "nan";
    G4String      fProcessName = "nan";
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

using TrackerHitsCollection = G4THitsCollection<TrackerHit>;

extern G4ThreadLocal G4Allocator<TrackerHit>* TrackerHitAllocator;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

inline void* TrackerHit::operator new(size_t)
{
  if(!TrackerHitAllocator)
      TrackerHitAllocator = new G4Allocator<TrackerHit>;
  return (void *) TrackerHitAllocator->MallocSingle();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

inline void TrackerHit::operator delete(void *hit)
{
  TrackerHitAllocator->FreeSingle((TrackerHit*) hit);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

}

#endif
