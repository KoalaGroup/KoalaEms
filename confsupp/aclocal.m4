dnl
dnl aclocal.m4
dnl created 10.10.95
dnl changed 11.10.95
dnl
dnl PW
dnl

dnl $Id: aclocal.m4,v 1.18 2010/09/01 22:12:27 wuestner Exp $

dnl
dnl sucht einen File (als Repraesentant eines Softwarepakets) in einer
dnl Liste von Direktories
dnl EMS_SEARCH(
dnl     <Paketname>, <Kommentar>,
dnl     <File>, <Variable>,
dnl     <with-Directory-Liste>,  <without-Directory-Liste>,
dnl     <"fatal" (or not)>
dnl )
dnl
AC_DEFUN([EMS_SEARCH], [
	AC_ARG_WITH($1, [  --with-]$1[=DIR  ]$2, [given=$withval], [given=])
	AC_CHECKING($1)
	if test -n "$given" ; then
	  if test "$given" = "no" ; then
	    $4="no"
	  else
		AC_MSG_CHECKING(for $3 in $given)
                dnl echo testing test -f "$given/$3"
		if test -f "$given/$3" ; then
			AC_MSG_RESULT(yes)
			$4="`(cd "$given"; pwd)`"
			break
		else
			AC_MSG_RESULT(no)
		fi
	  fi
	else
		for f in $6 ; do
			AC_MSG_CHECKING(for $3 in $f)
			if test -f "$f/$3" ; then
				AC_MSG_RESULT(yes)
				$4="`(cd "$f"; pwd)`"
				break
			else
				AC_MSG_RESULT(no)
			fi
		done
	fi
	if test -z "$$4" -a "$7" = "fatal" ; then
		AC_MSG_ERROR($1 not found; use --with-$1)
	fi
])

dnl
dnl sucht nach Includefiles aus readline
dnl READLINE: Directory mit readline.h
dnl READLINE_ROOT: gegebenes Directory oder leer
dnl
AC_DEFUN(EMS_SEARCH_READLINE, [
	AC_BEFORE([$0], [EMS_SEARCH_READLINELIB])
	EMS_SEARCH([readline], [readline includes],
		[readline/readline.h], [READLINE],
		[include .],
		[/usr/local/readline/include /usr/local/readline /usr/local/include \
			/usr/include], [fatal])
	READLINE_ROOT="$given"
	AC_PROVIDE([$0])
])

dnl
dnl sucht nach readline library
dnl READLINELIB: Directory mit libreadline.a
dnl
AC_DEFUN(EMS_SEARCH_READLINELIB, [
  AC_REQUIRE([AC_CANONICAL_SYSTEM])
  if test -n "$READLINE_ROOT" ; then
    root=$READLINE_ROOT
  else
    root=/usr/local/readline
  fi
  if test -z "$1" ; then
    suffix1=.so
    suffix2=.a
  else
    suffix2=$1
    if test -z "$2" ; then
      suffix1=none
    else
      suffix1=$2
    fi
  fi
  if test "$suffix1" != "none" ; then
    EMS_SEARCH([readline-lib], [readline library],
      [libreadline$suffix1], [READLINELIB],
      [. lib $target $target_cpu $target_os $target/lib $target_cpu/lib \
	      $target_os/lib],
      [/usr/lib /usr/local/lib $root $root/lib $root/$target $root/$target_cpu $root/$target_os \
	      $root/$target/lib $root/$target_cpu/lib $root/$target_os/lib])
  fi
  if test -z "$READLINELIB" ; then
    EMS_SEARCH([readline-lib], [readline library],
      [libreadline$suffix2], [READLINELIB],
      [. lib $target $target_cpu $target_os $target/lib $target_cpu/lib \
	      $target_os/lib],
      [/usr/lib /usr/local/lib $root $root/lib $root/$target $root/$target_cpu $root/$target_os \
	      $root/$target/lib $root/$target_cpu/lib $root/$target_os/lib],
              [fatal])
  fi
  AC_PROVIDE([$0])
])

