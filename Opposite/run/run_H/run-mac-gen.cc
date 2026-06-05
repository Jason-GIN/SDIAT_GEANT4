#include <iostream>
#include <cstdio>
#include <fstream>
#include <bits/stdc++.h>
#include <string>

using namespace std;

int main()
{
	FILE *fp = fopen("./H.txt", "rt");
	int nu = 25;
	char ss[100];
	int n, n2;
	double H;
	double lat, lon;
	lat = -25.0;
	lon = -170.0;
	int Z = 1, A = 1;
	if (!fp)
	{
		printf("Error on opening file Al.txt . ");
	}
	else
	{
		for (int i = 0; i < nu; i++)
		{
			fgets(ss, 100, fp);
			sscanf(ss, "%d%le%d", &n, &H, &n2);

			ostringstream oss;
			oss << "run_H_" << n << ".mac";
			string s = oss.str();

			ofstream fpf(s.c_str());
			if (!fpf)
			{
				printf("fiald to build file . ");
			}
			fpf << "/run/numberOfThreads 32" << "\n"
				<< "/myrun/setOutputFileName /home/ams/lchen/GEANT4/test-sphere/G4py/Opposite/result/protondata_" << std::to_string(n2) << ".root" << "\n"
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
				<< "/run/beamOn 10000";
			fpf.close();
		};
	};
	fclose(fp);
};
