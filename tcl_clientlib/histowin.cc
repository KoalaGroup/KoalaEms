/*
 * histowin.cc
 * 
 * created 19.02.1996 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include "histowin.hxx"
#include <cstdlib>
#include <cmath>
#include <sys/time.h>
#include <cfloat>
#include <cstring>
#include <tk.h>
#include "findstring.hxx"
#include "compat.h"
#include <errors.hxx>
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: histowin.cc,v 1.17 2010/09/10 23:18:58 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
E_histowin::E_histowin(Tcl_Interp* interp, ClientData clientdata,
    int objc, Tcl_Obj* const objv[])
:
    xscalemode(XS_lastdata),
    yscalemode(YS_constant),
    interp(interp),
    xwindow(0),
    graphics_active(0),
    cross_active(0),
    updatereason(HW_none),
    updatepending(0),
    crossreason(CR_none),
    crosspending(0),
    visibility(VisibilityFullyObscured),
    mapped(0),
    n_width(0),
    n_height(0),
    xoffs(0),
    yoffs(0),
    arrays(((inithints*)clientdata)->harrs),
    arrlist(0),
    arrnum(0),
    xscalecommand(""),
    yscalecommand(""),
    crosscommand(""),
    scrollcommand("")
{
    if (objc<2) {
        Tcl_WrongNumArgs(interp, 1, objv, "name ...");
        throw TCL_ERROR;
    }

    // fill idiotic defined ConfigSpecs
    for (int i=0; config_specs[i].type!=TK_CONFIG_END; i++) {
        config_specs[i].argvName=strdup(config_specnames[i]);
    }
    for (int i=0; arrconfig_specs[i].type!=TK_CONFIG_END; i++) {
        arrconfig_specs[i].argvName=strdup(arrconfig_specnames[i]);
    }

    winpath=Tcl_GetString(objv[1]);
    window=Tk_CreateWindowFromPath(interp, ((inithints*)clientdata)->mainwin,
            (char*)winpath.c_str(), 0);
    if (window==0)
        throw TCL_ERROR;
    Tk_SetClass(window, "Histowin");
    char bits[32];
    for (int i=0; i<32; i++)
        bits[i]=0;
    emptycursor=Tk_GetCursorFromData(interp, window, bits, bits,
        16, 16, 0, 0, Tk_GetUid("black"), Tk_GetUid("black"));
    Tk_MakeWindowExist(window);
    xwindow=Tk_WindowId(window);
    xdisplay=Tk_Display(window);
    xScreen=Tk_Screen(window);
    xscreen=Tk_ScreenNumber(window);
    XGCValues values;
    values.function=GXxor;
    values.line_width=0;
    values.line_style=LineSolid;
    values.cap_style=CapButt;
    crossgc=XCreateGC(xdisplay, xwindow,
        GCLineWidth|GCLineStyle|GCCapStyle|GCFunction, &values);
    gc=XCreateGC(xdisplay, xwindow, 0, 0);

    config.c_background=0;
    config.c_foreground=0;
    config.c_crosscolor=0;
    config.c_cursor=0;
    if (configure(objc-2, objv+2, 0)!=TCL_OK) {
        XFreeGC(xdisplay, gc);
        XFreeGC(xdisplay, crossgc);
        Tk_DestroyWindow(window);
        throw TCL_ERROR;
    }

    Tk_CreateEventHandler(window, ExposureMask|StructureNotifyMask|
        VisibilityChangeMask|EnterWindowMask|LeaveWindowMask|PointerMotionMask,
        eventhandler, (ClientData)this);
    tclcommand=Tcl_CreateObjCommand(interp, (char*)winpath.c_str(), command,
        ClientData(this), destroy);
#if 0
#ifdef __osf__
    write_rnd(FP_RND_RN); // Round toward nearest
#endif
#endif
    initscaling();
}
/*****************************************************************************/
E_histowin::~E_histowin()
{
    detachallarrays();
    delete[] arrlist;
    if (xwindow) {
        Tk_FreeOptions(config_specs, (char*)&config, xdisplay, 0);
        XFreeGC(xdisplay, gc);
        XFreeGC(xdisplay, crossgc);
        Tk_FreeCursor(xdisplay, emptycursor);
    }
    for (int i=0; config_specs[i].type!=TK_CONFIG_END; i++)
        free(config_specs[i].argvName);
    for (int i=0; arrconfig_specs[i].type!=TK_CONFIG_END; i++)
        free(arrconfig_specs[i].argvName);
}
/*****************************************************************************/
void E_histowin::inithints::destroy(ClientData clientdata, Tcl_Interp*)
{
    delete (E_histowin::inithints*)clientdata;
}
/*****************************************************************************/
int E_histowin::create(ClientData clientdata, Tcl_Interp* interp,
    int objc, Tcl_Obj* const objv[])
{
    int res;
    try {
        E_histowin* hwin=new E_histowin(interp, clientdata, objc, objv);
        Tcl_SetResult_String(interp, hwin->tclprocname());
        res=TCL_OK;
    } catch (int e) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("E_histowin::E_histowin() failed", -1));
        res=e;
    }
    return res;
}
/*****************************************************************************/
void E_histowin::destroy(ClientData clientdata)
{
    delete (E_histowin*)clientdata;
}
/*****************************************************************************/
int E_histowin::command(ClientData clientdata, Tcl_Interp* interp,
    int objc, Tcl_Obj* const objv[])
{
    E_histowin* win=(E_histowin*)clientdata;
    return win->e_command(objc, objv);
}
/*****************************************************************************/
void E_histowin::updateproc(ClientData clientdata)
{
    ((E_histowin*)clientdata)->update();
}
/*****************************************************************************/
void E_histowin::update_cross()
{
    crreason reason=crossreason;
    crossreason=CR_none;
    crosspending=0;
    if (crosscommand=="")
        return;
    if (reason&CR_create) {
        if (reason&CR_delete) {
            return;
        } else {
            OSTRINGSTREAM st;
            st << crosscommand << " create " << setprecision(20)
                << cross_x+xoffs << ' ' << t_xv(cross_x) << ' '
                << cross_y+yoffs << ' '  << t_yv(cross_y);
            STRING s=st.str();
            if (Tcl_GlobalEval(interp, (char*)s.c_str())!=TCL_OK)
                Tcl_BackgroundError(interp);
        }
    } else if (reason&CR_delete) {
        OSTRINGSTREAM st;
        st << crosscommand << " delete " << setprecision(20)
            << cross_x+xoffs << ' ' << t_xv(cross_x) << ' ' << cross_y+yoffs
            << ' '  << t_yv(cross_y);
        STRING s=st.str();
        if (Tcl_GlobalEval(interp, (char*)s.c_str())!=TCL_OK)
            Tcl_BackgroundError(interp);
    } else if (reason&CR_move) {
        OSTRINGSTREAM st;
        st << crosscommand << " move " << setprecision(20)
            << cross_x+xoffs << ' ' << t_xv(cross_x) << ' ' << cross_y+yoffs
            << ' '  << t_yv(cross_y);
        STRING s=st.str();
        if (Tcl_GlobalEval(interp, (char*)s.c_str())!=TCL_OK)
            Tcl_BackgroundError(interp);
    }
}
/*****************************************************************************/
void E_histowin::updatecrossproc(ClientData data)
{
    ((E_histowin*)data)->update_cross();
}
/*****************************************************************************/
void E_histowin::cross_create(int x, int y)
{
    if (graphics_active) {
        cross_hidden=1;
    } else {
        XDrawLine(xdisplay, xwindow, crossgc, 0, y, n_width-1, y);
        XDrawLine(xdisplay, xwindow, crossgc, x, 0, x, n_height-1);
        cross_hidden=0;
    }
    cross_x=x;
    cross_y=y;
    cross_active=1;
    if (crosscommand!="") {
        crossreason=CR_create;
        if (!crosspending) {
            Tk_DoWhenIdle(updatecrossproc, (ClientData)this);
            crosspending=1;
        }
    }
}
/*****************************************************************************/
void E_histowin::cross_delete()
{
    if (graphics_active)
        cross_hidden=0;
    else {
        XDrawLine(xdisplay, xwindow, crossgc, 0, cross_y, n_width-1, cross_y);
        XDrawLine(xdisplay, xwindow, crossgc, cross_x, 0, cross_x, n_height-1);
    }
    cross_active=0;
    if (crosscommand!="") {
        crossreason=(crreason)(crossreason|CR_delete);
        if (!crosspending) {
            Tk_DoWhenIdle(updatecrossproc, (ClientData)this);
            crosspending=1;
        }
    }
}
/*****************************************************************************/
void E_histowin::cross_show()
{
    if (!cross_active || graphics_active) return;
    XDrawLine(xdisplay, xwindow, crossgc, 0, cross_y, n_width-1, cross_y);
    XDrawLine(xdisplay, xwindow, crossgc, cross_x, 0, cross_x, n_height-1);
    cross_hidden=0;
}
/*****************************************************************************/
void E_histowin::cross_hide()
{
    if (!cross_active) return;
    XDrawLine(xdisplay, xwindow, crossgc, 0, cross_y, n_width-1, cross_y);
    XDrawLine(xdisplay, xwindow, crossgc, cross_x, 0, cross_x, n_height-1);
    cross_hidden=1;
}
/*****************************************************************************/
void E_histowin::cross_move(int x, int y)
{
    if (!cross_active) return;

    if (!graphics_active) {
        XDrawLine(xdisplay, xwindow, crossgc, 0, cross_y, n_width-1, cross_y);
        XDrawLine(xdisplay, xwindow, crossgc, cross_x, 0, cross_x, n_height-1);
        XDrawLine(xdisplay, xwindow, crossgc, 0, y, n_width-1, y);
        XDrawLine(xdisplay, xwindow, crossgc, x, 0, x, n_height-1);
    }
    cross_x=x;
    cross_y=y;
    if (crosscommand!="") {
        crossreason=(crreason)(crossreason|CR_move);
        if (!crosspending) {
            Tk_DoWhenIdle(updatecrossproc, (ClientData)this);
            crosspending=1;
        }
    }
}
/*****************************************************************************/
void E_histowin::pointer_enter(int x, int y)
{
    cross_create(x, y);
}
/*****************************************************************************/
void E_histowin::pointer_move(int x, int y)
{
    if (!cross_active) return;
    cross_move(x, y);
}
/*****************************************************************************/
void E_histowin::pointer_leave()
{
    if (!cross_active) return;
    cross_delete();
}
/*****************************************************************************/
int E_histowin::t_vx(double v) const
{
    return (int)(rint((v-dxbase)*df))+xzero;
}
/*****************************************************************************/
int E_histowin::t_vy(double v) const
{
    return n_height-(int)(rint((v-dy0)/(dy1-dy0)*(n_height-1)))-1;
}
/*****************************************************************************/
double E_histowin::t_xv(int x) const
{
    return (x-xzero)/df+dxbase;
}
/*****************************************************************************/
double E_histowin::t_yv(int y) const
{
    return (dy1-dy0)*(n_height-y-1)/(n_height-1)+dy0;
}
/*****************************************************************************/
void E_histowin::initscaling(void)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    anfang=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
    deltax=100;
    dx0=0;
    dx1=dx0+deltax;
    dy0=0;
    dy1=100;
    arrminx=dx0;
    arrmaxx=dx1;
}
/*****************************************************************************/
STRING E_histowin::zeit(double t)
{
    OSTRINGSTREAM st;
    if (fabs(t-anfang)<10000)
        st << t-anfang << "-start";
    else
        st << t;
    return st.str();
}
/*****************************************************************************/
void E_histowin::doxscalecommand()
{
    OSTRINGSTREAM st;
    st<<xscalecommand<<" "<<setprecision(20)<<dx0<<" "<<dx1;
    STRING s=st.str();
    if (Tcl_GlobalEval(interp, (char*)s.c_str())!=TCL_OK)
        Tcl_BackgroundError(interp);
    char* command=new char[strlen("update idletasks")+1];
    strcpy(command, "update idletasks");
    if (Tcl_GlobalEval(interp, command)!=TCL_OK)
        Tcl_BackgroundError(interp);
}
/*****************************************************************************/
void E_histowin::doscrollcommand()
{
    double p0, p1;

    if ((arrminx>=dx0) || (arrmaxx==arrminx))
        p0=0;
    else
        p0=(dx0-arrminx)/(arrmaxx-arrminx);
    if ((arrmaxx<=dx1) || (arrmaxx==arrminx))
        p1=1;
    else
        p1=(dx1-arrminx)/(arrmaxx-arrminx);
    OSTRINGSTREAM st;
    st << scrollcommand << ' ' << p0 << ' ' << p1;
    STRING s=st.str();
    if (Tcl_GlobalEval(interp, (char*)s.c_str())!=TCL_OK)
        Tcl_BackgroundError(interp);
}
/*****************************************************************************/
void E_histowin::docrosscommand()
{
    OSTRINGSTREAM st;
    st << crosscommand << " move " << setprecision(20)
        << cross_x+xoffs << ' ' << t_xv(cross_x) << ' ' << cross_y+yoffs
        << ' ' << t_yv(cross_y);
    STRING s=st.str();
    if (Tcl_GlobalEval(interp, (char*)s.c_str())!=TCL_OK)
        Tcl_BackgroundError(interp);
}
/*****************************************************************************/
void E_histowin::rescale()
{
    // dx1 und deltax muessen vorher gesetzt sein
    // n_width wird verwendet
    // dx0, df und xzero werden berechnet
    // dxbase wird auf dx1 gesetzt
    if (deltax>dx1) {
        dx0=0;
        dx1=deltax;
    } else {
        dx0=dx1-deltax;
    }
    df=(double)(n_width-1)/deltax;
    dxbase=dx1;
    xzero=n_width-1;
    if (xscalecommand!="") doxscalecommand();
    if (scrollcommand!="") doscrollcommand();
    if (cross_active && (crosscommand!="")) docrosscommand();
}
/*****************************************************************************/
void E_histowin::shiftscale(int shift)
{
    // deltax und n_width muessen seit dem letzten rescale unveraendert sein
    // dx0, dx1 und xzero werden berechnet
    // dxbase und df bleiben unveraendert
    xzero-=shift;
    dx1=t_xv(n_width-1);
    dx0=dx1-deltax;
    if (xscalecommand!="") doxscalecommand();
    if (scrollcommand!="") doscrollcommand();
    if (cross_active && (crosscommand!="")) docrosscommand();
}
/*****************************************************************************/
void E_histowin::update()
{
    updatepending=0;
    if (graphics_active)
        return;
    if (!mapped)
        return;

    cross_hide();

    if ((updatereason&HW_redraw)) {
        do_redraw();
    } else {
        if (updatereason&HW_expose)
            do_expose();
        if (updatereason&HW_v_shift)
            do_v_shift();
        else if (updatereason&HW_x_shift)
            do_x_shift(0);
        if (updatereason&HW_newdata)
            do_newdata();
    }

    updatereason=HW_none;
    cross_show();
}
/*****************************************************************************/
void E_histowin::draw_arr_region(arrentry* arrent, int top, int bottom,
    double left, double right, int exact)
{
    E_histoarray* arr=arrent->arr;
    GC gc=arrent->gc;
    int rexact=0;
    double ll=arrent->last;
    if (left>=ll)
        return;
    if (ll<right) {
        right=ll;
        rexact=1;
    }

    int lidx=arr->leftidx(left, exact);
    int ridx=arr->rightidx(right, rexact||exact);
    {
        int x0, x1;
        x0=t_vx(arr->time(lidx));
        x1=t_vx(arr->time(lidx+1));
        if ((x0<-1000) || (x0>x1))
            lidx++;
    }

    switch (arrent->drawmode) {
    case arrentry::DM_line: {
        double dx1, dy1;
        int x0, y0, x1, y1;
        if (arrent->exp==0) {
            x0=t_vx(arr->time(lidx));
            y0=t_vy(arr->val(lidx));
            for (int idx=lidx+1; idx<=ridx; idx++) {
                dx1=arr->time(idx); x1=t_vx(dx1);
                dy1=arr->val(idx);  y1=t_vy(dy1);
                if (((y0>top) || (y1>top)) && ((y0<bottom) || (y1<bottom)))
                    XDrawLine(xdisplay, xwindow, gc, x0, y0, x1, y1);
                x0=x1; y0=y1;
            }
        } else {
            double scale=arrent->scale;
            x0=t_vx(arr->time(lidx));
            y0=t_vy(arr->val(lidx)*scale);
            for (int idx=lidx+1; idx<=ridx; idx++) {
                dx1=arr->time(idx); x1=t_vx(dx1);
                dy1=arr->val(idx)*scale;  y1=t_vy(dy1);
                if (((y0>top) || (y1>top)) && ((y0<bottom) || (y1<bottom)))
                    XDrawLine(xdisplay, xwindow, gc, x0, y0, x1, y1);
                x0=x1; y0=y1;
            }
        }
        } break;
    case arrentry::DM_box: {
        double dx1, dy1;
        int x0, y0, x1, y1;
        if (arrent->exp==0) {
            x0=t_vx(arr->time(lidx));
            y0=t_vy(arr->val(lidx));
            //cerr<<"x0 "<<x0<<endl;
            for (int idx=lidx+1; idx<=ridx; idx++) {
                dx1=arr->time(idx); x1=t_vx(dx1);
                dy1=arr->val(idx);  y1=t_vy(dy1);
                //cerr<<"x1 "<<x1;
                if (((y0>=top) || (y1>=top)) && ((y0<=bottom) || (y1<=bottom))) {
                    //cerr<<" d";
                    XDrawLine(xdisplay, xwindow, gc, x0, y0, x0, y1);
                    XDrawLine(xdisplay, xwindow, gc, x0, y1, x1, y1);
                }
                //cerr<<endl;
                x0=x1; y0=y1;
            }
        } else {
            double scale=arrent->scale;
            x0=t_vx(arr->time(lidx));
            y0=t_vy(arr->val(lidx)*scale);
            for (int idx=lidx+1; idx<=ridx; idx++) {
                dx1=arr->time(idx); x1=t_vx(dx1);
                dy1=arr->val(idx)*scale;  y1=t_vy(dy1);
                if (((y0>=top) || (y1>=top)) && ((y0<=bottom) || (y1<=bottom))) {
                    XDrawLine(xdisplay, xwindow, gc, x0, y0, x0, y1);
                    XDrawLine(xdisplay, xwindow, gc, x0, y1, x1, y1);
                }
                x0=x1; y0=y1;
            }
        }
        } break;
    default:
        cerr << "Programmfehler in E_histowin::draw_region; drawmode="
            << arrent->drawmode << endl;
        break;
    }
}
/*****************************************************************************/
void E_histowin::draw_region(int top, int bottom, double left, double right)
{
    for (int array=0; array<arrnum; array++) { // Iteration ueber Arrays
        arrentry* arrent=arrlist[array];
        if (arrent->enabled && (arrent->arr->size()>1))
            draw_arr_region(arrent, top, bottom, left, right, 0);
    }
}
/*****************************************************************************/
void E_histowin::do_redraw()
{
    rescale();
    XClearWindow(xdisplay, xwindow);
    draw_region(0, n_height, dx0, dx1);
}
/*****************************************************************************/
void E_histowin::do_expose()
{
    double r_dx0, r_dx1;
    r_dx0=t_xv(r_x0);
    r_dx1=t_xv(r_x1);
    draw_region(r_y0, r_y1, r_dx0, r_dx1);
}
/*****************************************************************************/
void E_histowin::do_x_shift(int checked)
{
    if (shiftx==0) return;
    if (!checked) {
        if (shiftx<0) {
            if (t_xv(shiftx)<arrminx) shiftx=t_vx(arrminx);
        } else {
            if (t_xv(n_width-1+shiftx)>arrmaxx) shiftx=t_vx(arrmaxx)-n_width+1;
        }
    }
    if (shiftx==0) return;
    if (abs(shiftx)>n_width) { // redraw ist besser
        xzero-=shiftx;
        dx1=t_xv(n_width-1);
        do_redraw();
    } else {
        // shiftscale veraendert dx0 und dx1
        double olddx0=dx0, olddx1=dx1;

        shiftscale(shiftx);
        graphics_active=1;
        if (shiftx>0) { // nach links schieben
            XCopyArea(xdisplay, xwindow, xwindow, gc, shiftx, 0,
                n_width-shiftx, n_height, 0, 0);
            XClearArea(xdisplay, xwindow, n_width-shiftx, 0,
                shiftx, n_height, False);
            draw_region(0, n_height, olddx1, dx1);
        } else { // nach rechts schieben
            XCopyArea(xdisplay, xwindow, xwindow, gc, 0, 0,
                n_width+shiftx, n_height, -shiftx, 0);
            XClearArea(xdisplay, xwindow, 0, 0, -shiftx, n_height, False);
            draw_region(0, n_height, dx0, olddx0);
        }
    }
}
/*****************************************************************************/
void E_histowin::do_v_shift()
{
    if (newdx1<dx1) {
        if ((newdx1-deltax)<arrminx) newdx1=arrminx+deltax;
    } else {
        if (newdx1>arrmaxx) newdx1=arrmaxx;
    }
    if (fabs(newdx1-dx1)>deltax) { // redraw ist besser
        dx1=newdx1;
        do_redraw();
    } else {
        shiftx=(int)rint((newdx1-dx1)*df);
        do_x_shift(1);
    }
}
/*****************************************************************************/
void E_histowin::do_newdata()
{
    for (int array=0; array<arrnum; array++) { // Iteration ueber Arrays
        if (arrlist[array]->enabled && (arrlist[array]->newdata)) {
            arrlist[array]->newdata=0;
            double ll=arrlist[array]->last;
            arrlist[array]->last=arrlist[array]->arr->maxtime();
            draw_arr_region(arrlist[array], 0, n_height, ll, arrlist[array]->last, 1);
        }
    }
}
/*****************************************************************************/
void E_histowin::do_GraphicsExpose(int x, int y, int width, int height)
{
    draw_region(y, y+height, t_xv(x), t_xv(x+width));
}
/*****************************************************************************/
void E_histowin::databack(E_histoarray* arr)
{
    int size=arr->size();
    if (size>0) {
        if (xscalemode==XS_lastdata) {
            double tt=arr->maxtime();
            if (tt>dx1) {
                newdx1=tt;
                installidleproc(HW_v_shift);
            }
        }
        int i=0;
        while ((i<arrnum) && (arrlist[i]->arr!=arr)) i++;
        if (i>=arrnum) {
            cerr << "E_histowin::databack: array " << arr->name()
                << " not found" << endl;
            return;
        }
        if (size>1) {
            arrlist[i]->newdata=1;
            installidleproc(HW_newdata);
        } else {
            arrlist[i]->last=arrlist[i]->arr->time(0);
        }
    } else {
        cerr << "E_histowin::databack: empty array " << arr->name()
            << "has new data" << endl;
    }
}
/*****************************************************************************/
void E_histowin::installidleproc(ureason reason)
{
    updatereason=(ureason)(updatereason|reason);
    if (!updatepending) {
        Tk_DoWhenIdle(updateproc, (ClientData)this);
        updatepending=1;
    }
}
/*****************************************************************************/
STRING E_histowin::tclprocname(void) const
{
    STRING s;
    s=Tcl_GetCommandName(interp, tclcommand);
    return s;
}
/*****************************************************************************/
Tk_ConfigSpec E_histowin::config_specs[]= {
    {
        TK_CONFIG_PIXELS, 0, "width", "Width", "300",
        Tk_Offset(struct E_histowin::config, c_width), 0, 0
    },
    {
        TK_CONFIG_PIXELS, 0, "height", "Height", "100",
        Tk_Offset(struct E_histowin::config, c_height), 0, 0
    },
    {
        TK_CONFIG_SYNONYM, 0, "background", 0, 0, 0, 0, 0
    },
    {
        TK_CONFIG_COLOR, 0, "background", "Background", "white",
        Tk_Offset(struct E_histowin::config, c_background), 0, 0
    },
    {
        TK_CONFIG_SYNONYM, 0, "foreground", 0, 0, 0, 0, 0
    },
    {
        TK_CONFIG_STRING, 0, "foreground", "Foreground", "black",
        Tk_Offset(struct E_histowin::config, c_foreground), 0, 0
    },
    {
        TK_CONFIG_ACTIVE_CURSOR, 0, "cursor", "Cursor", "",
        Tk_Offset(struct E_histowin::config, c_cursor), TK_CONFIG_NULL_OK, 0
    },
    {
        TK_CONFIG_PIXELS, 0, "xScrollIncrement",
            "ScrollIncrement", "0",
        Tk_Offset(struct E_histowin::config, c_scrollincr), 0, 0
    },  
    {
        TK_CONFIG_COLOR, 0, "crosscolor", "Crosscolor", "black",
        Tk_Offset(struct E_histowin::config, c_crosscolor), 0, 0
    },
    {
        TK_CONFIG_END, 0, 0, 0, 0, 0, 0, 0
    }
};
const char *E_histowin::config_specnames[]= {
    "-width",
    "-height",
    "-bg",
    "-background",
    "-fg",
    "-foreground",
    "-cursor",
    "-xscrollincrement",
    "-crosscolor",
    0
};