dnl
dnl sucht nach termcap library
dnl TERMCAPLIB: Directory mit libreadline.a
dnl
AC_DEFUN(EMS_SEARCH_TERMCAPLIB, [
  AC_REQUIRE([AC_CANONICAL_SYSTEM])
  if test -z "$1" ; then
    suffix1=.so
    suffix2=.a
  else
    suffix2=$1
    if test -z "$2" ; then
      suffix1=none
    else
      suffix1=$2
    fi
  fi
  if test "$suffix1" != "none" ; then
    EMS_SEARCH([termcap-lib], [termcap library],
      [libtermcap$suffix1], [TERMCAPLIB],
      [. lib $target $target_cpu $target_os $target/lib $target_cpu/lib \
	      $target_os/lib],
      [/usr/lib /usr/local/lib /usr/lib/termcap])
  fi
  if test -z "$TERMCAPLIB" ; then
    EMS_SEARCH([termcap-lib], [termcap library],
      [libtermcap$suffix2], [TERMCAPLIB],
      [. lib $target $target_cpu $target_os $target/lib $target_cpu/lib \
	      $target_os/lib],
      [/usr/lib /usr/local/lib /usr/lib/termcap],
      [fatal])
  fi
  AC_PROVIDE([$0])
])

AC_DEFUN(COMBINE_SYSTEM, [
  AC_REQUIRE([AC_CANONICAL_SYSTEM])
  xxx=""
  for f in `echo $1` ; do
    xxx="$xxx $f $f/$target $f/$target_cpu-$target_os $f/$target_cpu $f/$target_os"
    xxx="$xxx $f/lib $f/$target/lib $f/$target_cpu-$target_os/lib $f/$target_cpu/lib $f/$target_os/lib"
  done
  $2=$xxx
])

dnl
dnl sucht nach Includefiles aus ems-common
dnl EMSCOMMON: Directory mit objecttypes.h
dnl EMSEXCOMMON: Directory mit requesttypes.h
dnl
AC_DEFUN(EMS_SEARCH_COMMON, [
	AC_BEFORE([$0], [EMS_SEARCH_COMMONLIB])
	AC_REQUIRE([AC_CANONICAL_SYSTEM])
	if test -n "$lems" ; then
		root=$lems
	else
		root=/usr/local/ems
	fi
	EMS_SEARCH([ems-common], [EMS common includes],
		[objecttypes.h], [EMSCOMMON],
		[. include include/common common/include],
		[../common $srcdir/../common $root $root/include],
                [fatal])
	EMS_COMMONROOT="$given"
	if test -n "$lems" ; then
		root=$lems
	else
		root=/usr/local/ems
	fi
        COMBINE_SYSTEM("$EMSCOMMON ../common ../../common", x1)
        COMBINE_SYSTEM($root, x2)
        COMBINE_SYSTEM(".", x3)
        pfad="$x1 $root $root/include $root/include/common $root/common/include $x2"
        EMS_SEARCH([ems-excommon], [EMS common extra includes],
		[requesttypes.h], [EMSEXCOMMON],
		[. include include/common common/include $x3],
		[$pfad],
                [fatal])
	AC_PROVIDE([$0])
])

dnl
dnl sucht nach ems-common library
dnl EMSCOMMONLIB: Directory mit libcommon.a
dnl
AC_DEFUN(EMS_SEARCH_COMMONLIB, [
	AC_REQUIRE([AC_CANONICAL_SYSTEM])
	if test -n "$lems" ; then
		root=$lems
	else
		root=/usr/local/ems
	fi
        if test -z "$1" ; then
          suffix1=.so
          suffix2=.a
        else
          suffix2=$1
          if test -z "$2" ; then
            suffix1=none
          else
            suffix1=$2
          fi
        fi
        COMBINE_SYSTEM(". lib", pfad1)
        COMBINE_SYSTEM("$EMSCOMMON ../common ../../common $root $root/lib", pfad2)
        if test "$suffix1" != "none" ; then
	  EMS_SEARCH([ems-commonlib], [EMS common library],
		  [libcommon$suffix1], [EMSCOMMONLIB],
		  [$pfad1],
		  [$pfad2])
	  AC_PROVIDE([$0])
        fi
        if test -z "$EMSCOMMONLIB" ; then
	  EMS_SEARCH([ems-commonlib], [EMS common library],
		  [libcommon$suffix2], [EMSCOMMONLIB],
		  [$pfad1],
		  [$pfad2],
                  [fatal])
        fi
	AC_PROVIDE([$0])
])

