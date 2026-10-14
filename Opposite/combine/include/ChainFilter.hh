#ifndef CHAINFILTER_H
#define CHAINFILTER_H

#include <TFile.h>
#include <TTree.h>
#include <cmath>
#include <vector>
#include <string>

namespace mylib
{

    class ChainFilter
    {
    public:
        ChainFilter(const char *filename, const char *treename);
        ~ChainFilter();

        ChainFilter(const ChainFilter&) = delete;
        ChainFilter& operator=(const ChainFilter&) = delete;

        ChainFilter &filterStep(const char *expression);

        void save(const char *filename);
        TTree* getTree() { return currentTree; }

    private:
        TTree *currentTree = nullptr;
        TString originalFileName;
        void applyFinalOptimization(TTree*& tree);
    };

}
#endif