int E_histowin::configure(int objc, Tcl_Obj* const objv[], int flags)
{
    int res, i;
    const char** argv=new const char*[objc];
    for (i=0; i<objc; i++) argv[i]=Tcl_GetString(objv[i]);
    res=Tk_ConfigureWidget(interp, window, config_specs, objc, argv,
            (char*)&config, flags);
    delete argv;
    if (res!=TCL_OK)
        return TCL_ERROR;
    Tk_GeometryRequest(window, config.c_width, config.c_height);
    Tk_SetWindowBackground(window, config.c_background->pixel);
    if (!config.c_cursor)
        Tk_DefineCursor(window, emptycursor);
    XGCValues values;
    values.foreground=config.c_background->pixel^config.c_crosscolor->pixel;
    values.line_width=0;
    values.line_style=LineSolid;
    values.cap_style=CapButt;
    XChangeGC(xdisplay, crossgc,
        GCForeground|GCLineWidth|GCLineStyle|GCCapStyle,
        &values);
    installidleproc(HW_redraw);
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_command(int objc, Tcl_Obj* const objv[])
{
    int res;
    if (objc<2) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("argument required", -1));
        res=TCL_ERROR;
    } else {
        static const char* names[]={
            "configure",
            "arrconfigure",
            "cget",
            "arrcget",
            "attacharray",
            "detacharray",
            "xscalemode",
            "xmax",
            "xmin",
            "xdiff",
            "yscalemode",
            "ymin",
            "ymax",
            "redraw",
            "delete",
            "xshift",
            "transxoffs",
            "transyoffs",
            "transxv",
            "transvx",
            "transyv",
            "transvy",
            "transexv",
            "transevx",
            "transeyv",
            "transevy",
            "xscalecommand",
            "yscalecommand",
            "arrays",
            "crosscommand",
            "firstx",
            "lastx",
            "xview",
            "scrollcommand",
            0
        };
        switch (findstring(interp, objv[1], names)) {
            case 0: res=e_configure(objc, objv); break;
            case 1: res=e_arrconfigure(objc, objv); break;
            case 2: res=e_cget(objc, objv); break;
            case 3: res=e_arrcget(objc, objv); break;
            case 4: res=e_attacharray(objc, objv); break;
            case 5: res=e_detacharray(objc, objv); break;
            case 6: res=e_xscalemode(objc, objv); break;
            case 7: res=e_xmax(objc, objv); break;
            case 8: res=e_xmin(objc, objv); break;
            case 9: res=e_xdiff(objc, objv); break;
            case 10: res=e_yscalemode(objc, objv); break;
            case 11: res=e_ymin(objc, objv); break;
            case 12: res=e_ymax(objc, objv); break;
            case 13: res=e_redraw(objc, objv); break;
            case 14: res=e_delete(objc, objv); break;
            case 15: res=e_xshift(objc, objv); break;
            case 16: res=e_transxoffs(objc, objv); break;
            case 17: res=e_transyoffs(objc, objv); break;
            case 18: res=e_transxv(objc, objv); break;
            case 19: res=e_transvx(objc, objv); break;
            case 20: res=e_transyv(objc, objv); break;
            case 21: res=e_transvy(objc, objv); break;
            case 22: res=e_transexv(objc, objv); break;
            case 23: res=e_transevx(objc, objv); break;
            case 24: res=e_transeyv(objc, objv); break;
            case 25: res=e_transevy(objc, objv); break;
            case 26: res=e_xscalecommand(objc, objv); break;
            case 27: res=e_xscalecommand(objc, objv); break;
            case 28: res=e_arrays(objc, objv); break;
            case 29: res=e_crosscommand(objc, objv); break;
            case 30: res=e_firstx(objc, objv); break;
            case 31: res=e_lastx(objc, objv); break;
            case 32: res=e_xview(objc, objv); break;
            case 33: res=e_scrollcommand(objc, objv); break;
        //    case xx: res=e_integral_start(objc, objv); break;
        //    case xx: res=e_integral_end(objc, objv); break;
            default: res=TCL_ERROR; break;
        }
    }
    return res;
}
/*****************************************************************************/
int E_histowin::e_delete(int objc, Tcl_Obj* const objv[])
{
    if (objc>2) 
        return Tcl_NoArgs(interp, 2, objv);

    Tk_DestroyWindow(window);
    //return Tcl_DeleteCommand(interp, tclprocname());
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_configure(int objc, Tcl_Obj* const objv[])
{
    int res;
    if (objc==2) {
        res=Tk_ConfigureInfo(interp, window, config_specs, (char*)&config, 0, 0);
    } else if (objc==3) {
        res=Tk_ConfigureInfo(interp, window, config_specs, (char*)&config,
                Tcl_GetString(objv[2]), 0);
    } else {
        res=configure(objc-2, objv+2, TK_CONFIG_ARGV_ONLY);
    }
    return res;
}
/*****************************************************************************/
int E_histowin::e_cget(int objc, Tcl_Obj* const objv[])
{
    int res;
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 2, objv, "name");
        res=TCL_ERROR;
    } else {
        res=Tk_ConfigureValue(interp, window, config_specs, (char*)&config,
            Tcl_GetString(objv[2]), 0);
    }
    return res;
}
/*****************************************************************************/
E_histowin::arrentry* E_histowin::findarr(const char* name)
{
    int i=0;
    while ((i<arrnum) && (arrlist[i]->arr->tclprocname()!=name)) i++;
    if (i>=arrnum) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("no such array", -1));
        return 0;
    } else {
      return arrlist[i];
    }
}
/*****************************************************************************/
E_histowin::arrentry* E_histowin::findarr(Tcl_Obj* const name)
{
    return findarr(Tcl_GetString(name));
}
/*****************************************************************************/

