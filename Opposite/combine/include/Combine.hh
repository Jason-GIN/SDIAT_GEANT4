#ifndef COMBINE_H
#define COMBINE_H

#include<string>
#include<TChain.h>
#include<TFile.h>
#include<TTree.h>

namespace mylib
{
    class Combine{
        public:
            Combine();
            ~Combine();

            void comb(std::string str1, std::string str2 ,int num);
            
            void supercomb(std::string str1, std::string str2 ,int num);

            void Pcomb(std::string str1, std::string str2 ,int num);
        private:

    };


}



#endif 