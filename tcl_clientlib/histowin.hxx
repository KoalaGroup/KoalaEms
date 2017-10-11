/*
 * tcl_clientlib/histowin.hxx
 * 
 * $ZEL: histowin.hxx,v 1.12 2014/07/16 16:54:28 wuestner Exp $
 * 
 * created 19.02.96
 */

#ifndef _histowin_hxx_
#define _histowin_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <tk.h>
#include "newhistoarr.hxx"

/*
 * special solution for different definitions of Tk_ConfigSpec
 * 
 * the definition of member argvName changed for tk8.5 to tk8.6
 * in tk8.5 it is nonconstant, therefore it can not be initialised
 * in tk8.6 it is constant (CONST86), therefore it has to be initialised
 * the value is never modified, so a const_cast is safe. 
 */
#ifndef CONST86
#define NCC86(v) const_cast<char*>(v) // NCC: Non Constant Character
#define CONST86
#else
#define NCC86(v) v
#endif

using namespace std;

/*****************************************************************************/

class E_histowin {
  public:
    struct inithints {
        C_histoarrays* harrs;
        Tk_Window mainwin;
        static void destroy(ClientData, Tcl_Interp*);
    };
    E_histowin(Tcl_Interp*, ClientData, int, Tcl_Obj* const[]);
    ~E_histowin();

    string tclprocname() const;
    string origtclprocname() const {return winpath;}

    struct arrentry {
        E_histoarray* arr;
        GC gc;
        enum {DM_line, DM_box} drawmode;
        double last;
        int newdata;
        int width;
        int enabled;
        double scale;
        int exp;
        XColor* color;
    };

  protected:
    enum ureason {HW_none=0, HW_redraw=1, HW_x_shift=2, HW_v_shift=4,
        HW_expose=8, HW_newdata=16};
    enum crreason {CR_none=0, CR_create=1, CR_move=2, CR_delete=4};
    enum {XS_localtime, XS_lastdata, XS_constant} xscalemode;
    enum {YS_auto, YS_constant} yscalemode;

    struct config {
        int       c_width;
        int       c_height;
        XColor*   c_background;
        char*     c_foreground;
        Tk_Cursor c_cursor;
        int       c_scrollincr;
        XColor*   c_crosscolor;
    };

    static Tk_ConfigSpec config_specs[];
    static Tk_ConfigSpec arrconfig_specs[];
    static const char* plotstyles[];
    static int intranges[2];
    static Tk_CustomOption configstyleoption;
    static Tk_CustomOption configintoption;

    Tcl_Interp* interp;
    Tcl_Command tclcommand;
    string winpath;
    Tk_Window window;
    Window xwindow;
    Display* xdisplay;
    Screen* xScreen;
    int xscreen;
    GC gc, crossgc;

    struct config config;
    int graphics_active;
    int cross_active;
    int cross_hidden;
    ureason updatereason;
    int updatepending;
    crreason crossreason;
    int crosspending;
    int visibility;
    int mapped;

    // size from ConfigureNotify
    int n_width, n_height;
    // neu zu zeichnende Flaeche fuer HW_expose
    int r_x0, r_x1, r_y0, r_y1;
    // nur zum debuggen
    string zeit(double);
    double anfang;
    // dargestellter Bereich
    double dy0, dy1;
    double dx1, deltax; // primaere Werte fuer Zeitachse
    double dx0;         // der Bequemlichkeit halber
    double df;          // Faktor fuer Zeitachse; delta_x/delta_dx
    double newdx1;      // neues dx1 nach v_shift
    double dxbase;
    int xzero;          // Konstante fuer Zeitachse
    int shiftx;         // Verschiebung der Zeitachse bei x_shift
    int xoffs, yoffs;   // offsets fuer umschliessendes Window
    // Koordinatentransformationen
    int t_vx(double) const;
    double t_xv(int) const;
    int t_vy(double) const;
    double t_yv(int) const;
    double max_x0, max_y0, max_x1, max_y1;
    double arrminx, arrmaxx;
    int cross_x, cross_y;
    Tk_Cursor emptycursor;
    C_histoarrays* arrays;
    arrentry** arrlist;
    int arrnum;
    string xscalecommand;
    string yscalecommand;
    string crosscommand;
    string scrollcommand;

