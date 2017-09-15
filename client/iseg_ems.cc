/*
 * iseg_ems.cc
 * 
 * created: 2006-Nov-15 PW
 */

#include "config.h"
#include <iostream>
#include <cerrno>

#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <versions.hxx>

#include "iseg_ems.hxx"

VERSION("2006-Nov-29", __FILE__, __DATE__, __TIME__,
"$ZEL: iseg_ems.cc,v 1.5 2006/11/29 14:51:44 wuestner Exp $")
#define XVERSION

struct cache_element {
    cache_element() {timestamp.tv_sec=0; timestamp.tv_usec=0;}
    struct timeval timestamp; // rescan if stamp is older than 60 s
                              // timestamp.tv_sec==0: cache is invalid
    int mclass[64];           // -1: no module with this address
};

struct ved_handle {
    C_VED* ved;
    C_instr_system *is0;
    int can_w;
    int can_wr;
    int hv_r;
    int hv_w;
    struct cache_element *cache; // one cache for each CANbus
    int cache_max;               // number of elements in cache
};

float resol[2]={50000., 10.e6};

//---------------------------------------------------------------------------//
void
iseg_ems_done_communication(void)
{
    communication.done();
}
//---------------------------------------------------------------------------//
int
iseg_ems_init_communication(const char* hostname, int port)
{
    try {
        communication.init(hostname, port);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_init_communication(const char* sockname)
{
    try {
        communication.init(sockname);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
void
iseg_ems_close_is(void* handle)
{
    ved_handle *vhandle=static_cast<ved_handle*>(handle);

    delete vhandle->ved;
    delete[] vhandle->cache;
}
//---------------------------------------------------------------------------//
void*
iseg_ems_open_is(const char* vedname)
{
    struct ved_handle* handle=0;

    try {
        handle=new struct ved_handle;
        handle->ved=new C_VED(vedname);
        handle->is0=handle->ved->is0();
        if (handle->is0==0) {
            cerr<<"VED "<<handle->ved->name()<<" has no procedures."<<endl;
            delete handle;
            return 0;
        }
        handle->is0->execution_mode(immediate);
        handle->can_w =handle->is0->procnum("canbus_write");
        handle->can_wr=handle->is0->procnum("canbus_write_read");
        handle->hv_r =handle->is0->procnum("iseghv_read");
        handle->hv_w  =handle->is0->procnum("iseghv_write");
        handle->cache=0;
        handle->cache_max=0;
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        delete handle;
        return 0;
    }
    return static_cast<void*>(handle);
}
//---------------------------------------------------------------------------//
void
iseg_ems_debug(void *handle, int bus, int on)
{
    ved_handle *vhandle=static_cast<ved_handle*>(handle);
    try {
        delete vhandle->is0->command("canbus_debug", 2, bus, !!on);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
    }
}
//---------------------------------------------------------------------------//
void
iseg_ems_debug(struct iseg_ems_module* module, int on)
{
    iseg_ems_debug(module->ved_handle, module->bus, on);
}
//---------------------------------------------------------------------------//
static int
update_cache(struct ved_handle *handle, int bus, bool always)
{
    struct timeval now;

    if (handle->cache_max<=bus) {
        cache_element *help=new cache_element[bus+1];
        if (help==0) {
            cerr<<"alloc cache for bus "<<bus<<": "<<strerror(errno)<<endl;
            return -1;
        }
        for (int i=0; i<handle->cache_max; i++)
            help[i]=handle->cache[i];
        delete[] handle->cache;
        handle->cache=help;
        handle->cache_max=bus+1;
    }

    gettimeofday(&now, 0);

    struct cache_element *cache=handle->cache+bus;
    if (!always) {
        if (now.tv_sec-cache->timestamp.tv_sec<60) {
            return 0;
        }
    }

    C_confirmation *conf;
    ems_i32 *buf;
    int nr_modules;

    try {
        conf=handle->is0->command("iseghv_enumerate", 1, bus);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }

    cache->timestamp=now;
    for (int i=0; i<64; i++)
        cache->mclass[i]=-1;

    buf=conf->buffer();
    buf++; // skip error code
    nr_modules=*buf++;

    for (int i=0; i<nr_modules; i++) {
        int addr=*buf++;
        cache->mclass[addr]=*buf++;
        buf++; // status
    }

    delete conf;

    return nr_modules;
}
//---------------------------------------------------------------------------//
int
iseg_ems_enumerate(void *ved_handle, int bus, struct iseg_ems_module **modules_)
{
    struct ved_handle *handle=static_cast<struct ved_handle*>(ved_handle);
    struct iseg_ems_module* modules;
    int nr_modules;

    nr_modules=update_cache(handle, bus, true);
    if (nr_modules<=0) {
        *modules_=0;
        return nr_modules;
    }

    modules=new struct iseg_ems_module[nr_modules];
    if (modules==0) {
        cerr<<"alloc mem for "<<nr_modules<<" modules: "<<strerror(errno)<<endl;
        return -1;
    }

    struct cache_element *cache=handle->cache+bus;
    for (int i=0, idx=0; i<64; i++) {
        if (cache->mclass[i]>=0) {
            modules[idx].addr=i;
            modules[idx].mclass=cache->mclass[i];
            modules[idx].ved_handle=ved_handle;
            modules[idx].bus=bus;
            idx++;
        }
    }

    *modules_=modules;
    return nr_modules;
}
//---------------------------------------------------------------------------//
void
iseg_ems_dump(ostream& os, struct iseg_ems_module* module)
{
    os<<setw(2)<<module->addr
        <<" class="<<module->mclass
        <<"  ser="<<module->serial
        <<"  fw="<<module->firmware
        <<"  chans="<<module->channels
        <<"  chanmask=0x"<<hex<<module->equipped_channels<<dec;
    if (module->mclass>=3) {
        for (int chan=0; chan<module->channels; chan++) {
            if (module->equipped_channels&(1<<chan)) {
                os  <<"  umax="<<module->u_max[chan]
                    <<"  imax="<<module->i_max[chan];
            }
        }
    } else {
        os  <<"  umax="<<module->u_max[0]
            <<"  imax="<<module->i_max[0];
    }
    os<<endl;
}
//---------------------------------------------------------------------------//
static float
mant_exp2float(ems_u8 mant, ems_i8 exp)
{
    float val=mant;
    if (exp<0) {
        for (int i=0; i<-exp; i++)
            val/=10.;
    } else {
        for (int i=0; i<exp; i++)
            val*=10.;
    }
    return val;
}
//---------------------------------------------------------------------------//
int
iseg_ems_setup_module(struct iseg_ems_module* module)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation *conf;
    ems_i32* buf;
    int size, expected_size;

    if (update_cache(handle, module->bus, false)<0)
        return -1;

    // copy module class from cache
    module->mclass=handle->cache[module->bus].mclass[module->addr];
    if (module->mclass==-1) {
        cerr<<"iseg_ems_setup_module: no module at address "<<module->addr<<endl;
        return -1;
    }

    // set up some class dependent values
    module->anal_3byte=(module->mclass!=0)&&(module->mclass!=6);
    switch (module->mclass) {
    case 0: // no break
    case 1: // no break
    case 2: // no break
    case 6:
        module->status_mask=0xfe03;
        break;
    case 7:
        module->status_mask=0xfe83;
        break;
    default:
        module->status_mask=0xffff;
    }

    try {
        // serial number
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0xe0);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"read serial number"<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    expected_size=module->mclass==0?5:6;
    if (*buf!=expected_size) {
        cerr<<"read_serial: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter
    module->serial=
        ((buf[0]>>4)&0xf)*100000+
        ( buf[0]    &0xf)*10000+
        ((buf[1]>>4)&0xf)*1000+
        ( buf[1]    &0xf)*100+
        ((buf[2]>>4)&0xf)*10+
        ( buf[2]    &0xf)*1;
    module->firmware=
        ( buf[3]    &0xf)*100+
        ((buf[4]>>4)&0xf)*10+
        ( buf[4]    &0xf)*1;
    module->error_mode=(buf[3]>>4)&0xf;
    if (module->mclass>0)
        module->channels=buf[5]&0xf;
    else
        module->channels=16;

    delete conf;

    // read equipped channels
    if ((module->mclass==1)||(module->mclass==2)|(module->mclass==7)) {
        try {
            conf=handle->is0->command(handle->hv_r, 3, module->bus,
                module->addr, 0x1c8);
        } catch (C_error* e) {
            cerr<<(*e)<<endl;
            cerr<<"B read equipped channels"<<endl;
            delete e;
            return -1;
        }
        buf=conf->buffer();
        size=conf->size();
        buf++; size--;// skip error code
        if (*buf!=2) {
            cerr<<"equipped_channels: unexpected size for module "
                <<module->addr <<": "<<*buf<<endl;
            delete conf;
            return -1;
        }
        buf++; size--;// skip byte counter
        module->equipped_channels=((buf[0]<<8)&0xff)+(buf[1]&0xff);
        delete conf;
    } else {
        module->equipped_channels=0xffff;
    }

    // read nominal values of module
    try {
        conf=handle->is0->command(handle->hv_r, 3,
            module->bus, module->addr, 0xf4);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"D read nominal values"<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code
    if (*buf!=4) {
        cerr<<"read nominal values: unexpected size for module "
            <<module->addr <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter
    module->u_max_mod=mant_exp2float(buf[0], buf[1]);
    module->i_max_mod=mant_exp2float(buf[2], buf[3]);
    delete conf;

    // read nominal values of individual channels
    module->channels_identical=true;
    if ((module->mclass==3)||(module->mclass==6)|(module->mclass==7)) {
        for (int chan=0; chan<module->channels; chan++) {
            if (module->equipped_channels&(1<<chan)) {
                try {
                    conf=handle->is0->command(handle->hv_r, 3,
                        module->bus, module->addr, 0x190+chan);
                } catch (C_error* e) {
                    cerr<<(*e)<<endl;
                    cerr<<"C read nominal values"<<endl;
                    delete e;
                    return -1;
                }
                buf=conf->buffer();
                size=conf->size();
                buf++; size--;// skip error code
                if (*buf!=4) {
                    cerr<<"Z read nominal values: unexpected size for module "
                        <<module->addr <<": "<<*buf<<endl;
                    delete conf;
                    return -1;
                }
                buf++; size--;// skip byte counter
                module->u_max[chan]=mant_exp2float(buf[0], buf[1]);
                module->i_max[chan]=mant_exp2float(buf[2], buf[3]);
                delete conf;
                if (module->u_max[chan]!=module->u_max_mod)
                    module->channels_identical=false;
                if (module->i_max[chan]!=module->i_max_mod)
                    module->channels_identical=false;
            } else {
                module->u_max[chan]=0;
                module->i_max[chan]=0;
            }
        }
    } else {
        for (int chan=0; chan<module->channels; chan++) {
            module->u_max[chan]=module->u_max_mod;
            module->i_max[chan]=module->i_max_mod;
        }
    }

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_nmt(void* handle, int bus, int data_id)
{
    ved_handle *vhandle=static_cast<ved_handle*>(handle);

    try {
        delete vhandle->is0->command("iseghv_nmt", 2, bus, data_id);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"iseghv_nmt"<<endl;
        delete e;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_channel_mask(struct iseg_ems_module* module, unsigned int mask)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);

    try {
        delete handle->is0->command(handle->hv_w, 5, module->bus, module->addr,
                0xcc, (mask>>8)&0xff, mask&0xff);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"F channels_on_mask, module="<<module->addr<<endl;
        delete e;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_channel_mask(struct iseg_ems_module* module, unsigned int *mask)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation *conf;
    ems_i32* buf;
    int size;

    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0xcc);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"F1 channels_on_mask, module="<<module->addr<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=2) {
        cout<<"channels_on_mask: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter
    *mask=((buf[0]&0xff)<<8)+(buf[1]&0xff);
    delete conf;
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_channel_off(struct iseg_ems_module* module, int channel)
{
    unsigned int mask;

    if (channel<0) {
        return iseg_ems_channel_mask(module, 0U);
    } else {
        if (iseg_ems_channel_mask(module, &mask)<0)
            return -1;

        mask&=~(1<<channel);
        return iseg_ems_channel_mask(module, mask);
    }
}
//---------------------------------------------------------------------------//
int
iseg_ems_channel_on(struct iseg_ems_module* module, int channel)
{
    unsigned int mask;

    if (channel<0) {
        return iseg_ems_channel_mask(module, 0xffU);
    } else {
        if (iseg_ems_channel_mask(module, &mask)<0)
            return -1;

        mask|=1<<channel;
        return iseg_ems_channel_mask(module, mask);
    }
}
//---------------------------------------------------------------------------//
/*
 * sets or presets the voltage
 */
int
iseg_ems_voltage(struct iseg_ems_module* module, int channel, float voltage)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    int data_id, val;
    float max;

    if ((channel<0) && !module->channels_identical) {
        cout<<"set_voltage: channels are not identical!"<<endl;
        return -1;
    } else if (channel>=module->channels) {
        cerr<<"set_voltage: channel "<<channel<<" does not exist"<<endl;
        return -1;
    }

    if (channel<0)
        max=module->u_max_mod;
    else
        max=module->u_max[channel];
    if ((voltage<0) || (voltage>max)) {
        cerr<<"set_voltage: "<<voltage<<">"<<max<<endl;
        return -1;
    }
    val=static_cast<int>(voltage/max*resol[module->anal_3byte]);

    if (channel<0) {
        data_id=0xe4;
    } else {
        data_id=0xa0+channel;
    }

    try {
        if (module->anal_3byte) {
            ems_u8 data[3];
            data[0]=(val>>16)&0xff;
            data[1]=(val>>8)&0xff;
            data[2]=val&0xff;
            delete handle->is0->command(handle->hv_w, 6, module->bus,
                    module->addr, data_id, data[0], data[1], data[2]);
        } else {
            ems_u8 data[2];
            data[0]=(val>>8)&0xff;
            data[1]=val&0xff;
            delete handle->is0->command(handle->hv_w, 5, module->bus,
                    module->addr, data_id, data[0], data[1]);
        }
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"G set_voltage"<<endl;
        delete e;
        return -1;
    }

    return 0;
}
//---------------------------------------------------------------------------//
/*
 * reads the real voltage at the output
 */
int
iseg_ems_voltage(struct iseg_ems_module* module, int channel,
        float *voltage)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;
    float val;

    if ((channel>=0) && (channel>=module->channels)) {
        cerr<<"get_voltage: channel "<<channel<<" does not exist"<<endl;
        return -1;
    }

    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0x80+channel);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=(module->anal_3byte?3:2)) {
        cout<<"read_voltage: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter

    if (module->anal_3byte)
        val=((buf[0]&0xff)<<16)+((buf[1]&0xff)<<8)+(buf[2]&0xff);
    else
        val=((buf[0]&0xff)<<8)+(buf[1]&0xff);
    delete conf;

    *voltage=val*module->u_max[channel]/resol[module->anal_3byte];

    return 0;
}
//---------------------------------------------------------------------------//
/*
 * reads the preset voltage, not the real output voltage
 */
int
iseg_ems_voltage_set(struct iseg_ems_module* module, int channel,
        float *voltage)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;
    float val;

    if ((channel>=0) && (channel>=module->channels)) {
        cerr<<"get_voltage_set: channel "<<channel<<" does not exist"<<endl;
        return -1;
    }

    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0xa0+channel);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=(module->anal_3byte?3:2)) {
        cout<<"read_voltage: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter

    if (module->anal_3byte)
        val=((buf[0]&0xff)<<16)+((buf[1]&0xff)<<8)+(buf[2]&0xff);
    else
        val=((buf[0]&0xff)<<8)+(buf[1]&0xff);
    delete conf;

    *voltage=val*module->u_max[channel]/resol[module->anal_3byte];

    return 0;
}
//---------------------------------------------------------------------------//
/*
 * sets or presets the current trip (soft or hard)
 */
int
iseg_ems_current(struct iseg_ems_module* module, int channel, float current)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    int data_id, val;
    float max;

    if ((channel<0) && !module->channels_identical) {
        cout<<"set_current: channels are not identical!"<<endl;
        return -1;
    } else if (channel>=module->channels) {
        cerr<<"set_current: channel "<<channel<<" does not exist"<<endl;
        return -1;
    }

    if (channel<0)
        max=module->i_max_mod;
    else
        max=module->i_max[channel];
    if ((current<0) || (current>max)) {
        cerr<<"set_current: "<<current<<">"<<max<<endl;
        return -1;
    }
    val=static_cast<int>(current/max*resol[module->anal_3byte]);

    if (channel<0) {
        data_id=0x1e4;
    } else {
        if ((module->mclass==6) || (module->mclass==7))
            data_id=0x1a0+channel;
        else
            data_id=0x180+channel;
    }

    try {
        if (module->anal_3byte) {
            ems_u8 data[3];
            data[0]=(val>>16)&0xff;
            data[1]=(val>>8)&0xff;
            data[2]=val&0xff;
            delete handle->is0->command(handle->hv_w, 6, module->bus,
                    module->addr, data_id, data[0], data[1], data[2]);
        } else {
            ems_u8 data[2];
            data[0]=(val>>8)&0xff;
            data[1]=val&0xff;
            delete handle->is0->command(handle->hv_w, 5, module->bus,
                    module->addr, data_id, data[0], data[1]);
        }
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_current(struct iseg_ems_module* module, int channel,
        float *current)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;
    float val;

    if ((channel>=0) && (channel>=module->channels)) {
        cerr<<"get_current: channel "<<channel<<" does not exist"<<endl;
        return -1;
    }
    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0x90+channel);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=(module->anal_3byte?3:2)) {
        cout<<"read_current: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter

    if (module->anal_3byte)
        val=((buf[0]&0xff)<<16)+((buf[1]&0xff)<<8)+(buf[2]&0xff);
    else
        val=((buf[0]&0xff)<<8)+(buf[1]&0xff);
    delete conf;

    *current=val*module->i_max[channel]/resol[module->anal_3byte];

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_current_set(struct iseg_ems_module* module, int channel,
        float *current)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;
    float val;

    if ((channel>=0) && (channel>=module->channels)) {
        cerr<<"get_current_set: channel "<<channel<<" does not exist"<<endl;
        return -1;
    }
    try {
        int data_id;
        if ((module->mclass==6) || (module->mclass==7))
            data_id=0x1a0+channel;
        else
            data_id=0x180+channel;
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                data_id);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=(module->anal_3byte?3:2)) {
        cout<<"read_current: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter

    if (module->anal_3byte)
        val=((buf[0]&0xff)<<16)+((buf[1]&0xff)<<8)+(buf[2]&0xff);
    else
        val=((buf[0]&0xff)<<8)+(buf[1]&0xff);
    delete conf;

    *current=val*module->i_max[channel]/resol[module->anal_3byte];

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_current_limit(struct iseg_ems_module* module, float *fraction)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;
    float val;

    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0xe8);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=(module->anal_3byte?3:2)) {
        cout<<"read_current_limit: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter

    if (module->anal_3byte)
        val=((buf[0]&0xff)<<16)+((buf[1]&0xff)<<8)+(buf[2]&0xff);
    else
        val=((buf[0]&0xff)<<8)+(buf[1]&0xff);
    delete conf;

    *fraction=val/resol[module->anal_3byte];

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_voltage_limit(struct iseg_ems_module* module, float *fraction)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;
    float val;

    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0x1e8);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=(module->anal_3byte?3:2)) {
        cout<<"read_voltage_limit: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter

    if (module->anal_3byte)
        val=((buf[0]&0xff)<<16)+((buf[1]&0xff)<<8)+(buf[2]&0xff);
    else
        val=((buf[0]&0xff)<<8)+(buf[1]&0xff);
    delete conf;

    *fraction=val/resol[module->anal_3byte];

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_supply_voltages(struct iseg_ems_module* module,
    float *Vp24, float *Vp15, float *Vp5, float *Vn15, float *Vn5, float *T)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;

    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0x1c0);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=7) {
        cout<<"read_supply_voltages: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter

    if (Vp24)
        *Vp24=buf[0]*.1;
    if (Vp15)
        *Vp15=buf[1]*.1;
    if (Vp5)
        *Vp5=buf[2]*.1;
    if (Vn15)
        *Vn15=buf[3]*.1;
    if (Vn5)
        *Vn5=buf[4]*.1;
    if (T)
        *T=((buf[5]<<8)+buf[6])*.1;
    delete conf;

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_ramp(struct iseg_ems_module* module, int *ramp)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;
    int val;

    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0xd0);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=(module->anal_3byte?3:2)) {
        cout<<"read_ramp: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter

    if (module->anal_3byte)
        val=((buf[0]&0xff)<<16)+((buf[1]&0xff)<<8)+(buf[2]&0xff);
    else
        val=((buf[0]&0xff)<<8)+(buf[1]&0xff);
    delete conf;

    *ramp=val;

    return 0;
}
//---------------------------------------------------------------------------//
static int
iseg_ems_ramp_(struct iseg_ems_module* module, int ramp)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);

    try {
        if (module->anal_3byte) {
            ems_u8 data[3];
            data[0]=(ramp>>16)&0xff;
            data[1]=(ramp>>8)&0xff;
            data[2]=ramp&0xff;
            delete handle->is0->command(handle->hv_w, 6, module->bus,
                    module->addr, 0xd0, data[0], data[1], data[2]);
        } else {
            ems_u8 data[2];
            data[0]=(ramp>>8)&0xff;
            data[1]=ramp&0xff;
            delete handle->is0->command(handle->hv_w, 5, module->bus,
                    module->addr, 0xd0, data[0], data[1]);
        }
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_ramp(struct iseg_ems_module* module, int ramp)
{
    int min, max;

    if (module->anal_3byte) {
        min=4000;
        max=1000000;
    } else {
        min=20;
        max=5000;
    }
    if ((ramp<min)||(ramp>max)) {
        cerr<<"iseg_ems_ramp: value "<<ramp<<" out of range"<<endl;
        return -1;
    }

    int ramp_a=ramp;
    bool again;
    do {
        again=false;
        if (iseg_ems_ramp_(module, ramp_a)<0)
            return -1;
        int ramp_b;
        if (iseg_ems_ramp(module, &ramp_b)<0)
            return -1;
        if (ramp_b!=ramp_a) {
            ramp_a++;
            again=true;
        }
    } while (again);
    if (ramp_a!=ramp)
        cerr<<"ramp speed changed from "<<ramp<<" to "<<ramp_a<<endl;

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_ramp(struct iseg_ems_module* module, float ramp)
{
    float fval=resol[module->anal_3byte]/ramp;
    int val=static_cast<int>(fval);
    return iseg_ems_ramp(module, val);
}
//---------------------------------------------------------------------------//
int
iseg_ems_ramp(struct iseg_ems_module* module, float *ramp)
{
    int val;

    if (iseg_ems_ramp(module, &val)<0)
        return -1;

    *ramp=resol[module->anal_3byte]/val;

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_status_module(struct iseg_ems_module* module, unsigned int *flags)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;

    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0xc0);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=1) {
        cout<<"status_module: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter
    *flags=buf[0];

    delete conf;

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_status_module(struct iseg_ems_module* module, unsigned int flags)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);

    try {
        delete handle->is0->command(handle->hv_w, 4, module->bus, module->addr,
                0xc0, flags);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_status_channel(struct iseg_ems_module* module, int channel,
        unsigned int *flags)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;

    if ((channel>=0) && (channel>=module->channels)) {
        cerr<<"status_channel: channel "<<channel<<" does not exist"<<endl;
        return -1;
    }
    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                0xb0+channel);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=2) {
        cout<<"status_channel: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter
    *flags=(((buf[0]&0xff)<<8)+(buf[1]&0xff))&module->status_mask;

    delete conf;

    return 0;
}
//---------------------------------------------------------------------------//
static int
iseg_ems_16bit(struct iseg_ems_module* module, int id, unsigned int flags)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);

    try {
        delete handle->is0->command(handle->hv_w, 5, module->bus, module->addr,
                id, (flags>>8)&0xff, flags&0xff);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
static int
iseg_ems_16bit(struct iseg_ems_module* module, int id, unsigned int *flags)
{
    ved_handle *handle=static_cast<ved_handle*>(module->ved_handle);
    C_confirmation* conf;
    ems_i32* buf;
    int size;

    try {
        conf=handle->is0->command(handle->hv_r, 3, module->bus, module->addr,
                id);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    buf=conf->buffer();
    size=conf->size();
    buf++; size--;// skip error code

    if (*buf!=2) {
        cout<<"iseg_ems_16bit: unexpected size for module "<<module->addr
            <<": "<<*buf<<endl;
        delete conf;
        return -1;
    }
    buf++; size--;// skip byte counter
    *flags=(((buf[0]&0xff)<<8)+(buf[1]&0xff))&module->status_mask;

    delete conf;

    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_status_group(struct iseg_ems_module* module, int group,
        unsigned int flags)
{
    int id;

    switch (group) {
    case 1: id=0xc4; break;
    case 2: id=0xc8; break;
    case 3: id=0xf8; break;
    case 4: id=module->mclass<3?0xe0:0xd0; break;
    default:
        cerr<<"iseg_ems_status_group: illegal group "<<group<<endl;
        return -1;
    }

    return iseg_ems_16bit(module, id, flags);
}
//---------------------------------------------------------------------------//
int
iseg_ems_status_group(struct iseg_ems_module* module, int group,
        unsigned int *flags)
{
    int id;

    switch (group) {
    case 1: id=0xc4; break;
    case 2: id=0xc8; break;
    case 3: id=0xf8; break;
    case 4: id=module->mclass<3?0xe0:0xd0; break;
    default:
        cerr<<"iseg_ems_status_group: illegal group "<<group<<endl;
        return -1;
    }

    return iseg_ems_16bit(module, id, flags);
}
//---------------------------------------------------------------------------//
int
iseg_ems_emergency_cut_off(struct iseg_ems_module* module, unsigned int flags)
{
    return iseg_ems_16bit(module, 0xd4, flags);
}
//---------------------------------------------------------------------------//
int
iseg_ems_emergency_cut_off(struct iseg_ems_module* module, unsigned int *flags)
{
    return iseg_ems_16bit(module, 0xd4, flags);
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
int
iseg_ems_enumerate_crates(void *ved_handle, int bus,
        iseg_ems_crate **crates_)
{
    struct ved_handle *handle=static_cast<struct ved_handle*>(ved_handle);
    C_confirmation *conf;
    iseg_ems_crate *crates;
    int num_crates;

    try {
        conf=handle->is0->command("canbus_write_read_burst", 6, bus, 0x7e8,
                1, 100, 8, 0x7e7);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"crate_on="<<endl;
        delete e;
        return -1;
    }
    if (conf->size()<2) {
        cout<<"enumerate_crates: unexpected size "<<conf->size()<<endl;
        cout<<(*conf)<<endl;
        delete conf;
        return -1;
    }
    //cout<<(*conf)<<endl;
    ems_i32* buf=conf->buffer();

    buf++; // skip error code
    num_crates=*buf++;
    crates=new iseg_ems_crate[num_crates];
    for (int i=0; i<num_crates; i++) {
        if (*buf!=12) {
            cout<<"enumerate_crates: block size!=12"<<endl;
            cout<<(*conf)<<endl;
            delete conf;
            return -1;
        }
        buf+=5; // skip size, id, rtr, time, dlc
        crates[i].ved_handle=ved_handle;
        crates[i].bus=bus;
        crates[i].addr=buf[0];
        crates[i].emcy_id=((buf[3]&0xf)<<8)+buf[4];
        crates[i].sub_id=(buf[5]<<4)+((buf[6]>>4)&0xf);
        buf+=8;
    }
    delete conf;

    *crates_=crates;
    return num_crates;

#if 0
    struct ved_handle *handle=static_cast<struct ved_handle*>(ved_handle);
    struct iseg_ems_crate* crates;
    int nr_crates;
#endif
    return -1;
}
//---------------------------------------------------------------------------//
int
iseg_ems_setup_crate(struct iseg_ems_crate* crate)
{
    //ved_handle *handle=static_cast<ved_handle*>(crate->ved_handle);
    return iseg_ems_crate_serial(crate, &crate->serial, &crate->firmware);
}
//---------------------------------------------------------------------------//
int
iseg_ems_crate_on(iseg_ems_crate *crate, bool on)
{
    ved_handle *handle=static_cast<ved_handle*>(crate->ved_handle);
    try {
        delete handle->is0->command(handle->can_w, 6, crate->bus, crate->sub_id,
                0, 0x41, on?0:1, 1);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"crate_on"<<endl;
        delete e;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_crate_on(iseg_ems_crate *crate, bool *on, int timeout)
{
    ved_handle *handle=static_cast<ved_handle*>(crate->ved_handle);
    C_confirmation *conf;

    try {
        conf=handle->is0->command(handle->can_wr, 5, crate->bus, crate->sub_id,
                0, timeout, 0xc1);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"crate_on="<<endl;
        delete e;
        return -1;
    }
    if (conf->size()!=8) {
        cout<<"crate_on: unexpected size "<<conf->size()<<endl;
        cout<<(*conf)<<endl;
        delete conf;
        return -1;
    }
    *on=conf->buffer(7)==0;
    delete conf;
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_crate_voltage(iseg_ems_crate *crate, int id, float *val)
{
    ved_handle *handle=static_cast<ved_handle*>(crate->ved_handle);
    C_confirmation *conf;

    if ((id<0)||(id>2)) {
        cout<<"crate_voltage: illegal id "<<id<<endl;
    }

    try {
        conf=handle->is0->command(handle->can_wr, 5, crate->bus, crate->sub_id,
                0, 100, 0xa0+id);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"crate_on="<<endl;
        delete e;
        return -1;
    }
    if (conf->size()!=9) {
        cout<<"crate_voltage: unexpected size "<<conf->size()<<endl;
        cout<<(*conf)<<endl;
        delete conf;
        return -1;
    }
    int ival=((conf->buffer(7)&0xff)<<8)+(conf->buffer(8)&0xff);
    if (id==1)
        *val=ival*5./2048.;
    else
        *val=ival*24./2048.;

    delete conf;
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_crate_fans(iseg_ems_crate *crate, int id, int *fans, float *val)
{
    ved_handle *handle=static_cast<ved_handle*>(crate->ved_handle);
    C_confirmation *conf;

    if ((id<3)||(id>6)) {
        cout<<"crate_fans: illegal id "<<id<<endl;
    }

    try {
        conf=handle->is0->command(handle->can_wr, 5, crate->bus, crate->sub_id,
                0, 100, 0xa0+id);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"crate_on="<<endl;
        delete e;
        return -1;
    }
    if (conf->size()!=10) {
        cout<<"crate_fans: unexpected size "<<conf->size()<<endl;
        cout<<(*conf)<<endl;
        delete conf;
        return -1;
    }
    int ival=((conf->buffer(7)&0xff)<<8)+(conf->buffer(8)&0xff);
    *val=ival;
    *fans=conf->buffer(9)&0xff;

    delete conf;
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_crate_power(iseg_ems_crate *crate, int *ac, int *dc)
{
    ved_handle *handle=static_cast<ved_handle*>(crate->ved_handle);
    C_confirmation *conf;

    try {
        conf=handle->is0->command(handle->can_wr, 5, crate->bus, crate->sub_id,
                0, 100, 0xa7);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"crate_on="<<endl;
        delete e;
        return -1;
    }
    if (conf->size()==10) {
        cout<<"crate_power: invalid size 10"<<endl;
        return -1;
    }
    if (conf->size()!=9) {
        cout<<"crate_power: unexpected size "<<conf->size()<<endl;
        cout<<(*conf)<<endl;
        delete conf;
        return -1;
    }
    *ac=conf->buffer(7)&0xff;
    *dc=conf->buffer(8)&0xff;

    delete conf;
    return 0;
}
//---------------------------------------------------------------------------//
int
iseg_ems_crate_serial(iseg_ems_crate *crate, int *serial, int *release)
{
    ved_handle *handle=static_cast<ved_handle*>(crate->ved_handle);
    C_confirmation *conf;

    try {
        conf=handle->is0->command(handle->can_wr, 5, crate->bus, crate->sub_id,
                0, 100, 0xc6);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        cerr<<"crate_on="<<endl;
        delete e;
        return -1;
    }
    if (conf->size()!=12) {
        cout<<"crate_serial: unexpected size "<<conf->size()<<endl;
        cout<<(*conf)<<endl;
        delete conf;
        return -1;
    }
    int ser=0;
    ser+=((conf->buffer(7)>>4)&0xf)*100000;
    ser+=(conf->buffer(7)&0xf)*10000;
    ser+=((conf->buffer(8)>>4)&0xf)*1000;
    ser+=(conf->buffer(8)&0xf)*100;
    ser+=((conf->buffer(9)>>4)&0xf)*10;
    ser+=(conf->buffer(9)&0xf)*1;
    *serial=ser;

    int rel=0;
    rel+=((conf->buffer(10)>>4)&0xf)*1000;
    rel+=(conf->buffer(10)&0xf)*100;
    rel+=((conf->buffer(11)>>4)&0xf)*10;
    rel+=(conf->buffer(11)&0xf)*1;
    *release=rel;

    delete conf;
    return 0;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
