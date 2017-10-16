typedef unsigned int ems_u32;

#define SWAP_32(n) (((n & 0xff000000) >> 24) | \
    ((n & 0x00ff0000) >>  8) | \
    ((n & 0x0000ff00) <<  8) | \
    ((n & 0x000000ff) << 24))

struct event_buffer {
    ems_u32 size;
    ems_u32 evno;
    ems_u32 trigno;
    ems_u32 subevents;
    size_t max_size;
    ems_u32* data;
    size_t max_subevents;
    ems_u32** subeventptr;
};

int proceed_event(struct event_buffer* eb, TH1F* h1f,
        int sel_sev, int sel_chan);
int proceed_event2diff(struct event_buffer* eb, TH1F* h1f[3],
        int sel_sev, int* sel_chan);
