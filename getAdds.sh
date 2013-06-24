#!/bin/sh
cd ..
eval `scramv1 runtime -sh`

cd ${CMSSW_BASE}/src

# the things I use from the TopAnalysis code
#

if [[ "${CMSSW_VERSION}" != "CMSSW_5_3_3_patch3" && "${CMSSW_VERSION}" != "CMSSW_5_3_5" && "${CMSSW_VERSION}" != "CMSSW_5_3_7" && "${CMSSW_VERSION}" != "CMSSW_5_3_11" ]];
then
    echo 'warning! '"$CMSSW_VERSION"' not supported for automatic checkout of pat packages! add them manually!'
fi

if [[ $1 == "SUSY" ]];
then
    echo 'checking out additional SUSY packagesf for efficiency studies.'
    cvs co -d WWAnalysis/SkimStep UserCode/Mangano/WWAnalysis/SkimStep 
fi

#generator filter

cvs co -d TopAnalysis/TopFilter/plugins/ UserCode/Bromo/TopAnalysis/TopFilter/plugins/GeneratorZFilter.cc
cvs co -d TopAnalysis/TopFilter/plugins/ UserCode/Bromo/TopAnalysis/TopFilter/plugins/GeneratorTopFilter.cc
cvs co -d TopAnalysis/TopFilter/plugins/ UserCode/Bromo/TopAnalysis/TopFilter/plugins/BuildFile.xml
cvs co -d TopAnalysis/TopFilter/ UserCode/Bromo/TopAnalysis/TopFilter/BuildFile.xml
cvs co -d TopAnalysis/TopFilter/python/filters/ UserCode/Bromo/TopAnalysis/TopFilter/python/filters/GeneratorTopFilter_cfi.py
cvs co -d TopAnalysis/TopFilter/python/filters/ UserCode/Bromo/TopAnalysis/TopFilter/python/filters/GeneratorZFilter_cfi.py

#scripts

cvs co -d TopAnalysis/TopUtils/scripts UserCode/Bromo/TopAnalysis/TopUtils/scripts

#utilz moved to topAnalysis

cvs co -d TopAnalysis/ZTopUtils UserCode/Bromo/TopAnalysis/ZTopUtils

#b-jet stuff

cvs co -d TopAnalysis/TopUtils UserCode/Bromo/TopAnalysis/TopUtils

rm -r -f $CMSSW_BASE/src/TopAnalysis/TopUtils/data

#runallStuff

cvs co -d TopAnalysis/Configuration/analysis/diLeptonic/scripts  UserCode/Bromo/TopAnalysis/Configuration/analysis/diLeptonic/scripts/runallGC.pl
cvs co -d TopAnalysis/Configuration/analysis/diLeptonic/scripts  UserCode/Bromo/TopAnalysis/Configuration/analysis/diLeptonic/scripts/runallCrab.pl




cd ${CMSSW_BASE}/src/TtZAnalysis/Configuration/python/analysis
ln -s ${CMSSW_BASE}/src/TopAnalysis/Configuration/analysis/diLeptonic/scripts/runallGC.pl runallGC.pl
ln -s ${CMSSW_BASE}/src/TopAnalysis/Configuration/analysis/diLeptonic/scripts/runallCrab.pl runallCrab.pl

ln -s  ${CMSSW_BASE}/src/TtZAnalysis/Configuration/python/analysis/ReRecoNov2011.json ${CMSSW_BASE}/src/TtZAnalysis/Data/ReRecoNov2011.json
ln -s  ${CMSSW_BASE}/src/TtZAnalysis/Configuration/python/analysis/HCP.json ${CMSSW_BASE}/src/TtZAnalysis/Data/HCP.json

cd $CMSSW_BASE/src/TtZAnalysis/Data
ln -s /afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions12/8TeV/PileUp PileUpJsons
cd $CMSSW_BASE/src

# get sample files

#echo "adding samplefiles (update from DAS)"

#cd TtZAnalysis/Configuration/python/samples
#./update_samplefiles2012.sh
#cd mc
#./updatemcfiles12.sh
cd $CMSSW_BASE/src

# the private JEC uncertainties code (standalone) should be obsolete now

#echo "modified JEC Uncertainty utility.."

#cp -r /afs/desy.de/user/k/kiesej/public/code/CondFormats .


echo "Electron MVA stuff"

#electron mva id stuff (following top reference twiki)
cvs co -r V00-00-13 -d EGamma/EGammaAnalysisTools UserCode/EGamma/EGammaAnalysisTools
cd EGamma/EGammaAnalysisTools/data
cat download.url | xargs wget
cd $CMSSW_BASE/src

echo "MET  Filter"

