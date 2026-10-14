#ifndef FILTER_H
#define FILTER_H

#include<TTree.h>
#include<cmath>
#include<vector>
#include<string>

namespace mylib
{
    enum class CompareOp {
        Less_than,
        Greater_than,
        Equal_to,
        Not_Equal_to,
        Less_Equal_than,
        Greater_Equal_than
    };

    class Filter{
        public:
            Filter();
            ~Filter();

            template<typename T>
            TTree* FilterS(TTree * tree1, const T num, std::string BranchName, CompareOp op);


            int FilterB(std::string BranchName);

        private:

    };
} // namespace mylib


#endif 