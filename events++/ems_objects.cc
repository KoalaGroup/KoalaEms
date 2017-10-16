/*
 * ems/events++/ems_objects.cc
 * 
 * created 2004-12-05 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <vector>
#include <queue>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <xdrstring.h>
#include <clusterformat.h>
#include "ems_objects.hxx"
#include <versions.hxx>

VERSION("Dec 05 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: ems_objects.cc,v 1.1 2005/02/11 15:45:00 wuestner Exp $")
#define XVERSION

//---------------------------------------------------------------------------//
int
ems_object::get_vedindex(ems_u32 id)
{
    vector<ems_u32>::iterator ii=find(ved_ids.begin(), ved_ids.end(), id);
    if (ii!=ved_ids.end())
        return ii-ved_ids.begin();
    else
        return -1;
}
//---------------------------------------------------------------------------//
file_object&
ems_object::find_file_object(string name)
{
    for (int i=files.size()-1; i>=0; i--) {
        if (files[i].name==name) {
            return files[i];
        }
    }
    file_object file(name);
    files.push_back(file);
    return files[files.size()-1];
}
//---------------------------------------------------------------------------//
/*
 * Das geht garnicht, 'key' ist nicht eindeutig!
 */
// text_object&
// ems_object::find_text_object(string key)
// {
//     for (int i=texts.size()-1; i>=0; i--) {
//         if (texts[i].name==name) {
//             return texts[i];
//         }
//     }
//     text_object text(name);
//     texts.push_back(text);
//     return texts.back();
// }
//---------------------------------------------------------------------------//
text_object::text_object(void)
{
    //printf("text_object::text_object()\n");
}
//---------------------------------------------------------------------------//
text_object::~text_object()
{
    //printf("text_object::~text_object(%s)\n", key.c_str());
}
//---------------------------------------------------------------------------//
void
text_object::add_line(char* s)
{
    lines.push_back(s);
    if (lines.size()==1) {
        if (lines[0].substr(0, 5)!="Key: ") {
            //cerr<<"text_object::add_line(0): "<<lines[0]<<endl;
        } else {
            key=lines[0].substr(5, string::npos);
            //cerr<<"text: "<<key<<endl;
        }
    }
}
//---------------------------------------------------------------------------//
void
text_object::dump(void)
{
    printf("TEXT: size=%llu\n", (unsigned long long)lines.size());
    printf("TEXT: key=%s\n", key.c_str());
    for (size_t i=0; i<lines.size(); i++)
        printf("[%llu] %s\n", (unsigned long long)i, lines[i].c_str());
    printf("TEXT: ENDE\n");
}
//---------------------------------------------------------------------------//
void
file_object::dump()
{
    cerr<<"file "<<name<<"; size="<<content.size()<<endl;
    cerr<<content<<endl;
}
//---------------------------------------------------------------------------//
int
ems_object::parse_file(ems_u32* buf, int num)
{
    char* name;

    ems_u32 optsize=buf[0];
    ems_u32* p=buf+optsize+1;
    ems_u32 flags=*p++;
    ems_u32 fragment_id=*p++;
    p=xdrstrcdup(&name, p);
    char* bname=strrchr(name, '/');
    if (bname)
        bname+=1;
    else
        bname=name;

    file_object& file=find_file_object(bname);

    file.fragment=fragment_id;
    p+=3; // skip ctime, mtime, perm
    /* int size=* */p++;
    int osize=*p;

    char* s;
    p=xdrstrcdup(&s, p);
    if (fragment_id==0) {
        file.content=string(s, osize);
    } else {
        file.content.append(s, osize);
    }
    free(s);

    if (flags==1) { // file complete
        //cerr<<"file "<<bname<<" extracted"<<endl;
    }

    return 0;
}
//---------------------------------------------------------------------------//
int
ems_object::parse_text(ems_u32* buf, int num)
{
    ems_u32* p;
    int lnr;

    ems_u32 optsize=buf[0];
    p=buf+optsize+3;
    int lines=*p++;

    // oder text_object::find_text_object(key)
    text_object text;

    for (lnr=0; lnr<lines; lnr++) {
        char* s;
        p=xdrstrcdup(&s, p);
        text.add_line(s);
        free(s);
    }
    texts.push_back(text);
    if (text.key=="run_notes") {
        if (text.lines.size()>2)
            text.dump();
    } else if (text.key=="superheader") {
        //text.dump();
        parse_superheader(text);
    }
    return 0;
}
//---------------------------------------------------------------------------//
void
ems_object::parse_superheader(text_object& text)
{
    vector<string> lines=text.lines;
    for (vector<string>::iterator it=lines.begin(); it<lines.end(); it++) {
        string s=*it;
        if (s.substr(0, 5)=="Run: ") {
            string runstr=s.substr(5, string::npos);
            //cerr<<"runstr="<<runstr<<endl;
            _runnr=atoi(runstr.c_str());
            _runnr_valid=true;
            cerr<<"run="<<_runnr<<endl;
            break;
        }
    }
}
//---------------------------------------------------------------------------//
int
ems_object::parse_ved_info(ems_u32* buf, int num)
{
    if (!ved_ids.empty()) {
        printf("ved_info: ved_ids not empty!\n");
        return -1;
    }
    ems_u32 optsize=buf[0];
    ems_u32* p=buf+optsize+1;
    ems_u32 version=*p++;
    //printf("ved_info: version=%x\n", version);
    switch (version) {
    case 0x80000001: {
        int num_ved=*p++;
        //printf("%d veds\n", num_ved);
        for (int ved=0; ved<num_ved; ved++) {
            int ved_id=*p++;
            ved_ids.push_back(ved_id);
            int num_is=*p++;
            //printf("ved %d; %d ISs\n", ved_id, num_is);
            for (int is=0; is<num_is; is++) {
                int is_id=*p++;
                //printf(" %d", is_id);
                sev_ids.push_back(is_id);
            }
            //printf("\n");
        }
        }
        break;
    case 0x80000002:
        //break;
    case 0x80000003:
        //break;
    default:
        printf("unknown ved-info version %x\n", version);
        return -1;
    }

    sort(sev_ids.begin(), sev_ids.end());
    sort(ved_ids.begin(), ved_ids.end());
    vector<ems_u32>::iterator iter;
//     printf("vedIDs:");
//     for (iter=ved_ids.begin(); iter<ved_ids.end(); iter++) {
//         printf(" %d", *iter);
//     }
//     printf("\n");
//     printf("sevIDs:");
//     for (iter=sev_ids.begin(); iter<sev_ids.end(); iter++) {
//         printf(" %d", *iter);
//     }
//     printf("\n");

    num_ved=ved_ids.size();
    ved_slots=new deque<sev_object>[num_ved];

    return 0;
}
//---------------------------------------------------------------------------//
void
ems_object::save_file(string name, bool force)
{
    file_object& file=find_file_object(name);
    string fname;
    if (runnr_valid()) {
        ostringstream os;
        os<<name<<"_"<<_runnr;
        fname=os.str();
    } else {
        fname=name;
    }
    int flags=O_WRONLY|O_CREAT|(force?O_TRUNC:O_EXCL);
    int p=open(fname.c_str(), flags, 0666);
    if (p<0) {
        printf("create %s: %s\n", fname.c_str(), strerror(errno));
        return;
    }
    write(p, file.content.c_str(), file.content.size());
    close(p);
}
//---------------------------------------------------------------------------//
int
ems_object::replace_file_object(string name, string fname)
{
    int p=open(fname.c_str(), O_RDONLY, 0);
    if (p<0) {
        cerr<<"open "<<fname.c_str()<<": "<<strerror(errno)<<endl;
        return -1;
    }
    off_t len=lseek(p, 0, SEEK_END);
    if (len==(off_t)-1) {
        cerr<<"seek to end in "<<fname.c_str()<<": "<<strerror(errno)<<endl;
        return -1;
    }
    lseek(p, 0, SEEK_SET);
    char* buf=new char[len];
    if (!buf) {
        cerr<<"alloc "<<len<<" bytes: "<<strerror(errno)<<endl;
        close(p);
        return -1;
    }
    ssize_t res=read(p, buf, len);
    if (res!=len) {
        cerr<<"read "<<fname.c_str()<<": "<<strerror(errno)<<endl;
        close(p);
        return -1;
    }

    file_object& file=find_file_object(name);
    file.content=string(buf, len);
    delete[] buf;
// Warning!
// Other fields (fragment...) have to be adapted
    return 0;
}
//---------------------------------------------------------------------------//
// void
// ems_object::save_text(string name)
// {
//     text_object& text=find_text_object(name);
//     FILE* f=fopen(name.c_str(), "w");
//     if (!f) {
//         printf("create %s: %s\n", name.c_str(), strerror(errno));
//         return;
//     }
//     for (vector<string>::iterator iter=lines.begin(); iter<lines.end(); iter++) {
//         fprintf(f, "%s\n", (*iter).c_str());
//     }
//     fclose(f);
// }
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
