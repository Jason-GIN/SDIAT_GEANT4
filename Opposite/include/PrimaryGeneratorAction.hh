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
#include <unordered_map>
#include <utility>

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

        // G4ThreeVector GeneratorDirectionCSV(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow);
        G4ThreeVector GeneratorDirectionCSV1(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow);
        G4ThreeVector GeneratorDirectionCSV2(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow);

    private:
        G4ParticleGun *fParticleGun = nullptr; // pointer a to G4 gun class
        // G4GeneralParticleSource* fGPS = nullptr;
        G4Box *fEnvelopeBox = nullptr;
        DetectorConstruction *fDetectorConstruction;
        G4ThreeVector direction;
        G4ThreeVector posision;
        G4ThreeVector norm1;
        G4ThreeVector norm2;
        G4double E[6][54] = {
            {0.022, 0.028, 0.035, 0.043, 0.054, 0.068, 0.085, 0.105, 0.131, 0.162, 0.201, 0.247, 0.303, 0.37,
             0.45, 0.544, 0.655, 0.785, 0.935, 1.109, 1.309, 1.537, 1.798, 2.095, 2.431, 2.812, 3.242, 3.728,
             4.275, 4.892, 5.586, 6.367, 7.244, 8.23, 9.338, 10.582, 11.978, 13.547, 15.307, 17.283, 19.501,
             21.99, 24.783, 27.917, 31.435, 35.382, 39.811, 44.78, 50.356, 56.613, 63.634, 71.511, 80.35, 90.268},

            {0.022, 0.028, 0.035, 0.044, 0.055, 0.07, 0.087, 0.11, 0.138, 0.172, 0.216, 0.27, 0.337, 0.419, 0.521, 0.646, 0.799,
             0.984, 1.207, 1.474, 1.792, 2.168, 2.61, 3.127, 3.726, 4.419, 5.215, 6.127, 7.167, 8.35, 9.692, 11.211, 12.928, 14.866,
             17.051, 19.512, 22.281, 25.395, 28.896, 32.831, 37.251, 42.214, 47.788, 54.046, 61.071, 68.956, 77.806, 87.738,
             98.884, 111.393, 125.429, 141.179, 158.853, 178.684},

            {0.067, 0.084, 0.106, 0.133, 0.167, 0.21, 0.264, 0.331, 0.416, 0.521, 0.652, 0.815, 1.017, 1.267, 1.575,
             1.952, 2.412, 2.969, 3.642, 4.447, 5.405, 6.538, 7.868, 9.422, 11.224, 13.306, 15.698, 18.437, 21.56, 25.111,
             29.14, 33.701, 38.855, 44.671, 51.227, 58.611, 66.919, 76.264, 86.769, 98.573, 111.833, 126.726, 143.449, 162.223,
             183.298, 206.954, 233.504, 263.301, 296.74, 334.265, 376.374, 423.625, 476.647, 536.141},

            {0.089, 0.112, 0.141, 0.178, 0.223, 0.28, 0.352, 0.442, 0.554, 0.695, 0.87, 1.087, 1.357, 1.69, 2.1, 2.603,
             3.216, 3.96, 4.856, 5.93, 7.207, 8.718, 10.492, 12.563, 14.967, 17.742, 20.932, 24.583, 28.748, 33.483, 38.855,
             44.936, 51.808, 59.563, 68.305, 78.149, 89.227, 101.687, 115.693, 131.432, 149.113, 168.97, 191.266, 216.299,
             244.399, 275.94, 311.34, 351.07, 395.655, 445.688, 501.834, 564.836, 635.531, 714.856},

            {0.156, 0.196, 0.246, 0.31, 0.389, 0.489, 0.614, 0.771, 0.967, 1.212, 1.517, 1.897, 2.367, 2.949, 3.665, 4.543,
             5.613, 6.912, 8.478, 10.353, 12.586, 15.226, 18.327, 21.947, 26.15, 31.004, 36.583, 42.969, 50.253, 58.538,
             67.936, 78.576, 90.599, 104.169, 119.464, 136.691, 156.077, 177.88, 202.39, 229.932, 260.872, 295.621, 334.639,
             378.445, 427.62, 482.816, 544.766, 614.292, 692.316, 779.874, 878.128, 988.381, 1112.097, 1250.917},

            {0.27, 0.339, 0.427, 0.537, 0.675, 0.848, 1.066, 1.338, 1.679, 2.106, 2.638, 3.3, 4.123, 5.142, 6.4, 7.946,
             9.838, 12.14, 14.925, 18.276, 22.28, 27.033, 32.638, 39.207, 46.859, 55.722, 65.938, 77.66, 91.06, 106.325,
             123.668, 143.325, 165.562, 190.678, 219.008, 250.931, 286.871, 327.307, 372.775, 423.881, 481.301, 545.798,
             618.229, 699.554, 790.853, 893.338, 1008.368, 1137.469, 1282.355, 1444.949, 1627.407, 1832.152, 2061.9, 2319.699}

        };
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
                        if (cell.empty())
                        {
                            G4cout << "cell is empyt, please check your file: " << filename << G4endl;
                            return false;
                        }
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
                if (targetRow < 0 || targetRow >= (int)allowedIndices.size())
                    return 0; // 防越界
                const auto &indices = allowedIndices.at(targetRow);
                int idx = static_cast<int>(G4UniformRand() * indices.size());
                return indices[idx];
            }
        };

        DirectionMaskCache fDirectionCache;
        // 预计算常量三角函数（整个run不变）
        // double fCachedCosColat = 0.0;
        // double fCachedSinColat = 0.0;
        // double fCachedCosLon = 0.0;
        // double fCachedSinLon = 0.0;
        // bool fTrigCached = false;

        // 能量→行号 O(1) 映射
        std::vector< std::unordered_map<double, int> > fParticleEnergyToRow{6};
        // std::unordered_map<double, int> fHeliumEnergyToRow{54};
    };

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
