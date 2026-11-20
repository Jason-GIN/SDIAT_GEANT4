#include <iostream>
#include <cstdio>
#include <fstream>
#include <bits/stdc++.h>
//#include <string>

using namespace std;

int main(){
	FILE *fp=fopen("./He.txt", "rt");
	int nu=25;
	char ss[100];
	int n, n2;
	double  He;
	double lat, lon;
	lat = 31.15;
	lon = -100.0;
	int Z=2,A=4;
	if(!fp){
		printf("Error on opening file Al.txt . ");
	}else{
		for(int i=0;i<nu;i++){
			fgets(ss,100,fp);
			sscanf(ss, "%d%le%d", &n, &He, &n2);
			
			ostringstream oss;
			oss << "run_He_" << n << ".mac";
			string s = oss.str();
			
			ofstream fpf(s.c_str());
			if(!fpf){
				printf("fiald to build file . ");
			}
			fpf<<"/run/numberOfThreads 32"<<"\n"
			   <<"/myrun/setOutputFileName alphadata_" << std::to_string(n2) << ".root"<<"\n"
			   <<"/run/initialize"<<"\n"
			   <<"/control/verbose 0"<<"\n"
			   <<"/run/verbose 0"<<"\n"
			   <<"/event/verbose 0"<<"\n"
			   <<"/tracking/verbose 0"<<"\n"
			   <<"/gun/particle ion"<<"\n"
			   <<"/gun/ion "<<Z<<" "<<A<<"\n"
			   <<"/gun/energy "<<He<<" GeV"<<"\n"
			   //<<"/globalField/setValue -0.291050806052572 0 0 gauss"<<"\n"
			   <<"/Geo/lat "<<lat<<" deg"<<"\n"
                           <<"/Geo/lon "<<lon<<" deg"<<"\n"
                           <<"/field/Geolocation 1 "<<lat<<" "<<lon<<" degree"<<"\n"
                           <<"/field/enable true"<<"\n"
			   <<"/run/beamOn 80000";
			fpf.close();
		
		};

	
		
	};
	fclose(fp);
};
