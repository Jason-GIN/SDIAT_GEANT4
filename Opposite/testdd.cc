#include <iostream>
#include <string>
#include <cmath>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <vector>
#include <algorithm>
#include <numeric>
#include <tuple>



int SelectDirection(double theta, double phi, std::string str1){
  //放进来的是角度制
  double rlist[361];
  double thetalist[361];
  double philist[361];

  FILE *fp=fopen(str1.c_str(), "rt");
  char ss[100];
  if(!fp){
    printf("Error on opening file density_above86.data. ");
    }else{

    double ri,thetai, phii;
    
    for(int i=0;i<361;i++){
      fgets(ss,100,fp);
      sscanf(ss, "%le %le %le", &ri, &thetai, &phii);
      rlist [i]= ri;
      thetalist [i]= thetai;
      philist [i]= phii;
      
      };
    };
  fclose(fp);
  
  int a=0; int b=360; int c;

  //用二分法区分是否在允许锥内
  while (abs(a-b)!=1){

     if(philist[b]>phi && philist[a]<phi){
          c=b;
          b=(a+b+1)/2;
      }else if (philist[b]<phi){
          a=b;
          b=c;
      };
      if (philist[b]==phi || philist[a]==phi){
          a=b;
          break;
      };
  };
  

  if (a==b){
      double ruletheta=thetalist[a];
      //std::cout<<"thetalist[a]=thetalist[b]: "<<thetalist[a]<<std::endl;
      //std::cout<<"ruletheta: "<<ruletheta<<std::endl;
      if (ruletheta>theta){

          return 0;
      }else{

          return 1;
      }
  }else{
      double ruletheta=(thetalist[a]+thetalist[b])/2;
      //std::cout<<"thetalist[a]: "<<thetalist[a]<<std::endl;
      //std::cout<<"thetalist[b]: "<<thetalist[b]<<std::endl;
      //std::cout<<"ruletheta: "<<ruletheta<<std::endl;
      if (ruletheta>theta){

          return 0;
      }else{

          return 1;
      }
  }  
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

std::vector<double> xyz2r(const std::vector<double>& xyz){
  std::vector<double> rtp(3);
  rtp[0]=std::sqrt(xyz[0]*xyz[0]+xyz[1]*xyz[1]+xyz[2]*xyz[2]);
  rtp[1]=std::acos(xyz[2]/rtp[0]);
  rtp[2]=std::atan2(xyz[1], xyz[0]);
  
  return rtp; 

}

std::vector<double> VectorGTODinZenith(const std::vector<double>& xyz2, const std::vector<double>& xyz1){
  std::vector<double> xyzn(3);
  std::vector<double> rtp(3);
  double t,p;
  rtp = xyz2r(xyz1);
  t = rtp[1];
  p = rtp[2];
  xyzn[0]=std::cos(p)*std::cos(t)*xyz2[0]+std::sin(p)*std::cos(t)*xyz2[1]-std::sin(t)*xyz2[2];
  xyzn[1]=-std::sin(p)*xyz2[0]+std::cos(p)*xyz2[1];
  xyzn[2]=std::cos(p)*std::sin(t)*xyz2[0]+std::sin(p)*std::sin(t)*xyz2[1]+std::cos(t)*xyz2[2];
  
  return xyzn;

}

std::vector<double> VectorGTODinZenith1(const std::vector<double>& xyz1, double tt, double pp){
  std::vector<double> xyzn(3);
  double t = tt*M_PI/180.0;
  double p = pp*M_PI/180.0;
  xyzn[0]=std::cos(p)*std::cos(t)*xyz1[0]+std::sin(p)*std::cos(t)*xyz1[1]-std::sin(t)*xyz1[2];
  xyzn[1]=-std::sin(p)*xyz1[0]+std::cos(p)*xyz1[1];
  xyzn[2]=std::cos(p)*std::sin(t)*xyz1[0]+std::sin(p)*std::sin(t)*xyz1[1]+std::cos(t)*xyz1[2];
  
  return xyzn;

}

std::vector<double>  VectorZenithinGTOD(const std::vector<double>& xyz2, std::vector<double>  xyz1){
  std::vector<double> xyzn(3);
  std::vector<double> rtp(3);
  double t,p;
  rtp = xyz2r(xyz1);
  t = rtp[1];
  p = rtp[2];

  std::cout << "t: " << t/M_PI*180.0 << ", " << "p: " << p/M_PI*180.0 << std::endl;

  xyzn[0]=std::cos(p)*std::cos(t)*xyz2[0]-std::sin(p)*xyz2[1]+std::cos(p)*std::sin(t)*xyz2[2];
  xyzn[1]=std::sin(p)*std::cos(t)*xyz2[0]+std::cos(p)*xyz2[1]+std::sin(p)*std::sin(t)*xyz2[2];
  xyzn[2]=-std::sin(t)*xyz2[0]+std::cos(t)*xyz2[2];
  
  return xyzn;

}

std::vector<double> VectorZenithinGTOD1(const std::vector<double>& xyz1, double tt, double pp){
  std::vector<double> xyzn(3);
  double t = tt*M_PI/180.0;
  double p = pp*M_PI/180.0;
  xyzn[0]=std::cos(p)*std::cos(t)*xyz1[0]-std::sin(p)*xyz1[1]+std::cos(p)*std::sin(t)*xyz1[2];
  xyzn[1]=std::sin(p)*std::cos(t)*xyz1[0]+std::cos(p)*xyz1[1]+std::sin(p)*std::sin(t)*xyz1[2];
  xyzn[2]=-std::sin(t)*xyz1[0]+std::cos(t)*xyz1[2];
  
  return xyzn;

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

std::vector<double> GeneratorDirection(std::string str0, double x, double y, double z, double lat, double lon){ //  delete[] arrayname;      调用时莫忘释放内存
  
  //std::cout << "-------------GIN0.1---------------"<< std::endl;
  int aa;
  double pcostheta;
  double psintheta;
  double pphi;
  srand((unsigned int)time(NULL));
  std::random_device rd;  // 用于获取随机种子
  std::mt19937 gen(rd()); // 以随机设备为种子初始化Mersenne Twister生成器  
  std::uniform_real_distribution<> dis(-1, 1);
  do{
  pcostheta = dis(gen);
  psintheta = std::sqrt(1. - pcostheta*pcostheta);
  pphi = (rand()%361)-180;
  double theta = std::acos(pcostheta)/M_PI*180.0 ;
  
  std::cout << "theta: " << theta << ", " << "pphi: " << pphi << std::endl;
  std::cout << "-------------GIN0.11---------------"<< std::endl;
  aa = SelectDirection(theta, pphi, str0);
  std::cout << "-------------GIN0.12---------------"<< std::endl;

  }while(aa);
  
  std::vector<double> dp(3);
  //-----------posision--------------//
  //dp[0]=psintheta * std::cos(pphi)*r;
  //dp[1]=-psintheta * std::sin(pphi)*r;
  //dp[2]=-pcostheta*r;
  //-----------direction------------//
  dp[0]=-psintheta * std::cos(pphi*M_PI/180.0);
  dp[1]=-psintheta * std::sin(pphi*M_PI/180.0);
  dp[2]=-pcostheta;
  
  std::vector<double> xyz(3);
  std::vector<double> xyzm(3);
  std::vector<double> vm(3);
  std::vector<double> ddp(3);
  
  xyz[0]=x; xyz[1]=y; xyz[2]=z;
  
  //std::cout << "-------------GIN0.2---------------"<< std::endl;
  xyzm = VectorZenithinGTOD1(xyz,(90.0-lat),lon);
  vm = VectorZenithinGTOD(dp,xyzm);
  ddp = VectorGTODinZenith1(vm,(90.0-lat),lon);
  
  std::cout << "dp :\t" << dp[0] << ", " << dp[1] << ", " << dp[2] << std::endl;
  std::cout << "xyzm :\t" << xyzm[0] << ", " << xyzm[1] << ", " << xyzm[2] << std::endl;
  std::cout << "vm :\t" << vm[0] << ", " << vm[1] << ", " << vm[2] << std::endl;
  std::cout << "ddp :\t" << ddp[0] << ", " << ddp[1] << ", " << ddp[2] << std::endl;
  
  return ddp;

}

int main(){
  std::string str0="./ctable/stormer_allowed_mgeonum_Texas_0_RGVrer_35.dat";
  double lat = 31.15;
  double lon =-100.0;
  
  srand((unsigned int)time(NULL));
  
  std::random_device rd;  // 用于获取随机种子
  std::mt19937 gen(rd()); // 以随机设备为种子初始化Mersenne Twister生成器  
  std::uniform_real_distribution<> dis(0, 1);
  
  double R = 6371.2+100;
  double l = dis(gen)*100;
  double r = std::sqrt(R*R-l*l);
  double size = 0.8;
  //double pphi = 0. * deg;
  //double x0 = size * envSizeXY * (G4UniformRand()-0.5);
  //double y0 = size * envSizeXY * (G4UniformRand()-0.5);
  double theta = dis(gen) * 360.0*M_PI/180.0;
  double cos1 = std::cos(theta);
  double sin1 = std::sin(theta);
  
  double x0 = cos1*l;
  double y0 = sin1*l;
  double z0 = r-6371.2;
  std::vector<double> dpp;
  std::cout << "-----------------GIN-----------------" << std::endl;
  std::cout << "x0: " << x0 << ", " << "y0: " << y0 << ", " << "z0: " << z0 << std::endl;
  dpp = GeneratorDirection(str0,x0,y0,z0,lat,lon);
  std::cout << "dpp :\t" << dpp[0] << ", " << dpp[1] << ", " << dpp[2] << std::endl;
  
};