    static void eventhandler(ClientData, XEvent*);
    static void datacallback(E_histoarray*, ClientData);
    static void rangecallback(E_histoarray*, ClientData);
    static void deletecallback(E_histoarray*, ClientData);
    static int styleoption_parse(ClientData, Tcl_Interp*, Tk_Window,
        CONST84 char*, char*, int);
    static CONST86 char* styleoption_print(ClientData, Tk_Window, char*, int,
        Tcl_FreeProc**);
    static int intoption_parse(ClientData, Tcl_Interp*, Tk_Window,
        CONST84 char*, char*, int);
    static CONST86 char* intoption_print(ClientData, Tk_Window, char*, int,
        Tcl_FreeProc**);

    void initscaling();

    void databack(E_histoarray*);
    void rangeback(E_histoarray*);
    void deleteback(E_histoarray*);
    int configure(int, Tcl_Obj* const[], int);
    int arrconfigure(arrentry*, int, Tcl_Obj* const[], int);

    arrentry* findarr(const char*);
    arrentry* findarr(Tcl_Obj*const);
    void installidleproc(enum ureason);
    static void updateproc(ClientData);
    void update();
    static void updatecrossproc(ClientData);
    void update_cross();
    void draw_region(int, int, double, double);
    void draw_arr_region(arrentry*, int, int, double, double, int);
    void do_redraw();
    void do_expose();
    void do_v_shift();
    void do_x_shift(int);
    void do_newdata();
    void do_GraphicsExpose(int, int, int, int);
//    int getmaxxvals(void);
//    int getmaxyvals(double, double, double);
    void doxscalecommand();
    void doscrollcommand();
    void docrosscommand();
    void rescale();
    void shiftscale(int);
    int needredraw();
    void xshift(int);
    void set_x0(double);
    void set_x1(double);
    void set_y0(double);
    void set_y1(double);
    void set_xdiff(double);
    int attacharray(E_histoarray*, int, Tcl_Obj* const[]);
    int detacharray(E_histoarray*);
    void detachallarrays();
    int arrayattached(E_histoarray*);
    void cross_create(int, int);
    void cross_move(int, int);
    void cross_delete();
    void cross_hide();
    void cross_show();
    void pointer_enter(int, int);
    void pointer_move(int, int);
    void pointer_leave();
    double firstx();
    double lastx();
    void updatemaxx();
    void e_event(XEvent*);
    int e_command(int, Tcl_Obj* const[]);
    int e_delete(int, Tcl_Obj* const[]);
    int e_configure(int, Tcl_Obj* const[]);
    int e_arrconfigure(int, Tcl_Obj* const[]);
    int e_cget(int, Tcl_Obj* const[]);
    int e_arrcget(int, Tcl_Obj* const[]);
    int e_attacharray(int, Tcl_Obj* const[]);
    int e_detacharray(int, Tcl_Obj* const[]);
    int e_arrays(int, Tcl_Obj* const[]);
    int e_xscalemode(int, Tcl_Obj* const[]);
    int e_xmax(int, Tcl_Obj* const[]);
    int e_xmin(int, Tcl_Obj* const[]);
    int e_xdiff(int, Tcl_Obj* const[]);
    int e_xshift(int, Tcl_Obj* const[]);
    int e_yscalemode(int, Tcl_Obj* const[]);
    int e_ymin(int, Tcl_Obj* const[]);
    int e_ymax(int, Tcl_Obj* const[]);
    int e_redraw(int, Tcl_Obj* const[]);
    int e_transxoffs(int, Tcl_Obj* const[]);
    int e_transyoffs(int, Tcl_Obj* const[]);
    int e_transxv(int, Tcl_Obj* const[]);
    int e_transvx(int, Tcl_Obj* const[]);
    int e_transyv(int, Tcl_Obj* const[]);
    int e_transvy(int, Tcl_Obj* const[]);
    int e_transexv(int, Tcl_Obj* const[]);
    int e_transevx(int, Tcl_Obj* const[]);
    int e_transeyv(int, Tcl_Obj* const[]);
    int e_transevy(int, Tcl_Obj* const[]);
    int e_xscalecommand(int, Tcl_Obj* const[]);
    int e_yscalecommand(int, Tcl_Obj* const[]);
    int e_crosscommand(int, Tcl_Obj* const[]);
    int e_scrollcommand(int, Tcl_Obj* const[]);
    int e_firstx(int, Tcl_Obj* const[]);
    int e_lastx(int, Tcl_Obj* const[]);
    int e_diffx(int, Tcl_Obj* const[]);
    int e_xview(int, Tcl_Obj* const[]);

 public:
    static int create(ClientData, Tcl_Interp*, int, Tcl_Obj* const[]);
    static void destroy(ClientData);
    static int command(ClientData, Tcl_Interp*, int, Tcl_Obj* const[]);
};

#endif

/*****************************************************************************/
/*****************************************************************************/
