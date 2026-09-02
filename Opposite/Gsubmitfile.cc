#include <iostream>
#include <cstdio>
#include <fstream>
#include <bits/stdc++.h>
#include <typeinfo>
#include <string>

using namespace std;

int fileSet(int num,string str1,string str2, string l[]){

	for (int j=0;j<num;j++){
	
		string s="./JobWork_"+str2+"_"+l[j]+".jdf";
		cout<<s<<endl;
		
		ofstream fpf(s,ios::trunc);
		if(!fpf){
			printf("\nError on opening file.\n");
			return 1;
		};
		fpf<<"universe = vanilla"<<"\n"
		<<"executable = $(filename)"<<"\n"
		<<"output = output/$(ClusterId).$(ProcId).out"<<"\n"
		<<"error = output/$(ClusterId).$(ProcId).err"<<"\n"
		<<"request_cpus = 32"<<"\n"
		<<"log = log/$(ClusterId).$(ProcId).log"<<"\n"
		<<"queue filename matching files run_command/"<<l[j]<<"/"<<str2<<"/"<<str2<<"*.sh";
		fpf.close();
	};
	return 0;

};

int main(){
	//string str[28]={"H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P",
	//		"S","Cl","Ar","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni"};
	string str[2]={"H","He"};
	//int a[27]={4,7,9,11,12,14,16,19,20,23,24,27,39,28,31,32,35,40,40,45,48,51,52,55,56,59,59};
	string b[12]={"0_25","-120_20","120_35","150_-20","-150_-30","180_30","30_-25","-30_-35","-60_15","60_35",
		      "90_-15","-90_-35"};
	string str1="cd /home/ams/lchen/GEANT4/test-sphere/G4py/belt0405/build/"; //############要改的！！！！！
	for(int i=0;i<2;i++){
		if(i==0){
			int a=12;
			fileSet(a,str1,str[i],b);
		}else{
			//break;
			int a=12;
			fileSet(a,str1,str[i],b);
		};
	};
	return 0;
};
