#include <TApplication.h>
#include <TROOT.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TH1.h>
#include <TH2.h>
#include <TSystem.h>
#include <TStyle.h>
#include <iostream>
#include <TMapFile.h>
#include <TTree.h>
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TString.h"
#include <ctime>
#include <string.h>

using namespace std;

static char mf_name[256]="koala_online.map";
/*******************************************/
static void
printusage(char* argv0)
{
  printf("usage: %s [-m mapfile]\n",
         basename(argv0));
  printf("  -h: this help\n");
  printf("  -s mapfile: the mapfile to shown\n");
  printf("    default is \"koala_online.map\"\n");
}
/*******************************************/
static int
readargs(int argc, char* argv[])
{
  int c, err;
  //struct servent* service;

  err=0;
  while (!err && ((c=getopt(argc, argv, "m:")) != -1)) {
    switch (c) {
    case 'h': printusage(argv[0]); return 1;
    case 'm': {
      strcpy(mf_name,optarg);
      break;
    }
    default: err=1;
    }
  }
  if (err) {
    printusage(argv[0]);
    return -1;
  }

  return 0;
}
/******************************************************************************************************/
int histplot()
{
	gStyle->SetFrameFillColor(10);
	gStyle->SetFrameFillStyle(0);
	gStyle->SetPadBorderMode(1);
	gStyle->SetCanvasBorderMode(1);
	gStyle->SetCanvasColor(10);
	gStyle->SetOptStat();
  gStyle->SetTimeOffset(0);

  time_t cur_time=time(nullptr);
  Double_t xlimit=cur_time;
	//Create 2 new canvas and 4 pads for each
  // Forward Detector
	TCanvas *cFwdAmp;
	cFwdAmp = new TCanvas("cFwdAmp","Fwd Scintillator Amplitudes", 50,10, 1680, 820);
  cFwdAmp->Divide(4,2);
	cFwdAmp->Draw();
	cFwdAmp->Modified();
	cFwdAmp->Update();

	TCanvas *cFwdTime;
	cFwdTime = new TCanvas("cFwdTime","Fwd Scintillator Timestamp", 50,1000, 1680, 820);
  cFwdTime->Divide(4,2);
	cFwdTime->Draw();
	cFwdTime->Modified();
	cFwdTime->Update();

  // Trigger Timestamp
  // TCanvas *cTrigTime;
	// cTrigTime = new TCanvas("cTrigTime","TDC Trigger Timestamp", 2000,10, 800, 400);
  // cTrigTime->Divide(2,1);
  // cTrigTime->Draw();
	// cTrigTime->Modified();
	// cTrigTime->Update();

  // Recoil Timestamp
	// TCanvas *cSi1Time;
	// cSi1Time = new TCanvas("cSi1Time","Si#1 Timestamp (ns)", 2000,860, 3200, 1600);
  // cSi1Time->Divide(8,4);
	// cSi1Time->Draw();
	// cSi1Time->Modified();
	// cSi1Time->Update();
	TCanvas *cRecTime;
	cRecTime = new TCanvas("cRecTime","Recoil Timestamp (ns)", 2000,860, 1800, 600);
  cRecTime->Divide(3,1);
	cRecTime->Draw();
	cRecTime->Modified();
	cRecTime->Update();

  // Recoil Hits
	TCanvas *cRecHits;
	cRecHits = new TCanvas("cRecHits","Recoil FrontSide Hits", 2000,1200, 1200, 1200);
  cRecHits->Divide(2,2);
	cRecHits->Draw();
	cRecHits->Modified();
	cRecHits->Update();

	TCanvas *cRecHitsCut;
	cRecHitsCut = new TCanvas("cRecHitsCut","Recoil FrontSide Hits (Fwd Amplitude Cut)", 2000,1200, 1200, 1200);
  cRecHitsCut->Divide(2,2);
	cRecHitsCut->Draw();
	cRecHitsCut->Modified();
	cRecHitsCut->Update();

  // Recoil RearSide Amplitudes
	TCanvas *cRecRearAmp;
	cRecRearAmp = new TCanvas("cRecRearAmp","Recoil RearSide Amplitudes", 2000,1500, 800, 800);
  cRecRearAmp->Divide(2,2);
	cRecRearAmp->Draw();
	cRecRearAmp->Modified();
	cRecRearAmp->Update();
  
	TCanvas *cHitRateFwd;
	cHitRateFwd = new TCanvas("cHitRateFwd","Forward Hit Rates", 2000,1500, 2000, 800);
	cHitRateFwd->Draw();
	cHitRateFwd->Modified();
	cHitRateFwd->Update();

	TCanvas *cHitRateRec;
	cHitRateRec = new TCanvas("cHitRateRec","Recoil Hit Rates", 2000,1500, 2000, 800);
	cHitRateRec->Draw();
	cHitRateRec->Modified();
	cHitRateRec->Update();

	TCanvas *cHitRateGeOverlap;
	cHitRateGeOverlap = new TCanvas("cHitRateGeOverlap","Ge1 & Ge2 Overlapping Hit Rates", 2000,1500, 2000, 800);
	cHitRateGeOverlap->Draw();
	cHitRateGeOverlap->Modified();
	cHitRateGeOverlap->Update();

	TCanvas *cHitRateSiRear;
	cHitRateSiRear = new TCanvas("cHitRateSiRear","Si1 and Si2 Rear Side Hit Rates", 2000,1500, 2000, 800);
	cHitRateSiRear->Draw();
	cHitRateSiRear->Modified();
	cHitRateSiRear->Update();

	TCanvas *cDaqEfficiency;
	cDaqEfficiency = new TCanvas("cDaqEfficiency","DAQ Efficiency", 2000,1500, 2000, 800);
  cDaqEfficiency->Divide(2,1);
	cDaqEfficiency->Draw();
	cDaqEfficiency->Modified();
	cDaqEfficiency->Update();
  

	TCanvas *cCorrelation;
	cCorrelation = new TCanvas("cCorrelation","Correlation Graphs", 20,15, 2000, 1000);
  cCorrelation->Divide(4,2);
	cCorrelation->Draw();
	cCorrelation->Modified();
	cCorrelation->Update();
  ///////////////////////////////////////////
	TMapFile *mfile = 0;
	mfile = TMapFile::Create(Form("/var/tmp/%s",mf_name));
	mfile->Print();
	mfile->ls();

  // WARNING: should be initialized to nullptr
  TH1F     *hFwdAmp[8]={nullptr};
  TH1F     *hFwdTime[8]={nullptr};

  TH1F     *hTrigTime[2]={nullptr};

  // TH1F     *hSi1Time[32]={nullptr};
  // TH1F     *hSi2Time[24]={nullptr};
  TH1F     *hRecTime=nullptr;
  TH1F     *hRecRearTime[2]={nullptr};

  TH2F     *h2RecHits[4]={nullptr};
  TH2F     *h2RecHitsCut[4]={nullptr};
  TH1F     *hRecRearAmp[4]={nullptr};
  TH1F     *hRecRearAmpCut[4]={nullptr};

  TH2F*     h2FwdTimeVSAmp[2]={nullptr};
  TH2F*     h2FwdTimeVSRecAmp[2]={nullptr};
  TH2F*     h2FwdAmpVSRecAmp[2]={nullptr};
  TH1F*     hRecRearTimeDiff[2]={nullptr};
   
  // TGraph  *gScalerRec[4]={nullptr};
  // TGraph  *gScalerFwd[4]={nullptr};
  // TGraph  *gScalerCommonOr=nullptr;
  // TGraph  *gScalerGeOverlap[2]={nullptr};
  // TGraph  *gScalerSiRear[2]={nullptr};

  TGraph  *gHitRateRec[4]={nullptr};
  TGraph  *gHitRateFwd[4]={nullptr};
  TGraph  *gHitRateCommonOr=nullptr;
  TGraph  *gHitRateGeOverlap[2]={nullptr};
  TGraph  *gHitRateSiRear[2]={nullptr};

  TGraph  *gEventNr=nullptr;
  TGraph  *gEventRate=nullptr;
  TGraph  *gDaqEfficiency=nullptr;
  
  TMultiGraph *gMHitRateFwd = new TMultiGraph();
  TMultiGraph *gMHitRateRec = new TMultiGraph();
  TLegend  *legendHitRateFwd=new TLegend(0.1,0.7,0.2,0.9);
  TLegend  *legendHitRateRec=new TLegend(0.1,0.7,0.2,0.9);

  TMultiGraph *gMHitRateGeOverlap = new TMultiGraph();
  TMultiGraph *gMHitRateSiRear = new TMultiGraph();
  TLegend  *legendHitRateGeOverlap=new TLegend(0.1,0.7,0.4,0.9);
  TLegend  *legendHitRateSiRear=new TLegend(0.1,0.7,0.4,0.9);

  TMultiGraph *gMDaqRate = new TMultiGraph();
  TLegend  *legendDaqRate=new TLegend(0.1,0.7,0.4,0.9);
	//loop displaying the histograms, Once the producer stops this script will break out of the loop
	Double_t oldentries = 0;
	while(1) 
	{
    ///////// Get histograms
    // Forward Detector
    for(int i=0;i<8;i++){
      hFwdAmp[i] = (TH1F*) mfile->Get(Form("hFwdAmp_%d",i+1), hFwdAmp[i]);
      hFwdTime[i] = (TH1F*) mfile->Get(Form("hFwdTime_%d",i+1), hFwdTime[i]);
    }
    
    // Trigger Timestamp
    for(int i=0;i<2;i++){
      hTrigTime[i] = (TH1F*) mfile->Get(Form("hTrigTime_%d",i+1), hTrigTime[i]);
      hTrigTime[i]->SetLineColor(kRed);
    }

    // Recoil Timestamp
    // for(int i=0;i<32;i++){
    //   hSi1Time[i] = (TH1F*) mfile->Get(Form("hSi1Time_%d",i+17), hSi1Time[i]);
    // }
    // for(int i=0;i<24;i++){
    //   hSi2Time[i] = (TH1F*) mfile->Get(Form("hSi2Time_%d",i+1), hSi2Time[i]);
    // }
    hRecTime = (TH1F*) mfile->Get("hRecTime", hRecTime);
    for(int i=0;i<2;i++){
      hRecRearTime[i] = (TH1F*) mfile->Get(Form("hRecRearTime_%d",i+1), hRecRearTime[i]);
    }

    // Recoil Hits
    for(int i=0;i<4;i++){
      h2RecHits[i] = (TH2F*) mfile->Get(Form("h2RecHits_%d",i+1),h2RecHits[i]);
      h2RecHitsCut[i] = (TH2F*) mfile->Get(Form("h2RecHitsCut_%d",i+1), h2RecHitsCut[i]);

      hRecRearAmp[i] = (TH1F*) mfile->Get(Form("hRecRearAmp_%d",i+1),hRecRearAmp[i] );
      hRecRearAmpCut[i] = (TH1F*) mfile->Get(Form("hRecRearAmpCut_%d",i+1),hRecRearAmpCut[i] );
    }
    //
    for(int i=0;i<2;i++){
      h2FwdTimeVSAmp[i] = (TH2F*) mfile->Get(Form("h2FwdTimeVSAmp_%d",i+1),h2FwdTimeVSAmp[i] );
      h2FwdTimeVSRecAmp[i] = (TH2F*) mfile->Get(Form("h2FwdTimeVSRecAmp_%d",i+1),h2FwdTimeVSRecAmp[i] );
      h2FwdAmpVSRecAmp[i] = (TH2F*) mfile->Get(Form("h2FwdAmpVSRecAmp_%d",i+1),h2FwdAmpVSRecAmp[i] );
      hRecRearTimeDiff[i] = (TH1F*) mfile->Get(Form("hRecRearTimeDiff_%d",i+1),hRecRearTimeDiff[i] );
    }

    // Graphs
    TString RecName[4]={"Si#1","Si#2","Ge#1","Ge#2"};
    TString FwdName[4]={"Fwd#Out","Fwd#In","Fwd#Up","Fwd#Down"};

    // clean up first
    legendHitRateFwd->Clear();
    legendHitRateFwd->SetHeader("Forward Hit Rates");
    legendHitRateRec->Clear();
    legendHitRateRec->SetHeader("Recoil Hit Rates");
    legendHitRateGeOverlap->Clear();
    legendHitRateGeOverlap->SetHeader("Ge1 & Ge2 Overlap Hit Rates");
    legendHitRateSiRear->Clear();
    legendHitRateSiRear->SetHeader("Si1 & Si2 Rear Side Hit Rates");
    legendDaqRate->Clear();
    legendDaqRate->SetHeader("DAQ Rates");

    for(int i=0;i<4;i++){
      gMHitRateFwd->RecursiveRemove(gHitRateFwd[i]);
      gMHitRateRec->RecursiveRemove(gHitRateRec[i]);
    }
    gMHitRateFwd->RecursiveRemove(gHitRateCommonOr);
    gMHitRateRec->RecursiveRemove(gHitRateCommonOr);
    gMHitRateFwd->RecursiveRemove(gEventRate);
    gMHitRateRec->RecursiveRemove(gEventRate);

    for(int i=0;i<2;i++){
      gMHitRateGeOverlap->RecursiveRemove(gHitRateGeOverlap[i]);
      gMHitRateSiRear->RecursiveRemove(gHitRateSiRear[i]);
    }
    
    gMDaqRate->RecursiveRemove(gHitRateCommonOr);
    gMDaqRate->RecursiveRemove(gEventRate);
    // get the latest results
    for(int i=0;i<4;i++){
      gHitRateRec[i]= (TGraph*) mfile->Get(Form("gHitRateRec_%d",i+1), gHitRateRec[i]);
      gHitRateFwd[i]= (TGraph*) mfile->Get(Form("gHitRateFwd_%d",i+1), gHitRateFwd[i]);

      // gScalerRec[i]= (TGraph*) mfile->Get(Form("gScalerRec_%d",i+1), gScalerRec[i]);
      // gScalerFwd[i]= (TGraph*) mfile->Get(Form("gScalerFwd_%d",i+1), gScalerFwd[i]);
    }
    gHitRateCommonOr = (TGraph*) mfile->Get("gHitRateCommonOr", gHitRateCommonOr);
    // gScalerCommonOr = (TGraph*) mfile->Get("gScalerCommonOr", gScalerCommonOr);
    for(int i=0;i<2;i++){
      gHitRateGeOverlap[i]= (TGraph*) mfile->Get(Form("gHitRateGeOverlap_%d",i+1), gHitRateGeOverlap[i]);
      // gScalerGeOverlap[i]= (TGraph*) mfile->Get(Form("gScalerGeOverlap_%d",i+1), gScalerGeOverlap[i]);

      gHitRateSiRear[i]= (TGraph*) mfile->Get(Form("gHitRateSiRear_%d",i+1), gHitRateSiRear[i]);
      // gScalerSiRear[i]= (TGraph*) mfile->Get(Form("gScalerSiRear_%d",i+1), gScalerSiRear[i]);
    }

    gEventRate = (TGraph*) mfile->Get("gEventRate", gEventRate);
    gEventNr = (TGraph*) mfile->Get("gEventNr", gEventNr);
    gDaqEfficiency = (TGraph*) mfile->Get("gDaqEfficiency", gDaqEfficiency);

    /////////////////////////////////////////
		cout<<" --- - ---"<<endl;
		if(hTrigTime[0]->GetEntries() == oldentries) 
		{
			gSystem ->Sleep(500);
			cout<<"--- + ----"<<endl;
		}
		oldentries = hTrigTime[0]->GetEntries();
		cout<<"Entries: "<<oldentries<<endl;

    /////////////////////////
    // Draw
    /////////////////////////

    // Forward Detector
    for(int i=0;i<4;i++){
      cFwdAmp->cd(i+1);
      gPad->SetLogy();
      hFwdAmp[2*i]->Draw();
      cFwdAmp->cd(i+5);
      gPad->SetLogy();
      hFwdAmp[2*i+1]->Draw();
    }
		cFwdAmp->Modified();
		cFwdAmp->Update();

    for(int i=0;i<4;i++){
      cFwdTime->cd(i+1);
      gPad->SetLogy();
      hFwdTime[2*i]->Draw();
      hTrigTime[0]->Draw("same");
      cFwdTime->cd(i+5);
      gPad->SetLogy();
      hFwdTime[2*i+1]->Draw();
      hTrigTime[0]->Draw("same");
    }
		cFwdTime->Modified();
		cFwdTime->Update();

    // Trigger Timestamp
    // for(int i=0;i<2;i++){
    //   cTrigTime->cd(i+1);
    //   gPad->SetLogy();
    //   hTrigTime[i]->Draw();
    // }
		// cTrigTime->Modified();
		// cTrigTime->Update();

    // Recoil Timestamp
    // for(int i=0;i<32;i++){
    //   cSi1Time->cd(i+1);
    //   gPad->SetLogy();
    //   hSi1Time[i]->Draw();
    // }
		// cSi1Time->Modified();
		// cSi1Time->Update();

    cRecTime->cd(3);
    gPad->SetLogy();
    hRecTime->Draw();
    hTrigTime[0]->Draw("same");
    for(int i=0;i<2;i++){
      cRecTime->cd(i+1);
      gPad->SetLogy();
      hRecRearTime[i]->Draw();
      hTrigTime[0]->Draw("same");
    }
		cRecTime->Modified();
		cRecTime->Update();

    // Recoil Hits
    int order[4]={1,3,0,2};
    for(int i=0;i<4;i++){
      cRecHits->cd(i+1); gPad->SetLogz(); h2RecHits[order[i]]->Draw("colz");
      
    }
		cRecHits->Modified();
		cRecHits->Update();

    for(int i=0;i<4;i++){
      cRecHitsCut->cd(i+1); gPad->SetLogz(); h2RecHitsCut[order[i]]->Draw("colz");
      
    }
		cRecHitsCut->Modified();
		cRecHitsCut->Update();

    // Recoil RearSide Amplitude
    for(int i=0;i<4;i++){
      cRecRearAmp->cd(i+1); gPad->SetLogy();
      hRecRearAmp[order[i]]->SetLineColor(kBlack);
      hRecRearAmp[order[i]]->Draw();
      hRecRearAmpCut[order[i]]->SetLineColor(kRed);
      hRecRearAmpCut[order[i]]->Draw("same");
    }
		cRecRearAmp->Modified();
		cRecRearAmp->Update();

    cCorrelation->cd(1); gPad->SetLogz(); h2FwdTimeVSAmp[0]->Draw("colz");
    cCorrelation->cd(5); gPad->SetLogz();h2FwdTimeVSAmp[1]->Draw("colz");

    cCorrelation->cd(2); gPad->SetLogz();h2FwdAmpVSRecAmp[0]->Draw("colz");
    cCorrelation->cd(6); gPad->SetLogz();h2FwdAmpVSRecAmp[1]->Draw("colz");

    cCorrelation->cd(3); gPad->SetLogz();h2FwdTimeVSRecAmp[0]->Draw("colz");
    cCorrelation->cd(7); gPad->SetLogz();h2FwdTimeVSRecAmp[1]->Draw("colz");

    cCorrelation->cd(4); gPad->SetLogy();hRecRearTimeDiff[0]->Draw();
    cCorrelation->cd(8); gPad->SetLogy();hRecRearTimeDiff[1]->Draw();
		cCorrelation->Modified();
		cCorrelation->Update();


    // Graph
    cHitRateFwd->cd();
    gPad->SetLogy();
    for(int i=0;i<4;i++){
      gMHitRateFwd->Add(gHitRateFwd[i],"*L");
      legendHitRateFwd->AddEntry(gHitRateFwd[i],FwdName[i].Data(),"l");
    }
    gMHitRateFwd->Add(gHitRateCommonOr,"*L");
    legendHitRateFwd->AddEntry(gHitRateCommonOr,"Trigger Rate","l");
    gMHitRateFwd->Add(gEventRate,"*L");
    legendHitRateFwd->AddEntry(gEventRate,"DAQ Event Rate","l");
    if(gHitRateFwd[0]->GetN()){
      // gMHitRateFwd->GetXaxis()->SetLimits(xlimit,xlimit+1000);
      // gMHitRateFwd->GetHistogram()->SetMaximum(8000);
      // gMHitRateFwd->GetHistogram()->SetMinimum(0);
      gMHitRateFwd->Draw("AL");
      gPad->Update();
      gMHitRateFwd->GetXaxis()->SetTimeDisplay(1);
      gMHitRateFwd->GetXaxis()->SetLabelOffset(0.03);
      gMHitRateFwd->GetXaxis()->SetNdivisions(-503);
      gMHitRateFwd->GetXaxis()->SetTimeFormat("#splitline{%H:%M:%S}{%d\/%m}");
    }
    legendHitRateFwd->Draw();
    // cHitRateFwd->BuildLegend();
		cHitRateFwd->Modified();
		cHitRateFwd->Update();
    
    cHitRateRec->cd();
    gPad->SetLogy();
    for(int i=0;i<4;i++){
      gMHitRateRec->Add(gHitRateRec[i],"*L");
      legendHitRateRec->AddEntry(gHitRateRec[i],RecName[i].Data(),"l");
    }
    gMHitRateRec->Add(gHitRateCommonOr,"*L");
    legendHitRateRec->AddEntry(gHitRateCommonOr,"Trigger Rate","l");
    gMHitRateRec->Add(gEventRate,"*L");
    legendHitRateRec->AddEntry(gEventRate,"DAQ Event Rate","l");
    if(gHitRateRec[0]->GetN()){
      gMHitRateRec->Draw("AL");
      gPad->Update();
      gMHitRateRec->GetXaxis()->SetTimeDisplay(1);
      gMHitRateRec->GetXaxis()->SetLabelOffset(0.03);
      gMHitRateRec->GetXaxis()->SetNdivisions(-503);
      gMHitRateRec->GetXaxis()->SetTimeFormat("#splitline{%H:%M:%S}{%d\/%m}");
    }
    legendHitRateRec->Draw();
		cHitRateRec->Modified();
		cHitRateRec->Update();

    cHitRateGeOverlap->cd();
    for(int i=0;i<2;i++){
      gMHitRateGeOverlap->Add(gHitRateGeOverlap[i],"*L");
      legendHitRateGeOverlap->AddEntry(gHitRateGeOverlap[i],RecName[i+2].Data(),"l");
    }
    if(gHitRateGeOverlap[0]->GetN()){
      gMHitRateGeOverlap->Draw("AL");
      gPad->Update();
      gMHitRateGeOverlap->GetXaxis()->SetTimeDisplay(1);
      gMHitRateGeOverlap->GetXaxis()->SetLabelOffset(0.03);
      gMHitRateGeOverlap->GetXaxis()->SetNdivisions(-503);
      gMHitRateGeOverlap->GetXaxis()->SetTimeFormat("#splitline{%H:%M:%S}{%d\/%m}");
    }
    legendHitRateGeOverlap->Draw();
		cHitRateGeOverlap->Modified();
		cHitRateGeOverlap->Update();

    cHitRateSiRear->cd();
    for(int i=0;i<2;i++){
      gMHitRateSiRear->Add(gHitRateSiRear[i],"*L");
      legendHitRateSiRear->AddEntry(gHitRateSiRear[i],RecName[i].Data(),"l");
    }
    if(gHitRateSiRear[0]->GetN()){
      gMHitRateSiRear->Draw("AL");
      gPad->Update();
      gMHitRateSiRear->GetXaxis()->SetTimeDisplay(1);
      gMHitRateSiRear->GetXaxis()->SetLabelOffset(0.03);
      gMHitRateSiRear->GetXaxis()->SetNdivisions(-503);
      gMHitRateSiRear->GetXaxis()->SetTimeFormat("#splitline{%H:%M:%S}{%d\/%m}");
    }
    legendHitRateSiRear->Draw();
		cHitRateSiRear->Modified();
		cHitRateSiRear->Update();

    if(gDaqEfficiency->GetN()){
      cDaqEfficiency->cd(1);
      gMDaqRate->Add(gHitRateCommonOr,"*L");
      legendDaqRate->AddEntry(gHitRateCommonOr,"Trigger Rate","l");
      gMDaqRate->Add(gEventRate,"*L");
      legendDaqRate->AddEntry(gEventRate,"DAQ Event Rate","l");
      gMDaqRate->Draw("AL");

      cDaqEfficiency->cd(2);
      gDaqEfficiency->Draw("A*L");
      gPad->Update();
      gDaqEfficiency->GetXaxis()->SetTimeDisplay(1);
      gDaqEfficiency->GetXaxis()->SetLabelOffset(0.03);
      gDaqEfficiency->GetXaxis()->SetNdivisions(-503);
      gDaqEfficiency->GetXaxis()->SetTimeFormat("#splitline{%H:%M:%S}{%d\/%m}");
    }
		cDaqEfficiency->Modified();
		cDaqEfficiency->Update();

    //
		gSystem->Sleep(50);
		if(gSystem->ProcessEvents()) break; //Giving the interrup flag
	}

	return 0;

}

/*****************************************************************************************************************************/
int main(int argc, char** argv)
{
  readargs(argc,argv);
	TApplication app("app",0, 0);
	histplot();
	app.Run();
	return 0;
}




















