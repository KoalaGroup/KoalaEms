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

using namespace std;

/******************************************************************************************************/
int histplot()
{
	gStyle->SetFrameFillColor(10);
	gStyle->SetFrameFillStyle(0);
	gStyle->SetPadBorderMode(1);
	gStyle->SetCanvasBorderMode(1);
	gStyle->SetCanvasColor(10);
	gStyle->SetOptStat();

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
	cRecTime = new TCanvas("cRecTime","Summed Recoil Timestamp (ns)", 2000,860, 800, 800);
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

  // Recoil RearSide Amplitudes
	TCanvas *cRecRearAmp;
	cRecRearAmp = new TCanvas("cRecRearAmp","Recoil RearSide Amplitudes", 2000,1500, 800, 800);
  cRecRearAmp->Divide(2,2);
	cRecRearAmp->Draw();
	cRecRearAmp->Modified();
	cRecRearAmp->Update();
  
  ///////////////////////////////////////////
	TMapFile *mfile = 0;
	mfile = TMapFile::Create("/var/tmp/koala_online.map");
	mfile->Print();
	mfile->ls();

  // WARNING: should be initialized to nullptr
  TH1F     *hFwdAmp[8]={nullptr};
  TH1F     *hFwdTime[8]={nullptr};

  TH1F     *hTrigTime[2]={nullptr};

  // TH1F     *hSi1Time[32]={nullptr};
  // TH1F     *hSi2Time[24]={nullptr};
  TH1F     *hRecTime=nullptr;

  TH2F     *hSi1Hits=nullptr;
  TH2F     *hSi2Hits=nullptr;
  TH2F     *hGe1Hits=nullptr;
  TH2F     *hGe2Hits=nullptr;

  TH1F     *hSi1RearAmp=nullptr;
  TH1F     *hSi2RearAmp=nullptr;
  TH1F     *hGe1RearAmp=nullptr;
  TH1F     *hGe2RearAmp=nullptr;

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

    // Recoil Hits
    hSi1Hits = (TH2F*) mfile->Get("hSi1Hits", hSi1Hits);
    hSi2Hits = (TH2F*) mfile->Get("hSi2Hits", hSi2Hits);
    hGe1Hits = (TH2F*) mfile->Get("hGe1Hits", hGe1Hits);
    hGe2Hits = (TH2F*) mfile->Get("hGe2Hits", hGe2Hits);

    // Recoil RearSide Amplitude
    hSi1RearAmp = (TH1F*) mfile->Get("hSi1RearAmp", hSi1RearAmp);
    hSi2RearAmp = (TH1F*) mfile->Get("hSi2RearAmp", hSi2RearAmp);
    hGe1RearAmp = (TH1F*) mfile->Get("hGe1RearAmp", hGe1RearAmp);
    hGe2RearAmp = (TH1F*) mfile->Get("hGe2RearAmp", hGe2RearAmp);

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

    cRecTime->cd();
    gPad->SetLogy();
    hRecTime->Draw();
    hTrigTime[0]->Draw("same");
		cRecTime->Modified();
		cRecTime->Update();

    // Recoil Hits
    cRecHits->cd(1); hGe1Hits->Draw("colz");
    cRecHits->cd(2); hGe2Hits->Draw("colz");
    cRecHits->cd(3); hSi1Hits->Draw("colz");
    cRecHits->cd(4); hSi2Hits->Draw("colz");
		cRecHits->Modified();
		cRecHits->Update();

    // Recoil RearSide Amplitude
    cRecRearAmp->cd(1); gPad->SetLogy(); hGe1RearAmp->Draw();
    cRecRearAmp->cd(2); gPad->SetLogy(); hGe2RearAmp->Draw();
    cRecRearAmp->cd(3); gPad->SetLogy(); hSi1RearAmp->Draw();
    cRecRearAmp->cd(4); gPad->SetLogy(); hSi2RearAmp->Draw();
		cRecRearAmp->Modified();
		cRecRearAmp->Update();

    //
		gSystem->Sleep(50);
		if(gSystem->ProcessEvents()) break; //Giving the interrup flag

	}

	return 0;

}

/*****************************************************************************************************************************/
int main()
{
	TApplication app("app",0, 0);
	histplot();
	app.Run();
	return 0;
}




















