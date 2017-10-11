/*
 * emstcl.cc
 * created 01.09.95
 */

#include <tcl.h>

#ifdef USE_TK
#include <tk.h>
#endif

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <proc_communicator.hxx>
#include <proc_veds.hxx>
#include <proc_is.hxx>
#include "emstcl_ved.hxx"
#include <errors.hxx>
#include "emstcl.hxx"
#include "findstring.hxx"
#include "ems_xdr.hxx"
#include <errorcodes.h>
#include <conststrings.h>
#include <reqstrings.h>
#include <clientcomm.h>
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: emstcl.cc,v 1.32 2014/07/14 15:13:25 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

class globalcomm {
  public:
    globalcomm();
    Tcl_Interp* interp;
    string unsolcomm[NrOfUnsolmsg];
    string unknowncomm;
    int mtestpending;
//  Tcl_File commpath;
    int commpath;
};

globalcomm::globalcomm()
{
    for (int i=0; i<NrOfUnsolmsg; i++) unsolcomm[i]="";
    mtestpending=0;
}

globalcomm global;

/******************************************************************************
*
* Ems_connect nimmt Kontakt zu einem Kommunikationsprozess auf.
*
* argv[1]: Socktname bzw. Hostname
* argv[2]: Portnummer
*
* Wenn eine globale Tcl-Variable ems_nocount definiert ist und einen Wert
* ungleich 0 enthaelt, wird dieser Client als Debug-Client betrachtet.
*/
static int Ems_connect(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    Tcl_Obj* var;
    int ivar;

    policies=pol_none;

    var=Tcl_GetVar2Ex(interp, "ems_nodebug", 0, TCL_GLOBAL_ONLY);
    if (var && (Tcl_GetIntFromObj(interp, var, &ivar)==TCL_OK) && (ivar!=0))
        policies=(en_policies)(policies|pol_nodebug);

    var=Tcl_GetVar2Ex(interp, "ems_noshow", 0, TCL_GLOBAL_ONLY);
    if (var && (Tcl_GetIntFromObj(interp, var, &ivar)==TCL_OK) && (ivar!=0))
        policies=(en_policies)(policies|pol_noshow);

    var=Tcl_GetVar2Ex(interp, "ems_nocount", 0, TCL_GLOBAL_ONLY);
    if (var && (Tcl_GetIntFromObj(interp, var, &ivar)==TCL_OK) && (ivar!=0))
        policies=(en_policies)(policies|pol_nocount);

    var=Tcl_GetVar2Ex(interp, "ems_nowait", 0, TCL_GLOBAL_ONLY);
    if (var && (Tcl_GetIntFromObj(interp, var, &ivar)==TCL_OK) && (ivar!=0))
        policies=(en_policies)(policies|pol_nowait);

    try {
        switch (objc) {
        case 1:
            communication.init();
            break;
        case 2:
            communication.init(Tcl_GetString(objv[1]));
            break;
        case 3: {
            int port;
            if (Tcl_GetIntFromObj(interp, objv[2], &port)!=TCL_OK)
                return TCL_ERROR;
            communication.init(Tcl_GetString(objv[1]), port);
          } break;
        default:
            Tcl_WrongNumArgs(interp, 1, objv, "[socket] | [host port]");
            return TCL_ERROR;
        }
    } catch (C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        return TCL_ERROR;
    }
    return TCL_OK;
}
/******************************************************************************
*
* Ems_disconnect bricht den Kontakt zu einem Kommunikationsprozess ab.
*
*/
static int Ems_disconnect(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    if (objc!=1)
        return Tcl_NoArgs(interp, 1, objv);

    try {
        communication.done();
    } catch (C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        return TCL_ERROR;
    }
    return TCL_OK;
}
/******************************************************************************
*
* Ems_connected testet den Kontakt zu einem Kommunikationsprozess.
*
*/
static int Ems_connected(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    if (objc!=1)
        return Tcl_NoArgs(interp, 1, objv);

    Tcl_SetObjResult(interp, Tcl_NewIntObj(communication.valid()?1:0));
    return TCL_OK;
}
/******************************************************************************
*
* Ems_connection gibt die Verbindung eines Kommunikationsprozesses an.
*
*/
static int Ems_connection(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    if (objc!=1)
        return Tcl_NoArgs(interp, 1, objv);

    ostringstream ss;
    ss << communication;
    Tcl_SetResult_Stream(interp, ss);
    return TCL_OK;
}
/******************************************************************************
*
* Ems_veds liefert eine Liste der verfuegbaren veds
* Argumente: keine
* Resultat: VED-Liste
*/
static int Ems_veds(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    if (objc!=1)
        return Tcl_NoArgs(interp, 1, objv);

    C_VED* commu=0;
    C_instr_system* commu_is;
    try {
        commu=new C_VED("commu");
        commu_is=commu->is0();
        if (commu_is==0) {
            throw new C_program_error("commu has no IS 0");
        }
        commu->confmode(synchron);
        C_confirmation* conf;
        conf=commu_is->command("VEDnames");
        C_inbuf inbuf(conf->buffer(), conf->size());
        delete conf;
        inbuf++;
        int num;
        inbuf >> num;
        for (int i=0; i<num; i++) {
            string s;
            inbuf >> s;
            Tcl_AppendElement(interp, (char*)(s.c_str()));
        }
        delete commu;
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        delete commu;
        return TCL_ERROR;
    }
    return TCL_OK;
}
/******************************************************************************
*
* Ems_open oeffnet ein VED
*
* Argumente: VED-Name [prior]
* Result   : VED-Name
* Nebeneffekte: Es wird ein Commando ved_$VED-Name erzeugt
*/
static int Ems_open(ClientData clientdata, Tcl_Interp* interp, int argc,
    const char* argv[])
{
    if (argc!=2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong # args; must be VED-Name"), -1));
        return TCL_ERROR;
    }
    C_VED::VED_prior prior=C_VED::NO_prior;
    if (argc==3) {
        int v;
        if (Tcl_GetInt(interp, (char*)argv[2], &v)!=TCL_OK) return TCL_ERROR;
        prior=(C_VED::VED_prior)v;
  }

    E_ved* ved;
    try {
        ved=new E_ved(interp, argv[1], prior);
    } catch (C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        return TCL_ERROR;
    }
    ved->create_tcl_proc();
    return TCL_OK;
}
/******************************************************************************
*
* Ems_openveds liefert eine Liste der VED-Kommandos aller geoeffneten VEDs
*
*/
static int Ems_openveds(ClientData clientdata, Tcl_Interp* interp, int argc,
    const char* argv[])
{
    if (argc!=1) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("no args expected"), -1));
        return TCL_ERROR;
    }

    for (int i=0; i<eveds.count(); i++)
        Tcl_AppendElement(interp, (char*)(eveds[i]->tclprocname().c_str()));
    return TCL_OK;
}
/******************************************************************************
*
* Ems_unsolcommand installiert eine Callbackfunktion fuer unsolicited Messages
* vom Kommunikationsprozess.
*
* argv[1]: TCL_Kommando
*          %h wird durch den Header, %d durch die Daten ersetzt
*          falls type gleich ServerDisconnect ist, wird %v durch das
*          VED-Kommando ersetzt
*/
static int Ems_unsolcommand(ClientData clientdata, Tcl_Interp* interp, int argc,
    const char* argv[])
{
    // ems_unsolcommand type [command]
    if ((argc<2) || (argc>3)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong # args; must be type [tcl_command]"),
            -1));
        return TCL_ERROR;
    }
    int type, res;
    switch (res=findstring(interp, argv[1], unsolstrs, /*NrOfUnsolmsg,*/ &type)) {
    case -1: // nicht gefunden
        return TCL_ERROR;
    case -2: // war integer
        break;
    default:
        type=res;
        break;
    }

    Tcl_SetResult_String(interp, global.unsolcomm[type]);
    if (argc==2)
        global.unsolcomm[type]="";
    else
        global.unsolcomm[type]=argv[2];
    return TCL_OK;
}
/******************************************************************************
*
* Ems_confmodecommand setzt einen Default-Wert fuer den Confirmationmode. 
* Alle neu geoffneten VEDs uebernehmen diesen.
* wenn '-all' angegeben ist, uebernehmen ihn auch geoeffnete VEDs.
* Ganz ohne Argumente tut es nichts.
* Rueckgabe ist der vorherige Modus.
* Nach Programmstart steht er auf 'synchron' (in proc_ved.cc festgelegt).
*/
static int Ems_confmodecommand(ClientData clientdata, Tcl_Interp* interp,
    int argc, const char* argv[])
{
    // ems_confmodecommand [[-all] synchron|asynchron]
    if (argc>3) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be [[-all] synchron|asynchron]",
            -1));
        return TCL_ERROR;
    }
    conf_mode oldmode;
    if (argc==1) {
        oldmode=veds.def_confmode();
    } else {
        int all=0;
        const char* arg=argv[1];
        conf_mode mode;
        if (argc==3) {
            if (strcmp(argv[1], "-all")!=0) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong args; must be [[-all] synchron|asynchron]",
                    -1));
                return TCL_ERROR;
            }
            all=1;
            arg=argv[2];
        }
        const char* names[]={
            "synchron",
            "asynchron",
            0
        };
        switch (findstring(interp, arg, names)) {
        case 0: mode=synchron; break;
        case 1: mode=asynchron; break;
        default: return TCL_ERROR;
        }
        if (all) {
            oldmode=veds.def_confmode();
            veds.set_confmode(mode);
        } else {
            oldmode=veds.def_confmode(mode);
        }
    }
    Tcl_SetObjResult(interp,
        Tcl_NewStringObj((oldmode==synchron?"synchron":"asynchron"),
        -1));
    return TCL_OK;
}
/*****************************************************************************/
void E_veds_timerdummy(ClientData)
{
    cerr << "E_veds::timerdummy called" << endl;
}
/*****************************************************************************/
int E_veds::pending(void) const
{
    int num=0;
    for (int i=0; i<count(); i++)
        num+=list[i]->confbacknum();
    return num;
}
/******************************************************************************
*
* Ems_pendingcommand stellt fest, wieviele Confirmations noch erwartet werden.
*/
static int Ems_pendingcommand(ClientData clientdata, Tcl_Interp* interp,
    int argc, const char* argv[])
{
    // ems_pendingcommand
    if (argc>1) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("no args expected"), -1));
        return TCL_ERROR;
    }
    ostringstream st;
    st << eveds.pending();
    Tcl_SetResult_Stream(interp, st);
    return TCL_OK;
}
/******************************************************************************
*
* Ems_flushcommand wartet, bis alle Confirmations eingesammelt sind, bricht
* jedoch nach Ablauf eines Timeouts ab.
* Gibt die Anzahl noch fehlender Confirmations zurueck.
*/
static int Ems_flushcommand(ClientData clientdata, Tcl_Interp* interp,
    int argc, const char* argv[])
{
    // ems_flushcommand [timeout]
    if (argc>2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong # args; must be flush [timeout/ms]"),
            -1));
        return TCL_ERROR;
    }
    struct timeval start, jetzt;
    int timeout=0, msec;
    Tcl_TimerToken token=0;
    if ((argc>1) && (Tcl_GetInt(interp, (char*)argv[1], &timeout)!=TCL_OK))
        return TCL_ERROR;
    if (timeout) {
        gettimeofday(&start, 0);
        token=Tcl_CreateTimerHandler(timeout, E_veds_timerdummy, 0);
    }
    int num, nochzeit=1;
    while (((num=eveds.pending())>0) && nochzeit) {
        Tcl_DoOneEvent(TCL_ALL_EVENTS);
        if (timeout) {
            gettimeofday(&jetzt, 0);
            msec=(jetzt.tv_usec-start.tv_usec)/1000+(jetzt.tv_sec-start.tv_sec)*1000;
            nochzeit=msec<timeout;
        }
    }
    if (timeout) Tcl_DeleteTimerHandler(token);
    ostringstream st;
    st << num;
    Tcl_SetResult_Stream(interp, st);
    return TCL_OK;
}
/******************************************************************************
*
* Ems_timeoutcommand legt einen Timeout fuer Confirmations fest
*
*/
static int Ems_timeoutcommand(ClientData clientdata, Tcl_Interp* interp,
    int argc, const char* argv[])
{
    if (argc>2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(strdup("wrong # args; must be timeout [timeout/sec]"),
            -1));
        return TCL_ERROR;
    }
    int to;
    if (argc>1) {
        int nto;
        if (Tcl_GetInt(interp, (char*)argv[1], &nto)!=TCL_OK) return TCL_ERROR;
        to=communication.deftimeout(nto);
    } else {
        to=communication.deftimeout();
    }
    ostringstream st;
    st << to;
    Tcl_SetResult_Stream(interp, st);
    return TCL_OK;
}
/*****************************************************************************/
static void printunsol(Tcl_Interp* interp, C_confirmation* conf)
{
    ostringstream ss;
    msgheader* header=conf->header();
    ss << "got message:" << endl
      << "  size  :" << header->size << endl
      << "  client:" << header->client << endl
      << "  ved   :" << header->ved << endl
      << "  type  :" << header->type.unsoltype << endl
      << "  flags :" << header->flags << endl
      << "  xid   :" << header->transid;
    Tcl_SetResult_Stream(interp, ss);
    Tcl_BackgroundError(interp);
}
/*****************************************************************************/
static void
unpackheader(string& s, C_confirmation* conf)
{
    ostringstream ss;
    msgheader* header=conf->header();
    ss << '{' << header->size << ' '
        << header->client << ' '
        << header->ved << ' '
        << header->type.reqtype << ' '
        << header->flags << ' '
        << header->transid << '}';
    s=ss.str();
}
/*****************************************************************************/
static void
unpackbody(string& s, C_confirmation* conf)
{
    ostringstream ss;
    int size=conf->header()->size;
    ss << '{';
    for (int i=0; i<size; i++) {
        if (i>0) ss << ' ';
        ss << conf->buffer(i);
    }
    ss << '}';
    s=ss.str();
}
/*****************************************************************************/
static void
dispatchresponse(Tcl_Interp* interp, C_confirmation* conf)
{
    msgheader* header=conf->header();
    E_ved* ved=eveds.find(header->ved);
    if (!ved) { // unknown VED
        ostringstream ss;
        ss << "confirmation from unknown ved " << header->ved;
        Tcl_SetResult_Stream(interp, ss);
        Tcl_BackgroundError(interp);
    } else { // VED found
        E_ved::confbackentry* entry;
        if ((entry=ved->extractconfback(header->transid))==0) {
            ostringstream ss;
                ss << "no callback for ved " << ved->tclprocname() << endl
                    << "  xid     : " << header->transid << endl
                    << "  size    : " << header->size << endl
                    << "  request : " << Req_str(header->type.reqtype);
            Tcl_SetResult_Stream(interp, ss);
            Tcl_BackgroundError(interp);
        } else { // callback found
            ostringstream ss;
            int err=TCL_OK;
            if ((conf->size()<1) || (conf->buffer(0)!=OK)) { // Ems-Fehler
                if (entry->err_command==0)
                    ss << "ems_async_error ";
                else
                    ss << Tcl_DStringValue(entry->err_command) << " ";
                ss << E_confirmation::new_E_confirmation(interp, ved, *conf);
            } else { // Ems Ok
                if (entry->ok_command==0)
                    ss << "ems_async_ok ";
                else
                    ss << Tcl_DStringValue(entry->ok_command) << " ";
                try {
                    //cerr << "call former " << ved->formername(entry->form) << endl;
                    err=(ved->*(entry->form))(ss, conf, !entry->former_default,
                            entry->numformargs, entry->formargs);
                } catch(C_error* error) {
                    ostringstream ss;
                    ss << (*error);
                    Tcl_SetResult_Stream(interp, ss);
                    set_error_code(interp, error);
                    delete error;
                    err=TCL_ERROR;
                }
                if (err!=TCL_OK) {
                    ostringstream ss;
                    ss << endl << "former '" << ved->formername(entry->form)
                        << "' called from callback for ved " << ved->name()
                        << " (" << ved->tclprocname() << ") " << ", xid "
                        << header->transid;
                    string s=ss.str();
                    Tcl_AddErrorInfo(interp, s.c_str());
                }
            }
            string s=ss.str();
            if (err==TCL_OK) {
                if ((err=Tcl_GlobalEval(interp, (char*)s.c_str()))!=TCL_OK) {
                    ostringstream ss;
                    ss << endl << "called from callback for ved " << ved->name()
                        << " (" << ved->tclprocname() << ") "
                        << ", xid " << header->transid;
                    string s=ss.str();
                    Tcl_AddErrorInfo(interp, s.c_str());
                }
            }
            if (err!=TCL_OK) Tcl_BackgroundError(interp);
            delete entry;
        }
    }
}
/*****************************************************************************/
#ifdef NO_ANSI_CXX
static void
dispatchunsol_pseudo(Tcl_Interp* interp, C_confirmation* conf)
{
    if (global.unsolcomm[header->type.unsoltype]!="") { // callback vorhanden
        string st;
        int x;
        st=global.unsolcomm[header->type.unsoltype];

        if ((x=st.index(string("%h")))>=0) { // header in Kommando einpacken
            string s=unpackheader(conf);
            do {
                st=st(0, x)+s+st(x+2, st.length()-x-2);
            } while ((x=st.index(string("%h")))>=0);
        }
        if ((x=st.index(string("%d")))>=0) { // Daten in Kommando einpacken
            string s=unpackbody(conf);
            do {
                st=st(0, x)+s+st(x+2, st.length()-x-2);
            } while ((x=st.index(string("%d")))>=0);
        }
        if ((header->type.reqtype==Unsol_ServerDisconnect) && 
                ((x=st.index(string("%v")))>=0)) {
            // VED-Name in Kommando einpacken (wenn moeglich)
            string s;
            E_ved* ved=eveds.find(conf->buffer(0));
            if (ved)
                s=ved->tclprocname();
            else
                s="unknown_ved";
            do {
                st=st(0, x)+s+st(x+2, st.length()-x-2);
            } while ((x=st.index(string("%v")))>=0);
        }
        if (Tcl_GlobalEval(global.interp, (char*)(const char*)st)!=TCL_OK)
            Tcl_BackgroundError(interp);
    } else if (header->type.unsoltype==Unsol_ServerDisconnect) {
        OSSTREAM ss;
        ss << "server " << conf->buffer(0) << " starb." << ends;
        Tcl_SetResult_Stream(interp, ss);
        Tcl_BackgroundError(interp);
        // cerr << "server " << conf->buffer(0) << " starb." << endl;
    } else if (header->type.unsoltype==Unsol_Bye) {
        try {communication.done();} catch (...) {}
    } else {
        printunsol(interp, conf); // Notloesung
    }
}
static void
dispatchunsol_real(Tcl_Interp* interp, C_confirmation* conf)
{
    E_ved* ved=eveds.find(header->ved);
    if (!ved) {
        printunsol(interp, conf); // Notloesung, wenn VED nicht mehr offen
    } else {
        string st;
        int x;
        if ((header->type.unsoltype>0) && (header->type.unsoltype<NrOfUnsolmsg)
                && (ved->unsolcomm[header->type.unsoltype]!=""))
            st=ved->unsolcomm[header->type.unsoltype];
        else
            st=ved->defunsolcomm;
        if ((x=st.index(string("%h")))>=0) {
            string s=unpackheader(conf);
            do {
                st=st(0, x)+s+st(x+2, st.length()-x-2);
            } while ((x=st.index(string("%h")))>=0);
        }
        if ((x=st.index(string("%d")))>=0) {
            string s=unpackbody(conf);
            do {
                st=st(0, x)+s+st(x+2, st.length()-x-2);
            } while ((x=st.index(string("%d")))>=0);
        }
        if ((x=st.index(string("%v")))>=0) {
            string s;
            s=ved->tclprocname();
            do {
                st=st(0, x)+s+st(x+2, st.length()-x-2);
            } while ((x=st.index(string("%v")))>=0);
        }
        if (Tcl_GlobalEval(global.interp, (char*)(const char*)st)!=TCL_OK)
            Tcl_BackgroundError(interp);
    }
}
#else // ANSI_CXX
static void
substitute_commandargs(string& command, C_confirmation* conf, string ved)
{
// static int count=0;
// cerr<<"substitute: "<<command<<" "<<(*conf)<<" "<<ved<<endl;
// cerr<<"count="<<(count++)<<endl;
// if (count==14) exit(0);
    string::size_type pos;
    if ((pos=command.find("%h"))!=string::npos) {
        string s;
        unpackheader(s, conf);
        /*XXXX*/
        //do {
        //    //command.replace(pos, 2, s);
        //    command=command.substr(pos, 2)+s+command.substr(pos+2, string::npos);
        //} while ((pos=command.find("%h"))!=string::npos);
        command=command.substr(0, pos)+s+command.substr(pos+2, string::npos);
    }
    if ((pos=command.find("%d"))!=string::npos) {
        string s;
        unpackbody(s, conf);
        //do {
        //    //command.replace(pos, 2, s);
        //    command=command.substr(pos, 2)+s+command.substr(pos+2, string::npos);
        //} while ((pos=command.find("%d"))!=string::npos);
        command=command.substr(0, pos)+s+command.substr(pos+2, string::npos);
    }
    if ((ved!="") && ((pos=command.find("%v"))!=string::npos)) {
        do {
            command.replace(pos, 2, ved);
        } while ((pos=command.find("%v"))!=string::npos);
    }
}
static void
dispatchunsol_pseudo(Tcl_Interp* interp, C_confirmation* conf)
{
    msgheader* header=conf->header();
    if (global.unsolcomm[header->type.unsoltype]!="") { //callback vorhanden
        string st;
        st=global.unsolcomm[header->type.unsoltype];

        string vedcommand("");
        if (header->type.unsoltype==Unsol_ServerDisconnect) {
            E_ved* ved=eveds.find(conf->buffer(0));
            if (ved)
                vedcommand=ved->tclprocname();
            else
                vedcommand="unknown_ved";
        }
        substitute_commandargs(st, conf, vedcommand);

        if (Tcl_GlobalEval(global.interp, (char*)st.c_str())!=TCL_OK)
            Tcl_BackgroundError(interp);

    } else if (header->type.unsoltype==Unsol_ServerDisconnect) {
        ostringstream ss;
        ss << "server " << conf->buffer(0) << " starb." << ends;
        Tcl_SetResult_Stream(interp, ss);
        Tcl_BackgroundError(interp);
        // cerr << "server " << conf->buffer(0) << " starb." << endl;
    } else if (header->type.unsoltype==Unsol_Bye) {
        try {
            communication.done();
        } catch (...) {
        }
    } else {
      printunsol(interp, conf); // Notloesung
    }
}
static void
dispatchunsol_real(Tcl_Interp* interp, C_confirmation* conf)
{
    msgheader* header=conf->header();
    E_ved* ved=eveds.find(header->ved);
    if (!ved) {
        printunsol(interp, conf); // Notloesung, wenn VED nicht mehr offen
    } else {
        string st;
        if ((header->type.unsoltype>0) && (header->type.unsoltype<NrOfUnsolmsg)
                && (ved->unsolcomm[header->type.unsoltype]!=""))
            st=ved->unsolcomm[header->type.unsoltype];
        else
            st=ved->defunsolcomm;

        substitute_commandargs(st, conf, ved->tclprocname());

        if (Tcl_GlobalEval(global.interp, (char*)st.c_str())!=TCL_OK)
            Tcl_BackgroundError(interp);
    }
}
#endif // NO_ANSI_CXX

