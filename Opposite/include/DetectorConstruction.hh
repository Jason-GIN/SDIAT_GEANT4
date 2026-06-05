#ifndef Test02DetectorConstruction_h
#define Test02DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
// #include "tls.hh"
#include "G4Threading.hh"
// #include "MyDipoleField.hh"
#include "MyMagneticField.hh"
#include "FieldMessenger.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4GlobalMagFieldMessenger;
class G4GenericMessenger;

/// Detector construction class to define materials and geometry.

namespace Test02
{

  class DetectorConstruction : public G4VUserDetectorConstruction
  {
  public:
    DetectorConstruction(MyMagneticField *masterField);
    virtual ~DetectorConstruction();

    G4VPhysicalVolume *Construct() override;
    void ConstructSDandField() override;

    void SetAtmLayer(G4int lay) { layer = lay; };

    G4int GetAtmLayer() const { return layer; };

    void SetSD1Height(G4int H) { SD1H = H; };

    G4int GetSD1Height() const { return SD1H; };
    G4double GetfLat() const { return fLat; };
    G4double GetfLon() const { return fLon; };

    G4LogicalVolume *GetScoringVolume() const { return fScoringVolume; };

    //   G4Sphere *solidShapeAtm[];
    //   G4LogicalVolume *logicShapeAtm[];

    inline static G4int givenum;

  protected:
    G4LogicalVolume *fScoringVolume = nullptr;

  private:
    void DefineCommands();                                  // 用于定义UI命令
    std::vector<G4LogicalVolume *> logicSpherelayerinAir;

    std::vector<G4LogicalVolume *> logicSphereSDlayerinAir; // pointer to the logical Chamber
    std::vector<double> fLayRmininAir;

    std::vector<G4LogicalVolume *> logicSphereSDlayerinOrbit; // pointer to the logical Chamber
    std::vector<double> fLayRmininOrbit;

    G4LogicalVolume *logicSD1; // printer of the top SD of atmosphere
    G4LogicalVolume *logicSD2; // printer of the buttom SD of atmasphere
    G4LogicalVolume *logicSD4; // printer of the SD above SD3 (Layer_Systom) of stopping particles

    // G4GlobalMagFieldMessenger* fMagFieldMessenger;
    //  magnetic field messenger
    G4int layer = 1000; // 设定大气层数
    G4int SD1H = 565;  // 设定SD1所在高度

    G4GenericMessenger *f_lat_lon_Messenger = nullptr;
    G4double fLat = 0.0;
    G4double fLon = 0.0;

    MyMagneticField *fMagneticField = nullptr;
    FieldMessenger *fFieldMessenger = nullptr;
    G4double AtmLayer[209] = {0,40.897, 81.955, 123.176, 164.559, 206.108, 247.825, 289.71, 331.767, 373.995, 416.397,
                              458.973, 501.725, 544.656, 587.768, 631.06, 674.535, 718.195, 762.043, 806.079, 850.305, 894.725, 939.339, 984.149,
                              1029.155, 1074.362, 1119.771, 1165.383, 1211.201, 1257.229, 1303.466, 1349.914, 1396.576, 1443.455, 1490.554,
                              1537.875, 1585.419, 1633.191, 1681.192, 1729.423, 1777.887, 1826.587, 1875.526, 1924.705, 1974.129, 2023.796,
                              2073.715, 2123.885, 2174.311, 2224.994, 2275.939, 2327.147, 2378.622, 2430.367, 2482.385, 2534.679, 2587.253, 2640.111,
                              2693.254, 2746.688, 2800.415, 2854.44, 2908.765, 2963.395, 3018.333, 3073.584, 3129.151, 3185.039, 3241.251, 3297.792,
                              3354.667, 3411.878, 3469.432, 3527.333, 3585.585, 3644.193, 3703.162, 3762.497, 3822.204, 3882.287, 3942.751,
                              4003.603, 4064.848, 4126.491, 4188.539, 4250.996, 4313.87, 4377.167, 4440.893, 4505.055, 4569.659, 4634.712,
                              4700.222, 4766.195, 4832.638, 4899.56, 4966.967, 5034.869, 5103.273, 5172.187, 5241.621, 5311.582, 5382.081,
                              5453.126, 5524.727, 5596.894, 5669.636, 5742.964, 5816.889, 5891.421, 5966.573, 6042.355, 6118.779, 6195.859,
                              6273.605, 6352.032, 6431.153, 6510.982, 6591.533, 6672.82, 6754.859, 6837.665, 6921.256, 7005.648, 7090.857,
                              7176.902, 7263.802, 7351.575, 7440.242, 7529.823, 7620.339, 7711.813, 7804.267, 7897.725, 7992.211, 8087.752, 8184.374,
                              8282.104, 8380.972, 8481.007, 8582.241, 8684.706, 8788.436, 8893.468, 8999.837, 9107.581, 9216.742, 9327.362,
                              9439.485, 9553.158, 9668.428, 9785.348, 9903.971, 10024.352, 10146.552, 10270.633, 10396.661, 10524.705, 10654.84,
                              10787.141, 10921.693, 11058.622, 11198.401, 11341.314, 11487.512, 11637.145, 11790.381, 11947.396, 12108.382,
                              12273.547, 12443.111, 12617.314, 12796.419, 12980.707, 13170.489, 13366.102, 13567.914, 13776.328, 13991.797,
                              14214.809, 14445.915, 14685.721, 14934.905, 15194.232, 15464.56, 15746.875, 16042.275, 16352.03, 16677.607,
                              17020.715, 17383.34, 17767.84, 18177.025, 18614.263, 19083.691, 19590.424, 20140.923, 20744.455, 21412.887,
                              22161.582, 23012.054, 23995.748, 25161.254, 26589.508, 28430.756, 31015.795, 35389.605, 52361.906, 100000};
    G4double SDLayer[100] = {0., 23., 44., 63., 80., 96., 110., 122., 134., 144., 152., 160., 167., 173., 178., 182., 185., 188.,
                             191., 193., 195., 197., 198., 199., 201., 201., 202., 203., 203., 204., 204., 204., 205.,
                             205., 205., 205., 206., 206., 206., 206., 206., 206., 206., 206., 206., 206., 206., 206.,
                             206., 206., 206., 206., 206., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207.,
                             207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207.,
                             207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207., 207.,
                             207., 207., 207., 207., 207., 207., 207.};
  };

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