const char* E_histowin::plotstyles[]= {
    "lines",
    "boxes",
    0
};

Tk_CustomOption E_histowin::configstyleoption= {
    styleoption_parse,
    styleoption_print,
    (ClientData)plotstyles,
};

int E_histowin::styleoption_parse(ClientData clientData, Tcl_Interp *interp,
    Tk_Window tkwin, const char *value, char *widgRec, int offset)
{
    int res=TCL_OK;
    switch (findstring(interp, value, (const char**)clientData, 0)) {
        case 0: *(int*)(widgRec+offset)=arrentry::DM_line; break;
        case 1: *(int*)(widgRec+offset)=arrentry::DM_box; break;
        default: res=TCL_ERROR;
    }
    return res;
}

char* E_histowin::styleoption_print(ClientData clientData, Tk_Window tkwin,
    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr)
{
    return ((char**)clientData)[*(int*)(widgRec+offset)];
}

/*****************************************************************************/

int E_histowin::intranges[2]={-100, 100};

Tk_CustomOption E_histowin::configintoption= {
    intoption_parse,           /* Tk_OptionParseProc* */
    intoption_print,           /* Tk_OptionPrintProc* */
    (ClientData)intranges,
};

int E_histowin::intoption_parse(ClientData clientData, Tcl_Interp *interp,
    Tk_Window tkwin, const char *value, char *widgRec, int offset)
{
    int res=TCL_OK;
    int val;
    if (Tcl_GetInt(interp, value, &val)!=TCL_OK) return TCL_ERROR;
    if (val<((int*)clientData)[0]) {
        OSTRINGSTREAM ss;
        ss << "value has to be greater or equal than " << ((int*)clientData)[0];
        Tcl_SetResult_Stream(interp, ss);
        return TCL_ERROR;
    } else if (val>((int*)clientData)[1]) {
        OSTRINGSTREAM ss;
        ss << "value has to be less or equal than " << ((int*)clientData)[1];
        Tcl_SetResult_Stream(interp, ss);
        return TCL_ERROR;
    }
    *(int*)(widgRec+offset)=val;
    return res;
}

