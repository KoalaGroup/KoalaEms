/******************************************************************************
*                                                                             *
* strlist.hxx                                                                 *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* created 21.01.95                                                            *
* last changed 23.11.95                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _strlist_hxx_
#define _strlist_hxx_

class C_strlist
  {
  public:
    C_strlist(int);
    C_strlist(const C_strlist&);
    ~C_strlist();
  private:
    int num;
    char **list;
    int* valid;
  public:
    void set(int i, const char *s);
    const char* operator[](int i) const;
    int size() const {return(num);}
    C_strlist& operator =(const C_strlist&);
    int operator ==(const C_strlist&) const;

  };

#endif

/*****************************************************************************/
/*****************************************************************************/
