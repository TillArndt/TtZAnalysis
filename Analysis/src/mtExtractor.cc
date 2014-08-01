/*
 * mtExtractor.cc
 *
 *  Created on: Feb 20, 2014
 *      Author: kiesej
 */

#include "../interface/mtExtractor.h"
#include "TtZAnalysis/Tools/interface/container1DUnfold.h"
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <omp.h>

#include "TtZAnalysis/Tools/interface/plotterCompare.h"
#include "TtZAnalysis/Tools/interface/plotterMultiplePlots.h"
#include "TCanvas.h"
#include "TStyle.h"
#include <cmath>
#include "TLine.h"
#include "TList.h"
#include "TIterator.h"
#include "TKey.h"
#include "TtZAnalysis/Tools/interface/fileReader.h"
#include "TLatex.h"
#include "TtZAnalysis/Tools/interface/textFormatter.h"
#include "TtZAnalysis/Tools/interface/formatter.h"

#include "TtZAnalysis/Tools/interface/plotterInlay.h"

namespace ztop{

bool mtExtractor::debug=false;


mtExtractor::mtExtractor():plotnamedata_(""),plotnamemc_(""),plottypemc_("cuf"),
		minbin_(-1),maxbin_(-1),excludebin_(-1),tmpglgraph_(0),tfitf_(0),
		iseighttev_(true),defmtop_(172.5),setup_(false),textboxesmarker_("CMS"),syspidx_(1),
		dofolding_(false),isexternalgen_(false),rescalepreds_(false),usenormalized_(false),defmtidx_(0)
{
	reset();
}

void mtExtractor::setConfigFile(const TString& pl){
	if(debug)
		std::cout << "mtExtractor::setExternalGenInputFilesFormat" <<std::endl;
	setup_=false;
	if(!fileExists(pl.Data())){
		std::cout << "mtExtractor::setExternalGenInputFilesFormat: file " << pl <<" does not exist"<<std::endl;
		throw std::runtime_error("mtExtractor::setExternalGenInputFilesFormat: file does not exist");
	}
	extfileformatfile_=pl;

}
void mtExtractor::setInputFiles(const std::vector<TString>& pl){
	if(debug)
		std::cout << "mtExtractor::setInputFiles" <<std::endl;

	setup_=false;
	if(extfileformatfile_.Length() < 1){
		throw std::runtime_error("mtExtractor::setInputFiles: set external input file format first");
	}
	//read format
	plottypemc_="cuf";
	fileReader fr;
	fr.setStartMarker("[naming scheme]");
	fr.setEndMarker("[end naming scheme]");
	fr.readFile(extfileformatfile_.Data());
	//extfilepreamble_ = fr.getValue<TString>("filepreamble");
	TString extfileendpattern = fr.getValue<TString>("fileendpattern");
	ztop::textFormatter tf;
	isexternalgen_=false;
	extgenfiles_.clear();cufinputfiles_.clear();
	for(size_t i=0;i<pl.size();i++){
		TString filename=tf.getFilename(pl.at(i).Data());
		if(filename.BeginsWith(extfilepreamble_) && filename.EndsWith(extfileendpattern)){ //this is external gen input
			if(debug) std::cout << "mtExtractor::setInputFiles: part of input is external" <<std::endl;
			plottypemc_="ext"; //at least one external!
			extgenfiles_.push_back(pl.at(i));
			isexternalgen_=true;
		}
		else{
			cufinputfiles_.push_back(pl.at(i));

		}

	}


}


void mtExtractor::setup(){
	if(debug)
		std::cout << "mtExtractor::setup" <<std::endl;
	std::cout << "containerUnfold inputfiles: " << cufinputfiles_.size() << std::endl;
	std::cout << "External generated inputfiles: " << extgenfiles_.size() << std::endl;




	getMtValues();
	readFiles();
	if(!usenormalized_)
		addLumiUncert();
	renormalize();
	mergeSyst();


	makeGraphs();
	makeBinGraphs();
	setup_=true;
}
void  mtExtractor::cleanMem(){
	if(debug)
		std::cout << "mtExtractor::cleanMem" <<std::endl;
	for(size_t i=0;i<pltrptrs_.size();i++){ if(pltrptrs_.at(i)) delete pltrptrs_.at(i);}
	pltrptrs_.clear();
	if(tmpglgraph_)  delete tmpglgraph_;tmpglgraph_=0;
	tObjectList::cleanMem();
}



void mtExtractor::drawXsecDependence(TCanvas *c, bool fordata){
	if(debug)
		std::cout << "mtExtractor::drawXsecDependence" <<std::endl;
	if(!setup_)
		throw std::logic_error("mtExtractor::drawXsecDependence: first setup!");
	//read style file heplotterComparere
	using namespace ztop;
	TString add="MC";
	if(fordata) add="data";
	c->SetName("full distribution "+add);
	c->SetTitle("full distribution "+add);

	plotterCompare * pltrptr=0;
	pltrptr = new plotterCompare();
	pltrptrs_.push_back(pltrptr);
	std::vector<std::string> cids_;
	pltrptr->readTextBoxesInCMSSW(textboxesfile_,textboxesmarker_);
	std::vector<graph> * graphs=&mcgraphs_;
	if(fordata) graphs=&datagraphs_;

	for(size_t i=0;i<mtvals_.size();i++){
		//if(fabs(DEFTOPMASSFORNNLOMASSDEP - mtvals_.at(i))<0.1) continue;
		std::string mt=toString(mtvals_.at(i));
		mt="_mt"+mt;
		pltrptr->compareIds().push_back(mt);
		if(debug) std::cout << "mtExtractor::drawMCXsecDependence: added mt: " <<mt<<std::endl;

	}
	//read style
	if(fordata)
		pltrptr->readStyleFromFileInCMSSW(compplotsstylefiledata_);
	else
		pltrptr->readStyleFromFileInCMSSW(compplotsstylefilemc_);
	//next loop set plots!
	size_t newidx=0;
	for(size_t i=0;i<mtvals_.size();i++){
		if(fabs(defmtop_ - mtvals_.at(i))<0.1){
			pltrptr->setNominalPlot(&graphs->at(i));

		}
		else{
			pltrptr->setComparePlot(&graphs->at(i),newidx);
			newidx++;
		}
	}
	TVirtualPad* pad=c;
	pltrptr->usePad(pad);
	pltrptr->draw();


}

void mtExtractor::drawIndivBins(TCanvas *c,int syst){
	if(debug)
		std::cout << "mtExtractor::drawIndivBins" <<std::endl;
	if(!setup_)
		throw std::logic_error("mtExtractor::drawIndivBins: first setup!");

	if(databingraphs_.size() < 1 || (databingraphs_.size()>0 && syst >= (int)databingraphs_.at(0).getSystSize() )){
		std::cout << "mtExtractor::drawIndivBins:  request for: " << syst << " in " << databingraphs_.at(0).getSystSize() << std::endl;
		throw std::out_of_range("mtExtractor::drawIndivBins: syst index out of range");
	}
	//read style file here
	plotterMultiplePlots plotterdef;
	plotterdef.readStyleFromFileInCMSSW(binsplotsstylefile_);
	plotterdef.readTextBoxesInCMSSW(textboxesfile_,textboxesmarker_);

	if(databingraphs_.size()>1)
		plotterdef.removeTextBoxes();

	c->SetName("events per bin");
	c->SetTitle("events per bin");


	//make pads
	int vdivs=((int)(sqrt(databingraphs_.size())-0.0001))+1;//+1;
	int hdivs=(int)((float)databingraphs_.size()/(float)vdivs)+1;
	if(databingraphs_.size()>1){
		c->Divide(vdivs,hdivs);
	}

	gStyle->SetOptTitle(1);
	for(size_t i=0;i<databingraphs_.size();i++){
		plotterMultiplePlots * pl=new plotterMultiplePlots(plotterdef);
		pltrptrs_.push_back(pl);
		TVirtualPad *p=c->cd(i+1);
		pl->usePad(p);
		graph td;
		graph tmc;
		if(syst < -1){
			td=databingraphs_.at(i);
			tmc=mcbingraphs_.at(i);
		}
		else if(syst<0){
			td=databingraphs_.at(i).getNominalGraph();
			tmc=mcbingraphs_.at(i).getNominalGraph();
		}
		else{
			td=databingraphs_.at(i).getSystGraph(syst);
			tmc=mcbingraphs_.at(i).getSystGraph(syst);
		}
		td.setName("data");
		tmc.setName("pred");
		pl->setTitle(databingraphs_.at(i).getName());
		pl->addPlot(&td);
		pl->addPlot(&tmc);
		pl->draw();
	}



}


void mtExtractor::reset(){
	if(debug)
		std::cout << "mtExtractor::reset" <<std::endl;
	cleanMem();
	datacont_.clear();
	mccont_.clear();
	datagraphs_.clear();
	mcgraphs_.clear();
	mtvals_.clear();
	databingraphs_.clear();
	mcbingraphs_.clear();
	tmpbinlhds_.clear();
	syspidx_=1;
	allsyst_=graph();
	allsystsl_=graph();
	allsystsh_=graph();

	//  allsyst_.setNPoints(1);
	//  allsyst_.addErrorGraph("systatd",graph(1,""));
	//  allsyst_.addErrorGraph("systatup",graph(1,""));
}


void mtExtractor::drawSystVariation(TCanvas *c,const TString & sys){
	if(debug)
		std::cout << "mtExtractor::drawSystVariation " << sys <<std::endl;
	if(!c){
		throw std::runtime_error("mtExtractor::drawSystVariation: no canvas!");
	}
	//get right plot
	container1D cont=mccont_.at(defmtidx_);
	cont.setYAxisName(datacont_.at(0).getYAxisName());
	size_t idxup,idxdown;
	idxup=cont.getSystErrorIndex(sys+"_up");
	idxdown=cont.getSystErrorIndex(sys+"_down");
	container1D up = cont.getSystContainer(idxup);

	//do translation
	formatter fmt;
	fmt.readInNameTranslateFile((std::string)getenv("CMSSW_BASE") + (std::string)"/src/TtZAnalysis/Analysis/configs/general/SystNames.txt");
	fmt.setRootLatex(true);//this is root

	up.setName(fmt.translateName(sys)+" up");
	up.setAllErrorsZero();
	container1D down = cont.getSystContainer(idxdown);
	down.setName(fmt.translateName(sys)+" down");
	down.setAllErrorsZero();
	container1D nominal=cont;
	nominal.setName("nominal");
	nominal.setAllErrorsZero();


	plotterCompare * pl=0;
	pl=new plotterCompare();
	pltrptrs_.push_back(pl);
	pl->usePad(c);
	pl->compareIds().push_back("SysUp");
	pl->compareIds().push_back("SysDown");
	pl->readStyleFromFileInCMSSW(sysvariationsfile_);
	pl->readTextBoxesInCMSSW(textboxesfile_,textboxesmarker_);
	pl->setNominalPlot(&nominal,true);
	pl->setComparePlot(&up,0,true);
	pl->setComparePlot(&down,1,true);

	pl->draw();



}

///private functions
double mtExtractor::getNewNorm(double deltam,bool eighttev)const{ //following  NNLO paper arXiv:1303.6254


	if(debug)
		std::cout << "mtExtractor::getNewNorm" <<std::endl;
	double a1=0;
	double a2=0;


#define DEFTOPMASSFORNNLOMASSDEP 172.5

	double mref=DEFTOPMASSFORNNLOMASSDEP;
	if(eighttev){
		a1=-1.1125;
		a2=0.070778;
	}
	else{
		a1=-1.24243;
		a2=0.890776;
	}
	double reldm=mref/(mref+deltam);

	double out= (reldm*reldm*reldm*reldm) * (1+ a1*(deltam)/mref + a2*(deltam/mref)*(deltam/mref));
	if(debug) std::cout << "sigmtopmulti deltam: " << deltam << "\tmulti="<< out <<std::endl;
	return out;
}

std::vector<float>  mtExtractor::getMtValues(){

	if(debug)
		std::cout << "mtExtractor::getMtValues" <<std::endl;
	mtvals_.clear();
	std::vector<TString> notfoundinext;
	for(size_t i=0;i<cufinputfiles_.size();i++){
		std::string file=cufinputfiles_.at(i).Data();
		//cut out the mass value *TeV_<mass>_
		size_t spos=file.find("TeV_")+4;
		size_t endpos=file.find("_",spos);
		file=file.substr(spos,endpos-spos);
		if(debug) std::cout << "mtExtractor::getMtValues: found "  << file << std::endl;
		//check in external filenames
		bool found=false;
		for(size_t j=0;j<extgenfiles_.size();j++){
			if(extgenfiles_.at(i).BeginsWith(extfilepreamble_+file.data())){
				found=true;
				break;
			}
		}
		if(!found)
			notfoundinext.push_back(file);

		mtvals_.push_back(atof(file.data()));
	}
	if(notfoundinext.size()>0){
		std::cout << "mtExtractor::getMtValues: no external input for top mass values ";
		for(size_t i=0;i<notfoundinext.size();i++)
			std::cout << notfoundinext.at(i)<<" ";
		std::cout << std::endl;
	}
	for(size_t i=0;i<mtvals_.size();i++){
		if(fabs(mtvals_.at(i)-defmtop_)<0.1)
			defmtidx_=i;
	}

	return mtvals_;
}


void mtExtractor::readFiles(){
	if(debug) std::cout << "mtExtractor::readFiles" <<std::endl;
	bool mciscuf=plottypemc_=="cuf";


	if(!mciscuf && cufinputfiles_.size() != extgenfiles_.size()){
		throw std::runtime_error("mtExtractor::readFiles: needs same number of data inputfiles and external gen files (one for each top mass)");
	}

	bool unfoldfolded=false;

	//read config
	fileReader fr;
	fr.setComment("$");
	fr.setStartMarker("[plotter styles]");
	fr.setEndMarker("[end plotter styles]");
	fr.readFile(extfileformatfile_.Data());

	compplotsstylefilemc_ = fr.getValue<std::string>("compareAllMassesMC");
	compplotsstylefiledata_ = fr.getValue<std::string>("compareAllMassesData");
	binsplotsstylefile_ = fr.getValue<std::string>("binDependencies");
	binschi2plotsstylefile_ = fr.getValue<std::string>("chi2PerBin");
	binsplusfitsstylefile_ = fr.getValue<std::string>("binDependenciesPlusFits");
	chi2plotsstylefile_ = fr.getValue<std::string>("chi2total");
	sysvariationsfile_ = fr.getValue<std::string>("systematicsVariations");
	allsyststylefile_ = fr.getValue<std::string>("results");
	spreadwithinlaystylefile_ = fr.getValue<std::string>("spreadWithInlay");
	textboxesfile_= fr.getValue<std::string>("textBoxesFile");




	TString extfileendpattern;
	TString histpreamble;
	TString sysupid,sysdownid,nomid;
	bool isdivbbw=true;
	bool usekeynames=false;
	float multiplygennorm=1;
	TString previousxsecunits,newxsecunits;
	if(!mciscuf){//read in external file format

		fr.clear();
		fr.setStartMarker("[naming scheme]");
		fr.setEndMarker("[end naming scheme]");
		fr.readFile(extfileformatfile_.Data());
		extfileendpattern = fr.getValue<TString>("fileendpattern");
		TString plotnamerepl=plotnamedata_;
		plotnamerepl.ReplaceAll(" ","_");
		histpreamble = fr.getValue<TString>(plotnamerepl.Data());
		sysupid = fr.getValue<TString>("systUpId");
		sysdownid = fr.getValue<TString>("systDownId");
		nomid = fr.getValue<TString>("nominalId");
		isdivbbw =fr.getValue<bool>("inputIsDividedByBinWidth");
		usekeynames=fr.getValue<bool>("useKeyNames");
		multiplygennorm=fr.getValue<float>("multiplynorm",1);
		previousxsecunits=fr.getValue<TString>("previousXsecUnits");
		newxsecunits=fr.getValue<TString>("newXSecUnits");
	}

	if(!dofolding_){
		throw std::runtime_error("using unfolded data not supported anymore!! exit");
	}

	datacont_.clear();
	mccont_.clear();

	for(size_t i=0;i<cufinputfiles_.size();i++){

		containerStackVector * csv=new containerStackVector();
		csv->loadFromTFile(cufinputfiles_.at(i));
		//  allanalysisplots_.push_back(*csv);
		containerStack stack=csv->getStack(plotnamedata_);
		delete csv;
		//BACKGROUND background uncertainties here
		stack.addRelErrorToBackgrounds(0.3,true,"BG");
		stack.mergePartialVariations("BG"); //-> no data containers>!
		//stack.mergePartialVariations("JES",false);


		container1DUnfold tempcuf=stack.produceUnfoldingContainer();


		// tempcuf.loadFromTFile(cufinputfiles_.at(i),plotnamedata_);
		if(debug) std::cout << "mtExtractor::readFiles: read " << tempcuf.getName() <<std::endl;
		if(tempcuf.isDummy())
			throw std::runtime_error("mtExtractor::readFiles: at least one file without container1DUnfold");

		container1D datareference;

		if(!dofolding_){ //FIXME
			//unfold here


			datareference=tempcuf.getUnfolded();
			if(datareference.isDummy()){
				throw std::runtime_error("mtExtractor::readFiles: cannot find an unfolded distribution even though \
            			 folding option was not selected.");
				//here an unfolding could be performed instead
			}
		}
		else{
			container1D temp=tempcuf.getRecoContainer();
			datareference=temp; // NEW POISSON NO BG SUBTRACTION! -tempcuf.getBackground();
			if(usenormalized_)
				datareference.setYAxisName("1/N_{tot} "+datareference.getYAxisName());
			//rebin at first to gen
			if(!mciscuf) //rebin to generator binning
				datareference=datareference.rebinToBinning(tempcuf.getBinnedGenContainer());
		}

		datacont_.push_back(datareference);

		TString extfilename;
		container1D gen;
		if(mciscuf){
			isexternalgen_=false;
			if(!dofolding_){
				gen=tempcuf.getBinnedGenContainer();
				gen*= (1/tempcuf.getLumi());
			}
			else{
				gen=tempcuf.getVisibleSignal();
				if(unfoldfolded){
					gen=tempcuf.getGenContainer(); //
					//gen*= (1/tempcuf.getLumi());
					gen=tempcuf.fold(gen); //
				}
				else{
					gen+=tempcuf.getBackground();
				}
				TString oldaxisname=gen.getXAxisName();
				oldaxisname.ReplaceAll("_gen","");
				gen.setXAxisName(oldaxisname);
				gen.setYAxisName(datareference.getYAxisName());
				gen=gen.rebinToBinning(datareference);
			}
			mccont_.push_back(gen);
		}
		else{ //external input for gen
			//find right file
			isexternalgen_=true;


			extfilename=extfilepreamble_ +toTString(mtvals_.at(i))+extfileendpattern;
			std::vector<TString>::iterator it=std::find(extgenfiles_.begin(),extgenfiles_.end(),extfilename);
			if(it==extgenfiles_.end()){
				std::cout << "mtExtractor::readFiles: external gen file " <<extfilename << " not found!" <<std::endl;
				throw std::runtime_error("mtExtractor::readFiles: external gen file not found");
			}
			//try to open file
			TFile * fin = new TFile(*it,"READ");
			if(!fin || fin->IsZombie()){
				throw std::runtime_error("mtExtractor::readFiles: external gen file corrupt");
			}
			if(debug){
				std::cout << "mtExtractor::readFiles: opened gen: " << extfilename << " for " << cufinputfiles_.at(i) << std::endl;
			}
			//read in everything you can get
			TList* list = fin->GetListOfKeys() ;
			if(!list)
				throw std::runtime_error("mtExtractor::readFiles: nothing found in external gen file");
			TIter next(list) ;
			TKey* key=(TKey*)next() ;
			TObject* obj ;
			std::vector<TString> histnames,sysnames,allnames;
			while ( key ) {
				obj = key->ReadObj() ;
				if (  (!obj->InheritsFrom("TH1")) ) {
					continue;
				}
				else{
					TString objname=obj->GetName();
					allnames.push_back((TString)key->GetName()+" "+(TString)objname);
					if(usekeynames)
						objname=key->GetName();
					if(objname.BeginsWith(histpreamble+extfilepdf_+"_")){
						//    if(debug)
						//       std::cout << "mtExtractor::readFiles: added histogram name " << objname << " from file "<< fin->GetName()<<std::endl;
						histnames.push_back(objname);
						objname.Remove(0,(histpreamble+extfilepdf_+"_").Length());
						sysnames.push_back(objname);
					}
				}
				key = (TKey*)next();
			}
			//get nominal
			size_t nompos = std::find(sysnames.begin(),sysnames.end(),nomid)-sysnames.begin();
			if(nompos==sysnames.size()){  //not found
				std::cout << "mtExtractor::readFiles: Histogram not found. Available histograms: " <<std::endl;
				for(size_t nameit=0;nameit<allnames.size();nameit++){
					std::cout << allnames.at(nameit) <<std::endl;
				}
				throw std::runtime_error("mtExtractor::readFiles: Histogram not found.");
			}
			container1D nominal;
			TH1F h=tryToGet<TH1F>(fin,histnames.at(nompos));

			nominal.import(&h,isdivbbw);
			TString newname=histnames.at(nompos);
			newname.ReplaceAll("central","");
			nominal.setName(newname);
			nominal=nominal.rebinToBinning(datareference);
			nominal *=multiplygennorm;
			TString yaxis=nominal.getYAxisName();
			yaxis.ReplaceAll("["+previousxsecunits+"]","["+newxsecunits+"]");
			formatter formatsys;
			formatsys.readInNameTranslateFile((std::string)getenv("CMSSW_BASE")+"/src/TtZAnalysis/Analysis/configs/topmass/MCFM_SystNames.txt");

			for(size_t histpos=0;histpos<histnames.size();histpos++){
				if(histpos==nompos) continue;
				TH1F hsys=tryToGet<TH1F>(fin,histnames.at(histpos));
				container1D tempgen;
				tempgen.import(&hsys,isdivbbw);
				tempgen=tempgen.rebinToBinning(datareference);
				tempgen*=multiplygennorm;
				TString sysname=sysnames.at(histpos);

				if(sysname.EndsWith(sysupid)){
					sysname.Replace(sysname.Length()-sysupid.Length(),sysupid.Length(),"");
					sysname=formatsys.translateName(sysname);
					sysname+="_up";
				}
				else if(sysname.EndsWith(sysdownid)){
					sysname.Replace(sysname.Length()-sysdownid.Length(),sysdownid.Length(),"");
					sysname=formatsys.translateName(sysname);
					sysname+="_down";
				}
				nominal.addErrorContainer(sysname,tempgen);

			}
			if(dofolding_){
				nominal*=tempcuf.getLumi();
				nominal=tempcuf.fold(nominal);
				nominal=nominal.rebinToBinning(datareference);
			}
			mccont_.push_back(nominal);
			delete fin;
		} //external
		if(debug){
			std::cout << "mtExtractor::readFiles: added " << datacont_.at(i).getName() << " ("<< cufinputfiles_.at(i)<<"), "
					<< mccont_.at(i).getName() << " (" << extfilename << ")" <<std::endl;
		}
	}


	if(datacont_.size() <1 || mccont_.size() < 1){
		throw std::runtime_error("mtExtractor::readFiles: no input distributions found");
	}
	if(datacont_.size() != mccont_.size()){
		std::cout << "mtExtractor::readFiles: data ("<<datacont_.size() << ") and MC ("<< mccont_.size() <<") input size don't match" <<std::endl;
		throw std::runtime_error("mtExtractor::readFiles: data and MC  input size don't match");
	}

}



void mtExtractor::addLumiUncert(){

	for(size_t i=0;i<mccont_.size();i++){
		mccont_.at(i).addGlobalRelError("LUMI",0.026);
		datacont_.at(i).addGlobalRelError("LUMI",0.);
	}


}



void mtExtractor::renormalize(){
	if(debug) std::cout << "mtExtractor::renormalize" <<std::endl;

	if(!usenormalized_){
		//read in renormalization map (k-factors)

		fileReader fr;

		fr.setStartMarker(((TString)"[k-factor map "+extfilepdf_+ "]").Data());
		fr.setEndMarker(((TString)"[end k-factor map " +extfilepdf_+  "]").Data());
		fr.readFile(extfileformatfile_.Data());

		predrescalers_.clear();
		for(size_t i=0;i<mtvals_.size();i++){
			predrescalers_.push_back(fr.getValue<float>(toString(mtvals_.at(i))));
		}

		//throw std::runtime_error("mtExtractor::renormalize: other mc input not supported yet");
		//std::cout << "external gen input. No renormalization needed - doing nothing" <<std::endl;
		// if(debug) std::cout << "mtExtractor::renormalize: rescaling gen input with"
		//}

		bool usecuf= plottypemc_==(TString)"cuf";

		for(size_t i=0;i<mccont_.size();i++){
			float renorm= 1;
			if(usecuf){
				renorm= getNewNorm(mtvals_.at(i)-DEFTOPMASSFORNNLOMASSDEP,iseighttev_);
				//add NNLO uncertainties

				mccont_.at(i).addGlobalRelError("scale (NNLO norm)",0.034);
				mccont_.at(i).addGlobalRelError("pdf (NNLO norm)",0.026);
			}
			//make suitable for n top masses
			else{
				if(rescalepreds_){
					renorm=predrescalers_.at(i);
				}
			}
			mccont_.at(i) *=renorm;

			if(debug) std::cout << "mtExtractor::renormalize: mt: " <<  mtvals_.at(i) << " sf: " << renorm << std::endl;
		}
	}
	else{
		// FIXED:
		// understand where large UF/OF entries in MC come from!
		// they come from ttbg entries that go to underflow bin
		// only for mc
		for(size_t i=0;i<mccont_.size();i++){
			mccont_.at(i).normalize(false,true,1);
		}
		for(size_t i=0;i<datacont_.size();i++){
			datacont_.at(i).normalize(false,true,1);
		}
	}
	//but rescale all csvs
	/*    for(size_t i=0;i<allanalysisplots_.size();i++){
        for(size_t j=0;j<allanalysisplots_.at(i).size();j++){
            std::vector<size_t> sigidx=allanalysisplots_.at(i).getStack(j).getSignalIdxs();
            for(size_t k=0;k<sigidx.size();k++)
                allanalysisplots_.at(i).getStack(j).multiplyNorm(sigidx.at(k),
                        getNewNorm(mtvals_.at(i)-DEFTOPMASSFORNNLOMASSDEP,iseighttev_));
        }
    }
	 */

}
void mtExtractor::mergeSyst(){

	//add from all
	if(debug) std::cout <<"mtExtractor::mergeSyst: MC" << std::endl;
	for(size_t i=0;i<mccont_.size();i++){
		for(size_t j=0;j<mccont_.size();j++){
			if(i==j) continue;
			mccont_.at(i).addRelSystematicsFrom(mccont_.at(j));
		}
	}
	if(debug) std::cout <<"mtExtractor::mergeSyst: data" << std::endl;
	for(size_t i=0;i<datacont_.size();i++){
		for(size_t j=0;j<datacont_.size();j++){
			if(i==j) continue;
			datacont_.at(i).addRelSystematicsFrom(datacont_.at(j));

		}
	}
	//merge data and MC syst (beware of naming because of correllations!)
	if(datacont_.size()>0){
		for(size_t id=1;id<datacont_.size();id++){//equalize data
			datacont_.at(id).equalizeSystematicsIdxs(datacont_.at(0));
		}
		for(size_t imc=0;imc<mccont_.size();imc++){//data to mc
			mccont_.at(imc).equalizeSystematicsIdxs(datacont_.at(0));
		}
		for(size_t id=1;id<datacont_.size();id++){//the ones added from mc back to data
			datacont_.at(id).equalizeSystematicsIdxs(datacont_.at(0));
		}
	}

}
//just transforms to graph
void mtExtractor::makeGraphs(){
	for(size_t i=0;i<mccont_.size();i++){
		graph tempgraph;
		tempgraph.import(&mccont_.at(i),true);
		tempgraph.setName("m_{t}="+toTString(mtvals_.at(i)));
		tempgraph.sortPointsByX();
		mcgraphs_.push_back(tempgraph);
	}
	// mccont_.clear();
	if(debug) std::cout << "mtExtractor::makeGraphs: created "<< mcgraphs_.size() << " MC graphs" << std::endl;
	for(size_t i=0;i<datacont_.size();i++){
		graph tempgraph;
		tempgraph.import(&datacont_.at(i),true);
		tempgraph.setName("m_{t}="+toTString(mtvals_.at(i)));
		tempgraph.sortPointsByX();
		datagraphs_.push_back(tempgraph);
	}
	// datacont_.clear();
	if(debug) std::cout << "mtExtractor::makeGraphs: created "<< datagraphs_.size() << " data graphs" << std::endl;

}

graph mtExtractor::makeDepInBin(const std::vector<graph> & graphs, size_t bin)const{
	graph massdep=graph(mtvals_.size());

	TString newname;
	for(size_t mtit=0;mtit<mtvals_.size();mtit++){
		const graph & g= graphs.at(mtit);
		if(mtit==0){
			//gives bin width
			float min=g.getPointXContent(bin) - g.getPointXError(bin,true);
			float max=g.getPointXContent(bin) + g.getPointXError(bin,true);
			newname = "m_{lb}: "+toTString(min) +"-" + toTString(max) +" GeV";
			if(debug) std::cout << "mtExtractor::makeDepInBin: new graph name: "
					<< newname << std::endl;
		}
		const float & mtval=mtvals_.at(mtit);
		for(int sys=-1;sys<(int)g.getSystSize();sys++){
			int innersys=-1;
			if(sys>-1){
				massdep.getXCoords().addLayer(g.getSystErrorName(sys));
				innersys=massdep.getYCoords().addLayer(g.getSystErrorName(sys));
			}
			float newval=g.getPointYContent(bin,sys);
			float newstat=g.getPointYStat(bin,sys);
			massdep.setPointYContent(mtit,newval,innersys);
			massdep.setPointXContent(mtit,mtval,innersys);
			massdep.setPointYStat(mtit,newstat,innersys);
			massdep.setPointXStat(mtit,0,innersys);
		}
	}



	massdep.sortPointsByX();
	massdep.setName(newname);
	setAxisXsecVsMt(massdep);
	return massdep;
}


void mtExtractor::makeBinGraphs(){
	//keep in mind: The points of each graph are properly sorted by X
	//So no fancy extra bin finding anymore!
	// But be careful aith the ordering of systematics in graphs

	//create graph with right amount of points, assume same for MC and data


	for(size_t bin=0;bin<datagraphs_.at(0).getNPoints();bin++){
		if((maxbin_>=0 && (int)bin>maxbin_) || (minbin_>=0 && (int)bin<minbin_) || (int)bin==excludebin_) continue;
		if(isEmptyForAnyMass( bin)) continue;
		databingraphs_.push_back(makeDepInBin(datagraphs_,bin));
	}
	if(debug) std::cout << "mtExtractor::makeBinGraphs: created "<< databingraphs_.size()<< " data graphs" << std::endl;
	for(size_t bin=0;bin<mcgraphs_.at(0).getNPoints();bin++){
		if((maxbin_>=0 && (int)bin>maxbin_) || (minbin_>=0 && (int)bin<minbin_) || (int)bin==excludebin_) continue;
		if(isEmptyForAnyMass( bin)) continue;
		mcbingraphs_.push_back(makeDepInBin(mcgraphs_,bin));
	}
	if(debug) std::cout << "mtExtractor::makeBinGraphs: created "<< mcbingraphs_.size() << "  mc graphs" << std::endl;

}
void mtExtractor::createBinLikelihoods(int syslayer,bool includesyst,bool datasyst){
	//get graphs for syst.
	if(debug)
		std::cout << "mtExtractor::createBinLikelihoods" <<std::endl;
	if(includesyst){
		paraExtr_.setInputA(databingraphs_);
		paraExtr_.setInputB(mcbingraphs_);
		tmpSysName_="all syst.";
	}
	else { //create inputs

		if(syslayer>=(int)databingraphs_.at(0).getSystSize())
			throw std::out_of_range("mtExtractor::createBinLikelihoods: out of range");
		//check for same variation in MC
		if(syslayer>=0)
			tmpSysName_=databingraphs_.at(0).getSystErrorName(syslayer);
		else
			tmpSysName_="nominal";
		std::vector<graph> dgs,mcgs;
		for(size_t i=0;i<databingraphs_.size();i++){
			if(syslayer>-1){
				size_t mcsysidx=mcbingraphs_.at(i).getSystErrorIndex(databingraphs_.at(i).getSystErrorName(syslayer));
				if(mcsysidx<mcbingraphs_.at(i).getSystSize()){
					mcgs.push_back(mcbingraphs_.at(i).getSystGraph(mcsysidx));
				}
				else{
					mcgs.push_back(mcbingraphs_.at(i));
				}
				dgs.push_back(databingraphs_.at(i).getSystGraph((size_t)syslayer));
			}
			else{ //nominal
				mcgs.push_back(mcbingraphs_.at(i).getNominalGraph());
				dgs.push_back(databingraphs_.at(i).getNominalGraph());
			}
		}
		paraExtr_.setInputA(dgs);
		paraExtr_.setInputB(mcgs);
	}
	//input prepared

	tmpbinlhds_=paraExtr_.createLikelihoods();
	//style
	for(size_t i=0;i<tmpbinlhds_.size();i++)
		setAxisLikelihoodVsMt(tmpbinlhds_.at(i));
	/*
    tmpbinfitsdata_=paraExtr_.getFitFunctionA();
    tmpbinfitsmc_=paraExtr_.getFitFunctionB(); */
	if(debug) std::cout << "mtExtractor::createBinLikelihoods: created." <<std::endl;



}

bool mtExtractor::isEmptyForAnyMass(size_t bin)const{
	if(datagraphs_.size()<1){
		throw std::logic_error("mtExtractor::isEmptyForAnyMass: first fill graphs");
	}
	if(bin>=datagraphs_.at(0).getNPoints()){
		throw std::out_of_range("mtExtractor::isEmptyForAnyMass: bin out of range");
	}

	for(size_t i=0;i<mtvals_.size();i++){
		for(int sys=-1;sys<(int)datagraphs_.at(0).getSystSize();sys++){
			if(datagraphs_.at(i).getPointYContent(bin,sys) == 0)
				return true;
			if(mcgraphs_.at(i).getPointYContent(bin,sys) == 0)
				return true;
		}
	}
	return false;

}


void mtExtractor::drawBinLikelihoods(TCanvas *c){
	plotterMultiplePlots plotterdef;
	plotterdef.readStyleFromFileInCMSSW(binschi2plotsstylefile_);
	plotterdef.readTextBoxesInCMSSW(textboxesfile_,textboxesmarker_);


	if(databingraphs_.size()>1)
		plotterdef.removeTextBoxes();

	if(getExtractor()->getLikelihoodMode()!= parameterExtractor::lh_fit
			|| getExtractor()->getLikelihoodMode()!= parameterExtractor::lh_fitintersect){
		c->SetName("-2*log likelihood per bin "+tmpSysName_);
		c->SetTitle("-2*log likelihood per bin "+tmpSysName_);
	}
	else{
		c->SetName("#chi^{2} per bin "+tmpSysName_);
		c->SetTitle("#chi^{2} per bin "+tmpSysName_);
	}
	//make pads
	int vdivs=((int)(sqrt(tmpbinlhds_.size())-0.0001))+1;//+1;
	int hdivs=(int)((float)tmpbinlhds_.size()/(float)vdivs)+1;
	if(databingraphs_.size()>1){
		c->Divide(vdivs,hdivs);
	}
	gStyle->SetOptTitle(1);
	for(size_t i=0;i<tmpbinlhds_.size();i++){
		plotterMultiplePlots * pl=new plotterMultiplePlots(plotterdef);
		pltrptrs_.push_back(pl);
		TVirtualPad *p=c->cd(i+1);
		pl->usePad(p);
		graph td=tmpbinlhds_.at(i);

		pl->setTitle(databingraphs_.at(i).getName());
		pl->addPlot(&td);
		pl->draw();
	}
}
/*
void mtExtractor::overlayBinFittedFunctions(TCanvas *c){
    return;
    if(getExtractor()->getLikelihoodMode() != parameterExtractor::lh_fit) return;
    for(size_t i=0;i<tmpbinchi2_.size();i++){
        TVirtualPad *p=c->cd(i+1);
        for(size_t j=0;j<tmpbinfitsmc_.at(i).size();j++){
            tmpbinfitsmc_.at(i).at(j)->Draw("same");
        }
        for(size_t j=0;j<tmpbinfitsdata_.at(i).size();j++){
            tmpbinfitsdata_.at(i).at(j)->Draw("same");
        }

    }
}
 */
void mtExtractor::drawBinsPlusFits(TCanvas *c,int syst){
	if(debug)
		std::cout << "mtExtractor::drawBinsPlusFits" <<std::endl;
	if(!(getExtractor()->getLikelihoodMode()== parameterExtractor::lh_fit
			|| getExtractor()->getLikelihoodMode()== parameterExtractor::lh_fitintersect))
		return;


	if(databingraphs_.size() < 1 || (databingraphs_.size()>0 && syst >= (int)databingraphs_.at(0).getSystSize() )){
		throw std::out_of_range("mtExtractor::drawBinsPlusFits: syst index out of range");
	}
	//read style file here
	plotterMultiplePlots plotterdef;
	plotterdef.readStyleFromFileInCMSSW(binsplusfitsstylefile_);
	plotterdef.readTextBoxesInCMSSW(textboxesfile_,textboxesmarker_);
	c->SetName("cross section per bin with fits");
	c->SetTitle("cross section per bin with fits");

	if(databingraphs_.size()>1)
		plotterdef.removeTextBoxes();

	//make pads
	int vdivs=((int)(sqrt(databingraphs_.size())-0.0001))+1;//+1;
	int hdivs=(int)((float)databingraphs_.size()/(float)vdivs)+1;

	if(databingraphs_.size()>1){
		c->Divide(vdivs,hdivs);
	}
	gStyle->SetOptTitle(1);
	for(size_t i=0;i<databingraphs_.size();i++){
		plotterMultiplePlots * pl=new plotterMultiplePlots(plotterdef);
		pltrptrs_.push_back(pl);
		TVirtualPad *p=c->cd(i+1);
		pl->usePad(p);
		graph td;
		graph tmc;
		// graph * fita=0,*fitb=0;
		if(syst < -1){
			td=databingraphs_.at(i);
			tmc=mcbingraphs_.at(i);

		}
		else if(syst<0){
			td=databingraphs_.at(i).getNominalGraph();
			tmc=mcbingraphs_.at(i).getNominalGraph();
		}
		else{
			td=databingraphs_.at(i).getSystGraph(syst);
			tmc=mcbingraphs_.at(i).getSystGraph(syst);
		}
		td.setName("data");
		tmc.setName("prediction");
		pl->setTitle(databingraphs_.at(i).getName());
		graph datafc=getExtractor()->getFittedGraphsA().at(i);
		graph mcfitc=getExtractor()->getFittedGraphsB().at(i);
		setAxisXsecVsMt(datafc);
		datafc.setName("data fit");
		mcfitc.setName("pred fit");
		pl->addPlot(&datafc);
		pl->setLastNoLegend();
		pl->addPlot(&mcfitc);
		pl->setLastNoLegend();
		pl->addPlot(&td);
		pl->addPlot(&tmc);

		pl->draw();
	}

}

void mtExtractor::createGlobalLikelihood(){
	if(tmpbinlhds_.size()<1)
		throw std::logic_error("mtExtractor::createGlobalLikelihood: first create (non empty) likelihoods for bins");
	if(paraExtr_.getLikelihoodMode() == parameterExtractor::lh_chi2
			|| paraExtr_.getLikelihoodMode() == parameterExtractor::lh_chi2Swapped
			||(paraExtr_.getLikelihoodMode() == parameterExtractor::lh_fit && tmpbinlhds_.at(0).getNPoints() > 1)){


		//just add upp everything
		tmpgllhd_=tmpbinlhds_.at(0);
		gStyle->SetOptTitle(1);
		for(size_t i=1;i<tmpbinlhds_.size();i++)
			tmpgllhd_=tmpgllhd_.addY(tmpbinlhds_.at(i));

		if(tmpglgraph_)  {delete tmpglgraph_;tmpglgraph_=0;}
		tmpglgraph_=(tmpgllhd_.getTGraph());

		if(tmpSysName_ == "nominal" || tmpSysName_=="")
			globalnominal_=tmpgllhd_;
	}
	/*
    else if(paraExtr_.getLikelihoodMode() == parameterExtractor::lh_fitintersect){
        float inverrorsum=0;
        float weightedsum=0;
        ///WARNING, ONLY ONE POINT EXPECTED
        if(tmpbinlhds_.size() <1)
            throw std::runtime_error("mtExtractor::createGlobalLikelihood: in fitting mode: at least one bin expected");
        if(tmpbinlhds_.at(0).getNPoints()<1 )
            throw std::runtime_error("mtExtractor::createGlobalLikelihood: in fitting mode: exaclty one point per graph expected");

        //get rid of above exception to do the other implementation


        if(tmpbinlhds_.at(0).getNPoints() == 1){
            tmpgllhd_=graph(1);
            tmpgllhd_.setPointYContent(0,1);

            for(size_t i=0;i<tmpbinlhds_.size();i++){


                float errormax=tmpbinlhds_.at(i).getPointXError(0,false,"");
                if(fabs(errormax) > 0.5 * tmpbinlhds_.at(i).getPointXContent(0)){
                    if(debug) std::cout << "mtExtractor::createGlobalLikelihood: in fit mode, bin " <<i << " not used (large error)" <<std::endl;
                    continue;
                }
                inverrorsum += 1/(errormax*errormax);
                weightedsum += tmpbinlhds_.at(i).getPointXContent(0)/(errormax*errormax);
            }
            float weightedmean=weightedsum/inverrorsum;
            tmpgllhd_.setPointXContent(0,weightedmean);
            tmpgllhd_.setPointXStat(0,1/inverrorsum);
            if(tmpglgraph_)  {delete tmpglgraph_;tmpglgraph_=0;}
            tmpglgraph_=(tmpgllhd_.getTGraph());
        }

    }
	 */

}
float mtExtractor::drawGlobalLikelihood(TCanvas *c,bool zoom){
	float centralYValue=0;
	if(paraExtr_.getLikelihoodMode() == parameterExtractor::lh_chi2
			|| paraExtr_.getLikelihoodMode() == parameterExtractor::lh_chi2Swapped
			|| (paraExtr_.getLikelihoodMode() == parameterExtractor::lh_fit  && tmpbinlhds_.at(0).getNPoints() > 1)){

		if(paraExtr_.getLikelihoodMode() == parameterExtractor::lh_fit ){
			c->SetName("global log likelihood "+tmpSysName_);
			c->SetTitle("global log likelihood "+tmpSysName_);
		}
		else if(paraExtr_.getLikelihoodMode() == parameterExtractor::lh_chi2
				|| paraExtr_.getLikelihoodMode() == parameterExtractor::lh_chi2Swapped){
			c->SetName("global #chi^{2} "+tmpSysName_);
			c->SetTitle("global #chi^{2} "+tmpSysName_);
		}

		plotterMultiplePlots * pl=new plotterMultiplePlots;
		pltrptrs_.push_back(pl);
		pl->readStyleFromFileInCMSSW(binschi2plotsstylefile_);
		pl->readTextBoxesInCMSSW(textboxesfile_,textboxesmarker_);
		pl->usePad(c);
		pl->setTitle("global "+tmpSysName_);
		pl->addPlot(&tmpgllhd_);
		pl->draw();




		float plotmin=0;


		float cetralXValue=0;
		float xpleft=0;
		float xpright=0;
		float intersecty=0;

		if(paraExtr_.getLikelihoodMode() == parameterExtractor::lh_fit){
			//extract from -2*log likelihood
			//look for maximum

			float errordeltay=1;

			size_t minpoint=0;

			centralYValue=tmpgllhd_.getYMin(minpoint,false);
			intersecty=centralYValue+errordeltay;
			cetralXValue=tmpgllhd_.getPointXContent(minpoint);

			plotmin=centralYValue-1;

			for(size_t n=minpoint;n<tmpgllhd_.getNPoints();n++){
				if(fabs(centralYValue - tmpgllhd_.getPointYContent(n)) > errordeltay){
					xpright=tmpgllhd_.getPointXContent(n);
					break;
				}
			}
			for(size_t n=minpoint;n>1;n--){ //1 to avoid warnings
				if(fabs(centralYValue - tmpgllhd_.getPointYContent(n)) > errordeltay){
					xpleft=tmpgllhd_.getPointXContent(n);
					break;
				}
			}

		}
		else if(paraExtr_.getLikelihoodMode() == parameterExtractor::lh_chi2 || paraExtr_.getLikelihoodMode() == parameterExtractor::lh_chi2Swapped){
			if(!tfitf_) throw std::logic_error("mtExtractor::drawGlobalLikelihood: first fitGlobalLikelihood");

			tfitf_->Draw("same");
			centralYValue=tfitf_->GetMinimum(tmpgllhd_.getXMin(),tmpgllhd_.getXMax());
			std::cout << "minimum: " << centralYValue <<std::endl;
			plotmin=centralYValue-1;
			intersecty=centralYValue+1;

			cetralXValue=tfitf_->GetX(centralYValue,tmpgllhd_.getXMin(),tmpgllhd_.getXMax());
			xpleft=tfitf_->GetX(centralYValue+1,tmpgllhd_.getXMin(),cetralXValue);
			xpright=tfitf_->GetX(centralYValue+1,cetralXValue,tmpgllhd_.getXMax());
		}

		TLine * xlinel=addObject(new TLine(xpleft,plotmin,xpleft,intersecty));
		TLine * xliner=addObject(new TLine(xpright,plotmin,xpright,intersecty));
		TLine * xlinec=addObject(new TLine(cetralXValue,plotmin,cetralXValue,centralYValue));
		TLine * yline=addObject(new TLine(tmpgllhd_.getXMin(),intersecty,xpleft,intersecty));
		TLine * yline2=addObject(new TLine(tmpgllhd_.getXMin(),centralYValue,cetralXValue,centralYValue));
		yline->Draw("same");
		yline2->Draw("same");
		xlinel->Draw("same");
		xliner->Draw("same");
		xlinec->Draw("same");

		TString pointname=tmpSysName_;
		if(tmpSysName_=="") pointname="nominal";

		results_.addEntry(tmpSysName_,cetralXValue,xpright-cetralXValue,xpleft-cetralXValue);

		allsyst_.addPoint(cetralXValue,syspidx_,pointname);
		allsystsl_.addPoint(xpleft,syspidx_,pointname);
		allsystsh_.addPoint(xpright,syspidx_,pointname);
		syspidx_++;
		if(!(tmpSysName_.Contains("up")||tmpSysName_.Contains("down")))
			syspidx_++;

	}

	return centralYValue;
	/*
    else if(paraExtr_.getLikelihoodMode() == parameterExtractor::lh_fitintersect){


        if(tmpgllhd_.getNPoints()>1 ||tmpgllhd_.getNPoints()<1 )
            throw std::runtime_error("mtExtractor::drawGlobalLikelihood: in fitting mode: exactly one point per graph expected");


        plotterMultiplePlots * pl=new plotterMultiplePlots;
        pltrptrs_.push_back(pl);
        pl->readStyleFromFileInCMSSW(binschi2plotsstylefile_);
        pl->usePad(c);
        pl->setTitle("global "+tmpSysName_);
        pl->addPlot(&tmpgllhd_);
        pl->draw();
        TString pointname=tmpSysName_;
        if(tmpSysName_=="") pointname="nominal";

        allsyst_.addPoint(tmpgllhd_.getPointXContent(0),syspidx_,pointname);
        allsystsl_.addPoint(tmpgllhd_.getPointXContent(0)+tmpgllhd_.getPointXStat(0),syspidx_,pointname);
        allsystsh_.addPoint(tmpgllhd_.getPointXContent(0)-tmpgllhd_.getPointXStat(0),syspidx_,pointname);
        syspidx_++;
        if(!(tmpSysName_.Contains("up")||tmpSysName_.Contains("down")))
            syspidx_++;

    }
	 */
}

void mtExtractor::fitGlobalLikelihood(){
	if(paraExtr_.getLikelihoodMode() != parameterExtractor::lh_chi2
			&& paraExtr_.getLikelihoodMode() != parameterExtractor::lh_chi2Swapped){

		return;
	}

	if(!tmpglgraph_)
		throw std::logic_error("mtExtractor::fitGlobalLikelihood: need to create global likelihood before fitting it");


	if(tfitf_) {delete tfitf_; tfitf_=0;}

	tfitf_ = new TF1("f1",fitmode_,tmpgllhd_.getXMin(),tmpgllhd_.getXMax());
	tmpglgraph_->Fit("f1","VR");




}

void mtExtractor::drawResultGraph(TCanvas *c, float * nomp, float * errdp, float * errup,float * syserrdp, float * syserrup){


	if(debug) std::cout << " mtExtractor::drawResultGraph " <<std::endl;
	c->SetName("result graph");
	c->SetTitle("result graph");

	if(tmpgllhd_.getNPoints()<1)return; //DEBUG

	//add sum point
	size_t nompoint=0;
	for(size_t i=0;i<allsyst_.getNPoints();i++){
		if(allsyst_.getPointName(i) == "nominal"){
			nompoint=i;
			break;
		}
	}



	allsyst_.removeXErrors();
	allsyst_.addErrorGraph("stat_down",allsystsl_);
	allsyst_.addErrorGraph("stat_up",allsystsh_);

	//make all syst graph: DOESNT include SYST STAT!!!
	graph allsystcombined=graph(1);
	float fakeycontent=allsyst_.getNPoints()/2 +1.5;
	float fakeystat=allsyst_.getNPoints()/2 +1.5;
	allsystcombined.setPointXContent(0,allsyst_.getPointXContent(nompoint));
	allsystcombined.setPointXStat(0,allsyst_.getPointXError(nompoint,false)); //this will symmetrize the stat error of nompoint
	allsystcombined.setPointYContent(0,fakeycontent);
	allsystcombined.setPointYStat(0,fakeystat);


	for(size_t i=0;i<allsyst_.getNPoints();i++){
		if(i!=nompoint){
			graph tmperr(1);
			tmperr.setPointYStat(0,fakeystat);
			tmperr.setPointYContent(0,fakeycontent);
			tmperr.setPointXContent(0,allsyst_.getPointXContent(i));
			tmperr.setPointXStat(0,allsyst_.getPointXError(i,false));
			allsystcombined.addErrorGraph(allsyst_.getPointName(i),tmperr);
		}
		else{
			float nom,erru,errd;
			nom=allsyst_.getPointXContent(i);
			erru=allsyst_.getPointXContent(i,1)-nom;
			errd=allsyst_.getPointXContent(i,0)-nom;
			if(erru<errd){
				errd=erru;
				erru=allsyst_.getPointXContent(i,0)-nom;
			}
			erru=fabs(erru);
			errd=fabs(errd);
			if(nomp)
				*nomp=nom;
			if(errup)
				*errup=erru;
			if(errdp)
				*errdp=errd;

		}
		std::cout << allsyst_.getPointXContent(i) << " for " << allsyst_.getPointName(i) <<std::endl;
		/* size_t newsys= */
	}

	if(syserrdp){
		*syserrdp=allsystcombined.getPointXErrorDown(0,false);
	}

	if(syserrup){
		*syserrup=allsystcombined.getPointXErrorUp(0,false);
	}

	setAxisXsecVsMt(allsystcombined);

	std::cout << "NOW" <<std::endl;

	plotterMultiplePlots * plotter=new plotterMultiplePlots();
	pltrptrs_.push_back(plotter);
	plotter->readStyleFromFileInCMSSW(allsyststylefile_);
	plotter->readTextBoxesInCMSSW(textboxesfile_,textboxesmarker_);
	plotter->setDrawLegend(false);
	plotter->addPlot(&allsystcombined);
	plotter->addPlot(&allsyst_);
	textBoxes tbxs=allsyst_.getTextBoxesFromPoints();
	plotter->setTextBoxes(tbxs);
	// plotterBase::debug=true;
	//  plotter->addTextBox(allsyst_.getPointXContent(nompoint),fakeycontent+fakeystat,"#chi^[2}_{min}");
	plotter->usePad(c);
	plotter->setTextBoxNDC(false);

	plotter->draw();



	float nominalmass=allsyst_.getPointXContent(nompoint);
	TLine * xlinel=addObject(new TLine(nominalmass,fakeycontent-fakeystat,nominalmass,fakeycontent+fakeystat));

	xlinel->Draw("same");




}


TString mtExtractor::printConfig()const{


	TString out;
	out+="plotname: " + plotnamedata_ + " / " + plotnamemc_+ " (data/MC)\n";
	out+="plottype: " + plottypemc_ +"\n";
	out+="minbin: "+ toTString(minbin_) +"\n";
	out+="maxbin: "+ toTString(maxbin_) +"\n";
	out+="excludebin: "+ toTString(excludebin_) +"\n";
	out+="def mtop: " + toTString(defmtop_)+"\n\n";
	out+= "style files: \n" +  (TString)"compplotsstylefile: " + (TString)compplotsstylefilemc_ +(TString)"\n";
	out+= "binsplotsstylefile: " + (TString)binsplotsstylefile_ +"\n";
	out+= "binschi2plotsstylefile: " +  (TString)binschi2plotsstylefile_ +"\n";
	out+= "binsplusfitsstylefile: " + (TString)binsplusfitsstylefile_ +"\n";
	out+= "allsyststylefile: " + (TString)allsyststylefile_ +"\n\n";
	out+= "fitmode: " +fitmode_;
	out+="\nused folding: "+toTString(dofolding_);
	out+="\nexternal gen: "+toTString(isexternalgen_);
	out+="\nrescalepreds: "+toTString(rescalepreds_);

	return out;
}

texTabler  mtExtractor::makeSystBreakdown(bool rel)const{

	if(results_.nEntries()<1){
		throw std::runtime_error("mtExtractor::makeSystBreakdown: first create syst vars");
	}

	resultsSummary cp=results_;
	cp.setSystBreakRelative(rel);
	cp=cp.createTotalError();

	formatter fmt;
	std::string sysnamesfile=(std::string)getenv("CMSSW_BASE") + (std::string)"/src/TtZAnalysis/Analysis/configs/general/SystNames.txt";
	fmt.readInNameTranslateFile(sysnamesfile);
	formatter::debug=true;


	texTabler out("l | c");
	if(rel)
		out << "Source" << "$\\Delta m_{t}$  [\\%]";
	else
		out << "Source" << "$\\Delta m_{t}$ [GeV]";
	out << texLine(1);

	for(size_t i=0;i<cp.nEntries();i++){
		TString entryname=cp.getEntryName(i);
		bool isneg=false;
		TString valuestr=cp.getValueString(i,isneg,0.01);
		if(isneg) continue;
		entryname=fmt.translateName(entryname);
		//  entryname.ReplaceAll("_"," ");
		if(entryname=="total")
			out << texLine(1);
		out << entryname << valuestr;
	}

	// out << texLine(1);
	return out;
}

//can be done for each syst.

void mtExtractor::drawSpreadWithInlay(TCanvas *c){

	if(!setup_) return;


	plotterInlay *pl = new plotterInlay();
	pltrptrs_.push_back(pl);

	pl->readStyleFromFileInCMSSW(spreadwithinlaystylefile_);
	pl->readTextBoxesInCMSSW(textboxesfile_,textboxesmarker_);
	pl->usePad(c);

	graph nomdata=datagraphs_.at(defmtidx_);
	nomdata.setName("data");

	pl->addPlot(&nomdata);

	//get max spread indices
	size_t maxidx=std::max_element(mtvals_.begin(),mtvals_.end())-mtvals_.begin();
	size_t minidx=std::min_element(mtvals_.begin(),mtvals_.end())-mtvals_.begin();

	formatter fmt;

	graph mtmaxup=mcgraphs_.at(maxidx);
	graph defmtmc=mcgraphs_.at(defmtidx_);
	graph mtmaxdown=mcgraphs_.at(minidx);
	if(isexternalgen_){
		mtmaxup.setName("MCFM (m_{t}="+fmt.toTString(mtvals_.at(maxidx))+" GeV) + BG");
		defmtmc.setName("MCFM (m_{t}="+fmt.toTString(mtvals_.at(defmtidx_))+" GeV) + BG");
		mtmaxdown.setName("MCFM (m_{t}="+fmt.toTString(mtvals_.at(minidx))+" GeV) + BG");
	}
	else{
		mtmaxup.setName("MG+PY (m_{t}="+fmt.toTString(mtvals_.at(maxidx))+" GeV) + BG");
		defmtmc.setName("MG+PY (m_{t}="+fmt.toTString(mtvals_.at(defmtidx_))+" GeV) + BG");
		mtmaxdown.setName("MG+PY (m_{t}="+fmt.toTString(mtvals_.at(minidx))+" GeV) + BG");
	}

	pl->addPlot(&mtmaxup);
	pl->addPlot(&defmtmc);
	pl->addPlot(&mtmaxdown);

	pl->addInlayPlot(&globalnominal_);

	pl->draw();


}


void mtExtractor::setAxisLikelihoodVsMt(graph & g)const{

	g.setXAxisName("m_{t} [GeV]");
	g.setYAxisName("-2log(L(m_{t}))");

}
void mtExtractor::setAxisXsecVsMt(graph & g)const{

	g.setXAxisName("m_{t} [GeV]");
	if(dofolding_)
		g.setYAxisName("N/GeV");
	else
		g.setYAxisName("d#sigma_{t#bar{T}}/dm_{lb} [pb/GeV]");
}


}
