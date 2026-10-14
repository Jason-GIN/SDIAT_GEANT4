#include <iostream>
#include "Filter.hh"

namespace mylib
{

    Filter::Filter()
    {
    }

    Filter::~Filter()
    {
    }

    int Filter::FilterB(std::string BranchName)
    {
        if (BranchName.substr(0, 2) == "SD" || BranchName.substr(BranchName.length() - 2, 2) == "Id" ||
            BranchName.substr(BranchName.length() - 2, 2) == "ID")
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    template <typename T>
    bool applyComparison(const T &a, const T &b, CompareOp op)
    {
        switch (op)
        {
        case CompareOp::Less_than:
            return a < b;
        case CompareOp::Greater_than:
            return a > b;
        case CompareOp::Equal_to:
            return a == b;
        case CompareOp::Not_Equal_to:
            return a != b;
        case CompareOp::Less_Equal_than:
            return a <= b;
        case CompareOp::Greater_Equal_than:
            return a >= b;
        default:
            return false;
        }
    }

    template <typename T>
    TTree *Filter::FilterS(TTree *tree1, const T num, std::string BranchName, CompareOp op)
    {
        if (!tree1)
        { // 处理获取失败
            std::cout << "怎么回事，不对啊 (:()" << std::endl;
        };

        Long64_t nEntries = tree1->GetEntries();
        TTree *newtree1 = tree1->CloneTree(0);
        T value;
        //tree1->ResetBranchAddresses();
        tree1->SetBranchAddress(BranchName.c_str(), &value);
        for (Long64_t i = 0; i < nEntries; i++)
        {
            tree1->GetEntry(i);
            if (applyComparison(value, num, op))
            {
                //std::cout << "value: " << value << " " << "number: " << num << std::endl;
                newtree1->Fill();
            };
        };
        // tree1->SetBranchAddress(BranchName.c_str(), &Id);
        tree1->ResetBranchAddresses();
        std::cout << "-------------------(=^W^=)GIN(=^V^+Y)-------------------" << std::endl;
        return newtree1;
    }

    template TTree *Filter::FilterS<Int_t>(TTree *, int, std::string, CompareOp);
    template TTree *Filter::FilterS<Double_t>(TTree *, double, std::string, CompareOp);
    // template TTree* Filter::FilterS<float>(TTree*, float, std::string, CompareOp);

}