addpkg RecoMET/METFilters V00-00-10
cvs co -r V00-00-08 RecoMET/METAnalyzers
cvs co -r V00-03-23 CommonTools/RecoAlgos
# Additional packages for the tracking POG filters
cvs co -r V01-00-11-01 DPGAnalysis/Skims
cvs co -r V00-11-17 DPGAnalysis/SiStripTools
cvs co -r V00-00-08 DataFormats/TrackerCommon
cvs co -r V01-09-05 RecoLocalTracker/SubCollectionProducers

if [[ "$CMSSW_VERSION" == "CMSSW_5_3_3_patch3"  ]]
then

echo "checking out release V08-09-42 for ${CMSSW_VERSION} "

addpkg DataFormats/PatCandidates       V06-05-06-02
addpkg PhysicsTools/PatAlgos           V08-09-42
addpkg PhysicsTools/PatUtils           V03-09-26
addpkg CommonTools/RecoUtils           V00-00-12
addpkg CommonTools/RecoAlgos           V00-03-24
addpkg CommonTools/ParticleFlow        V00-03-16
addpkg RecoParticleFlow/PFProducer   V15-02-05-01
addpkg DataFormats/ParticleFlowCandidate   V15-03-04      
addpkg DataFormats/TrackReco   V10-02-02      
addpkg DataFormats/VertexReco   V02-00-04 

echo 'checked out release V08-09-42 - check if newest one'

fi

if [[ "$CMSSW_VERSION" == "CMSSW_5_3_5" ]]
then

echo "checking out pat release V08-09-43 for ${CMSSW_VERSION} "

addpkg DataFormats/PatCandidates V06-05-06-03
addpkg PhysicsTools/PatAlgos     V08-09-43

fi

if [[ "$CMSSW_VERSION" == "CMSSW_5_3_7" ]]
then

addpkg DataFormats/PatCandidates V06-05-06-03
addpkg PhysicsTools/PatAlgos     V08-09-48
addpkg DataFormats/StdDictionaries V00-02-14
addpkg FWCore/GuiBrowsers V00-00-70


fi
if [[ "$CMSSW_VERSION" == "CMSSW_5_3_11" ]]
then

addpkg DataFormats/PatCandidates V06-05-06-12
addpkg PhysicsTools/PatAlgos     V08-09-61
addpkg PhysicsTools/PatUtils
addpkg RecoBTag/ImpactParameter V01-04-09-01
addpkg RecoBTag/SecondaryVertex V01-10-06
addpkg RecoBTag/SoftLepton      V05-09-11
addpkg RecoBTau/JetTagComputer  V02-03-02
addpkg RecoBTag/Configuration   V00-07-05
addpkg RecoParticleFlow/PFProducer V15-02-06

addpkg TopQuarkAnalysis/Configuration
addpkg TopQuarkAnalysis/TopSkimming V07-01-04
addpkg TopQuarkAnalysis/TopTools    V06-07-13

fi

#for analysis stuff:

addpkg FWCore/FWLite/src/FWCoreFWLite
addpkg ElectroWeakAnalysis/Utilities

echo "setting LHAPDF path to /afs/naf.desy.de/user/k/kieseler/public/lhapdf"

mv $CMSSW_BASE/config/toolbox/$SCRAM_ARCH/tools/selected/lhapdf.xml $CMSSW_BASE/config/toolbox/$SCRAM_ARCH/tools/selected/lhapdf.xml.old
sed  -e 's;environment name=\"LHAPDF_BASE\" default=\"/afs/naf.desy.de/group/cms/sw/slc5_amd64_gcc462/external/lhapdf/5.8.5-cms2\";environment name=\"LHAPDF_BASE\" default=\"/afs/naf.desy.de/user/k/kieseler/public/lhapdf\"";g' < $CMSSW_BASE/config/toolbox/$SCRAM_ARCH/tools/selected/lhapdf.xml.old  > $CMSSW_BASE/config/toolbox/$SCRAM_ARCH/tools/selected/lhapdf.xml


echo "Building eclipse project..."
/afs/naf.desy.de/user/k/kieseler/public/eclipsetemp/prepareEclipseProject.sh

#some copy-paste in case you want to use my root macros and the classes on command line
echo 'you will need this in the rootlogon.C:'
echo '
     gSystem->Load("libFWCoreFWLite.so"); 
     AutoLibraryLoader::enable();
     gSystem->Load("libDataFormatsFWLite.so");
     gSystem->Load("libDataFormatsPatCandidates.so");
     gSystem->Load("libTtZAnalysisDataFormats.so");
     gSystem->Load("libRooFit");
     gSystem->Load("libFWCoreUtilities.so");
     gSystem->Load("libCondFormatsJetMETObjects.so");
     cout << "libs loaded" <<endl;
     '



echo "\n\ncompile if you like"
