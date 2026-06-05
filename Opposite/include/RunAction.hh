#ifndef Test02RunAction_h
#define Test02RunAction_h 1

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"
#include "G4AnalysisManager.hh"
#include "G4String.hh"
#include "MyRun.hh"

class G4Run;

/// Run action class
///
/// In EndOfRunAction(), it calculates the dose in the selected volume
/// from the energy deposit accumulated via stepping and event actions.
/// The computed dose is then printed on the screen.

class G4GenericMessenger;

namespace Test02
{

class RunAction : public G4UserRunAction
{
  public:
    RunAction();
    ~RunAction();

    void BeginOfRunAction(const G4Run*) override;
    void   EndOfRunAction(const G4Run*) override;

    void AddEdep (G4double edep);
    
    void SetOutputFileName(G4String name) { fOutputFileName = name; }

    virtual G4Run* GenerateRun() override {
        return new MyRun();
    }
  private:
    G4Accumulable<G4double> fEdep = 0.;
    G4Accumulable<G4double> fEdep2 = 0.;
    G4String fOutputFileName; // 保存文件名的成员变量
    G4String fOutBoundaryFileName; // 保存文件名的成员变量
    G4GenericMessenger* fMessenger; // 用于处理宏命令
};

}

#endif

