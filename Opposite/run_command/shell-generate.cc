#include <iostream>
#include <cstdio>
#include <fstream>
#include <bits/stdc++.h>
#include <typeinfo>
#include <string>

using namespace std;

int fileSet(int num,string str1,string str2, string l[]){

	for (int j=0;j<num;j++){
	
		string s="./"+str2+"/" + str2+"_"+to_string(j+1)+".sh";
		cout<<s<<endl;
		
		ofstream fpf(s,ios::trunc);
		if(!fpf){
			printf("\nError on opening file.\n");
			return 1;
		};
		fpf<<"#!/bin/bash"<<"\n"<<"\n"
		<<"source /home/ams/lchen/miniconda3/etc/profile.d/conda.sh"<<"\n"
		<<"conda activate my_py310"<<"\n"
		<<"export PATH=/home/ams/lchen/cmake-3.27.4-linux-x86_64/bin:$PATH"<<"\n"
		<<"export LD_LIBRARY_PATH=/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/lib64:$LD_LIBRARY_PATH"<<"\n"
		<<"export G4NEUTRONHPDATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4NDL4.7.1\""<<"\n"
		<<"export G4LEDATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4EMLOW8.6.1\""<<"\n"
		<<"export G4LEVELGAMMADATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/PhotonEvaporation6.1\""<<"\n"
		<<"export G4RADIOACTIVEDATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/RadioactiveDecay6.1.2\""<<"\n"
		<<"export G4PARTICLEXSDATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4PARTICLEXS4.1\""<<"\n"
		<<"export G4PIIDATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4PII1.3\""<<"\n"
		<<"export G4REALSURFACEDATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/RealSurface2.2\""<<"\n"
		<<"export G4SAIDXSDATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4SAIDDATA2.0\""<<"\n"
		<<"export G4ABLADATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4ABLA3.3\""<<"\n"
		<<"export G4INCLDATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4INCL1.2\""<<"\n"
		<<"export G4ENSDFSTATEDATA=\"/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4ENSDFSTATE3.0\""<<"\n"<<"\n"
		<<"export LD_LIBRARY_PATH=/home/ams/lchen/libstdc++:$LD_LIBRARY_PATH"<<"\n"<<"\n"
		<<str1<<"\n"<<"\n"
		<<"python3 manager.py ../run/run_"<<str2<<"/run_"<<str2<<"_"<<j+1<<".mac "<<l[j];
		fpf.close();
	};
	return 0;

};

int main(){
	//string str[28]={"H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P",
	//		"S","Cl","Ar","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni"};
	string str[2]={"H","He"};
	//int a[27]={4,7,9,11,12,14,16,19,20,23,24,27,39,28,31,32,35,40,40,45,48,51,52,55,56,59,59};
	string b[24]={"30","31","32","33","34","35","36","37","38","39","40",
		"41","42","43","44","45","46","47","48","49","50","51","52","53"};
	string str1="cd /home/ams/lchen/GEANT4/test-sphere/G4py/Opposite/build/"; //############要改的！！！！！
	for(int i=0;i<2;i++){
		if(i==0){
			int a=24;
			fileSet(a,str1,str[i],b);
		}else{
			int a=24;
			fileSet(a,str1,str[i],b);
		};
	};
	return 0;
};
