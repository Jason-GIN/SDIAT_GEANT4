#!/bin/bash

source /home/ams/lchen/miniconda3/etc/profile.d/conda.sh
conda activate my_py310
export PATH=/home/ams/lchen/cmake-3.27.4-linux-x86_64/bin:$PATH
export LD_LIBRARY_PATH=/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/lib64:$LD_LIBRARY_PATH
export G4NEUTRONHPDATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4NDL4.7.1"
export G4LEDATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4EMLOW8.6.1"
export G4LEVELGAMMADATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/PhotonEvaporation6.1"
export G4RADIOACTIVEDATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/RadioactiveDecay6.1.2"
export G4PARTICLEXSDATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4PARTICLEXS4.1"
export G4PIIDATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4PII1.3"
export G4REALSURFACEDATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/RealSurface2.2"
export G4SAIDXSDATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4SAIDDATA2.0"
export G4ABLADATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4ABLA3.3"
export G4INCLDATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4INCL1.2"
export G4ENSDFSTATEDATA="/home/ams/lchen/GEANT4/geant4v11/geant4-v11.3.0-install/share/Geant4/data/G4ENSDFSTATE3.0"

export LD_LIBRARY_PATH=/home/ams/lchen/libstdc++:$LD_LIBRARY_PATH

cd /home/ams/lchen/GEANT4/test-sphere/G4py/Opposite/build/

python3 manager.py ../run/run_He/run_He_8.mac 37