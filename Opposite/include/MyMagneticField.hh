#ifndef myMAGFIELD_HH
#define myMAGFIELD_HH

#include "G4MagneticField.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"


class MyMagneticField : public G4MagneticField
{
  public:  // with description
                       
    MyMagneticField();

    //MyMagneticField(G4bool FieldEnabled, 
    //                G4String FieldType,
    //                G4ThreeVector Geolocation,
    //                G4ThreeVector UniStrength);

    virtual ~MyMagneticField() override;

    void GetFieldValue(const G4double Track[7], G4double B[3]) const;
    void EnableField(G4bool enable){fFieldEnabled = enable;};
    void SetFieldType(G4String type){fFieldType = type;};
    void SetDipoleFieldParameters(G4ThreeVector geo){fGeolocation = geo;};
    void SetUniformFieldParameters(G4ThreeVector stre){fUniStrength = stre;};
    


    G4ThreeVector GetDipoleFieldParameters() const {return fGeolocation;};
    G4ThreeVector GetUniformFieldParameters() const {return fUniStrength;};
    G4bool IsFieldEnabled() const {return fFieldEnabled;};
    G4String GetFieldType() const {return fFieldType;};
    
    MyMagneticField* Clone() const override;
    //G4Field* Clone() const;

  private:
    G4ThreeVector  fGeolocation = {0.0, 0.0, 0.0};   //{r, lat,lon};
    G4ThreeVector  fUniStrength = {0.0, 0.0, 0.0};  //{Bx, By , Bz} in Gauss;
    G4String fFieldType = "uiform";
    G4bool fFieldEnabled = false;

    void DipoleField(const G4double Track[7], G4double B[3]) const;
    void GlogalDipoleField(const G4double Track[7], G4double B[3]) const;
    void UniformField(const G4double Track[7], G4double B[3]) const;

};



#endif