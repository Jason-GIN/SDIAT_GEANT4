#ifndef Test02PrimaryGeneratorAction_h
#define Test02PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4GeneralParticleSource.hh" 
#include "globals.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UImanager.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcommand.hh"
#include "G4Threading.hh"
#include "Randomize.hh"

#include <string>
#include <bitset>
#include <vector>

class G4ParticleGun;
class G4Event;
class G4Box;
class DetectorConstruction;

/// The primary generator action class with particle gun.
///
/// The default kinematic is a 6 MeV gamma, randomly distribued
/// in front of the phantom across 80% of the (X,Y) phantom size.

namespace Test02
{
class DetectorConstruction;

struct ParticleInfo {
    G4ThreeVector position;
    G4ThreeVector direction;
    G4double      kineticEnergy;
    G4int         pdgCode;   // 粒子种类，如 PDG 编码
};


class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction, G4UImessenger
{
  public:
    PrimaryGeneratorAction (DetectorConstruction* detCon = nullptr);
    ~PrimaryGeneratorAction() override;

    // method from the base class
    void GeneratePrimaries(G4Event*) override;
    void SetNewValue(G4UIcommand *command, G4String newValue) override;

    
    // method to access particle gun
    //const G4ParticleGun* GetParticleGun() const { return fParticleGun; }
    
    
    //inline static G4String filename1;
    //inline static G4String filename2;
    //G4String Getfilename1() { return filename1; }
    //G4String Getfilename2() { return filename2; }
    
    G4ThreeVector GeneratorDirectionRoot(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow);
    
    G4ThreeVector GeneratorDirectionCSV(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow);
    G4ThreeVector GeneratorDirectionCSV1(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow);
    
  private:
    G4ParticleGun* fParticleGun = nullptr; // pointer a to G4 gun class
    //G4GeneralParticleSource* fGPS = nullptr;
    G4Box* fEnvelopeBox = nullptr;
    DetectorConstruction* fDetectorConstruction;
    G4ThreeVector direction;
    G4ThreeVector posision;
    G4ThreeVector norm1;
    G4ThreeVector norm2;
    G4double E1[43]={ 250, 300, 370, 450, 540, 660, 780, 940, 1110, 1310, 1540, 1800, 2090, 2430, 
                      2810, 3240, 3730, 4280, 4890, 5590, 6370, 7240, 8230, 9340, 10580, 11980, 13550, 
                      15310, 17280, 19500, 21990, 24780, 27920, 31430, 35380, 39810, 44780, 50360, 56610, 
                      63630, 71510, 80350, 90270};
    G4double E2[43]={ 270, 340, 420, 520, 650, 800, 980, 1210, 1470, 1790, 2170, 2610, 3130, 
                      3730, 4420, 5220, 6130, 7170, 8350, 9690, 11210, 12930, 14870, 17050, 
                      19510, 22280, 25400, 28900, 32830, 37250, 42210,  47790, 54050, 61070, 
                      68960, 77810, 87740, 98880, 111390, 125430, 141180, 158850, 178680};
    G4int num[43]={11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
                   32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53};
    //std::bitset<13963> rowdata;

    G4UIdirectory *fDir;
    G4UIcmdWithAString *fSelectGeneratorCmd;
    G4UIcmdWithAString *fReadfileCmd;
    std::vector<ParticleInfo> fParticleList;
    G4String fCurrentGenerator;

    struct DirectionMaskCache {
        std::string cachedFilename;
        std::vector<std::bitset<13963>> rows;  // 54行全量bitset
        // 预计算每行中设为1的索引列表（用于O(1)随机选择）
        std::vector<std::vector<int>> allowedIndices;  // 每行的允许方向索引
        
        bool load(const std::string& filename) {
            if (filename == cachedFilename && !rows.empty()) return true;
            
            rows.clear();
            allowedIndices.clear();
            
            std::ifstream file(filename);
            if (!file.is_open()) return false;
            
            std::string line;
            while (std::getline(file, line)) {
                std::bitset<13963> row;
                std::vector<int> indices;
                
                std::stringstream ss(line);
                std::string cell;
                int col = 0;
                while (std::getline(ss, cell, ',') && col < 13963) {
                    if (cell[0] == '1') {  // 比 cell == "1.0" 快得多！
                        row.set(col);
                        indices.push_back(col);
                    }
                    col++;
                }
                rows.push_back(row);
                allowedIndices.push_back(std::move(indices));
            }
            cachedFilename = filename;
            return true;
        }
        
        // O(1) 随机选择允许方向（无拒绝采样！）
        int getRandomAllowed(int targetRow) const {
            const auto& indices = allowedIndices.at(targetRow);
            int idx = static_cast<int>(G4UniformRand() * indices.size());
            return indices[idx];
        }
    };
    
    DirectionMaskCache fDirectionCache;
    
};

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
