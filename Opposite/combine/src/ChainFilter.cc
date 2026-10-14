#include <iostream>
#include <TTreeFormula.h>
#include <TSystem.h>
#include "ChainFilter.hh"

namespace mylib
{

    ChainFilter::ChainFilter(const char *filename, const char *treename)
    {
        originalFileName = filename;
        TFile *f = TFile::Open(filename);
        currentTree = (TTree *)f->Get(treename);
    };
    ChainFilter::~ChainFilter()
    {
        if (currentTree)
        {
            delete currentTree;
            currentTree = nullptr;
        }
    };

    ChainFilter &ChainFilter::filterStep(const char *expression)
    {
        if (!currentTree)
            return *this;
        // 使用TTreeFormula筛选
        TTreeFormula formula("filter", expression, currentTree);
        formula.SetQuickLoad(true);
        // 收集符合条件的entries
        std::vector<Long64_t> selected;
        for (Long64_t i = 0; i < currentTree->GetEntries(); i++)
        {
            currentTree->GetEntry(i);
            if (formula.EvalInstance(0) != 0)
            {
                selected.push_back(i);
            }
        }

        // 创建新树并优化
        TTree *newTree = currentTree->CloneTree(0);
        // optimizeBufferForSize(newTree, selected.size());

        for (Long64_t idx : selected)
        {
            currentTree->GetEntry(idx);
            newTree->Fill();
        }

        // 替换当前树
        delete currentTree;
        currentTree = newTree;

        return *this;
    }

    void ChainFilter::save(const char *filename)
    {
        TFile out(filename, "RECREATE");

        // 关键：写入前应用优化
        applyFinalOptimization(currentTree);

        currentTree->Write();
        out.Close();
    }

    void ChainFilter::applyFinalOptimization(TTree *&tree)
    {
        // 写入临时文件强制优化
        TString tempName = TString::Format("temp_%p.root", tree);

        // 写入临时文件
        TFile tempFile(tempName, "RECREATE");
        tree->Write("temp_tree");
        tempFile.Close();

        // 重新打开
        TFile *reopened = TFile::Open(tempName);
        if (!reopened || reopened->IsZombie())
        {
            std::cerr << "错误：无法打开临时文件 " << tempName << std::endl;
            return;
        }
        TTree *optimized = (TTree *)reopened->Get("temp_tree");

        // 关键：删除原树，替换为新树
        delete tree;                   // 释放原树内存
        tree = optimized->CloneTree(); // 指向优化后的树

        reopened->Close();
        delete reopened;

        if (gSystem->Unlink(tempName) != 0)
        {
            std::cerr << "警告：无法删除临时文件 " << tempName << std::endl;
        }
    }

}