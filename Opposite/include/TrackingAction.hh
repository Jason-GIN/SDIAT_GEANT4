#ifndef Test02TrackingAction_h
#define Test02TrackingAction_h 1

#include "G4UserTrackingAction.hh"

#include <map>
#include "G4Threading.hh"

struct ParentInfo {
    G4String name;
    G4int pdg;
};

namespace Test02
{


class TrackingAction : public G4UserTrackingAction {
public:
    static G4ThreadLocal std::map<G4int, ParentInfo> *parentInfoMap;
    TrackingAction();
    virtual ~TrackingAction();
    void PreUserTrackingAction(const G4Track*) override ;
    static std::map<G4int, ParentInfo>* GetThreadLocalMap() {
       if (!parentInfoMap) {
           parentInfoMap = new std::map<G4int, ParentInfo>();
       }
       return parentInfoMap;
    }
};

}
#endif
