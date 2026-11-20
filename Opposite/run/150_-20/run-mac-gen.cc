#include <iostream>
#include <cstdio>
#include <fstream>
#include <bits/stdc++.h>
//#include <string>

using namespace std;

int fileSet(int num,string str1,string str2,int Z, int A, int lat,int lon){
	
	string s="./run_"+str1+"/"+str1+".txt";
	FILE *fp = fopen(s.c_str(), "rt");
	char ss[100];
	int n, n2;
	double H;
	if (!fp)
	{
		printf("Error on opening file Al.txt . ");
		return 1;
	}
	else
	{
		for (int i = 0; i < num; i++)
		{
			fgets(ss, 100, fp);
			sscanf(ss, "%d%le%d", &n, &H, &n2);
			ostringstream oss;
			oss << "run_"+str1+"/run_"+str1+"_" << n << ".mac";
			string s = oss.str();
			ofstream fpf(s.c_str());
			if (!fpf)
			{
				printf("fiald to build file . ");
			}
			fpf << "/run/numberOfThreads 32" << "\n"
				<< "/myrun/setOutputFileName "
				<<"/home/ams/lchen/GEANT4/test-sphere/G4py/belt0405/"
				<<"result/" << lon << "_" << lat << "/"+str2+"data_" << std::to_string(n2) << ".root" << "\n"
				<< "/field/enable true" << "\n"
				<< "/field/SetType globaldipole" << "\n"
				<< "/field/Geolocation 1 " << lat << " " << lon << " degree" << "\n"
				//<< "/run/setCut 10.0 mm" << "\n"
				//<< "/process/em/lowestElectronEnergy 0.3 MeV" << "\n"
				//<< "/process/em/lowestMuHadEnergy 0.3 MeV" << "\n"
				<< "/run/initialize" << "\n"
				<< "/control/verbose 0" << "\n"
				<< "/run/verbose 0" << "\n"
				<< "/event/verbose 0" << "\n"
				<< "/tracking/verbose 0" << "\n"
				<< "/generator/select Primary" << "\n"
				<< "/gun/particle ion" << "\n"
				<< "/gun/ion " << Z << " " << A << "\n"
				<< "/gun/energy " << H << " GeV" << "\n"
				//<<"/globalField/setValue -0.291050806052572 0 0 gauss"<<"\n"
				<< "/Geo/lat " << lat << " deg" << "\n"
				<< "/Geo/lon " << lon << " deg" << "\n"
				<< "/run/beamOn 80000";
			fpf.close();
		};
	};
	fclose(fp);
	return 0;
};

int main(){
	string str[2]={"H","He"};
	string b[2]={"proton","alpha"};
	int Al[2]={1,4};
	int Zl[2]={1,2};
	int nu = 24;
	double lat, lon;
	lat = -20.0;
	lon = 150.0;
	for(int i=0;i<2;i++){
		fileSet(nu,str[i],b[i],Zl[i],Al[i],lat,lon);
	}
};
