#ifndef Test02ActionInitialization_h
#define Test02ActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

/// Action initialization class.

namespace Test02
{

class ActionInitialization : public G4VUserActionInitialization
{
  public:
    ActionInitialization() = default;
    ~ActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;
};

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
