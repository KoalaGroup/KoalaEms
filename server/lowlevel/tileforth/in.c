/******************************************************************************
*                                                                             *
* in.c                                                                        *
*                                                                             *
* created 02.09.94                                                            *
* last changed 23.09.94                                                       *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include "in.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USE_OTHER

#ifdef USE_READLINE
#include <readline/readline.h>
#endif

/*****************************************************************************/

typedef struct source
  {
  struct source *next;
  struct source *prev;
  char *name;
  FILE *file;
  int line;
  char* lastchar;
  char* nextchar;
  char* buffer;
  }* source;

static struct source standard_source=
  {
  (source)0,
  (source)0,
  "stdin",
  (FILE*)0,
  0,
  (char*)0,
  (char*)0,
  (char*)0,
  };

static source sources=&standard_source;
static source actual_source=&standard_source;
static int letzter, saved_key, key_saved;

/*****************************************************************************/

static void close_source()
{
source old_entry;

fclose(actual_source->file);
free(actual_source->name);
old_entry=actual_source;
actual_source=old_entry->prev;
actual_source->next=0;
free(old_entry->buffer);
free(old_entry);
}

/*****************************************************************************/
#ifdef USE_READLINE
char* do_readline(char* old)
{
char *s, *line;
int l;

if (old) free(old);
printf("\n");
s=readline("-->");
if (!s) return(0);
l=strlen(s);
if (l>0) add_history(s);
line=(char*)malloc(l+2);
strcpy(line, s);
free(s);
line[l]='\n';
line[l+1]=0;
return(line);
}
#endif
/*****************************************************************************/
#ifdef USE_STDIN
char* do_readstdin(char* old)
{
static char line[1024];
return(fgets(line, 1024, stdin));
}
#endif
/*****************************************************************************/
#ifdef USE_OTHER
extern char* zeile(char*);
#endif
/*****************************************************************************/

int key()
{
if (key_saved)
  {
  key_saved=0;
  letzter=saved_key;
  return(letzter);
  }
if (actual_source->buffer && (actual_source->lastchar>=actual_source->nextchar))
  {
  letzter=*actual_source->nextchar++;
  return(letzter);
  }
while ((actual_source!=sources) && (fgets(actual_source->buffer, 1024,
    actual_source->file)==0))
  {
  close_source();
  if (letzter!='\n')
    {
    letzter='\n';
    return(letzter);
    }
  }
if (actual_source==sources)
  {
#if defined (USE_READLINE)
  actual_source->buffer=do_readline(actual_source->buffer);
#elif defined (USE_STDIN)
  actual_source->buffer=do_readstdin(actual_source->buffer);
#elif defined (USE_OTHER)
  actual_source->buffer=zeile(actual_source->buffer);
#else
#error da muss was definiert sein!
#endif
  if (actual_source->buffer==0)
    {
    letzter=-1;
    return(-1);
    }
  }
actual_source->lastchar=actual_source->buffer+strlen(actual_source->buffer)-1;
actual_source->nextchar=actual_source->buffer;
actual_source->line++;

letzter=*actual_source->nextchar++;
return(letzter);
}

/*****************************************************************************/

static int _isspace(char c)
{
return(c<=' ');
}

/*****************************************************************************/

void input_skipspace()
{
int c;

while (((c=key())!=-1) && _isspace(c));
saved_key=c;
key_saved=1;
}

/*****************************************************************************/

int input_scan(char* dest, char delimiter)
{
int char_count;
char c;

if (delimiter==' ')
  {
  c=key();
  while (!_isspace(c) && (c!=-1))
    {
    *dest++=c;
    char_count++;
    c=key();
    }
  *dest=0;
  return(char_count);
  }
else
  {
  c=key();
  while ((c!=delimiter) && (c!=-1))
    {
    *dest++=c;
    char_count++;
    c=key();
    }
  *dest=0;
  return(char_count);
  }
}

/*****************************************************************************/

int input_counted_scan(char* dest, char delimiter, int count)
{
int char_count;
char c;

if (count==0)
  {
  *dest=0;
  return(0);
  }

if (delimiter==' ')
  {
  c=key();
  while ((char_count<count) && !_isspace(c) && (c!=-1))
    {
    *dest++=c;
    char_count++;
    if (char_count<count) c=key();
    }
  *dest=0;
  return(char_count);
  }
else
  {
  c=key();
    while ((char_count<count) && (c!=delimiter) && (c!=-1))
    {
    *dest++=c;
    char_count++;
    if (char_count<count) c=key();
    }
  *dest=0;
  return(char_count);
  }
}

/*****************************************************************************/

int include_file(char* name)
{
FILE* f;
source new_entry;

f=fopen(name, "r");
if (f==0) return(errno);
new_entry=(source)malloc(sizeof(struct source));
new_entry->next=0;
new_entry->prev=actual_source;
new_entry->name=(char*)malloc(strlen(name)+1);
strcpy(new_entry->name, name);
new_entry->file=f;
new_entry->line=0;
new_entry->buffer=(char*)malloc(1024);
new_entry->lastchar=new_entry->buffer;
new_entry->nextchar=new_entry->buffer;
actual_source->next=new_entry;
actual_source=new_entry;
return(0);
}

/*****************************************************************************/

void input_flush()
{
while (actual_source!=sources) close_source();
}

/*****************************************************************************/

int input_eof()
{
return(letzter==-1);
}

/*****************************************************************************/
static char infstr[1024];

char* input_source()
{
return(actual_source->name);
}

/*****************************************************************************/

int input_line()
{
return(actual_source->line);
}

/*****************************************************************************/

char* input_info()
{
/*
if (actual_source!=sources)
*/
  {
  sprintf(infstr, "%s:%d:", input_source(), input_line());
  return(infstr);
  }
/*
else
  return("");
*/
}

/*****************************************************************************/

void in_initiate()
{
sources=&standard_source;
actual_source=&standard_source;
letzter=0;
key_saved=0;
}


/*****************************************************************************/

void in_finish()
{
input_flush();
}

/*****************************************************************************/
/*****************************************************************************/
