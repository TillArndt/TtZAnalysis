#ifndef containerStackVector_h
#define containerStackVector_h

#include "containerStack.h"
//#include <vector>
#include "TStyle.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TTree.h"

// class to handle containerstacks and to add systematics to the MC contributions etc.
// writes plots with and without ratio to TFiles


namespace ztop{

  class container1DStackVector{

  public:
    container1DStackVector();
    container1DStackVector(TString);
    ~container1DStackVector();
    void setName(TString Name){name_=Name;}
    TString getName(){return name_;}
    void setSyst(TString sys){isSyst_=sys;}
    TString getSyst(){return isSyst_;}

    void listStacks();

    void add(ztop::container1D &, TString, int, double);
    void addList( TString, int, double);
    std::vector<ztop::container1DStack> getVector(){return stacks_;}
    ztop::container1DStack getStack(TString);
    ztop::container1DStack getStack(unsigned int n){return stacks_[n];}
    void removeContribution(TString);

    void addMCErrorStackVector(TString,ztop::container1DStackVector, bool ignoreMCStat=true);
    void addMCErrorStackVector(ztop::container1DStackVector, bool ignoreMCStat=true);
    void addGlobalRelMCError(TString,double);
    void addRelSystematicsFrom(ztop::container1DStackVector);
    void removeError(TString);
    void renameSyst(TString,TString); //! old new

    void multiplyNorm(TString, double, TString); //! legendname, multiplier, stepID
    void multiplyNorms(TString, std::vector<double>, std::vector<TString>, bool showmessages=false);   //! multiplies norm of all MC with legendname  ,  with factor  ,  for step identifier string (step_string contains the id string)
    void multiplyAllMCNorms(double);

    void writeAllToTFile(TString, bool recreate=false, TString treename="stored_objects");
    void writeAllToTFile(TFile * , TString treename="stored_objects");
    void loadFromTree(TTree *, TString);
    container1DStackVector* getFromTree(TTree *, TString);
    void listAllInTree(TTree *);

    void clear(){stacks_.clear();name_="";}
    static std::vector<container1DStackVector*> csv_list;
    static bool csv_makelist;
    static void csv_clearlist(){csv_list.clear();}

  private:
    TString name_;
    TString isSyst_;
    std::vector<ztop::container1DStack> stacks_;



  };


}


#endif
