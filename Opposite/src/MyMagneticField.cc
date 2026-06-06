#include "MyMagneticField.hh"
#include "globals.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4ThreeVector.hh"
#include "G4Threading.hh"
#include <cmath>

MyMagneticField::MyMagneticField()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
/*
MyMagneticField::MyMagneticField(G4bool FieldEnabled,
                    G4String FieldType,
                    G4ThreeVector Geolocation,
                    G4ThreeVector UniStrength)
{
    fGeolocation = Geolocation;
    fUniStrength = UniStrength;
    fFieldType = FieldType;
    fFieldEnabled = FieldEnabled;
}
*/
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

MyMagneticField *MyMagneticField::Clone() const
{

    MyMagneticField *clonedField = new MyMagneticField();

    // 复制所有成员变量
    clonedField->fFieldType = fFieldType;
    clonedField->fGeolocation = fGeolocation;
    clonedField->fUniStrength = fUniStrength; // vector的深拷贝
    // clonedField->fFieldCenter = fFieldCenter;
    // clonedField->fFieldRadius = fFieldRadius;
    clonedField->fFieldEnabled = fFieldEnabled;

    return clonedField;
}

/*
G4Field* MyMagneticField::Clone() const{
    return new MyMagneticField::MyMagneticField(fFieldEnabled,fFieldEnabled,fGeolocation,fUniStrength);
}
*/
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

