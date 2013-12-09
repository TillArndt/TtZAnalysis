/*
 * graph.h
 *
 *  Created on: Nov 14, 2013
 *      Author: kiesej
 */

#ifndef GRAPH_H_
#define GRAPH_H_

#include "histoContent.h"

#include "TString.h"
#include <vector>

class TGraphAsymmErrors;

namespace ztop{
class container1D;

/**
 * Class for handling x-y points with n errors (each including optional statistics)
 * Be careful! X points contain Y variations and Y points contain X variations!
 *
 */
class graph{
	friend class container1D;
public:
	graph(const TString &name="");
	graph(const size_t & npoints,const TString &name="");
	~graph();

	void setName(const TString& name){name_=name;}
	const TString& getName()const {return name_;}

	void setXAxisName(const TString& name){xname_=name;}
	const TString& getXAxisName()const {return xname_;}

	void setYAxisName(const TString& name){yname_=name;}
	const TString& getYAxisName()const {return yname_;}

	void setNPoints(const size_t & npoints); //for high perf deletes all content
	size_t getNPoints()const{return xcoords_.size();}

	size_t addPoint(const float& x, const float & y);//returns point index

	void setPointContents(const size_t & point, bool nomstat, const float & xcont, const float & ycont, const int & syslayer=-1);
	void setPointXContent(const size_t & point, const float & xcont, const int & syslayer=-1);
	void setPointYContent(const size_t & point, const float & xcont, const int & syslayer=-1);
	void setPointXStat(const size_t & point, const float & xcont, const int & syslayer=-1);
	void setPointYStat(const size_t & point, const float & xcont, const int & syslayer=-1);

	const float &  getPointXContent(const size_t & point, const int & syslayer=-1) const;
	const float &  getPointYContent(const size_t & point, const int & syslayer=-1) const;
	 float   getPointXStat(const size_t & point, const int & syslayer=-1) const;
	 float   getPointYStat(const size_t & point, const int & syslayer=-1) const;



	float getPointError(const size_t & point, bool yvar, bool updown, bool onlystat,const TString &limittosys="")const;
	//wrappers
	float getPointXErrorUp(const size_t & point, bool onlystat,const TString &limittosys="")const;
	float getPointXErrorDown(const size_t & point, bool onlystat,const TString &limittosys="")const;
	float getPointYErrorUp(const size_t & point, bool onlystat,const TString &limittosys="")const;
	float getPointYErrorDown(const size_t & point, bool onlystat,const TString &limittosys="")const;

	float getPointXError(const size_t & point, bool onlystat,const TString &limittosys="")const;
	float getPointYError(const size_t & point, bool onlystat,const TString &limittosys="")const;


	 float  getXMax() const;
	 float  getXMin() const;
	 float  getYMax() const;
	 float  getYMin() const;


	size_t addErrorGraph(const TString &name,const graph &); //needs to have same point indices... if not check by closest point?
	graph getSystGraph(const size_t sysidx)const;
	graph getNominalGraph()const;


	const TString & getSystErrorName(const size_t & number) const;
	const size_t & getSystErrorIndex(const TString & ) const;
	size_t getSystSize() const{return ycoords_.layerSize();}

	void removeYErrors(); //comes handy

	void sortPointsByX();

	/*
	 * operators... define carefully based on histocontent. Not too much overlap with container1D
	 */

	graph addY(const graph &) const;//if any x errors differ, let it fail
	void clear();

	/*
	 * import/export + root interface
	 */

	void import(const container1D*,bool dividebybinwidth=true); //uses bin centers, adds x errors binwidth_up/down

	TGraphAsymmErrors * getTGraph(TString name="",bool onlystat=false) const;


	/*
	 *
	 * high level functions for fitting
	 */
	enum chi2definitions{symmetrize,swap,parameter};
	chi2definitions chi2definition;

	graph getChi2Points(const graph&)const;


	static std::vector<ztop::graph *> g_list;
	static bool g_makelist;
	static bool debug;

private:

	TString name_;
	/**
	 * a bit weird.. but X points/coordinates have a y error and y points have an x error (due to reusing of histocontent)
	 */
	histoContent xcoords_,ycoords_;
	float labelmultiplier_;
	TString yname_,xname_;

	TString stripVariation(const TString&) const;
	float getDominantVariation( TString ,const size_t &,bool,bool ) const;
	void addToList();

	/*
	 * for all these functions be careful: due to use of histobins, x errors are represented by the y points and vice versa
	 * makes interface to container1D easier but is not intuitive
	 */
	histoBin & getPointX(const size_t & pointidx,const int & layer=-1); //histoc getbin
	const histoBin & getPointX(const size_t & pointidx,const int & layer=-1) const; //histoc getbin

	histoBin & getPointY(const size_t & pointidx,const int & layer=-1); //histoc getbin
	const histoBin & getPointY(const size_t & pointidx,const int & layer=-1) const; //histoc getbin


};
}//namespace
#endif /* GRAPH_H_ */