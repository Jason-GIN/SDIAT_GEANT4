#include <iostream>
#include <cstdio>
#include <fstream>
#include <bits/stdc++.h>
#include <string>

using namespace std;

int Gmacfile(int lat, int lon, int n2, int Enum, double E, string element, int Z, int A)
{
	ostringstream oss;
	oss << "run/"<< std::to_string(lon) << "_" <<std::to_string(lat)<<"/run_" << element << "/run_" << element << "_" << n2 << ".mac";
	string s = oss.str();
	//cout << s << endl;
	ofstream fpf(s.c_str());
	if (!fpf)
	{
		printf("fiald to build file . ");
		perror("ofstream open failed"); 
		return 1;
	}
	fpf << "/run/numberOfThreads 32" << "\n"
		<< "/myrun/setOutputFileName "
		<< "/home/ams/lchen/GEANT4/test-sphere/G4py/belt0405/result/"
		<< lon << "_" << lat << "/protondata_" << std::to_string(Enum) << ".root" << "\n"
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
		<< "/gun/energy " << E << " GeV" << "\n"
		<< "/Geo/lat " << lat << " deg" << "\n"
		<< "/Geo/lon " << lon << " deg" << "\n"
		<< "/run/beamOn 80000";
	fpf.close();
	return 0;
};

int main()
{
	double E[54] = {
		0.022, 0.028, 0.035, 0.043, 0.054, 0.068, 0.085, 0.105, 0.131, 0.162, 0.201, 0.247, 0.303, 0.37,
		0.45, 0.544, 0.655, 0.785, 0.935, 1.109, 1.309, 1.537, 1.798, 2.095, 2.431, 2.812, 3.242, 3.728,
		4.275, 4.892, 5.586, 6.367, 7.244, 8.23, 9.338, 10.582, 11.978, 13.547, 15.307, 17.283, 19.501, 
		21.99, 24.783, 27.917, 31.435, 35.382, 39.811, 44.78, 50.356, 56.613, 63.634, 71.511, 80.35, 90.268
	};
	FILE *fp = fopen("./table.txt", "rt");
	int nu = 36;
	char ss[100];
	int lat, lon;
	int Enum;
	if (!fp)
	{
		printf("Error on opening file Al.txt . ");
	}
	else
	{
		for (int i = 0; i < nu; i++)
		{
			fgets(ss, 100, fp);
			sscanf(ss, "%d%d%d", &lon, &lat, &Enum);
			int n = 1;
			cout << "lon:" << lon << "lat: " << lat <<endl;
			for (int j = Enum; j < 54; j++)
			{
				Gmacfile(lat, lon, n, j, E[j], "H", 1, 1);
				n++;
			}
		}
	}
}