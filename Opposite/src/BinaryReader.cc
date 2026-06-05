#include "BinaryReader.hh"

namespace Test02
{

  BinaryReader::BinaryReader()
  {
  }

  BinaryReader::~BinaryReader()
  {
  }
  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  bool BinaryReader::readCSVinAll(const std::string &filename)
  {
    std::ifstream file(filename);
    if (!file.is_open())
    {
      return false;
    };

    std::string line;
    while (std::getline(file, line))
    {
      std::bitset<13963> row;
      std::stringstream ss(line);
      std::string cell;
      int col = 0;

      while (std::getline(ss, cell, ','))
      {
        if (cell == "1.0")
        {
          row.set(col);
          // G4cout << "----------------GIN-b------------------" << G4endl;
          // G4cout << "cell: " << cell << G4endl;
          // G4cout << "----------------GIN-b------------------" << G4endl;
        };

        col++;
      };
      data.push_back(row);
    };
    return true;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  bool BinaryReader::readCSVSpecificRow(const std::string &filename, int targetRow, std::bitset<13963> &result)
  {
    std::ifstream file(filename);
    if (!file.is_open())
    {
      return false;
    };

    std::string line;
    int currentRow = 0;

    while (std::getline(file, line))
    {
      if (currentRow == targetRow)
      {
        // 找到目标行，解析数据
        std::stringstream ss(line);
        std::string cell;
        int col = 0;

        while (std::getline(ss, cell, ',') && col < 13963)
        {
          if (cell == "1.0")
          {
            result.set(col);
            // G4cout << "----------------GIN-b------------------" << G4endl;
            // G4cout << "cell: " << cell << G4endl;
            // G4cout << "----------------GIN-b------------------" << G4endl;
          };
          col++;
        }
        return true;
      }
      currentRow++;
    }
    return false; // 未找到该行
  }
  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  std::vector<int> BinaryReader::readROOTSpecificBranch(const std::string filename, int targetBranch)
  {
    std::vector<int> result;
    TFile *file = TFile::Open(filename.c_str());
    TTree *allowed = (TTree *)file->Get("allowed");
    Double_t aflag;
    allowed->SetBranchAddress(std::to_string(targetBranch).c_str(), &aflag);
    Long64_t nEntries = allowed->GetEntries();
    for (int i = 0; i < nEntries; i++)
    {
      allowed->GetEntry(i);
      result.push_back(aflag);
    };
    file->Close();
    return result;
  };

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  int BinaryReader::getValue(int row, int col)
  {
    if (row < 0 || row >= 54 || col < 0 || col >= 13963)
    {
      return -1; // 错误处理
    }
    return data[row].test(col) ? 1 : 0;
  }

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  std::vector<std::vector<double>> BinaryReader::readAtmLayer(const std::string filename)
  {
    G4cout << "---------------GINs---------------" << G4endl;
    std::vector<std::vector<double>> result;
    std::ifstream file(filename);
    if (!file.is_open())
    {
      G4cout << "Error! Something wrong with opening file." << G4endl;
      return result;
    };
    std::string line;
    int currentRow = 0;

    while (std::getline(file, line))
    {
      if (line.empty())
        continue;
      std::istringstream ss(line);
      std::string cell;

      // 直接在 result 尾部添加一个空行
      result.emplace_back(); // 等价于 result.push_back({});

      int col = 0;
      while (col < 4 && std::getline(ss, cell, ','))
      {
        try
        {
          result.back().push_back(std::stod(cell));
          //G4cout<< "cell: " << cell << G4endl;
        }
        catch (...)
        {
          result.back().push_back(0.0); // 转换失败时填 0.0
        }
        ++col;
      }

      // 如果这行一个有效数据都没有，把刚加的空行删掉
      if (result.back().empty())
      {
        result.pop_back();
      }
    };

    return result;

  }
  
}
