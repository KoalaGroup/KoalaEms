int compare_trees(const char* file1, const char* file2)
{
  const char treenames[][16]={"ADC1","ADC2","ADC3","ADC4","ADC5","ADC6","QDC","TDC1","TDC2"};

  TFile *f1=new TFile(file1);
  TFile *f2=new TFile(file2);

  TCanvas* can=new TCanvas("can","can");
  can->SetBatch(kTRUE);
  can->SetGridx();
  can->SetGridy();
  can->Print(Form("compare_trees.pdf["));
  for(int i=0;i<9;i++){
    printf("Tree: %s\n",treenames[i]);
    TTree *t1=(TTree*)f1->Get(treenames[i]);
    TTree *t2=(TTree*)f2->Get(treenames[i]);
    t1->AddFriend(t2,"t2");
    t1->Draw("Data:t2.Data","","text");
    can->Print(Form("compare_trees.pdf"));

    delete t1;
    delete t2;
  }
  can->Print(Form("compare_trees.pdf]"));
  delete can;

  return 0;
}
