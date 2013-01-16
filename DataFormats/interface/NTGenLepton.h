#ifndef NTGenLepton_h
#define NTGenLepton_h


#include "Math/GenVector/LorentzVector.h"
namespace top{
  typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > LorentzVector;

  class NTGenLepton{
  public:
    explicit NTGenLepton(){}
    ~NTGenLepton(){}

    void setP4(LorentzVector p4In){p4_=p4In;}
    void setPdgId(int id){pdgid_=id;}
    void setMothers(std::vector<int> mothers){mothers_=mothers;}
    void setDaughters(std::vector<int> daughters){daughters_=daughters;}
    void setStatus(int Status){status_=Status;}

    int pdgId(){return pdgid_;}
    int status(){return status_;}

    LorentzVector p4(){return p4_;}
    double pt(){return p4_.Pt();}
    double E() {return p4_.E();}
    double e() {return p4_.E();}
    double phi(){return p4_.Phi();}
    double eta(){return p4_.Eta();}
    double m(){return p4_.M();}
    int q(){if(pdgid_<0) return -1; else return 1;}

    int getMother(){if(mothers_.size()>0) return mothers_.at(0); else return 0;}
    std::vector<int> getMothers(){return mothers_;}
    int getDaughter(){if(daughters_.size()>0) return daughters_.at(0); else return 0;}
    std::vector<int> getDaughters(){return daughters_;}

  private:


    int pdgid_, status_;
    LorentzVector p4_;
    std::vector<int> mothers_, daughters_;

  };
}


#endif
