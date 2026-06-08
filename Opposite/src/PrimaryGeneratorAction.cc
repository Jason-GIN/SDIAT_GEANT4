#include "PrimaryGeneratorAction.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4RunManager.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

#include "DetectorConstruction.hh"

#include <iostream>
#include <string>
#include <cmath>
#include <thread>
#include <fstream>
#include <sstream>

#include "DirectionData.hh"
#include "BinaryReader.hh"

namespace Test02
{
  //const int nu = 91;
  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  PrimaryGeneratorAction::PrimaryGeneratorAction(DetectorConstruction *detCon)
      : G4VUserPrimaryGeneratorAction(), fDetectorConstruction(detCon)
  {
    G4int n_particle = 1;
    fParticleGun = new G4ParticleGun(n_particle);
    // fGPS = new G4GeneralParticleSource();
    // G4String fCurrentGenerator = "Primary";
    fDir = new G4UIdirectory("/generator/");
    fDir->SetGuidance("Select primary generator");
    fSelectGeneratorCmd = new G4UIcmdWithAString("/generator/select", this);
    fSelectGeneratorCmd->SetGuidance("Choose Primary or Secondary");
    fSelectGeneratorCmd->SetParameterName("type", false);
    fSelectGeneratorCmd->SetCandidates("Primary Secondary");
    fSelectGeneratorCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fReadfileCmd = new G4UIcmdWithAString("/generator/Readfile", this);
    fReadfileCmd->SetGuidance("Set particle by PDG code for the current source");
    fReadfileCmd->SetParameterName("filename", false);
    fReadfileCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
    
    /*
    if (!G4Threading::IsWorkerThread())
    {
      // 注意：这里必须 new 一个静态/全局的生命期较长的 Messenger，
      // 不能是局部变量，否则离开构造函数就被销毁。
      static MyGenMessenger messenger; // 静态局部，只构造一次
      (void)messenger;                 // 避免未使用警告
    }
    */
    // default particle kinematic
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4String particleName;
    G4ParticleDefinition *particle = particleTable->FindParticle(particleName = "proton");
    fParticleGun->SetParticleDefinition(particle);

    fParticleGun->SetParticleEnergy(11.98 * GeV);
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  PrimaryGeneratorAction::~PrimaryGeneratorAction()
  {
    delete fParticleGun;
  }
  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  void PrimaryGeneratorAction::SetNewValue(G4UIcommand *command, G4String newValue)
  {
    if (command == fSelectGeneratorCmd)
    {
      fCurrentGenerator = newValue;
      G4cout << "Generator set to: " << newValue << G4endl;
    }
    if (command == fReadfileCmd)
    {
      std::string filename = newValue;
      std::ifstream file(filename);
      if (!file.is_open())
      {
        G4cout << "Error! Something wrong with opening file." << G4endl;
        return;
      };
      std::string line;
      int currentRow = 0;
      ParticleInfo p;
      while (std::getline(file, line))
      {
        if (line.empty())
          continue;
        std::istringstream ss(line);
        std::string cell;
        std::vector<std::string> fields;
        int col = 0;
        while (col < 8 && std::getline(ss, cell, ','))
        {
          fields.push_back(cell);
        }
        // 前3列 -> position
        p.position = G4ThreeVector(std::stod(fields[0]) * km, std::stod(fields[1]) * km, std::stod(fields[2]) * km);
        // 第4-6列 -> direction
        p.direction = G4ThreeVector(std::stod(fields[3]), std::stod(fields[4]), std::stod(fields[4]));
        // 第7列 -> kineticEnergy
        p.kineticEnergy = std::stod(fields[6]) * MeV;
        // 第8列 -> pdgCode
        p.pdgCode = std::stoi(fields[7]);

        fParticleList.push_back(p);
      }
      // G4cout << "Particle code set to: " << newValue << G4endl;
    }
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4double GeneratorEnergyFromSpectrum(int nu)
  {

    G4double energies[nu]; // 这个就是输入能谱的分bin的每个bin上的值。
    G4double weights[nu];  // 这个是每个bin的权重，通过这个可以做出不同形状的谱。

    FILE *fp = fopen("Spectrum.dat", "rt");
    char ss[100];
    if (!fp)
    {
      printf("Error on opening file density_above86.data. ");
    }
    else
    {
      // fgets(ss,100,fp);
      double en, we;

      for (int i = 0; i < nu; i++)
      {
        fgets(ss, 100, fp);
        sscanf(ss, "%le %le", &en, &we);
        energies[i] = en * GeV;
        weights[i] = we;
        /*
        G4cout << "-------------GIN---------------"<<G4endl;
        G4cout << "en:" << "\t"<< en <<G4endl;
        G4cout << "we:" << "\t"<< we <<G4endl;
        G4cout << "energy" << "\t" << energies[i] <<"\t"<<"i:"<<i<< G4endl;
        G4cout << "-------------GIN---------------"<<G4endl;
        */
      };
    };

    fclose(fp);

    G4RandGeneral randGen(weights, nu); // 这里的3是bin数，也就是数组的长度。
    int index = static_cast<int>(std::floor(randGen.fire() * nu));
    /*
    G4cout << "-------------GIN---------------"<<G4endl;
    G4cout << "index:" << "\t"<< index <<G4endl;
    G4cout << "energy:" << "\t"<< energies[index] <<G4endl;
    G4cout << "energyt:" << "\t"<< energies[84] <<G4endl;
    G4cout << "-------------GIN---------------"<<G4endl;
    */
    return energies[index];
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4int SelectDirection(G4double theta, G4double phi, std::string str1)
  {
    // 放进来的是角度制
    double rlist[361];
    double thetalist[361];
    double philist[361];

    FILE *fp = fopen(str1.c_str(), "rt");
    char ss[100];
    if (!fp)
    {
      printf("Error on opening file density_above86.data. ");
    }
    else
    {

      double ri, thetai, phii;

      for (int i = 0; i < 361; i++)
      {
        fgets(ss, 100, fp);
        sscanf(ss, "%le %le %le", &ri, &thetai, &phii);
        rlist[i] = ri;
        thetalist[i] = thetai;
        philist[i] = phii;
        if (i == 0 && phii != -180.0)
        {
          fclose(fp);
          return -1;
        };
      };
    };
    fclose(fp);

    int a = 0;
    int b = 360;
    int c;

    // 用二分法区分是否在允许锥内
    while (abs(a - b) != 1)
    {

      if (philist[b] > phi && philist[a] < phi)
      {
        c = b;
        b = (a + b + 1) / 2;
      }
      else if (philist[b] < phi)
      {
        a = b;
        b = c;
      };
      if (philist[b] == phi || philist[a] == phi)
      {
        a = b;
        break;
      };
    };

    if (a == b)
    {
      double ruletheta = thetalist[a];
      // G4cout<<"thetalist[a]=thetalist[b]: "<<thetalist[a]<<G4endl;
      // G4cout<<"ruletheta: "<<ruletheta<<G4endl;
      if (ruletheta > theta)
      {

        return 0;
      }
      else
      {

        return 1;
      }
    }
    else
    {
      double ruletheta = (thetalist[a] + thetalist[b]) / 2;
      // G4cout<<"thetalist[a]: "<<thetalist[a]<<G4endl;
      // G4cout<<"thetalist[b]: "<<thetalist[b]<<G4endl;
      // G4cout<<"ruletheta: "<<ruletheta<<G4endl;
      if (ruletheta > theta)
      {

        return 0;
      }
      else
      {

        return 1;
      }
    }
  };

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
  std::tuple<std::vector<double>, std::vector<double>>
  myselect(const std::vector<double> array, const std::vector<double> array0, double up, double down)
  {
    // this is the function to see in a give region of one parameter
    std::stringstream ss;
    // ss << "Thread " << std::this_thread::get_id()
    //    << ": myselect called with arrays of size " << array.size()
    //    << " and " << array0.size() << G4endl;
    // G4cout << ss.str();
    std::vector<double> result, result0;
    for (int i = 0; i < static_cast<int>(array.size()); i++)
    {
      if (down <= array.at(i) && array.at(i) <= up)
      {
        result.push_back(array.at(i));
        result0.push_back(array0.at(i));
      }
      else
      {
        continue;
      };
    };

    if (result0.size() > 1000000)
    { // 设置合理的上限
      G4cout << "警告：异常大的结果向量 size=" << result0.size()
             << " in thread " << std::this_thread::get_id() << G4endl;
    }

    return {result, result0};
  };
  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4int SelectDirection2(G4double theta, G4double phi, std::string str1)
  {
    // 放进来的是角度制
    std::vector<G4double> rlist;
    std::vector<G4double> thetalist;
    std::vector<G4double> philist;

    // Read the data of allowed cone. And save in vector.
    FILE *fp = fopen(str1.c_str(), "rt");
    char ss[100];
    if (!fp)
    {
      printf("Error on opening file density_above86.data. ");
    }
    else
    {

      G4double ri, thetai, phii;

      while (fgets(ss, 100, fp))
      {
        sscanf(ss, "%le %le %le", &ri, &thetai, &phii);
        rlist.push_back(ri);
        thetalist.push_back(thetai);
        philist.push_back(phii);
      };
    };
    fclose(fp);

    // G4cout << "-------GIN0---------" << G4endl;
    // G4cout << "thetalist.at(0):\t" << thetalist.at(0) << G4endl;
    // G4cout << "thetalist.at(thetalist.size()-1):\t" << thetalist.at(thetalist.size()-1) << G4endl;

    // chack if the theta are in the reagion
    if (theta < thetalist.at(0) || theta > thetalist.at(thetalist.size() - 1))
    {
      return 1;
    };

    // divied the cone into two part.
    //  larger than  theta and smaller than theta.
    //  and get the smaller one.
    std::vector<G4double> philistl, thetalistl;
    for (int i = 0; i < static_cast<int>(philist.size()); i++)
    {

      if (thetalist.at(i) <= theta)
      {
        philistl.push_back(philist.at(i));
        thetalistl.push_back(thetalist.at(i));
      }
      else
      {
        continue;
      };
    };

    auto results1 = myselect(philistl, thetalistl, phi + 1, phi - 1);
    auto &ordphi = std::get<0>(results1);    // 获取第一个vector
    auto &ordptheta = std::get<1>(results1); // 获取第二个vector

    if (ordptheta.empty())
    {
      return 1;
    };

    // G4cout << "ordptheta.size():\t" << ordptheta.size() << G4endl;

    // for(int i=0; i < static_cast<int>(ordptheta.size()) ; i++){
    //   if (i!=static_cast<int>(ordptheta.size()-1)){
    //     G4cout << ordptheta.at(i) << " ,";
    //   }else{
    //     G4cout << ordptheta.at(i) << G4endl;
    //   };
    // }

    int n = 1;
    int s = 0;
    int e = 0;
    // G4cout << "s & e :\t" << s  << "," << e << G4endl;
    for (int i = 0; i < static_cast<int>(ordptheta.size() - 1); i++)
    {
      if (fabs(ordptheta.at(i) - ordptheta.at(i + 1)) < 2)
      {
        e++;
        continue;
      }
      else
      {
        if (fabs(ordphi.at(s) - ordphi.at(e)) < 1)
        {
          e++;
          s = e;
          continue;
        }
        else
        {
          e++;
          s = e;
          n++;
        };
      };
    };

    // G4cout << "s & e :\t" << s  << "," << e << G4endl;
    // G4cout << "fabs(ordphi.at(s)-ordphi.at(e)): " << fabs(ordphi.at(s)-ordphi.at(e)) << G4endl;
    if (s == 0 && e == static_cast<int>(ordptheta.size() - 1) && fabs(ordphi.at(s) - ordphi.at(e)) < 1)
    {
      n = 0;
      return 1;
    };

    // G4cout << "n: \t" << n << G4endl;

    if ((n % 2) == 0)
    {
      return 1;
    }
    else
    {
      return 0;
    };
  };

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4ThreeVector xyz2r(G4ThreeVector xyz)
  {
    G4ThreeVector rtp;
    rtp[0] = std::sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);
    rtp[1] = std::acos(xyz[2] / rtp[0]);
    rtp[2] = std::atan2(xyz[1], xyz[0]);

    return rtp;
  }

  G4ThreeVector VectorGTODinZenith(G4ThreeVector xyz2, G4ThreeVector xyz1)
  {
    G4ThreeVector xyzn, rtp;
    rtp = xyz2r(xyz1);
    double cos_p = std::cos(rtp[2]), sin_p = std::sin(rtp[2]);
    double cos_t = std::cos(rtp[1]), sin_t = std::sin(rtp[1]);
    xyzn[0] = cos_p * cos_t * xyz2[0] + sin_p * cos_t * xyz2[1] - sin_t * xyz2[2];
    xyzn[1] = -sin_p * xyz2[0] + cos_p * xyz2[1];
    xyzn[2] = cos_p * sin_t * xyz2[0] + sin_p * sin_t * xyz2[1] + cos_t * xyz2[2];

    return xyzn;
  }

  G4ThreeVector VectorGTODinZenith1(G4ThreeVector xyz1, G4double t, G4double p)
{
    G4ThreeVector xyzn;
    double cos_p = std::cos(p), sin_p = std::sin(p);
    double cos_t = std::cos(t), sin_t = std::sin(t);
    xyzn[0] = cos_p * cos_t * xyz1[0] + sin_p * cos_t * xyz1[1] - sin_t * xyz1[2];
    xyzn[1] = -sin_p * xyz1[0] + cos_p * xyz1[1];
    xyzn[2] = cos_p * sin_t * xyz1[0] + sin_p * sin_t * xyz1[1] + cos_t * xyz1[2];

    return xyzn;
}

  G4ThreeVector VectorZenithinGTOD(G4ThreeVector xyz2, G4ThreeVector xyz1)
  {
    G4ThreeVector xyzn, rtp;
    rtp = xyz2r(xyz1);
    double cos_p = std::cos(rtp[2]), sin_p = std::sin(rtp[2]);
    double cos_t = std::cos(rtp[1]), sin_t = std::sin(rtp[1]);
    xyzn[0] = cos_p * cos_t * xyz2[0] - sin_p * xyz2[1] + cos_p * sin_t * xyz2[2];
    xyzn[1] = sin_p * cos_t * xyz2[0] + cos_p * xyz2[1] + sin_p * sin_t * xyz2[2];
    xyzn[2] = -sin_t * xyz2[0] + cos_t * xyz2[2];

    return xyzn;
  }

  G4ThreeVector VectorZenithinGTOD1(G4ThreeVector xyz1, G4double t, G4double p)
{
    G4ThreeVector xyzn;
    double cos_p = std::cos(p), sin_p = std::sin(p);
    double cos_t = std::cos(t), sin_t = std::sin(t);
    xyzn[0] = cos_p * cos_t * xyz1[0] - sin_p * xyz1[1] + cos_p * sin_t * xyz1[2];
    xyzn[1] = sin_p * cos_t * xyz1[0] + cos_p * xyz1[1] + sin_p * sin_t * xyz1[2];
    xyzn[2] = -sin_t * xyz1[0] + cos_t * xyz1[2];

    return xyzn;
}

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4ThreeVector GeneratorDirection(std::string str0, G4double x, G4double y, G4double z, G4double lat, G4double lon)
  { //  delete[] arrayname;      调用时莫忘释放内存
    //
    // input parameter
    // str0 is the file adress
    // x,y,z is the position of partilce in zenith frame at geolocation lat, lon.
    //
    // G4cout << "-------------GIN0.1---------------"<< G4endl;
    G4int aa; // a number used to see the select result of direction select funtion.
              // aa=1 -> in cone; aa=0 -> not in cone need to regaenerate a derection.
              // aa=-1 -> SelectDirection1 is not useful in this case, need to turn to SelectDirection2.

    G4double pcostheta; // costheta of direction generate
    G4double psintheta; // sintheta of direction generate
    G4double pphi;      // phi of direction generate
    do
    {
      // use random to generate
      pcostheta = 2. * (G4UniformRand() - 0.5);
      psintheta = std::sqrt(1. - pcostheta * pcostheta);
      pphi = (G4UniformRand() * 360.0) - 180;
      G4double theta = std::acos(pcostheta) / M_PI * 180.0;

      // G4cout << "-------------GIN0.11---------------"<< G4endl;
      aa = SelectDirection(theta, pphi, str0);
      // G4cout << "-------------GIN0.12---------------"<< G4endl;

      if (aa == -1)
      {
        aa = SelectDirection2(theta, pphi, str0);
      };

    } while (aa);

    G4ThreeVector dp; // a vector of direction in xyz in zenith at position of particle.
    //-----------posision--------------//
    // dp[0]=psintheta * std::cos(pphi)*r;
    // dp[1]=-psintheta * std::sin(pphi)*r;
    // dp[2]=-pcostheta*r;
    //-----------direction------------//
    dp[0] = -psintheta * std::cos(pphi * deg);
    dp[1] = -psintheta * std::sin(pphi * deg);
    dp[2] = -pcostheta;

    G4ThreeVector xyz, xyzm, vm, ddp;
    // xyz is the position in zenith at (lat,lon)
    // xyzm is the position in GTOD
    // vm is the direction in GTOD
    // ddp id the direction in zenith at (lat,lon)

    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;

    // G4cout << "-------------GIN0.2---------------"<< G4endl;
    xyzm = VectorZenithinGTOD1(xyz, (90.0 * deg - lat), lon);
    vm = VectorZenithinGTOD(dp, xyzm);
    ddp = VectorGTODinZenith1(vm, (90.0 * deg - lat), lon);

    return ddp;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4ThreeVector PrimaryGeneratorAction::GeneratorDirectionRoot(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow)
  {
    // waiting for solve the multethrould problem.

    std::vector<int> rowdata;
    unsigned id;
    rowdata = BinaryReader::readROOTSpecificBranch(filename, targetrow);
    // G4cout << "-------------GIN0.21---------------"<< G4endl;
    // G4cout << "rowdata.at(505): " << rowdata.at(505) << G4endl;
    // G4cout << "-------------GIN0.21---------------"<< G4endl;
    if (!rowdata.empty())
    {
      do
      {
        id = static_cast<unsigned>(G4UniformRand() * 12000);
        // aflag =
      } while (rowdata.at(id) == 0);
    }
    else
    {
      G4cout << "Error!!! No this file or no this row. "
             << "Please chack the file name and row number."
             << "Current row number is:" << targetrow
             << "Current file name is:" << filename
             << " in thread " << std::this_thread::get_id() << G4endl;
      return G4ThreeVector(0, 0, 0);
    };

    G4ThreeVector dp = DirectionData::getDirection(static_cast<size_t>(id));

    G4ThreeVector xyz, xyzm, vm, ddp, oddp;
    // xyz is the position in zenith at (lat,lon)
    // xyzm is the position in GTOD
    // vm is the direction in GTOD
    // ddp id the direction in zenith at (lat,lon)

    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    /*
    G4cout << "-------------GIN0.22---------------"<< G4endl;
    //G4cout << "dp: " << dp << G4endl;
    G4cout << "id: " << id << G4endl;
    G4cout << "rowdata.at(id): " << rowdata.at(id) << G4endl;
    G4cout << "-------------GIN0.22---------------"<< G4endl;
    */
    xyzm = VectorZenithinGTOD1(xyz, (90.0 * deg - lat), lon);
    vm = VectorZenithinGTOD(dp, xyzm);
    ddp = VectorGTODinZenith1(vm, (90.0 * deg - lat), lon);
    oddp[0] = -ddp[0];
    oddp[1] = -ddp[1];
    oddp[2] = -ddp[2];

    return oddp;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4ThreeVector PrimaryGeneratorAction::GeneratorDirectionCSV(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow)
  {
    std::bitset<13963> rowdata;
    int id;

    // G4cout << "-------------GIN0.20---------------"<< G4endl;
    // G4cout << "bf rowdata.test(505): " << rowdata.test(505) << G4endl;
    // G4cout << "-------------GIN0.20---------------"<< G4endl;

    if (BinaryReader::readCSVSpecificRow(filename, targetrow, rowdata))
    {

      // G4cout << "-------------GIN0.21---------------"<< G4endl;
      // G4cout << "af rowdata.test(505): " << rowdata.test(505) << G4endl;
      // G4cout << "-------------GIN0.21---------------"<< G4endl;

      do
      {

        id = static_cast<int>(G4UniformRand() * 7500);
        // aflag =
      } while (rowdata.test(id) == 0);
    }
    else
    {
      G4cout << "Error!!! No this file or no this row. "
             << "Please chack the file name and row number."
             << "Current row number is:" << targetrow
             << "Current file name is:" << filename
             << " in thread " << std::this_thread::get_id() << G4endl;
      return G4ThreeVector(0, 0, 0);
    };

    G4ThreeVector dp = DirectionData::getDirection(static_cast<size_t>(id));

    G4ThreeVector xyz, xyzm, vm, ddp, oddp;
    // xyz is the position in zenith at (lat,lon)
    // xyzm is the position in GTOD
    // vm is the direction in GTOD
    // ddp id the direction in zenith at (lat,lon)

    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    /*
    G4cout << "-------------GIN0.22---------------"<< G4endl;
    //G4cout << "dp: " << dp << G4endl;
    G4cout << "id: " << id << G4endl;
    G4cout << "rowdata.at(id): " << rowdata.at(id) << G4endl;
    G4cout << "-------------GIN0.22---------------"<< G4endl;
    */
    xyzm = VectorZenithinGTOD1(xyz, (90.0 * deg - lat), lon);
    vm = VectorZenithinGTOD(dp, xyzm);
    ddp = VectorGTODinZenith1(vm, (90.0 * deg - lat), lon);
    /*
    oddp[0]=-ddp[0];
    oddp[1]=-ddp[1];
    oddp[2]=-ddp[2];
    */
    oddp = -ddp;

    return oddp;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  G4ThreeVector PrimaryGeneratorAction::GeneratorDirectionCSV1(std::string filename, G4double x, G4double y, G4double z, G4double lat, G4double lon, int targetrow)
  {
    /*
    std::bitset<13963> rowdata;
    int id;
    // G4cout << "-------------GIN0.20---------------"<< G4endl;
    // G4cout << "bf rowdata.test(505): " << rowdata.test(505) << G4endl;
    // G4cout << "-------------GIN0.20---------------"<< G4endl;
    
    if (BinaryReader::readCSVSpecificRow(filename, targetrow, rowdata))
    {

      // G4cout << "-------------GIN0.21---------------"<< G4endl;
      // G4cout << "af rowdata.test(505): " << rowdata.test(505) << G4endl;
      // G4cout << "-------------GIN0.21---------------"<< G4endl;

      do
      {

        id = static_cast<int>(G4UniformRand() * 7500);
        // aflag =
      } while (rowdata.test(id) == 0);
    }
    else
    {
      G4cout << "Error!!! No this file or no this row. "
             << "Please chack the file name and row number."
             << "Current row number is:" << targetrow
             << "Current file name is:" << filename
             << " in thread " << std::this_thread::get_id() << G4endl;
      return G4ThreeVector(0, 0, 0);
    };
    */

    if (!fDirectionCache.load(filename)) {
        G4cout << "Error loading: " << filename << G4endl;
        return G4ThreeVector(0, 0, 0);
    }
    int id = fDirectionCache.getRandomAllowed(targetrow);

    G4ThreeVector dp = DirectionData::getDirection(static_cast<size_t>(id));

    G4ThreeVector  xyzm, ddp, oddp;
    // xyz is the position in zenith at (lat,lon)
    // xyzm is the position in GTOD
    // vm is the direction in GTOD
    // ddp id the direction in zenith at (lat,lon)

    //xyz[0] = x;
    //xyz[1] = y;
    //xyz[2] = z;
    /*
    G4cout << "-------------GIN0.22---------------"<< G4endl;
    //G4cout << "dp: " << dp << G4endl;
    G4cout << "id: " << id << G4endl;
    G4cout << "rowdata.at(id): " << rowdata.at(id) << G4endl;
    G4cout << "-------------GIN0.22---------------"<< G4endl;
    */

    xyzm = VectorZenithinGTOD1(G4ThreeVector(x,y,z), (90.0 * deg - lat), lon);
    ddp = VectorZenithinGTOD(dp, xyzm);
    // ddp = VectorGTODinZenith1(vm,(90.0*deg-lat),lon);
    
    oddp = -ddp;

    return oddp;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  void PrimaryGeneratorAction::GeneratePrimaries(G4Event *anEvent)
  {
    // this function is called at the begining of ecah event
    //

    // In order to avoid dependence of PrimaryGeneratorAction
    // on DetectorConstruction class we get Envelope volume
    // from G4LogicalVolumeStore.
    if (fCurrentGenerator == "Primary")
    {
      G4double envSizeXY = 0;
      G4double envSizeZ = 0;

      if (!fEnvelopeBox)
      {
        G4LogicalVolume *envLV = G4LogicalVolumeStore::GetInstance()->GetVolume("Envelope");
        if (envLV)
          fEnvelopeBox = dynamic_cast<G4Box *>(envLV->GetSolid());
      }

      if (fEnvelopeBox)
      {
        envSizeXY = fEnvelopeBox->GetXHalfLength() * 2.;
        envSizeZ = fEnvelopeBox->GetZHalfLength() * 2.;
      }
      else
      {
        G4ExceptionDescription msg;
        msg << "Envelope volume of box shape not found.\n";
        msg << "Perhaps you have changed geometry.\n";
        msg << "The gun will be place at the center.";
        G4Exception("PrimaryGeneratorAction::GeneratePrimaries()",
                    "MyCode0002", JustWarning, msg);
      }

      G4double tod = DetectorConstruction::givenum;
      G4double lat, lon;
      if (fDetectorConstruction)
      { // 安全检查指针是否有效
        // 通过指针调用公共成员函数获取值
        lat = fDetectorConstruction->GetfLat();
        lon = fDetectorConstruction->GetfLon();

        // 现在你可以使用fieldZ和detPos来影响粒子生成了
        // 例如，根据探测器位置调整粒子发射起点
        // particleGun->SetParticlePosition(detPos);
      }
      else
      {
        G4ExceptionDescription msg;
        msg << "DetectorConstruction pointer is not set in PrimaryGeneratorAction.";
        G4Exception("PrimaryGeneratorAction::GeneratePrimaries", "PG001", FatalException, msg);
      };
      // G4double lat = 31.15*deg;
      // G4double lon =-100.0*deg;
      G4cout << "-------------GIN0.3---------------" << G4endl;
      G4cout << "lat: " << lat / deg << "\t" << "lon: " << lon / deg << G4endl;
      G4cout << "-------------GIN0.3---------------" << G4endl;

      G4double R = 6371.2 * km + tod + 0.5 * km + 9.5 * cm;
      // G4double l = G4UniformRand()*300*km; // arc length
      G4double l = G4UniformRand() * R * (10 * deg); // arc length
      G4double r = std::sin(l / R) * R;              // string
      // G4double size = 0.8;
      // G4double pphi = 0. * deg;
      // G4double x0 = size * envSizeXY * (G4UniformRand()-0.5);
      // G4double y0 = size * envSizeXY * (G4UniformRand()-0.5);
      G4double theta = G4UniformRand() * 360. * deg;
      G4double cos1 = std::cos(theta);
      G4double sin1 = std::sin(theta);

      G4double x0 = cos1 * r;                          // 100*km;
      G4double y0 = sin1 * r;                          // 0;
      G4double z0 = std::cos(l / R) * R - 6371.2 * km; // 99.2273*km;
      G4double z1 = std::cos(l / R) * R;

      G4ThreeVector dpp, pos;
      pos = VectorZenithinGTOD1(G4ThreeVector(x0, y0, z1), 90. * deg - lat, lon);
      G4double Ek = fParticleGun->GetParticleEnergy();
      G4String Pname = fParticleGun->GetParticleDefinition()->GetParticleName();

      int targetrow;
      if (Pname == "proton")
      {
        for (int i = 0; i < 43; i++)
        {
          if (E1[i] == Ek)
          {
            targetrow = num[i];
            break;
          }
          else
          {
            continue;
          };
        };
      }
      else
      {
        for (int i = 0; i < 43; i++)
        {
          if (E2[i] == Ek)
          {
            targetrow = num[i];
            break;
          }
          else
          {
            continue;
          };
        };
      };
      std::string str0 = "../ctable/all_" + std::to_string(int(std::round(lon / deg))) + "_" + std::to_string(int(std::round(lat / deg))) + ".csv";
      G4cout << "-------------GIN---------------" << G4endl;
      G4cout << "Ek: " << Ek << G4endl;
      G4cout << "bf targetrow: " << targetrow << G4endl;
      // targetrow=53;
      // G4cout << "fulltest targetrow: " << targetrow << G4endl;
      G4cout << "str0: " << str0 << G4endl;
      G4cout << "-------------GIN---------------" << G4endl;

      dpp = GeneratorDirectionCSV1(str0,pos[0],pos[1],pos[2],lat,lon,targetrow);
      // G4double energy= GeneratorEnergyFromSpectrum(nu);
      // G4cout << "-------------GIN1---------------"<< G4endl;

      // fParticleGun->SetParticleMomentumDirection(-pos);
      // fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0 ,1));
      fParticleGun->SetParticleMomentumDirection(dpp);
      fParticleGun->SetParticlePosition(pos);
      // fParticleGun->SetParticleEnergy(energy);

      fParticleGun->GeneratePrimaryVertex(anEvent);
    }
    else if (fCurrentGenerator == "Secondary")
    {
      {
        G4int evtID = anEvent->GetEventID();
        //G4cout << "evtID: " << evtID << G4endl;
        //G4cout << "fParticleList.size(): " << fParticleList.size() << G4endl;
        if (evtID < 0 || evtID >= (G4int)fParticleList.size())
        {
          //G4cout << "evtID: " << evtID << G4endl;
          //G4cout << "fParticleList.size(): " << fParticleList.size() << G4endl;
          G4Exception("MyPrimaryGenerator::GeneratePrimaries", "EvtOutOfRange",
                      FatalException, "Event ID out of range of loaded particle list");
          return;
        }
        const ParticleInfo &info = fParticleList[evtID];
        G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
        // G4cout << "---------------GIN----------------" << G4endl;
        // G4cout << "fPDGcode: " << fPDGcode << G4endl;
        // G4cout << "---------------GIN----------------" << G4endl;
        G4ParticleDefinition *particle = particleTable->FindParticle(info.pdgCode);
        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticlePosition(info.position);
        fParticleGun->SetParticleMomentumDirection(info.direction);
        fParticleGun->SetParticleEnergy(info.kineticEnergy);
        fParticleGun->GeneratePrimaryVertex(anEvent);
      }
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
  }
}
