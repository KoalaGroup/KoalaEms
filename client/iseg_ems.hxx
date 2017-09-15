/*
 * iseg_ems.hxx
 * 
 * created: 2006-Nov-15 PW
 */

#ifndef _iseg_ems_hxx_
#define _iseg_ems_hxx_

struct iseg_ems_module {
    void* ved_handle;
    int bus;
    int addr;
    int mclass;
    int error_mode;
    int serial;
    int firmware;
    int channels;
    unsigned int equipped_channels; // bit mask
    unsigned int status_mask; // valid bits in channel status
    bool anal_3byte; // 3 or 2 bytes used for analog values
    bool channels_identical; // all channels have identical limits
    float u_max_mod; // V
    float i_max_mod; // A
    float u_max[16]; // V
    float i_max[16]; // A
};

struct iseg_ems_crate {
    void* ved_handle;
    int bus;
    int addr;
    int sub_id;
    int emcy_id;
    int serial;
    int firmware;
};

int   iseg_ems_init_communication(const char* hostname, int port);
int   iseg_ems_init_communication(const char* sockname);
void  iseg_ems_done_communication(void);
void* iseg_ems_open_is(const char* vedname);
void  iseg_ems_close_is(void* ved_handle);
int   iseg_ems_enumerate(void* ved_handle, int bus, struct iseg_ems_module**);
void  iseg_ems_dump(std::ostream&, struct iseg_ems_module*);
int   iseg_ems_setup_module(struct iseg_ems_module*);
void  iseg_ems_debug(struct iseg_ems_module* module, int on);
void  iseg_ems_debug(void* ved_handle, int bus, int on);

int   iseg_ems_nmt(void* ved_handle, int bus, int data_id);

int   iseg_ems_voltage(struct iseg_ems_module*, int channel, float voltage);
int   iseg_ems_voltage(struct iseg_ems_module*, int channel, float *voltage);
int   iseg_ems_voltage_set(struct iseg_ems_module*, int channel, float *voltage);

int   iseg_ems_current(struct iseg_ems_module*, int channel, float current);
int   iseg_ems_current(struct iseg_ems_module*, int channel, float *current);
int   iseg_ems_current_set(struct iseg_ems_module*, int channel, float *current);

int   iseg_ems_channel_on(struct iseg_ems_module*, int channel);
int   iseg_ems_channel_off(struct iseg_ems_module*, int channel);
int   iseg_ems_channel_mask(struct iseg_ems_module*, unsigned int mask);
int   iseg_ems_channel_mask(struct iseg_ems_module*, unsigned int *mask);

int   iseg_ems_ramp(struct iseg_ems_module*, float speed);
int   iseg_ems_ramp(struct iseg_ems_module*, float *speed);

int   iseg_ems_status_module(struct iseg_ems_module*, unsigned int *flags);
int   iseg_ems_status_module(struct iseg_ems_module*, unsigned int flags);
int   iseg_ems_status_channel(struct iseg_ems_module*, int channel,
            unsigned int *flags);

int   iseg_ems_status_group(struct iseg_ems_module*, int group, unsigned int *flags);
int   iseg_ems_status_group(struct iseg_ems_module*, int group, unsigned int flags);

int   iseg_ems_current_limit(struct iseg_ems_module* module, float *fraction);
int   iseg_ems_voltage_limit(struct iseg_ems_module* module, float *fraction);

int   iseg_ems_supply_voltages(struct iseg_ems_module*,
            float *Vp24, float *Vp15, float *Vp5, float *Vn15, float *Vn5,
            float *T);

//int   iseg_ems_ramp(struct iseg_ems_module*, int speed);
//int   iseg_ems_ramp(struct iseg_ems_module*, int *speed);

int   iseg_ems_enumerate_crates(void* ved_handle, int bus, struct iseg_ems_crate**);
int   iseg_ems_setup_crate(struct iseg_ems_crate*);
int   iseg_ems_crate_nmt(void* ved_handle, int bus, int data);
int   iseg_ems_crate_on(struct iseg_ems_crate*, bool on);
int   iseg_ems_crate_on(struct iseg_ems_crate*, bool *on, int timeout=100);
int   iseg_ems_crate_voltage(struct iseg_ems_crate*, int id, float *val);
int   iseg_ems_crate_fans(struct iseg_ems_crate*, int id, int *fans, float *val);
int   iseg_ems_crate_power(struct iseg_ems_crate*, int *ac, int *dc);
int   iseg_ems_crate_serial(struct iseg_ems_crate*, int *serial, int *release);

#endif
