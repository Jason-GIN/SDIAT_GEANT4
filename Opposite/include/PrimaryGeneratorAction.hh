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

    struct ParticleInfo
    {
        G4ThreeVector position;
        G4ThreeVector direction;
        G4double kineticEnergy;
        G4int pdgCode; // 粒子种类，如 PDG 编码
        G4double time;
    };

    class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction, G4UImessenger
    {
    public:
        PrimaryGeneratorAction(DetectorConstruction *detCon = nullptr);
        ~PrimaryGeneratorAction() override;

        // method from the base class
        void GeneratePrimaries(G4Event *) override;
        void SetNewValue(G4UIcommand *command, G4String newValue) override;

        // method to access particle gun
        // const G4ParticleGun* GetParticleGun() const { return fParticleGun; }

        // inline static G4String filename1;
        // inline static G4String filename2;
        // G4String Getfilename1() { return filename1; }
        // G4String Getfilename2() { return filename2; }

        G4ThreeVector GeneratorDirectionRoot(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow);

        G4ThreeVector GeneratorDirectionCSV(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow);
        G4ThreeVector GeneratorDirectionCSV1(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow);

    private:
        G4ParticleGun *fParticleGun = nullptr; // pointer a to G4 gun class
        // G4GeneralParticleSource* fGPS = nullptr;
        G4Box *fEnvelopeBox = nullptr;
        DetectorConstruction *fDetectorConstruction;
        G4ThreeVector direction;
        G4ThreeVector posision;
        G4ThreeVector norm1;
        G4ThreeVector norm2;
        G4double E1[54] = {
            22, 28, 35, 43, 54, 68, 85, 105, 131, 162, 201, 247, 303, 370,
            450, 544, 655, 785, 935, 1109, 1309, 1537, 1798, 2095, 2431, 2812, 3242, 3728,
            4275, 4892, 5586, 6367, 7244, 8230, 9338, 10582, 11978, 13547, 15307, 17283, 19501,
            21990, 24783, 27917, 31435, 35382, 39811, 44780, 50356, 56613, 63634, 71511, 80350, 90268};
        G4double E2[54] = {
            22, 28, 35, 44, 55, 70, 87, 110, 138, 172, 216, 270, 337, 419, 521, 646,
            799, 984, 1207, 1474, 1792, 2168, 2610, 3127, 3726, 4419, 5215, 6127, 7167, 8350,
            9692, 11211, 12928, 14866, 17051, 19512, 22281, 25395, 28896, 32831, 37251, 42214,
            47788, 54046, 61071, 68956, 77806, 87738, 98884, 111393, 125429, 141179, 158853, 178684};

        // std::bitset<13963> rowdata;

        G4UIdirectory *fDir;
        G4UIcmdWithAString *fSelectGeneratorCmd;
        G4UIcmdWithAString *fReadfileCmd;
        std::vector<ParticleInfo> fParticleList;
        G4String fCurrentGenerator;

        struct DirectionMaskCache
        {
            std::string cachedFilename;
            std::vector<std::bitset<13963>> rows; // 54行全量bitset
            // 预计算每行中设为1的索引列表（用于O(1)随机选择）
            std::vector<std::vector<int>> allowedIndices; // 每行的允许方向索引

            bool load(const std::string &filename)
            {
                if (filename == cachedFilename && !rows.empty())
                    return true;

                rows.clear();
                allowedIndices.clear();

                std::ifstream file(filename);
                if (!file.is_open())
                    return false;

                std::string line;
                while (std::getline(file, line))
                {
                    std::bitset<13963> row;
                    std::vector<int> indices;

                    std::stringstream ss(line);
                    std::string cell;
                    int col = 0;
                    while (std::getline(ss, cell, ',') && col < 13963)
                    {
                        if (cell[0] == '1')
                        { // 比 cell == "1.0" 快得多！
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
            int getRandomAllowed(int targetRow) const
            {
                const auto &indices = allowedIndices.at(targetRow);
                int idx = static_cast<int>(G4UniformRand() * indices.size());
                return indices[idx];
            }
        };

        DirectionMaskCache fDirectionCache;
    };

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
