#include "TrackingAction.hh"
#include "G4Track.hh"
#include "G4TrackStatus.hh"
#include "G4StepStatus.hh"
#include "G4Step.hh"
#include "G4ChordFinder.hh"
#include "G4TransportationManager.hh"
#include "G4FieldManager.hh"

namespace Test02
{

   G4ThreadLocal std::map<G4int, ParentInfo> *TrackingAction::parentInfoMap = nullptr;

   TrackingAction::TrackingAction()
   {
      GetThreadLocalMap();
   }

   TrackingAction::~TrackingAction()
   {
      if (parentInfoMap)
      {
         delete parentInfoMap;
         parentInfoMap = nullptr;
      }
   }

   void TrackingAction::PreUserTrackingAction(const G4Track *track)
   {
      auto *localMap = GetThreadLocalMap();

      G4int parentID = track->GetParentID();
      G4int trackID = track->GetTrackID();
      if (parentID >= 0)
      {
         // 只存储父粒子信息，不存储当前粒子
         if (localMap->find(trackID) == localMap->end())
         {
            const G4ParticleDefinition *def = track->GetDefinition();
            (*localMap)[trackID] = {
                def->GetParticleName(),
                def->GetPDGEncoding()};
         }
      }

   }
}