dnl
dnl sucht nach Includefiles aus ems-client
dnl EMSCLIENT: Directory mit clientcomm.h
dnl EMS_CLIENTROOT: gegebenes Directory oder leer
dnl
AC_DEFUN(EMS_SEARCH_CLIENT, [
	AC_BEFORE([$0], [EMS_SEARCH_CLIENTLIB])
	if test -n "$lems" ; then
		root=$lems
	else
		root=/usr/local/ems
	fi
	EMS_SEARCH([ems-client], [EMS client includes],
		[clientcomm.h], [EMSCLIENT],
		[. include include/clientlib clientlib/include],
		[../clientlib $srcdir/../clientlib $root $root/include])
	EMS_CLIENTROOT="$given"
	AC_PROVIDE([$0])
])

dnl
dnl sucht nach ems-client library
dnl EMSCLIENTLIB: Directory mit libemscomm.a
dnl
AC_DEFUN(EMS_SEARCH_CLIENTLIB, [
	AC_REQUIRE([AC_CANONICAL_SYSTEM])
	if test -n "$EMS_CLIENTROOT" ; then
		root=$EMS_CLIENTROOT
	else
		if test -n "$lems" ; then
			root=$lems
		else
			root=/usr/local/ems
		fi
	fi
        if test -z "$1" ; then
          suffix1=.so
          suffix2=.a
        else
          suffix2=$1
          if test -z "$2" ; then
            suffix1=none
          else
            suffix1=$2
          fi
        fi
        COMBINE_SYSTEM(". lib", pfad1)
        COMBINE_SYSTEM("$emslibs ../clientlib ../../clientlib $root $root/lib", pfad2)
        if test "$suffix1" != "none" ; then
	  EMS_SEARCH([ems-clientlib], [EMS client library],
		  [libemscomm$suffix1], [EMSCLIENTLIB],
		  [$pfad1],
		  [$pfad2])
        fi
        if test -z "$EMSCLIENTLIB" ; then
	  EMS_SEARCH([ems-clientlib], [EMS client library],
		  [libemscomm$suffix2], [EMSCLIENTLIB],
		  [$pfad1],
		  [$pfad2],
                  [fatal])
        fi
	AC_PROVIDE([$0])
])

dnl
dnl sucht nach Includefiles aus ems-support
dnl EMSSUPPORT: Directory mit readargs.hxx
dnl EMS_SUPPORTROOT: gegebenes Directory oder leer
dnl
AC_DEFUN(EMS_SEARCH_SUPPORT, [
	AC_BEFORE([$0], [EMS_SEARCH_SUPPORTLIB])
	if test -n "$lems" ; then
		root=$lems
	else
		root=/usr/local/ems
	fi
	EMS_SEARCH([ems-support], [EMS support includes],
		[readargs.hxx], [EMSSUPPORT],
		[. include include/support support/include],
		[../support $srcdir/../support $root $root/include])
	EMS_SUPPORTROOT="$given"
	AC_PROVIDE([$0])
])

