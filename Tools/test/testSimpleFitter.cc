/*
 * testSimpleFitter.cc
 *
 *  Created on: May 22, 2014
 *      Author: kiesej
 */


#include "../interface/simpleFitter.h"
#include <iostream>

void chisq(Int_t &npar, Double_t *gin, Double_t &f, Double_t *par, Int_t iflag){
    f=1;
}

int main(){
    using namespace ztop;
    using namespace std;


    simpleFitter testfitter;

    testfitter.addPoint(1,2);
    testfitter.addPoint(2,3);
    testfitter.addPoint(3,1);
    testfitter.addYError(0.1);
    testfitter.addYError(0.1);
    testfitter.addYError(0.1);

    simpleFitter::printlevel=1;

    testfitter.setFitMode(simpleFitter::fm_pol2);
    testfitter.setMinimizer(simpleFitter::mm_minuit2);

    testfitter.fit();

    for(float i=0;i<3;i+=0.1){
        //std::cout << testfitter.getFitOutput(i) <<std::endl;
        std::cout << i << ": " <<testfitter.getFitOutput(i) << " \t";
        for(float j=-5;j<testfitter.getFitOutput(i);j+=0.1)

            if(j<0)
                std::cout << "*";

            else
                std::cout << "X";
        std::cout <<std::endl;
    }

    return 0;
}