namespace {
void ch_free(char* p)
{
    free(p);
}
}

char* E_histowin::intoption_print(ClientData clientData, Tk_Window tkwin,
    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr)
{
    OSTRINGSTREAM ss;
    ss << *(int*)(widgRec+offset);
    STRING s=ss.str();
    *freeProcPtr=ch_free;
    return strdup(s.c_str());
}

/*****************************************************************************/

Tk_ConfigSpec E_histowin::arrconfig_specs[]= {
    {
        TK_CONFIG_COLOR, 0, "color", "Color", 0,
            Tk_Offset(E_histowin::arrentry, color), 0, 0
    },
    {
        TK_CONFIG_PIXELS, 0, "width", "Width", "0",
            Tk_Offset(E_histowin::arrentry, width), 0, 0
    },
    {   TK_CONFIG_CUSTOM, 0, "style", "Style", "boxes",
            Tk_Offset(E_histowin::arrentry, drawmode), 0, &configstyleoption
    },
//  {   TK_CONFIG_DOUBLE, 0, "scale", "Scale", "1",
//          Tk_Offset(E_histowin::arrentry, scale), 0, 0
//  },
    {   TK_CONFIG_CUSTOM, 0, "exp", "Exp", "0",
            Tk_Offset(E_histowin::arrentry, exp), 0, &configintoption
    },
    {   TK_CONFIG_BOOLEAN, 0, "enabled", "Enabled", "1",
            Tk_Offset(E_histowin::arrentry, enabled), 0, 0
    },
    {
        TK_CONFIG_END, 0, 0, 0, 0, 0, 0, 0
    }
};

