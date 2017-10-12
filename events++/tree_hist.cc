class hist_node {
  public:
    hist_node(int v): more(0), less(0), val(v)
    {
        for (int i=0; i<10; i++)
            who[i]=0;
    }
    void dump(void);
    hist_node *more;
    hist_node *less;
    int val;
    int who[10];
};

void hist_node::dump()
{
    if (less)
        less->dump();
    printf("%05d ", id);
    for (i=0; i<10; i++) {
        printf("\t%5d", who[i]);
    }
    printf("\n");
    if (more)
        more->dump();
}

class tree_hist {
  public:
    hist(double quant): quantum(quant), nodecount(0)
    {
        root=new hist_node(0.);
    }
    void add(double val, int idx);
    double quantum;
    hist_node* root;
    int nodecount;
};


void tree_hist::add(double val, int idx)
{
    int id=lrint(val/quantum);

    hist_node *node=root;
    while (node->id!=id) {
        if (node->id<id) {
            if (!node->more) {
                node->more=new hist_node(id);
                nodecount++;
            }
            node=node->more;
        } else {
            if (!node->less) {
                node->less=new hist_node(id);
                nodecount++;
            }
            node=node->less;
        }
    }
    node->who[idx]++;
}

void tree_hist::dump(void)
{
    printf("tree_hist: %d nodes\n");

    root->dump();
}
