#include <fstream>
#include <iostream>
#include "Filter.hh"
#include "Combine.hh"
#include <string>
#include <TTree.h>
#include <TFile.h>
#include <TH1F.h>
#include <TTreeReader.h>
#include "TApplication.h"
#include "TCanvas.h"
#include <TFrame.h>

bool fileExists(const std::string& name) {
    std::ifstream file(name);
    return file.good(); // 或者使用 file.is_open()
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        /*
        std::string pathname[2] = {"30N30S", "60N60S"};
        // int snum[9] = {32, 32, 34, 30, 33, 30, 27, 31, 27};
        for (int j = 1; j < 2; j++)
        {
            for (int i = 30; i < 50; i++)
            {
                std::string strr = "/mnt/d/TestData/LowAlt/" + pathname[j] + "/protondata_" + std::to_string(i);
                std::string strw = "/mnt/d/TestData/LowAlt/" + pathname[j] + "/combineROOTfile/protondata_" + std::to_string(i);
                mylib::Combine *f = new mylib::Combine();
                f->Pcomb(strr, strw, 32);
            }
        }
        */

        return 0;
    }
    else
    {
        //------------------------Multipelo Model------------------------
        //用法很简单，在命令行中用GNU Parallel，建议在调试模式中调试无误了再整这个
        //在./build 目录下运行 命令如下：
        //parallel --bar --delay 0.5 -j 5 ./执行文件名 {} ::: {31..100 （参数组，这里的意思是从31累加到100，包括100）}
        
        //std::string pathname[12] = {"result/0_25","result/30_-25","result/60_35","result/90_-15","result/120_35",
        //    "result/150_-20","result/180_30","result/-150_-30","result/-120_20","result/-30_-35",
        //    "result/-60_15","result/-90_-35"};
        std::string pathname[48] = {
            "result/0_25","result/30_-25","result/60_35","result/90_-15","result/120_35",
            "result/150_-20","result/180_30","result/-150_-30","result/-120_20","result/-30_-35",
            "result/-60_15","result/-90_-35","result/0_40","result/100_30","result/-100_55",
            "result/10_-10","result/-10_-50","result/-110_-5","result/110_50","result/180_-40",
            "result/120_-35","result/-120_45","result/130_20","result/-130_-45","result/140_40",
            "result/-140_5","result/150_-10","result/-150_20","result/160_10",
            "result/-160_-35","result/-170_15","result/170_30","result/-20_0","result/20_45",
            "result/-30_15","result/30_-40","result/40_15","result/-40_-40","result/-50_-25",
            "result/50_35","result/60_-15","result/-60_25","result/-70_30","result/70_5",
            "result/-80_-15","result/80_60","result/90_-25","result/-90_35"
        };
        // std::string pathname[36] = {
        //     "result/0_40","result/100_30","result/-100_55","result/10_-10","result/-10_-50","result/-110_-5",
        //     "result/110_50","result/180_-40","result/120_-35","result/-120_45","result/130_20","result/-130_-45",
        //     "result/140_40","result/-140_5","result/150_-10","result/-150_20","result/160_10",
        //     "result/-160_-35","result/-170_15","result/170_30","result/-20_0","result/20_45",
        //     "result/-30_15","result/30_-40","result/40_15","result/-40_-40","result/-50_-25",
        //     "result/50_35","result/60_-15","result/-60_25","result/-70_30","result/70_5",
        //     "result/-80_-15","result/80_60","result/90_-25","result/-90_35"
        // };
        //int snum[9] = {32, 27, 29, 31, 30, 30, 31, 29, 30};
        int file_index = std::stoi(argv[1]);
        int pathnumup = std::stoi(argv[2]);
        int pathnumdown = std::stoi(argv[3]);
        for (int j = pathnumup; j < pathnumdown; j++)
        {
            //std::string strr = "/mnt/f/TestData/G4py/" + pathname[j] + "/protondata_" + std::to_string(file_index);
            //std::string strw = "/mnt/f/TestData/G4py/" + pathname[j] + "/combineROOTfile/protondata_" + std::to_string(file_index);
            
            std::string strr = "/home/ams/lchen/GEANT4/test-sphere/G4py/belt0405/" + pathname[j] + "/protondata_" + std::to_string(file_index);
            std::string strw = "/home/ams/lchen/GEANT4/test-sphere/G4py/belt0405/" + pathname[j] + "/protondata_" + std::to_string(file_index);
            std::string strrt=strr+"_t0.root";
            if (fileExists(strrt)) {
                mylib::Combine *f = new mylib::Combine();
                f->Pcomb(strr, strw, 32);
            }else{
                std::cout<<"Warrning! file dose not exit! Please check the path:"<<std::endl;
                std::cout<<strr<<std::endl;
                continue;
            }
            
        }

        // for (int j = 53; j < 54; j++)
        //{
        //    std::string strr = "/mnt/d/TestData/LowAlt/30N30S/protondata_" + std::to_string(j);
        //    std::string strw = "/mnt/d/TestData/LowAlt/30N30S/combineROOTfile/protondata_" + std::to_string(j);
        //    mylib::Combine *f = new mylib::Combine();
        //    f->Pcomb(strr, strw, 32);

        /*
        std::string filename= strw +"_all.root";
        TFile *file = TFile::Open(filename.c_str());
        TTree *DetectionInfo = (TTree *)file->Get("DetectionInfo");
        if (!DetectionInfo)
        { // 处理获取失败
            file->Close();
            delete file;
            std::cout << ":(, (-A=w=A-) 数据读取失败" << std::endl;
        }

        std::string filename1 = "./25_-160/combineROOTfile/protondata_"+ std::to_string(j) +"_gamma.root";
        TFile outputFile(filename1.c_str(), "RECREATE");
        TTree *newtree;
        Int_t num =  22;
        std::string branchname = "PID";
        mylib::Filter *fi = new mylib::Filter();
        newtree = fi->FilterS(DetectionInfo, num, branchname, mylib::CompareOp::Equal_to);

        std::cout << "-------------------(o_L)GIN(<#-_>)-------------------" << std::endl;
        newtree->OptimizeBaskets();
        newtree->Write();
        std::cout << "-------------------(_o7L)GIN(<#-_>)-------------------" << std::endl;
        outputFile.Close();
        */
        /*
        std::string filename2 = "./0_0/combineROOTfile/protondata_"+ std::to_string(j) +"_primary.root";
        TFile outputFile1(filename2.c_str(), "RECREATE");
        TTree *newtree1;
        Int_t num1 =  0;
        std::string branchname1 = "ParentId";
        mylib::Filter *fi1 = new mylib::Filter();
        newtree1 = fi1->FilterS(DetectionInfo, num1, branchname1, mylib::CompareOp::Equal_to);

        std::cout << "-------------------(o_L)GIN(<-_>)-------------------" << std::endl;
        newtree1->OptimizeBaskets();
        newtree1->Write();
        std::cout << "-------------------(_o7L)GIN(<-_>)-------------------" << std::endl;
        outputFile1.Close();
        */
        /*
        file->Close();
        delete file;
        */
        //};

        /*
        std::string filename = "./protondata_0.root";
        TFile *file = TFile::Open(filename.c_str());
        TTree *DetectionInfo = (TTree *)file->Get("DetectionInfo");
        if (!DetectionInfo) {// 处理获取失败
            file->Close();
            delete file;
            std::cout<< ":(, (-A=w=A-) 数据读取失败"<< std::endl;
        }

        std::string filename1 = "./data/combinefile/protondata_0_nu.root";
        TFile outputFile(filename1.c_str(), "RECREATE");
        TTree *newtree;
        Int_t num = 2112;
        std::string branchname = "PID";
        mylib::Filter* f = new mylib::Filter();
        newtree = f->FilterS( DetectionInfo, num, branchname, mylib::CompareOp::Equal_to);

        std::cout<<"-------------------(o_L)GIN(<-_>)-------------------"<<std::endl;
        newtree ->OptimizeBaskets();
        newtree ->Write();
         std::cout<<"-------------------(_o7L)GIN(<-_>)-------------------"<<std::endl;
        outputFile.Close();
        file->Close();
        delete file;
        */

        /*
        TApplication app("MyApp", &argc, argv);
        std::string strr = "./combineROOTfile/protondata_10_gamma.root";
        TFile *file = TFile::Open(strr.c_str());
        // TTree *DetectionInfo = (TTree *)file->Get("DetectionInfo");
        TTreeReader reader("DetectionInfo", file);
        TTreeReaderValue<Int_t> branchValue(reader, "SDnum");

        TCanvas *c1 = new TCanvas("c1", "My Canvas", 800, 600);
        TH1F *h1 = new TH1F("h1", "Histogram of altitude", 100, 0, 100);
        while (reader.Next())
        {
            h1->Fill(*branchValue);
        };

        h1->SetLineColor(kBlue); // 设置线条颜色为蓝色
        h1->SetLineWidth(3);     // 设置线条粗细

        h1->Draw();

        h1->GetXaxis()->SetAxisColor(kBlack, 2);
        h1->GetYaxis()->SetAxisColor(kBlack, 2); // 颜色和线宽

        c1->SetLogy();

        //c1->GetFrame()->SetLineWidth(3);
        //c1->SetGridx();
        //c1->SetGridy();

        gPad->Modified();
        gPad->Update();
        // std::cout << "Before app.Run()" << std::endl;
        app.Run();
        // std::cout << "After app.Run() - this should not print!" << std::endl;


        */
        return 0;
    }
}