const char *E_histowin::arrconfig_specnames[]= {
    "-color",
    "-width",
    "-style",
    //"-scale",
    "-exp",
    "-enabled",
    0
};

int E_histowin::arrconfigure(E_histowin::arrentry* arrent, int objc,
    Tcl_Obj* const objv[], int flags)
{
    int i, res;
    arrconfig_specs[0].defValue=Tk_GetUid(config.c_foreground);
    const char** argv=new const char*[objc];
    for (i=0; i<objc; i++) argv[i]=Tcl_GetString(objv[i]);
    res=Tk_ConfigureWidget(interp, window, arrconfig_specs, objc, argv,
            (char*)arrent, flags);
    delete argv;
    if (res!=TCL_OK)
        return TCL_ERROR;
    XGCValues values;
    values.foreground=arrent->color->pixel;
    values.line_width=arrent->width;
    values.line_style=LineSolid;
    values.cap_style=CapRound;
    XChangeGC(xdisplay, arrent->gc,
        GCForeground|GCLineWidth|GCLineStyle|GCCapStyle, &values);
    arrent->scale=1.0;
    if (arrent->exp<0) {
        for (i=0; i>arrent->exp; i--) arrent->scale/=10.0;
    } else if (arrent->exp>0) {
        for (i=0; i<arrent->exp; i++) arrent->scale*=10.0;
    }
    if (arrent->arr->size()>1)
        installidleproc(HW_redraw);
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_arrconfigure(int objc, Tcl_Obj* const objv[])
{
    // <WIN> arrconfigure arrname ...
    if (objc<3) {
        Tcl_WrongNumArgs(interp, 2, objv, "arrname");
        return TCL_ERROR;
    }
    arrentry* arrent=findarr(objv[2]);
    if (arrent==0)
        return TCL_ERROR;
    int res;
    if (objc==3) {
        res=Tk_ConfigureInfo(interp, window, arrconfig_specs, (char*)arrent,
                0, 0);
    } else if (objc==4) {
        res=Tk_ConfigureInfo(interp, window, arrconfig_specs, (char*)arrent,
                Tcl_GetString(objv[3]), 0);
    } else {
        res=arrconfigure(arrent, objc-3, objv+3, TK_CONFIG_ARGV_ONLY);
    }
    return res;
}
/*****************************************************************************/
int E_histowin::e_arrcget(int objc, Tcl_Obj* const objv[])
{
    // <WIN> arrcget arrname name
    if (objc!=4) {
        Tcl_WrongNumArgs(interp, 2, objv, "arrname name");
        return TCL_ERROR;
    }
    arrentry* arrent=findarr(objv[2]);
    if (arrent==0)
        return TCL_ERROR;
    int res;
    res=Tk_ConfigureValue(interp, window, arrconfig_specs, (char*)arrent,
        Tcl_GetString(objv[3]), 0);
    return res;
}
/*****************************************************************************/
int E_histowin::e_attacharray(int objc, Tcl_Obj* const objv[])
{
    // <WIN> attacharray name ...
    if (objc<3) {
        Tcl_WrongNumArgs(interp, 2, objv, "name ...");
        return TCL_ERROR;
    }

    Tcl_CmdInfo info;
    int res;
    char* argv2=Tcl_GetString(objv[2]);
    if (!Tcl_GetCommandInfo(interp, argv2, &info)
            || !arrays->test((E_histoarray*)info.objClientData)) {
        Tcl_AppendResult(interp, argv2, " is not a valid historyarray",
                (char*)0);
        res=TCL_ERROR;
    } else {
        res=attacharray((E_histoarray*)info.objClientData, objc-3, objv+3);
    }
    return res;
}
/*****************************************************************************/
int E_histowin::attacharray(E_histoarray* arr, int objc, Tcl_Obj* const objv[])
{
    if (arrayattached(arr)>=0) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("array is already attached", -1));
        return TCL_ERROR;
    }
    arrentry* newentry=new arrentry;
    newentry->arr=arr;
    newentry->newdata=0;
    newentry->drawmode=arrentry::DM_box;
    newentry->color=0;
    newentry->gc=XCreateGC(xdisplay, xwindow, 0, 0);
    if (arr->size()>0) {
        double xx0, xx1;
        arr->getmaxxvals(xx0, xx1);
        newentry->last=xx1;
    }
    int res=arrconfigure(newentry, objc, objv, 0);
    if (res!=TCL_OK) {
        XFreeGC(xdisplay, newentry->gc);
        Tk_FreeOptions(arrconfig_specs, (char*)newentry, xdisplay, 0);
        delete newentry;
        return TCL_ERROR;
    }
    newentry->arr->installdatacallback(datacallback, (ClientData)this);
    newentry->arr->installrangecallback(rangecallback, (ClientData)this);
    newentry->arr->installdeletecallback(deletecallback, (ClientData)this);

    arrentry** help=new arrentry*[arrnum+1];
    for (int i=0; i<arrnum; i++)
        help[i]=arrlist[i];
    delete[] arrlist;
    arrlist=help;
    arrlist[arrnum]=newentry;
    arrnum++;
    updatemaxx();
    if (newentry->enabled)
        installidleproc(HW_redraw);
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_detacharray(int objc, Tcl_Obj* const objv[])
{
    // <WIN> detacharray name
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "name");
        return TCL_ERROR;
    }
    Tcl_CmdInfo info;
    char* argv2=Tcl_GetString(objv[2]);  
    if (!Tcl_GetCommandInfo(interp, argv2, &info)
            || !arrays->test((E_histoarray*)info.objClientData)) {
        Tcl_AppendResult(interp, argv2, " is not a valid historyarray",
            (char*)0);
        return TCL_ERROR;
    }
    return detacharray((E_histoarray*)info.objClientData);
}
/*****************************************************************************/
int E_histowin::detacharray(E_histoarray* arr)
{
    int idx, enabled;
    if ((idx=arrayattached(arr))<0) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("array is not attached", -1));
        return TCL_ERROR;
    }
    enabled=arrlist[idx]->enabled;
    XFreeGC(xdisplay, arrlist[idx]->gc);
    Tk_FreeOptions(arrconfig_specs, (char*)arrlist[idx], xdisplay, 0);
    arrlist[idx]->arr->deletedatacallback((ClientData)this);
    arrlist[idx]->arr->deletedeletecallback((ClientData)this);
    delete arrlist[idx];
    for (int i=idx; i<arrnum; i++)
        arrlist[i]=arrlist[i+1];
    arrnum--;
    updatemaxx();
    if (enabled)
        installidleproc(HW_redraw);
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::arrayattached(E_histoarray* arr)
{
    if (!arrnum)
        return -1;
    int i;
    for (i=0; i<arrnum; i++) {
        if (arrlist[i]->arr==arr)
            return i;
    }
    return -1;
}
/*****************************************************************************/
void E_histowin::detachallarrays()
{
    for (int i=0; i<arrnum; i++) {
        XFreeGC(xdisplay, arrlist[i]->gc);
        Tk_FreeOptions(arrconfig_specs, (char*)arrlist[i], xdisplay, 0);
        arrlist[i]->arr->deletedatacallback((ClientData)this);
        arrlist[i]->arr->deletedeletecallback((ClientData)this);
        delete arrlist[i];
    }
    arrnum=0;
}
/*****************************************************************************/
int E_histowin::e_arrays(int objc, Tcl_Obj* const objv[])
{
    // <WIN> arrays
    if (objc!=2) {
        Tcl_WrongNumArgs(interp, 1, objv, "arrays");
        return TCL_ERROR;
    }
    for (int i=0; i<arrnum; i++) {
        Tcl_AppendElement(interp,
                (char*)(arrlist[i]->arr->tclprocname().c_str()));
    }
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_xscalemode(int objc, Tcl_Obj* const objv[])
{
    // <WIN> xscalemode (localtime| lastdata| constant)
    if ((objc<2) || (objc>3)) {
        Tcl_WrongNumArgs(interp, 1, objv, "[mode]");
        return TCL_ERROR;
    }
    static const char* names[]={
        "localtime",
        "lastdata",
        "constant",
        0
    };
    if (objc==2) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(names[xscalemode], -1));
      return TCL_OK;
    }
    switch (findstring(interp, objv[2], names)) {
        case 0: xscalemode=XS_localtime; break;
        case 1: xscalemode=XS_lastdata; break;
        case 2: xscalemode=XS_constant; break;
        default: return TCL_ERROR; break;
    }
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_yscalemode(int objc, Tcl_Obj* const objv[])
{
    // <WIN> yscalemode (auto| constant)
    if ((objc<2) || (objc>3)) {
        Tcl_WrongNumArgs(interp, 1, objv, "[mode]");
        return TCL_ERROR;
    }
    static const char* names[]={
        "auto",
        "constant",
        0
    };
    if (objc==2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(names[yscalemode], -1));
        return TCL_OK;
    }
    switch (findstring(interp, objv[2], names)) {
        case 0: yscalemode=YS_auto; break;
        case 1: yscalemode=YS_constant; break;
        default: return TCL_ERROR; break;
    }
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_xshift(int objc, Tcl_Obj* const objv[])
{
    // <WIN> xshift value
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "value");
        return TCL_ERROR;
    }
    int val;
    if (Tcl_GetIntFromObj(interp, objv[2], &val)!=TCL_OK)
        return TCL_ERROR;
    xshift(val);
    return TCL_OK;
}
/*****************************************************************************/
void E_histowin::xshift(int d)
{
    if (updatereason&HW_x_shift)
        shiftx+=d;
    else
        shiftx=d;
    installidleproc(HW_x_shift);
}
/*****************************************************************************/
void E_histowin::set_x0(double v)
{
    deltax=dx1-v;
    installidleproc(HW_redraw);
}
/*****************************************************************************/
void E_histowin::set_x1(double v)
{
    newdx1=v;
    installidleproc(HW_v_shift);
}
/*****************************************************************************/
void E_histowin::set_y0(double v)
{
    dy0=v;
    installidleproc(HW_redraw);
}
/*****************************************************************************/
void E_histowin::set_y1(double v)
{
    dy1=v;
    installidleproc(HW_redraw);
}
/*****************************************************************************/
void E_histowin::set_xdiff(double v)
{
    deltax=v;
    installidleproc(HW_redraw);
}
/*****************************************************************************/
int E_histowin::e_xdiff(int objc, Tcl_Obj* const objv[])
{
    // <WIN> xdiff [value]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[value]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetObjResult(interp, Tcl_NewDoubleObj(deltax));
        return TCL_OK;
     }

    int val;
    if (Tcl_GetIntFromObj(interp, objv[2], &val)!=TCL_OK)
        return TCL_ERROR;
    if (val<=0) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("xdiff has to be greater 0", -1));
        return TCL_ERROR;
    }
    set_xdiff(val);
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_xmin(int objc, Tcl_Obj* const objv[])
{
    // <WIN> xmin [value]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[value]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetObjResult(interp, Tcl_NewDoubleObj(dx0));
        return TCL_OK;
    }

    int val;
    if (Tcl_GetIntFromObj(interp, objv[2], &val)!=TCL_OK)
        return TCL_ERROR;
    if (val>=dx1) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("xmin has to be less than xmax", -1));
        return TCL_ERROR;
    }
    set_x0(val);
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_xmax(int objc, Tcl_Obj* const objv[])
{
    // <WIN> xmax [value]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[value]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetObjResult(interp, Tcl_NewDoubleObj(dx1));
        return TCL_OK;
    }

    int val;
    if (Tcl_GetIntFromObj(interp, objv[2], &val)!=TCL_OK)
        return TCL_ERROR;
    if (val<=0) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("xmax has to be greater than 0", -1));
        return TCL_ERROR;
    }
    set_x1(val);
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_ymin(int objc, Tcl_Obj* const objv[])
{
    // <WIN> ymin [value]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[value]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetObjResult(interp, Tcl_NewDoubleObj(dy0));
        return TCL_OK;
    }
    int val;
    if (Tcl_GetIntFromObj(interp, objv[2], &val)!=TCL_OK)
        return TCL_ERROR;
    if (val>=dy1) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("ymin has to be less than ymax", -1));
        return TCL_ERROR;
    }
    set_y0(val);
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_ymax(int objc, Tcl_Obj* const objv[])
{
    // <WIN> ymax [value]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[value]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetObjResult(interp, Tcl_NewDoubleObj(dy1));
        return TCL_OK;
    }
    int val;
    if (Tcl_GetIntFromObj(interp, objv[2], &val)!=TCL_OK)
        return TCL_ERROR;
    if (val<=dy0) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("ymax has to be greater than ymin", -1));
        return TCL_ERROR;
    }
    set_y1(val);
    return TCL_OK;
}
/*****************************************************************************/
double E_histowin::firstx()
{
    int num=0;
    double x=0;
    for (int array=0; array<arrnum; array++) { // Iteration ueber Arrays
        E_histoarray* arr=arrlist[array]->arr;
        if (arr->size()>0) {
            double x0, x1;
            arr->getmaxxvals(x0, x1);
            if (num++) {
                if (x0<x) x=x0;
            } else {
                x=x0;
            }
        }
    }
    return x;
}
/*****************************************************************************/
int E_histowin::e_firstx(int objc, Tcl_Obj* const objv[])
{
    // <WIN> firstx [i|d]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[i|d]");
        return TCL_ERROR;
    }
    double x=firstx();
    int i=(int)rint(x);
    if (objc>2) {
        char* argv2=Tcl_GetString(objv[2]);
        if (argv2[0]=='i') {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(i));
        } else if (argv2[0]=='d') {
            Tcl_SetObjResult(interp, Tcl_NewDoubleObj(x));
        } else {
            Tcl_AppendResult(interp, "wrong argument ", Tcl_GetString(objv[2]),
                "; must be 'i' or 'd'", 0);
            return TCL_ERROR;
        }
    } else {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(i));
    }
    return TCL_OK;
}
/*****************************************************************************/
double E_histowin::lastx()
{
    int num=0;
    double x=1;
    for (int array=0; array<arrnum; array++) { // Iteration ueber Arrays
        E_histoarray* arr=arrlist[array]->arr;
        if (arr->size()>0) {
            double x0, x1;
            arr->getmaxxvals(x0, x1);
            if (num++) {
                if (x1>x) x=x1;
            } else {
                x=x1;
            }
        }
    }
    return x;
}
/*****************************************************************************/
int E_histowin::e_lastx(int objc, Tcl_Obj* const objv[])
{
    // <WIN> lastx [i|d]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[i|d]");
        return TCL_ERROR;
    }
    double x=lastx();
    int i=(int)rint(x);
    if (objc>2) {
        char* argv2=Tcl_GetString(objv[2]);
        if (argv2[0]=='i') {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(i));
        } else if (argv2[0]=='d') {
            Tcl_SetObjResult(interp, Tcl_NewDoubleObj(x));
        } else {
            Tcl_AppendResult(interp, "wrong argument ", Tcl_GetString(objv[2]),
                "; must be 'i' or 'd'", 0);
            return TCL_ERROR;
        }
    } else {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(i));
    }
    return TCL_OK;
}
/*****************************************************************************/
void E_histowin::updatemaxx()
{
    int num=0;
    arrminx=dx0;
    arrmaxx=dx1;
    for (int array=0; array<arrnum; array++) { // Iteration ueber Arrays
        E_histoarray* arr=arrlist[array]->arr;
        if (arr->size()>0) {
            double xx0, xx1;
            arr->getmaxxvals(xx0, xx1);
            if (num++) {
                if (xx0<arrminx) arrminx=xx0;
                if (xx1>arrmaxx) arrmaxx=xx1;
            } else {
                arrminx=xx0;
                arrmaxx=xx1;
            }
        }
    }
}
/*****************************************************************************/
int E_histowin::e_xview(int objc, Tcl_Obj* const objv[])
{
    // <WIN> xview (moveto fraction)|(scroll number units)|(scroll number pages)
    if (objc<4) {
        Tcl_WrongNumArgs(interp, 1, objv,
            "(moveto fraction)|(scroll number units)|(scroll number pages)");
        return TCL_ERROR;
    }
    char* argv2=Tcl_GetString(objv[2]);
    if (strcmp(argv2, "moveto")==0) {
        double v;
        if (Tcl_GetDoubleFromObj(interp, objv[3], &v)!=TCL_OK)
            return TCL_ERROR;
        double diff=arrmaxx-arrminx;
        newdx1=diff*v+arrminx+deltax;
        installidleproc(HW_v_shift);
    } else if (strcmp(argv2, "scroll")==0) {
        char* argv4=Tcl_GetString(objv[4]);
        int v;
        if (Tcl_GetIntFromObj(interp, objv[3], &v)!=TCL_OK)
            return TCL_ERROR;
        if (strcmp(argv4, "units")==0) {
            xshift(v);
        } else if (strcmp(argv4, "pages")==0) {
            xshift((int)rint(0.9*n_width*v));
        } else {
            Tcl_SetObjResult(interp,
                Tcl_NewStringObj("wrong argument; must be 'units' or 'pages'",
                -1));
            return TCL_ERROR;
        }
    } else {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong argument; must be 'moveto' or 'scroll'", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_redraw(int objc, Tcl_Obj* const objv[])
{
    // <WIN> redraw
    if (objc!=2) {
        Tcl_WrongNumArgs(interp, 1, objv, "redraw");
        return TCL_ERROR;
    }
    installidleproc(HW_redraw);
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transxoffs(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transxoffs [val]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[value]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(xoffs));
        return TCL_OK;
    }

    int val;
    if (Tcl_GetIntFromObj(interp, objv[2], &val)!=TCL_OK) return TCL_ERROR;
    xoffs=val;
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transyoffs(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transyoffs [val]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[value]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(yoffs));
        return TCL_OK;
    }

    int val;
    if (Tcl_GetIntFromObj(interp, objv[2], &val)!=TCL_OK) return TCL_ERROR;
    yoffs=val;
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transxv(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transxv val
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "value");
        return TCL_ERROR;
    }
    double v;
    int x;
    if (Tcl_GetIntFromObj(interp, objv[2], &x)!=TCL_OK) return TCL_ERROR;
    v=t_xv(x);
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(v));
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transvx(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transvx val
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "value");
        return TCL_ERROR;
    }
    double v;
    int x;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &v)!=TCL_OK) return TCL_ERROR;
    x=t_vx(v);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(x));
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transyv(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transyv val
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "value");
        return TCL_ERROR;
    }
    double v;
    int x;
    if (Tcl_GetIntFromObj(interp, objv[2], &x)!=TCL_OK) return TCL_ERROR;
    v=t_yv(x);
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(v));
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transvy(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transvy val
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "value");
        return TCL_ERROR;
    }
    double v;
    int x;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &v)!=TCL_OK) return TCL_ERROR;
    x=t_vy(v);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(x));
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transexv(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transxv val
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "value");
        return TCL_ERROR;
    }
    double v;
    int x;
    if (Tcl_GetIntFromObj(interp, objv[2], &x)!=TCL_OK) return TCL_ERROR;
    v=t_xv(x-xoffs);
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(v));
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transevx(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transvx val
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "value");
        return TCL_ERROR;
    }
    double v;
    int x;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &v)!=TCL_OK) return TCL_ERROR;
    x=t_vx(v)+xoffs;
    Tcl_SetObjResult(interp, Tcl_NewIntObj(x));
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transeyv(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transyv val
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "value");
        return TCL_ERROR;
    }
    double v;
    int x;
    if (Tcl_GetIntFromObj(interp, objv[2], &x)!=TCL_OK) return TCL_ERROR;
    v=t_yv(x-yoffs);
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(v));
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_transevy(int objc, Tcl_Obj* const objv[])
{
    // <WIN> transvy val
    if (objc!=3) {
        Tcl_WrongNumArgs(interp, 1, objv, "value");
        return TCL_ERROR;
    }
    double v;
    int x;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &v)!=TCL_OK) return TCL_ERROR;
    x=t_vy(v)+yoffs;
    Tcl_SetObjResult(interp, Tcl_NewIntObj(x));
    return TCL_OK;
}
/*****************************************************************************/
int E_histowin::e_xscalecommand(int objc, Tcl_Obj* const objv[])
{
    // <WIN> xscalecommand [command]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[command]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetResult_String(interp, xscalecommand);
        return TCL_OK;
    } else {
        xscalecommand=Tcl_GetString(objv[2]);
        return TCL_OK;
    }
}
/*****************************************************************************/
int E_histowin::e_yscalecommand(int objc, Tcl_Obj* const objv[])
{
    // <WIN> yscalecommand [command]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[command]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetResult_String(interp, yscalecommand);
        return TCL_OK;
    } else {
        yscalecommand=Tcl_GetString(objv[2]);
        return TCL_OK;
    }
}
/*****************************************************************************/
int E_histowin::e_crosscommand(int objc, Tcl_Obj* const objv[])
{
    // <WIN> crosscommand [command]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 2, objv, "[command]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetResult_String(interp, crosscommand);
        return TCL_OK;
    } else {
        crosscommand=Tcl_GetString(objv[2]);
        return TCL_OK;
    }
}
/*****************************************************************************/
int E_histowin::e_scrollcommand(int objc, Tcl_Obj* const objv[])
{
    // <WIN> scrollcommand [command]
    if (objc>3) {
        Tcl_WrongNumArgs(interp, 1, objv, "[command]");
        return TCL_ERROR;
    }
    if (objc==2) {
        Tcl_SetResult_String(interp, scrollcommand);
        return TCL_OK;
    } else {
        scrollcommand=Tcl_GetString(objv[2]);
        return TCL_OK;
    }
}
/*****************************************************************************/
void E_histowin::eventhandler(ClientData clientdata, XEvent* event)
{
    ((E_histowin*)clientdata)->e_event(event);
}
/*****************************************************************************/
void E_histowin::e_event(XEvent* event)
{
    switch (event->type) {
    case KeyPress         : cerr << "KeyPress" << endl; break;
    case KeyRelease       : cerr << "KeyRelease" << endl; break;
    case ButtonPress      : cerr << "ButtonPress" << endl; break;
    case ButtonRelease    : cerr << "ButtonRelease" << endl; break;
    case MotionNotify     : {
        XMotionEvent* ev=(XMotionEvent*)event;
        pointer_move(ev->x, ev->y);
        }
        break;
    case EnterNotify      : { //cerr << "EnterNotify" << endl;
        XCrossingEvent* ev=(XCrossingEvent*)event;
        pointer_enter(ev->x, ev->y);
        }
        break;
    case LeaveNotify      : //cerr << "LeaveNotify" << endl;
        pointer_leave();
        break;
    case Expose           : {
        XExposeEvent* ev=(XExposeEvent*)event;
        //cerr << "ex: x=" << ev->x << "; y=" << ev->y;
        //cerr << "; width=" << ev->width << "; height=" << ev->height;
        //cerr << "; count=" << ev->count << endl;
        //cerr << "exp: " <<ev->x<<'+'<<ev->width<<'x'<<ev->y<<'+'<<ev->height<<endl;
        if (updatereason & HW_expose) {
            int x0, x1, y0, y1;
            x0=ev->x;
            x1=ev->x+ev->width-1;
            y0=ev->y;
            y1=ev->y+ev->height-1;
            if (x0<r_x0) r_x0=x0;
            if (x1>r_x1) r_x1=x1;
            if (y0<r_y0) r_y0=y0;
            if (y1>r_y1) r_y1=y1;
        } else {
            r_x0=ev->x;
            r_x1=ev->x+ev->width-1;
            r_y0=ev->y;
            r_y1=ev->y+ev->height-1;
        }
        installidleproc(HW_expose);
        }
        break;
    case GraphicsExpose   : {
        XGraphicsExposeEvent* ev=(XGraphicsExposeEvent*)event;
        //cerr << "gex: x=" << ev->x << "; y=" << ev->y;
        //cerr << "; width=" << ev->width << "; height=" << ev->height;
        //cerr << "; count=" << ev->count << endl;
        do_GraphicsExpose(ev->x, ev->y, ev->width, ev->height);
        if (ev->count==0) {
            graphics_active=0;
            cross_show();
        }
        }
        break;
    case NoExpose         :
        graphics_active=0;
        cross_show();
        break;
    case VisibilityNotify : {
        XVisibilityEvent* ev=(XVisibilityEvent*)event;
        visibility=ev->state;
        }
        break;
    case CreateNotify     : cerr << "CreateNotify" << endl; break;
    case DestroyNotify    :
        Tcl_DeleteCommand(interp, (char*)tclprocname().c_str());
        break;
    case UnmapNotify      : //cerr << "UnmapNotify" << endl;
        mapped=0;
        break;
    case MapNotify        : //cerr << "MapNotify" << endl;
        mapped=1;
        break;
    case MapRequest       : cerr << "MapRequest" << endl; break;
    case ReparentNotify   : cerr << "ReparentNotify" << endl; break;
    case ConfigureNotify  : {
        XConfigureEvent* ev=(XConfigureEvent*)event;
//         cerr<<"ConfigureEvent: ev.width="<<ev->width<<"; ev.height="
//             <<ev->height<<endl;
//         cerr<<"                 n_width="<<  n_width<<";  n_height="
//             <<  n_height<<endl;
//         cerr<<"                 c_width="<<config.c_width<<";  c_height="
//             <<config.c_height<<endl;
        int width, height;
        width=ev->width>0?ev->width:1;
        height=ev->height>0?ev->height:1;
        if ((width!=n_width) || (height!=n_height)) {
            installidleproc(HW_redraw);
            n_width=width;
            n_height=height;
        }
        }
        break;
    case ConfigureRequest : cerr << "ConfigureRequest" << endl; break;
    case ResizeRequest    : cerr << "ResizeRequest" << endl; break;
    case ClientMessage    : cerr << "ClientMessage" << endl; break;
    case MappingNotify    : cerr << "MappingNotify" << endl; break;
    }
}
/*****************************************************************************/
void E_histowin::datacallback(E_histoarray* arr, void* data)
{
    ((E_histowin*)data)->databack(arr);
}
/*****************************************************************************/
void E_histowin::rangecallback(E_histoarray* arr, void* data)
{
    ((E_histowin*)data)->rangeback(arr);
}
/*****************************************************************************/
void E_histowin::rangeback(E_histoarray* arr)
{
    updatemaxx();
}
/*****************************************************************************/
void E_histowin::deletecallback(E_histoarray* arr, void* data)
{
    ((E_histowin*)data)->deleteback(arr);
}
/*****************************************************************************/
void E_histowin::deleteback(E_histoarray* arr)
{
    detacharray(arr);
}
/*****************************************************************************/
/*****************************************************************************/