dnl
dnl sucht nach ems-support library
dnl EMSSUPPORTLIB: Directory mit libsupp.a
dnl
AC_DEFUN(EMS_SEARCH_SUPPORTLIB, [
	AC_REQUIRE([AC_CANONICAL_SYSTEM])
	if test -n "$EMS_SUPPORTROOT" ; then
		root=$EMS_SUPPORTROOT
	else
		if test -n "$lems" ; then
			root=$lems
		else
			root=/usr/local/ems
		fi
	fi
        if test -z "$1" ; then
          suffix1=.so
          suffix2=.a
        else
          suffix2=$1
          if test -z "$2" ; then
            suffix1=none
          else
            suffix1=$2
          fi
        fi
        COMBINE_SYSTEM(". lib", pfad1)
        COMBINE_SYSTEM("$emslibs ../support ../../support $root $root/lib", pfad2)
        if test "$suffix1" != "none" ; then
	  EMS_SEARCH([ems-supportlib], [EMS support library],
		  [libsupp$suffix1], [EMSSUPPORTLIB],
		  [$pfad1],
		  [$pfad2])
        fi
        if test -z "$EMSSUPPORTLIB" ; then
	  EMS_SEARCH([ems-supportlib], [EMS support library],
		  [libsupp$suffix2], [EMSSUPPORTLIB],
		  [$pfad1],
		  [$pfad2],
                  [fatal])
        fi
	AC_PROVIDE([$0])
])

dnl
dnl sucht nach Includefiles aus ems-proclib
dnl EMSPROC: Directory mit proc_ved.hxx
dnl EMS_PROCROOT: gegebenes Directory oder leer
dnl
AC_DEFUN(EMS_SEARCH_PROC, [
	AC_BEFORE([$0], [EMS_SEARCH_PROCLIB])
	if test -n "$lems" ; then
		root=$lems
	else
		root=/usr/local/ems
	fi
	EMS_SEARCH([ems-proclib], [EMS proclib includes],
		[proc_ved.hxx], [EMSPROC],
		[. include include/proclib proclib/include],
		[../proclib $srcdir/../proclib $root $root/include])
	EMS_PROCROOT="$given"
	AC_PROVIDE([$0])
])

dnl
dnl sucht nach ems-proclib library
dnl EMSPROCLIB: Directory mit libsupp.a
dnl
AC_DEFUN(EMS_SEARCH_PROCLIB, [
	AC_REQUIRE([AC_CANONICAL_SYSTEM])
	if test -n "$EMS_PROCROOT" ; then
		root=$EMS_PROCROOT
	else
		if test -n "$lems" ; then
			root=$lems
		else
			root=/usr/local/ems
		fi
	fi
        if test -z "$1" ; then
          suffix1=.so
          suffix2=.a
        else
          suffix2=$1
          if test -z "$2" ; then
            suffix1=none
          else
            suffix1=$2
          fi
        fi
        COMBINE_SYSTEM(". lib", pfad1)
        COMBINE_SYSTEM("$emslibs ../proclib ../../proclib $root $root/lib", pfad2)
        if test "$suffix1" != "none" ; then
	  EMS_SEARCH([ems-procliblib], [EMS proclib library],
		  [libproc$suffix1], [EMSPROCLIB],
		  [$pfad1],
		  [$pfad2])
        fi
        if test -z "$EMSPROCLIB" ; then
	  EMS_SEARCH([ems-procliblib], [EMS proclib library],
		  [libproc$suffix2], [EMSPROCLIB],
		  [$pfad1],
		  [$pfad2],
                  [fatal])
        fi
	AC_PROVIDE([$0])
])

dnl
dnl sucht nach Sourcefiles aus klientsupport
dnl EMSKLIENTSUPPORT: Directory mit klientcore.c
dnl
AC_DEFUN(EMS_SEARCH_KLIENTSUPPORT, [
	if test -n "$lems" ; then
		root=$lems
	else
		root=/usr/local/ems
	fi
	EMS_SEARCH([ems-klientsupport], [EMS klient support functions],
		[klientcore.c], [EMSKLIENTSUPPORT],
		[. klientsupport],
		[../klientsupport $srcdir/../klientsupport $root $root/klientsupport])
	EMS_PROCROOT="$given"
	AC_PROVIDE([$0])
])