MyMagneticField::~MyMagneticField()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4ThreeVector xyz2rtp(G4ThreeVector xyz)
{
    G4ThreeVector rtp;
    rtp[0] = std::sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);
    rtp[1] = std::acos(xyz[2] / rtp[0]);
    rtp[2] = std::atan2(xyz[1], xyz[0]);

    return rtp;
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
void MyMagneticField::GetFieldValue(const G4double Track[7], G4double B[3]) const
{
    // G4cout<<"-----------------GINGIN------------------"<< G4endl;
    // G4cout<<"fFieldEnabled: "<<fFieldEnabled<<G4endl;
    // G4cout<<"fFieldType: "<<fFieldType<<G4endl;
    // G4cout<<"fUniStrength: "<<fUniStrength[0]<<", "<<fUniStrength[1]<<", "<<fUniStrength[2]<<G4endl;
    // G4cout<<"fGeolocation: "<<fGeolocation[0]<<", "<<fGeolocation[1]<<", "<<fGeolocation[2]<<G4endl;
    // G4cout<<"-----------------GINGIN------------------"<<G4endl;

    if (!fFieldEnabled)
    {
        // G4cout<<"----------GIN----------"<<G4endl;
        // G4cout<<"fFieldEnabled: "<<fFieldEnabled<<G4endl;
        B[0] = B[1] = B[2] = 0.0;
        return;
    }
    // if (G4Threading::IsWorkerThread()) {

    //}

    switch (fFieldTypeEnum) {  // 编译为跳转表，O(1)
        case FieldType::kUniform: UniformField(Track, B); break;
        case FieldType::kDipole: DipoleField(Track, B); break;
        case FieldType::kGlobalDipole: GlogalDipoleField(Track, B); break;
        default: B[0]=B[1]=B[2]=0.0;
    }

    /*
    if (fFieldType == "uinform")
    {
        UniformField(Track, B);
        // G4cout<<"-----------------GINGIN1------------------"<< G4endl;
        //G4cout << "fFieldType: " << fFieldType << G4endl;
        // G4cout<<"-----------------GINGIN1------------------"<< G4endl;
    }
    else if (fFieldType == "dipole")
    {
        DipoleField(Track, B);
        // G4cout<<"-----------------GINGIN2------------------"<< G4endl;
        //G4cout << "fFieldType: " << fFieldType << G4endl;
        // G4cout<<"-----------------GINGIN2------------------"<< G4endl;
    }
    else if (fFieldType == "globaldipole")
    {
        GlogalDipoleField(Track, B);
        // G4cout<<"-----------------GINGIN2------------------"<< G4endl;
        //G4cout << "fFieldType: " << fFieldType << G4endl;
        // G4cout<<"-----------------GINGIN2------------------"<< G4endl;
    }
    else
    {
        // 默认零场
        B[0] = B[1] = B[2] = 0.0;
    }
    */
    return;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void MyMagneticField::UniformField(const G4double Track[7], G4double B[3]) const
{
    B[0] = fUniStrength[0];
    B[1] = fUniStrength[1];
    B[2] = fUniStrength[2];
    return;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void MyMagneticField::DipoleField(const G4double Track[7], G4double B[3]) const
{
    // 初始化参数，Track[7]这个应该是一个包括了粒子信息的东西，[0],[1],[2]是x,y,z;[3],[4],[5]是归一化的vx,vy,vz.
    // We need to claim that this field value should be in the local zenith frame.
    // And the geolocation is given by the vector fGeolacation.

    // G4cout<< "-------------GIN0.040---------------"<< G4endl;
    // G4cout<< "fFieldEnabled: " << fFieldEnabled << G4endl;
    // G4cout<< "!fFieldEnabled: " << (!fFieldEnabled) << G4endl;
    // G4cout << "mlat: " << fGeolocation[1]/deg << "\t" << "mlon: " << fGeolocation[2]/deg << G4endl;
    // G4cout<< "-------------GIN0.040---------------"<< G4endl;

    G4double a = 6371.2 * km;
    G4double g0 = -29404.8 * (1e-9) * tesla;
    G4double g1 = -1450.9 * (1e-9) * tesla;
    G4double h1 = 4652.5 * (1e-9) * tesla;
    //G4double M = (a * a * a) * std::sqrt(g0 * g0 + g1 * g1 + h1 * h1);
    G4double lat = fGeolocation[1];
    G4double lon = fGeolocation[2];

    // G4cout<< "-------------GIN0.03---------------"<< G4endl;
    // G4cout << "mlat: " << lat/deg << "\t" << "mlon: " << lon/deg << G4endl;
    // G4cout<< "-------------GIN0.03---------------"<< G4endl;

    G4double x = Track[0], y = Track[1], z = Track[2] + a;
    // G4double vx = Track[3], vy = Track[4], vz = Track[5];

    G4ThreeVector xyzInGTOD, rtpInGTOD, BxyzInGTOD, BxyzInZenith;
    xyzInGTOD = VectorZenithinGTOD1(G4ThreeVector(x, y, z), (90 * deg - lat), lon);
    rtpInGTOD = xyz2rtp(xyzInGTOD);
    double coeff = a * a * a / (rtpInGTOD[0] * rtpInGTOD[0] * rtpInGTOD[0]);

    double Br = 2 * coeff * (g0 * std::cos(rtpInGTOD[1]) + (g1 * std::cos(rtpInGTOD[2]) + h1 * std::sin(rtpInGTOD[2])) * std::sin(rtpInGTOD[1]));
    double Bt = -coeff * (-g0 * std::sin(rtpInGTOD[1]) + (g1 * std::cos(rtpInGTOD[2]) + h1 * std::sin(rtpInGTOD[2])) * std::cos(rtpInGTOD[1]));
    double Bp = -coeff * (-g1 * std::sin(rtpInGTOD[2]) + h1 * std::cos(rtpInGTOD[2]));

    // 对称偶极场
    // BrtpInGTOD[0] = -2*M/(rtpInGTOD[0]*rtpInGTOD[0]*rtpInGTOD[0])*np.cos(rtpInGTOD[1]);
    // BrtpInGTOD[1] = -M/(rtpInGTOD[0]*rtpInGTOD[0]*rtpInGTOD[0])*np.sin(rtpInGTOD[1]);
    // BrtpInGTOD[2] = 0;

    BxyzInGTOD = VectorZenithinGTOD1(G4ThreeVector(Bt, Bp, Br)
                                     // 球坐标可以看作是天顶坐标，所以球坐标的转换可以当成天顶坐标转GTOD
                                     ,
                                     rtpInGTOD[1], rtpInGTOD[2]);

    BxyzInZenith = VectorGTODinZenith1(BxyzInGTOD, (90 * deg - lat), lon);

    B[0] = BxyzInZenith[0];
    B[1] = BxyzInZenith[1];
    B[2] = BxyzInZenith[2];

    // G4cout<< "-------------GIN0.04---------------"<< G4endl;
    // G4cout<<G4BestUnit(B[0] ,"Magnetic flux density") <<"\t"
    //          <<G4BestUnit(B[1] ,"Magnetic flux density") <<"\t"
    //          <<G4BestUnit(B[2] ,"Magnetic flux density")<<"final磁场"<<G4endl;
    // G4cout<< "-------------GIN0.04---------------"<< G4endl;

    return;
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void MyMagneticField::GlogalDipoleField(const G4double Track[7], G4double B[3]) const
{
    // 初始化参数，Track[7]这个应该是一个包括了粒子信息的东西，[0],[1],[2]是x,y,z;[3],[4],[5]是归一化的vx,vy,vz.
    // We need to claim that this field value should be in the local zenith frame.
    // And the geolocation is given by the vector fGeolacation.

    // G4cout<< "-------------GIN0.040---------------"<< G4endl;
    // G4cout<< "fFieldEnabled: " << fFieldEnabled << G4endl;
    // G4cout<< "!fFieldEnabled: " << (!fFieldEnabled) << G4endl;
    // G4cout << "mlat: " << fGeolocation[1]/deg << "\t" << "mlon: " << fGeolocation[2]/deg << G4endl;
    // G4cout<< "-------------GIN0.040---------------"<< G4endl;

    G4double a = 6371.2 * km;
    G4double g0 = -29404.8 * (1e-9) * tesla;
    G4double g1 = -1450.9 * (1e-9) * tesla;
    G4double h1 = 4652.5 * (1e-9) * tesla;
    //G4double M = (a * a * a) * std::sqrt(g0 * g0 + g1 * g1 + h1 * h1);
    G4double lat = fGeolocation[1];
    G4double lon = fGeolocation[2];

    // G4cout<< "-------------GIN0.03---------------"<< G4endl;
    // G4cout << "mlat: " << lat/deg << "\t" << "mlon: " << lon/deg << G4endl;
    // G4cout<< "-------------GIN0.03---------------"<< G4endl;

    G4double x = Track[0], y = Track[1], z = Track[2];
    // G4double vx = Track[3], vy = Track[4], vz = Track[5];

    double r = std::sqrt(x*x + y*y + z*z);
    double cos_theta = z / r;
    double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);
    double phi = std::atan2(y, x);
    double cos_phi = std::cos(phi), sin_phi = std::sin(phi);
    double coeff = a * a * a / (r * r * r);

    double Br = 2 * coeff * (g0*cos_theta + (g1*cos_phi + h1*sin_phi)*sin_theta);
    double Bt = -coeff * (-g0*sin_theta + (g1*cos_phi + h1*sin_phi)*cos_theta);
    double Bp = -coeff * (-g1*sin_phi + h1*cos_phi);
    
    //直角坐标分量（展开旋转矩阵）其实就是VectorZenithinGTOD1，这样减小开销
    B[0] = Br*sin_theta*cos_phi + Bt*cos_theta*cos_phi - Bp*sin_phi;
    B[1] = Br*sin_theta*sin_phi + Bt*cos_theta*sin_phi + Bp*cos_phi;
    B[2] = Br*cos_theta - Bt*sin_theta;

    // G4cout<< "-------------GIN0.04---------------"<< G4endl;
    // G4cout<<G4BestUnit(B[0] ,"Magnetic flux density") <<"\t"
    //          <<G4BestUnit(B[1] ,"Magnetic flux density") <<"\t"
    //          <<G4BestUnit(B[2] ,"Magnetic flux density")<<"final磁场"<<G4endl;
    // G4cout<< "-------------GIN0.04---------------"<< G4endl;

    return;
}
