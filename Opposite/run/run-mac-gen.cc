#include <iostream>
#include <cstdio>
#include <fstream>
#include <bits/stdc++.h>
//#include <string>

using namespace std;

int main(){
	FILE *fp=fopen("./H.txt", "rt");
	int nu=24;
	char ss[100];
	int Z=13,A=27;
	int n;
	double  Al;
	if(!fp){
		printf("Error on opening file Al.txt . ");
	}else{
		for(int i=0;i<nu;i++){
			fgets(ss,100,fp);
			sscanf(ss, "%d%le", &n, &Al);
			
			ostringstream oss;
			oss << "run_Al_" << n << ".mac";
			string s = oss.str();
			
			ofstream fpf(s.c_str());
			if(!fpf){
				printf("fiald to build file . ");
			}
			fpf<<"/control/verbose 0"<<"\n"<<"/control/saveHistory"<<"\n"<<"/run/verbose 0"<<"\n"
				<<"/run/initialize"<<"\n"<<"/gun/particle ion"<<"\n"<<"/gun/ion "<<Z<<" "<<A<<"\n"
				<<"/gun/energy "<<Al<<" MeV"<<"\n"<<"/run/beamOn 3000000";
			fpf.close();
		
		};

	
		
	};
	fclose(fp);
};