static void
dispatchunsol(Tcl_Interp* interp, C_confirmation* conf)
{
    msgheader* header=conf->header();
    if ((header->ved==EMS_commu) || (header->flags&Flag_IdGlobal)) {
        // Message kommt von commu oder mit globalem ID
        dispatchunsol_pseudo(interp, conf);
    } else {
        // Message kommt von "richtiger" VED
        dispatchunsol_real(interp, conf);
    }
}
/*****************************************************************************/
static void dispatchmessage(Tcl_Interp* interp, C_confirmation* conf)
{
    if ((conf->header()->flags & Flag_Unsolicited)==0) {
        // normale confirmation
        dispatchresponse(interp, conf);
    } else {
        // unsolicited message
        dispatchunsol(interp, conf);
    }
    delete conf;
}
/*****************************************************************************/
extern "C" {
static void commpathback(ClientData data, int mask)
{
    Tcl_Interp* interp=(Tcl_Interp*)data;
    if (interp==0) {
        cerr << "commpathback: interp=0" << endl;
        sleep(2);
        int x=*(int*)0; // crash it!
        cerr << "x=" << x << endl;
    }
    if (mask & TCL_EXCEPTION) {
        ostringstream ss;
        ss << "commpathback: exception: " << mask << ends;
        Tcl_SetResult_Stream(interp, ss);
        Tcl_BackgroundError(interp);
    }
    if (mask & TCL_READABLE) {
        struct timeval tv;
        tv.tv_sec=0; tv.tv_usec=0;
        C_confirmation* conf;
        try {
            conf=communication.GetConf(&tv);
        } catch (C_error* e) {
            Tcl_DeleteFileHandler(global.commpath);
            Tcl_SetResult_Err(interp, e);
            delete e;
            Tcl_BackgroundError(interp);
            conf=0;
        }
        if (conf!=0) dispatchmessage(interp, conf);
    }
}
}
/*****************************************************************************/
static void idleback(ClientData data)
{
    Tcl_Interp* interp=(Tcl_Interp*)data;
    if (interp==0) {
        cerr << "idleback: interp=0" << endl;
        sleep(2);
        int x=*(int*)0; // crash it
        cerr << "x=" << x << endl;
    }
    global.mtestpending=0;
    struct timeval tv;
    tv.tv_sec=0; tv.tv_usec=0;
    C_confirmation* conf=communication.GetConf(&tv);
    if (conf!=0)
        dispatchmessage(interp, conf);
}
/*****************************************************************************/
static void messageback(ClientData data)
{
    Tcl_Interp* interp=(Tcl_Interp*)data;
    if (interp==0) {
        cerr << "idleback: interp=0" << endl;
        sleep(2);
        int x=*(int*)0; // crash it
        cerr << "x=" << x << endl;
    }
    if (!global.mtestpending) {
        Tcl_DoWhenIdle(idleback, interp);
        global.mtestpending=1;
    }
}
/*****************************************************************************/
static void emsexit(ClientData)
{
    try {
        communication.done();
    } catch (C_error* e) {
        cerr << (*e) << endl;
        delete e;
    }
}
/*****************************************************************************/
static void commuback(ClientData data, int action, int reason, int path)
{
// action == -1: abort
//            0: close
//            1: open
    Tcl_Interp* interp=(Tcl_Interp*)data;
    if (interp==0) {
        cerr << "commuback: interp=0" << endl;
        sleep(2);
        int x=*(int*)0;
        cerr << "x=" << x << endl;
    }
    switch (action) {
    case -1: {
        ostringstream st;
        st << "Communication aborted: error " << reason << endl;
        Tcl_DeleteExitHandler(emsexit, 0);
        Tcl_DeleteFileHandler(global.commpath);
        // Tcl_FreeFile(global.commpath);
        Tcl_SetResult_Stream(interp, st);
        Tcl_BackgroundError(interp);
        }
        break;
    case 0:
        Tcl_DeleteExitHandler(emsexit, 0);
        Tcl_DeleteFileHandler(global.commpath);
        // Tcl_FreeFile(global.commpath);
        break;
    case 1:
        Tcl_CreateExitHandler(emsexit, 0);
        // global.commpath=Tcl_GetFile((ClientData)path, TCL_UNIX_FD);
        global.commpath=path;
        Tcl_CreateFileHandler(global.commpath, TCL_READABLE| TCL_EXCEPTION,
                commpathback, (ClientData)interp);
        break;
    }
}
/*****************************************************************************/
extern "C" {
int Ems_Init(Tcl_Interp* interp)
{

Tcl_CreateObjCommand(interp, "ems_connect", Ems_connect, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "ems_disconnect", Ems_disconnect, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "ems_connected", Ems_connected, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "ems_connection", Ems_connection,
    ClientData(0), (Tcl_CmdDeleteProc*)0);

Tcl_CreateCommand(interp, "ems_open", Ems_open, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "ems_veds", Ems_veds, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateCommand(interp, "ems_openveds", Ems_openveds, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateCommand(interp, "ems_unsolcommand", Ems_unsolcommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateCommand(interp, "ems_confmode", Ems_confmodecommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateCommand(interp, "ems_pending", Ems_pendingcommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateCommand(interp, "ems_flush", Ems_flushcommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateCommand(interp, "ems_timeout", Ems_timeoutcommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "xdr2float", XDR2floatcommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "float2xdr", float2XDRcommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "xdr2double", XDR2doublecommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "double2xdr", double2XDRcommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "xdr2string", XDR2stringcommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

Tcl_CreateObjCommand(interp, "string2xdr", string2XDRcommand, ClientData(0),
    (Tcl_CmdDeleteProc*)0);

global.interp=interp;
communication.installpcallback(commuback, ClientData(interp));
communication.installmcallback(messageback, ClientData(interp));
return TCL_OK;
}}
/*****************************************************************************/
/*****************************************************************************/
