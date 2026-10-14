#include <iostream>
#include "Combine.hh"

namespace mylib
{

    Combine::Combine()
    {
    }

    Combine::~Combine()
    {
    }

    void Combine::comb(std::string strr, std::string strw, int num)
    {
        TChain chain1("DetectionInfo");
        TChain chain2("EventInfo");
        for (int i = 0; i < num; i++)
        {
            std::string filename = strr + "_t" + std::to_string(i) + ".root";
            std::cout << filename << std::endl;
            chain1.Add(filename.c_str());
            chain2.Add(filename.c_str());
        }
        std::string filename1 = strw + ".root";
        TFile outputFile(filename1.c_str(), "RECREATE");
        TTree *DetectionInfo = chain1.CloneTree();
        TTree *EventInfo = chain2.CloneTree();
        Long64_t nEntries = DetectionInfo->GetEntries();
        Int_t ParentId;
        DetectionInfo->SetBranchAddress("ParentId", &ParentId);
        TTree *newtree1 = DetectionInfo->CloneTree(0); // 参数 0 表示只复制结构，不复制数据
        for (Long64_t i = 0; i < nEntries; i++)
        {
            DetectionInfo->GetEntry(i);
            if (ParentId != 0)
            {
                newtree1->Fill();
                //}else if(PrimaryType ==0 && DirZ < 0){
                //    std::cout << "--------------GIN-----------" << std::endl;
                //    std::cout << "PID:" << PID << std::endl;
                //    std::cout << "--------------GIN-----------" << std::endl;
                //    num++;
            };
        };

        newtree1->Write();
        EventInfo->Write();
        outputFile.Close();
    };

    void Combine::supercomb(std::string strr, std::string strw, int num)
    {
        TChain chain1("DetectionInfo");
        TChain chain2("EventInfo");
        for (int i = 0; i < num; i++)
        {
            std::string filename = strr + "_t" + std::to_string(i) + ".root";
            std::cout << filename << std::endl;
            chain1.Add(filename.c_str());
            chain2.Add(filename.c_str());
        };
        std::string filename1 = strw + ".root";
        TFile outputFile(filename1.c_str(), "RECREATE");

        // 设置较大的缓存提高读取效率
        chain1.SetCacheSize(10000000); // 10MB缓存
        chain2.SetCacheSize(10000000);

        // 使用CopyTree进行批量过滤，比逐事件处理快得多
        TTree *newtree1 = chain1.CopyTree("ParentId != 0");
        TTree *EventInfo = chain2.CloneTree();

        newtree1->Write();
        EventInfo->Write();
        outputFile.Close();
    };

    void Combine::Pcomb(std::string strr, std::string strw, int num)
    {
        TChain chain1("DetectionInfo");
        TChain chain2("EventInfo");
        TChain chain3("BoundaryInfo");

        for (int i = 0; i < num; i++)
        {
            std::string filename = strr + "_t" + std::to_string(i) + ".root";
            std::cout << "Adding: " << filename << std::endl;
            chain1.Add(filename.c_str());
            chain2.Add(filename.c_str());
            chain3.Add(filename.c_str());
        }

        std::string filename1 = strw + "_all.root";

        // 使用MERGE选项直接合并，这是最高效的方式
        chain1.Merge(filename1.c_str(), "RECREATE");
        
        // 对于第二个树，使用APPEND模式
        TFile outputFile(filename1.c_str(), "UPDATE");
        chain2.CloneTree()->Write();
        chain3.CloneTree()->Write();
        outputFile.Close();
    };

}
