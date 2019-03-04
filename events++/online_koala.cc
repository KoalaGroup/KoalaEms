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
	TCanvas *cFwdAmp;
	cFwdAmp = new TCanvas("cFwdAmp","Fwd Scintillator Amplitudes", 50,10, 1680, 820);
  cFwdAmp->Divide(4,2);
	cFwdAmp->Draw();
	cFwdAmp->Modified();
	cFwdAmp->Update();

	TCanvas *cFwdTime;
	cFwdTime = new TCanvas("cFwdTime","Fwd Scintillator Timestamp", 50,900, 1680, 820);
  cFwdTime->Divide(4,2);
	cFwdTime->Draw();
	cFwdTime->Modified();
	cFwdTime->Update();

	TMapFile *mfile = 0;
	mfile = TMapFile::Create("/var/tmp/koala_online.map");
	mfile->Print();
	mfile->ls();

  // TTree* tree_si1;
  // WARNING: should be initialized to nullptr
  TH1F     *hFwdAmp[8]={nullptr};
  TH1F     *hFwdTime[8]={nullptr};
	//loop displaying the histograms, Once the producer stops this script will break out of the loop

	Double_t oldentries = 0;
	while(1) 
	{
    for(int i=0;i<8;i++){
      hFwdAmp[i] = (TH1F*) mfile->Get(Form("hFwdAmp_%d",i+1), hFwdAmp[i]);
      hFwdTime[i] = (TH1F*) mfile->Get(Form("hFwdTime_%d",i+1), hFwdTime[i]);
    }

    // // tree_si1 = (TTree*) mfile->Get("Si1",tree_si1);

		cout<<" --- - ---"<<endl;
		if(hFwdTime[0]->GetEntries() == oldentries) 
		{
			gSystem ->Sleep(500);
			cout<<"--- + ----"<<endl;
		}
		oldentries = hFwdTime[0]->GetEntries();
		cout<<"Entries: "<<oldentries<<endl;

    // cFwdAmp->Divide(4,2);
    for(int i=0;i<4;i++){
      cFwdAmp->cd(i+1); hFwdAmp[2*i]->Draw();
      cFwdAmp->cd(i+5); hFwdAmp[2*i+1]->Draw();
    }
		cFwdAmp->Modified();
		cFwdAmp->Update();

    for(int i=0;i<4;i++){
      cFwdTime->cd(i+1); hFwdTime[2*i]->Draw();
      cFwdTime->cd(i+5); hFwdTime[2*i+1]->Draw();
    }
		cFwdTime->Modified();
		cFwdTime->Update();

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




















