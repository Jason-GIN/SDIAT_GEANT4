#include "RunAction.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"

#include "G4GenericMessenger.hh"
#include "G4Threading.hh"

namespace Test02
{

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  RunAction::RunAction() : G4UserRunAction(), fOutputFileName("default_output.root")
  {
    // set printing event number per each 100 events
    G4RunManager::GetRunManager()->SetPrintProgress(100000);
    // 创建宏命令管理器
    fMessenger = new G4GenericMessenger(this, "/myrun/", "Run control");

    // 定义命令： /myrun/setOutputFileName
    auto &setFileNameCmd1 = fMessenger->DeclareProperty("setOutputFileName", fOutputFileName, "Set the output file name for this run.");
    setFileNameCmd1.SetParameterName("filename", true);
    setFileNameCmd1.SetDefaultValue("default_output.root");
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  RunAction::~RunAction()
  {
    delete fMessenger; // 记得释放内存
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  void RunAction::BeginOfRunAction(const G4Run *)
  {
    // inform the runManager to save random number seed
    G4RunManager::GetRunManager()->SetRandomNumberStore(false);

    // Create analysis manager
    auto pdata = G4AnalysisManager::Instance();
    G4String fileName = /*"/home/ams/lchen/GEANT4/test-sphere/LowAlt/60N60S/result/"+*/ fOutputFileName;
    pdata->OpenFile(fileName);

    // 事件信息Ntuple
    pdata->CreateNtuple("EventInfo", "Event Level Information");
    pdata->CreateNtupleIColumn("EventID");       // 列0
    pdata->CreateNtupleIColumn("PrimaryType");   // 列1
    pdata->CreateNtupleDColumn("PrimaryEnergy"); // 列2
    pdata->FinishNtuple(0);                      // 明确指定Ntuple ID

    // 探测信息Ntuple - 只记录到达指定位置的粒子
    pdata->CreateNtuple("DetectionInfo", "Particles Reaching Target");
    pdata->CreateNtupleIColumn("SDnum");    // 列0
    pdata->CreateNtupleIColumn("TrackID");  // 列1
    pdata->CreateNtupleDColumn("Energy");   // 列2
    pdata->CreateNtupleDColumn("PosX");     // 列3
    pdata->CreateNtupleDColumn("PosY");     // 列4
    pdata->CreateNtupleDColumn("PosZ");     // 列5
    pdata->CreateNtupleDColumn("DirX");     // 列6
    pdata->CreateNtupleDColumn("DirY");     // 列7
    pdata->CreateNtupleDColumn("DirZ");     // 列8
    pdata->CreateNtupleDColumn("Charge");   // 列9
    pdata->CreateNtupleDColumn("Time");     // 列10
    pdata->CreateNtupleIColumn("PID");      // 列11
    pdata->CreateNtupleIColumn("ParentId"); // 列12
    pdata->CreateNtupleIColumn("pPID");     // 列13
    pdata->CreateNtupleSColumn("Detector"); // 列14
    pdata->CreateNtupleSColumn("Process");  // 列15

    pdata->FinishNtuple(1); // 明确指定Ntuple ID
    // G4int runID = run->GetRunID();

    // 探测信息Ntuple - 只记录到达指定位置的粒子
    pdata->CreateNtuple("BoundaryInfo", "Particles Reaching Target");
    pdata->CreateNtupleIColumn("SDnum");    // 列0
    pdata->CreateNtupleIColumn("TrackID");  // 列1
    pdata->CreateNtupleDColumn("Energy");   // 列2
    pdata->CreateNtupleDColumn("PosX");     // 列3
    pdata->CreateNtupleDColumn("PosY");     // 列4
    pdata->CreateNtupleDColumn("PosZ");     // 列5
    pdata->CreateNtupleDColumn("DirX");     // 列6
    pdata->CreateNtupleDColumn("DirY");     // 列7
    pdata->CreateNtupleDColumn("DirZ");     // 列8
    pdata->CreateNtupleDColumn("Charge");   // 列9
    pdata->CreateNtupleDColumn("Time");     // 列10
    pdata->CreateNtupleIColumn("PID");      // 列11
    pdata->CreateNtupleIColumn("ParentId"); // 列12
    pdata->CreateNtupleIColumn("pPID");     // 列13
    pdata->CreateNtupleSColumn("Detector"); // 列14
    pdata->CreateNtupleSColumn("Process");  // 列15

    pdata->FinishNtuple(2); // 明确指定Ntuple ID
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  void RunAction::EndOfRunAction(const G4Run *Run)
  {

    auto pdata = G4AnalysisManager::Instance();
    pdata->Write();     // 将数据写入文件
    pdata->CloseFile(); // 关闭输出文件

    if (!G4Threading::IsWorkerThread())
    {
      auto run = static_cast<const MyRun *>(Run);
      const auto &particles = run->GetAllParticles();

      // 主线程安全输出二进制数据到 stdout
      uint32_t count = particles.size();
      std::cerr.write(reinterpret_cast<const char *>(&count), sizeof(count));
      if (count)
      {
        std::cerr.write(reinterpret_cast<const char *>(particles.data()),
                        count * sizeof(ParticleData));
      }
      std::cerr.flush();
    }
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

}
