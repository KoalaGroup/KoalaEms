/*
 * events++/make_file.hxx
 * $ZEL: make_file.hxx,v 1.4 2004/11/26 14:40:19 wuestner Exp $
 *
 * created: ???
 */

#ifndef _make_file_hxx_
#define _make_file_hxx_

struct fileinfo {
    fileinfo(): name(0), pathname(0), dirname(0), path(-1) {}
    void clear();
    char* name;
    char* pathname;
    char* dirname;
    int path;
    int sequence;
    int size, filesize;
};

void make_file(fileinfo& info);

#endif
