#ifndef Test02BinaryCSVReader_h
#define Test02BinaryCSVReader_h 1

#include <fstream>
#include <sstream>
#include <vector>
#include <bitset>
#include <iostream>
#include <algorithm>
#include <TTree.h>
#include <TFile.h>

#include "G4ThreeVector.hh"

namespace Test02
{

class BinaryReader {

public:
    
    BinaryReader();
    ~BinaryReader();
    
    bool readCSVinAll(const std::string& filename); //read the total file
    
    //bool readTargetRows(const std::string& filename);
    //void setTargetRows(const std::vector<int>& rows);
    
    static bool readCSVSpecificRow(const std::string& filename, int targetRow, std::bitset<13963>& result) ;
                                   
    static std::vector<int> readROOTSpecificBranch(const std::string filename, int targetBranch);
    
    std::vector<std::vector<double>> readAtmLayer(const std::string filename);

    int getValue(int row, int col);
    
    // 内存使用估算
    size_t getMemoryUsage() const {
        return data.size() * (13963 / 8); // bitset内存占用
    }

private:
    std::vector<std::bitset<13963>> data;  // 每行一个bitset
    //std::bitset<13963> rowdata;
    //std::vector<int> sortedTargetRows;
    
    
};

}

#endif
