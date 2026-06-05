#include <iostream>
#include <string>

// #include <mat.h>
// #include <vector>

#include "DetectorConstruction.hh"

#include "TrackerSD.hh"
#include "SphericalSystemSDinAir.hh"
#include "SphericalSystemSDinOrbit.hh"
#include "G4SDManager.hh"

#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Isotope.hh"
#include "G4Element.hh"
#include "G4UnitsTable.hh"

#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4Orb.hh"
#include "G4Sphere.hh"
#include "G4Trd.hh"

// #include "G4GlobalMagFieldMessenger.hh"
// #include "G4EqMagElectricField.hh"
// #include "G4UniformElectricField.hh"

// #include "G4DormandPrince745.hh"
#include "G4FieldManager.hh"
#include "MyMagneticField.hh"
#include "FieldMessenger.hh"
#include "G4ChordFinder.hh"
#include "G4Mag_UsualEqRhs.hh"
#include "G4DormandPrinceRK78.hh"

// #include "G4Threading.hh"

#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

#include "G4AutoDelete.hh"
#include "G4GenericMessenger.hh"

#include "BinaryReader.hh"

namespace Test02
{

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  DetectorConstruction::DetectorConstruction(MyMagneticField *masterField)
      : G4VUserDetectorConstruction()
  {
    DefineCommands();
    fMagneticField = masterField;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  DetectorConstruction::~DetectorConstruction()
  {
    delete f_lat_lon_Messenger;
    delete fFieldMessenger;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  void DetectorConstruction::DefineCommands()
  {
    f_lat_lon_Messenger = new G4GenericMessenger(this, "/Geo/", "Field Control");

    // 定义命令：设置Lat
    f_lat_lon_Messenger->DeclarePropertyWithUnit("lat", "degree", fLat, "Set latitude");

    // 定义命令：设置Lon
    f_lat_lon_Messenger->DeclarePropertyWithUnit("lon", "degree", fLon, "Set longitude");
  };

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4VPhysicalVolume *DetectorConstruction::Construct()
  {

    G4int layer_loop = DetectorConstruction::GetAtmLayer(); // 获取大气层数
    G4NistManager *nist = G4NistManager::Instance();

    G4double Rmin = 6371.2 * km; // Rmax = 7371.2*km;
    G4double env_sizeX = Rmin * 1.5, env_sizeY = Rmin * 1.5, env_sizeZ = Rmin * 1.5;

    // Option to switch on/off checking of volumes overlaps
    //
    G4bool checkOverlaps = true;

    //
    // World
    //
    G4double world_sizeX = 1.2 * env_sizeX;
    G4double world_sizeY = 1.2 * env_sizeY;
    G4double world_sizeZ = 1.2 * env_sizeZ;
    G4Material *world_mat = nist->FindOrBuildMaterial("G4_Galactic");

    auto solidWorld = new G4Box("World",                                // its name
                                world_sizeX, world_sizeY, world_sizeZ); // its size

    auto logicWorld = new G4LogicalVolume(solidWorld, // its solid
                                          world_mat,  // its material
                                          "World");   // its name

    auto physWorld = new G4PVPlacement(nullptr,         // no rotation
                                       G4ThreeVector(), // at (0,0,0)
                                       logicWorld,      // its logical volume
                                       "World",         // its name
                                       nullptr,         // its mother  volume
                                       false,           // no boolean operation
                                       0,               // copy number
                                       checkOverlaps);  // overlaps checking

    //
    // Envelope
    //
    auto solidEnv = new G4Box("Envelope",                       // its name
                              env_sizeX, env_sizeY, env_sizeZ); // its size

    auto logicEnv = new G4LogicalVolume(solidEnv,    // its solid
                                        world_mat,   // its material
                                        "Envelope"); // its name

    new G4PVPlacement(nullptr,         // no rotation
                      G4ThreeVector(), // at (0,0,0)
                      logicEnv,        // its logical volume
                      "Envelope",      // its name
                      logicWorld,      // its mother  volume
                      false,           // no boolean operation
                      0,               // copy number
                      checkOverlaps);  // overlaps checking

    G4double sphi = 0 * deg, dphi = 360 * deg;
    G4double stheta = 0.0 * deg, dtheta = 180 * deg; // 这个dtheta是跨度不是末位值
    G4ThreeVector posSD = G4ThreeVector(0, 0, 0);
    // Sensitive Detector
    //
    // SD1 is the top detector use to stop simulation

    G4int SD1H = DetectorConstruction::GetSD1Height(); // 获取大气层数
    G4double SD1Rmin;

    SD1Rmin = Rmin + SD1H * km + 10.0 * cm;
    G4double SD1Rmax = SD1Rmin + 10 * cm;

    // Conical section shape

    auto solidSD1 = new G4Sphere("SD1",
                                 SD1Rmin,
                                 SD1Rmax,
                                 sphi, dphi,
                                 stheta, dtheta);

    logicSD1 = new G4LogicalVolume(solidSD1,  // its solid
                                   world_mat, // its material
                                   "SD1");    // its name

    new G4PVPlacement(nullptr,        // no rotation
                      posSD,          // at position
                      logicSD1,       // its logical volume
                      "SD1",          // its name
                      logicEnv,       // its mother  volume
                      false,          // no boolean operation
                      0,              // copy number
                      checkOverlaps); // overlaps checking

    //
    // SD2    放在原点下面->||.

    // Conical section shape
    G4double SD2Rmin = Rmin - 10 * cm;
    G4double SD2Rmax = SD2Rmin + 10 * cm;

    auto solidSD2 = new G4Sphere("SD2",
                                 SD2Rmin,
                                 SD2Rmax,
                                 sphi, dphi,
                                 stheta, dtheta);

    logicSD2 = new G4LogicalVolume(solidSD2,  // its solid
                                   world_mat, // its material
                                   "SD2");    // its name

    new G4PVPlacement(nullptr,        // no rotation
                      posSD,          // at position
                      logicSD2,       // its logical volume
                      "SD2",          // its name
                      logicEnv,       // its mother  volume
                      false,          // no boolean operation
                      0,              // copy number
                      checkOverlaps); // overlaps checking

    // G4int num = 100-layer_loop;

    givenum = 400 * km;

    //
    // Atmosphere combine with SD3

    // Material Preparing
    int layer = 208;

    // G4double s=0.001*mm;

    G4String name, symbol;
    G4int ncomponents;
    G4double a, z, fractionmass;

    // a = 1.01 * g / mole;
    // G4Element *elH = new G4Element(name = "Hydrogen", symbol = "H", z = 1., a);
    // a = 4.0026 * g / mole;
    // G4Element *elHe = new G4Element(name = "Helium", symbol = "He", z = 2., a);
    // a = 12.01 * g / mole;
    // G4Element *elC = new G4Element(name = "Carbon", symbol = "C", z = 6., a);
    a = 14.01 * g / mole;
    G4Element *elN = new G4Element(name = "Nitrogen", symbol = "N", z = 7., a);
    a = 16.00 * g / mole;
    G4Element *elO = new G4Element(name = "Oxygen", symbol = "O", z = 8., a);
    a = 39.948 * g / mole;
    G4Element *elAr = new G4Element(name = "Argon", symbol = "Ar", z = 18., a);
    /*
    FILE *fp = fopen("../ctable/datatest.dat", "rt");
    char ss[100];
    double n, o, ar, he, c, h, de_i;
    double de_n[layer], de_o[layer], de_c[layer], de_ar[layer], de_he[layer], de_h[layer], de[layer];
    if (!fp)
    {
      printf("Error on opening file density_above86.data. ");
    }
    else
    {
      fgets(ss, 100, fp);
      for (int i = 0; i < layer; i++)
      {
        fgets(ss, 100, fp);
        sscanf(ss, "%le%le%le%le%le%le%le", &h, &he, &c, &n, &o, &ar, &de_i);
        de_n[i] = n;
        de_o[i] = o;
        de_ar[i] = ar;
        de_he[i] = he;
        de_h[i] = h;
        de_c[i] = c;
        de[i] = de_i;
      };
    };

    fclose(fp);
    //  std::vector<std::string*> a;
    //  std::vector<std::string*> b;
    */

    BinaryReader reader;
    G4Material *Air[layer][36]; // 36 scale the 90N to 90S in 5 degree. 0->90N to 85N and so on.
    G4double SD3dtheta = 5 * degree;
    for (int nlat = 0; nlat < 36; nlat++)
    {
      std::string filenmae = "../ctable/layers_lat_" + std::to_string(nlat * 5 - 90) + ".csv";
      // G4cout << "filenmae: " << filenmae << G4endl;
      std::vector<std::vector<double>> detables = reader.readAtmLayer(filenmae);
      for (int cpnu = 0; cpnu < layer; cpnu++)
      {

        std::string autoSD3 = "LayerSD3_" + std::to_string(cpnu) + "_nlat_" + std::to_string(nlat);
        std::string autoair = "Air" + std::to_string(cpnu) + "_nlat_" + std::to_string(nlat);

        // G4cout << "-------------------GIN(det)---------------------" << G4endl;
        // G4cout << "layer_loop: " << layer_loop <<"  " << "num: " << cpnu << G4endl;
        // G4cout << "-------------------GIN(det)---------------------" << G4endl;

        G4double density = detables.at(cpnu).at(3) * mg / cm3;

        Air[cpnu][nlat] = new G4Material(name = autoair, density, ncomponents = 3);
        Air[cpnu][nlat]->AddElement(elN, fractionmass = detables.at(cpnu).at(0));
        Air[cpnu][nlat]->AddElement(elO, fractionmass = detables.at(cpnu).at(1));
        Air[cpnu][nlat]->AddElement(elAr, fractionmass = detables.at(cpnu).at(2));
        // Air[cpnu][nlat]->AddElement(elC, fractionmass = detables.at(cpnu).at(3));
        // Air[cpnu][nlat]->AddElement(elHe, fractionmass = detables.at(cpnu).at(3));
        // Air[cpnu][nlat]->AddElement(elH, fractionmass = detables.at(cpnu).at(3));

        // Geometry Setting

        G4double SD3Rmin, SD3Rmax;
        G4double SD3stheta = nlat * 5 * degree;
        SD3Rmin = Rmin + (AtmLayer[cpnu]) * m;
        SD3Rmax = Rmin + (AtmLayer[cpnu + 1]) * m;

        fLayRmininAir.push_back(SD3Rmin);

        G4Sphere *layerSolid = new G4Sphere(autoSD3, // its name
                                            SD3Rmin,
                                            SD3Rmax,
                                            sphi, dphi,
                                            SD3stheta, SD3dtheta); // its size

        G4LogicalVolume *layerLV = new G4LogicalVolume(layerSolid,      // its solid
                                                                        // world_mat,  // its material
                                                       Air[cpnu][nlat], // its material
                                                       autoSD3);        // its name

        logicSpherelayerinAir.push_back(layerLV);

        new G4PVPlacement(nullptr,        // no rotation
                          posSD,          // at position
                          layerLV,        // its logical volume
                          autoSD3,        // its name
                          logicEnv,       // its mother  volume
                          false,          // no boolean operation
                          cpnu,           // copy number
                          checkOverlaps); // overlaps checking

        // G4cout<<-25+i<<G4endl;
      };
    };

    // SD4 is the detector beyond the air layers to the given altitude, and have layer at each km.(每 1km 一层)

    // Conical section shape

    if (static_cast<int>(logicSphereSDlayerinOrbit.size()) != 0)
    {
      G4cout << "Error!!! The floglicSphereSDinOrbit should be empty before setting!! " << G4endl;
      return physWorld;
    };
    // Conical section shape
    for (int nlat = 0; nlat < 36; nlat++)
    {
      std::string filenmae = "../ctable/layers_lat_" + std::to_string(nlat * 5 - 90) + ".csv";
      // G4cout << "filenmae: " << filenmae << G4endl;
      std::vector<std::vector<double>> detables = reader.readAtmLayer(filenmae);
      for (int cpnu = 0; cpnu < 100; cpnu++)
      {
        std::string autoSD4 = "LayerSD4_" + std::to_string(cpnu)+"_nlat_" + std::to_string(nlat);

        G4double SD4Rmin, SD4Rmax;
        G4double SD4stheta = nlat * 5 * degree;
        SD4Rmin = Rmin + cpnu * km;
        SD4Rmax = SD4Rmin + 1 * cm;

        fLayRmininAir.push_back(SD4Rmin);

        G4Sphere *layerSolid = new G4Sphere(autoSD4, // its name
                                            SD4Rmin,
                                            SD4Rmax,
                                            sphi, dphi,
                                            SD4stheta, SD3dtheta); // its size

        G4LogicalVolume *layerLVSD4 = new G4LogicalVolume(layerSolid, // its solid
                                                       world_mat,  // its material
                                                       autoSD4);   // its name

        logicSphereSDlayerinAir.push_back(layerLVSD4);
        
        G4double n;
        
        n= nlat*layer+SDLayer[cpnu];
        
        new G4PVPlacement(nullptr,        // no rotation
                          posSD,          // at position
                          layerLVSD4,        // its logical volume
                          autoSD4,        // its name
                          logicSpherelayerinAir[n],       // its mother  volume
                          false,          // no boolean operation
                          cpnu,           // copy number
                          checkOverlaps); // overlaps checking
      };
    };

    // Set ShapeA as scoring volume
    fScoringVolume = logicEnv;

    //
    // always return the physical World
    //

    return physWorld;
  };

  void DetectorConstruction::ConstructSDandField()
  {
    // G4cout<<"GIN 1"<<G4endl;

    // Sensitive detectors activate

    G4String trackerChamberSD1name = "Test02/SD1";
    // G4cout<<"GIN 2"<<G4endl;
    auto aTrackerSD1 = new TrackerSD(trackerChamberSD1name, "TrackerHitsCollection1");
    // G4cout<<"GIN 3"<<G4endl;
    G4SDManager::GetSDMpointer()->AddNewDetector(aTrackerSD1);
    // G4cout<<"GIN 4"<<G4endl;
    SetSensitiveDetector(logicSD1, aTrackerSD1);
    // G4cout<<"GIN 5"<<G4endl;

    G4String trackerChamberSD2name = "Test02/SD2";
    // G4cout<<"GIN 2"<<G4endl;
    auto aTrackerSD2 = new TrackerSD(trackerChamberSD2name, "TrackerHitsCollection2");
    // G4cout<<"GIN 3"<<G4endl;
    G4SDManager::GetSDMpointer()->AddNewDetector(aTrackerSD2);
    // G4cout<<"GIN 4"<<G4endl;
    SetSensitiveDetector(logicSD2, aTrackerSD2);
    // G4cout<<"GIN 5"<<G4endl;

    auto SphereSDinAir = new SphericalSystemSDinAir("SphericalDetectorinAir", fLayRmininAir);
    G4SDManager *sdManagerinAir = G4SDManager::GetSDMpointer();
    sdManagerinAir->AddNewDetector(SphereSDinAir);

    for (size_t j = 0; j < logicSphereSDlayerinAir.size(); ++j)
    {
      logicSphereSDlayerinAir.at(j)->SetSensitiveDetector(SphereSDinAir);
    };
    /*
    auto SphereSDinOrbit = new SphericalSystemSDinOrbit("SphericalDetectorinOrbit", fLayRmininOrbit);
    G4SDManager *sdManagerinOrbit = G4SDManager::GetSDMpointer();
    sdManagerinOrbit->AddNewDetector(SphereSDinOrbit);

    for (size_t j = 0; j < logicSphereSDlayerinOrbit.size(); ++j)
    {
      logicSphereSDlayerinOrbit.at(j)->SetSensitiveDetector(SphereSDinOrbit);
    };
    */
    // Create global magnetic field messenger.
    // Uniform magnetic field is then created automatically if
    // the field value is not zero.
    G4bool localflag = true;

    // fMagneticField = new MyMagneticField();
    // fFieldMessenger = new FieldMessenger(fMagneticField);

    /*G4double fLat, fLon;
    fLat=0.0*deg;
    fLon=0.0*deg;*/

    // G4cout<< "-------------GIN0.03---------------"<< G4endl;
    // G4cout << "fLat: " << fLat/deg << "\t" << "fLon: " << fLon/deg << G4endl;
    // G4cout<< "-------------GIN0.03---------------"<< G4endl;

    static MyMagneticField *threadField = nullptr;

    if (G4Threading::IsWorkerThread())
    {
      // 工作线程：克隆主磁场
      // 关键：此时主线程的UI命令已经执行完毕！
      // G4cout<<"---------------GGB---------------"<<G4endl;
      // G4cout<<"clone magnetic field in worker thread"<<G4endl;
      // G4cout<<"---------------GGB---------------"<<G4endl;
      threadField = fMagneticField->Clone();
    }
    else
    {
      // 主线程：使用主磁场对象
      threadField = fMagneticField;
    }

    G4FieldManager *fieldMgr = new G4FieldManager(threadField);
    G4Mag_UsualEqRhs *equation = new G4Mag_UsualEqRhs(threadField);
    G4MagIntegratorStepper *stepper = new G4DormandPrinceRK78(equation);
    // fieldMgr->SetDetectorField(magField);
    G4double minStep = 1 * cm;
    G4ChordFinder *chordFinder = new G4ChordFinder(threadField, minStep, stepper);
    // fieldMgr->CreateChordFinder(threadField); // 这一句是自动设置计算积分，
    fieldMgr->SetChordFinder(chordFinder);
    fScoringVolume->SetFieldManager(fieldMgr, localflag);
    // Register the field messenger for deleting
    // G4AutoDelete::Register(fMagneticField);
  };
  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

};
