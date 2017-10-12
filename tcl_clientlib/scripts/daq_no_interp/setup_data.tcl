# $Id: setup_data.tcl,v 1.7 2000/08/11 13:24:07 wuestner Exp $
# copyright: 1998, 2000, 2001
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_setup(super_setup); zu lesender supersetupfile

# global_setupfiles(...)

# global_setupdir; directory of supersetup

# global_setupdata(super_setup); zuletzt gelesener supersetupfile
# global_setupdata(super_mtime)
# global_setupdata(names); Namen der VED-Namespaces
# global_setupdata(filename); nur temporaer
# global_setupdata(filetime); nur temporaer
# global_setupdata(spacename); nur temporaer

# global_init(data_read, init_done); ==1 if successfully done

# global_sourcelist; contains the name of the variable which source_new has to
#                    use for the list of setupfiles

# super_setup_sources; array of filenames 'sourced' by supersetupfile
#                      see proc 'source_new'
# saved_auto_path
#

proc add_library_path {path} {
  global auto_path
  if {[lsearch -exact $auto_path $path]==-1} {
    lappend auto_path $path
  }
}

proc reset_library_path {} {
  global auto_path saved_auto_path
  set auto_path $saved_auto_path
}

proc clear_setup_data {} {
  global global_setupdata global_init

  reset_library_path
  set global_init(data_read) 0
  set global_init(init_done) 0
  if [info exists global_setupdata(names)] {
    foreach i $global_setupdata(names) {
      if {[llength [namespace children :: ::vedsetup_$i]]==0} {
        output_append "namespace vedsetup_$i does not exist" tag_red
      } else {
        namespace delete ::vedsetup_$i
      }
    }
  }
  if {[llength [namespace children :: ::mastersetup]]>0} {
    namespace delete ::mastersetup
  } else {
    output_append "namespace mastersetup does not exist" tag_orange
  }
  if [info exists global_setupdata] {unset global_setupdata}
  if [info exists global_setupfiles] {unset global_setupfiles}
  change_status uninitialized "Setup cleared"
}

proc read_setup_data {} {
  global global_setup global_setupdata global_setupfiles global_init
  global global_setupdir
  set global_init(data_read) 0

# super_setup-file geaendert oder nie gelesen?
  set need_read 0
  if {([info exists global_setupdata(super_setup)]==0)
      || ("$global_setup(super_setup)"
          !="$global_setupdata(super_setup)")} {
#   filename changed oder file never read before 
    set need_read 1
  } else {
    if {[info exists ::mastersetup::read_always] && $::mastersetup::read_always} {
      set need_read 1
    } else {
      set need_read [filetimes_changed ::super_setup_sources]
    }
  }

# read super_setup file if necessary
  if {$need_read} {
    output "read master setup file ($global_setup(super_setup))"
    reset_library_path
    set global_setupdir [file dirname $global_setup(super_setup)]
    rename ::source ::source_orig
    rename ::source_new ::source
    global global_sourcelist super_setup_sources
    set global_sourcelist ::super_setup_sources
    array unset super_setup_sources
    namespace eval mastersetup {
      set setupdir $::global_setupdir
      if [catch {source $global_setup(super_setup)} mist] {
        rename ::source ::source_new
        rename ::source_orig ::source
        output "read master setupfile ($global_setup(super_setup)):\
          $mist" tag_red
        output "$::errorInfo" tag_blue
        return -1
      }
      if {![array exists setupfile]} {
        output "master setup file ($global_setup(super_setup))\
          does not set array setupfile" tag_red
        return -1
      }
      if [info exists ::global_setupfiles] {unset ::global_setupfiles}
      foreach i [lsort [array names setupfile]] {
        set ::global_setupfiles($i) $setupfile($i)
      }
    }
    rename ::source ::source_new
    rename ::source_orig ::source
    output "super_setup_sources:"
    foreach i [lsort -integer [array names super_setup_sources]] {
      output_append "$i: $super_setup_sources($i)"
    }
#    set global_setupdata(super_mtime) $super_mtime
    set global_setupdata(super_setup) $global_setup(super_setup)
  } else {
    output "no need to read master setup file\
      ($global_setup(super_setup))"
  }

# delete old setupdata if necessary
  if [info exists global_setupdata(names)] {
    foreach i $global_setupdata(names) {
      if {[info exists global_setupfiles($i)]==0} {
        output_append "delete old data for $i"
        if {[llength [namespace children :: ::vedsetup_$i]]==0} {
          output_append "namespace vedsetup_$i does not exist" tag_red
        } else {
          namespace delete ::vedsetup_$i
        }
      }
    }
    unset global_setupdata(names)
  }

# read individual setup files if necessary  
  output_append "read setupfiles:"
  foreach i [lsort [array names global_setupfiles]] {
    set global_setupdata(filename) [file join\
                              $global_setupdir $global_setupfiles($i)]
    set global_setupdata(spacename) $i
    lappend global_setupdata(names) $i
    set need_read 0
    if {([info exists vedsetup_${i}::filename]==0)
          || ([set vedsetup_${i}::filename]
                !=$global_setupdata(filename))} {
      set need_read 1
    } else {
      set need_read [filetimes_changed vedsetup_${i}::setup_sources]
    }

    if {$need_read} {
      if {[llength [namespace children :: ::vedsetup_$i]]>0} {
        namespace delete ::vedsetup_$i
      }
      namespace eval vedsetup_$i {
        output_append "  $global_setupdata(filename)"
        set setupdir $::global_setupdir
        output_append "  setupdir=$setupdir" tag_orange
        output_append "  which: [namespace which -variable setupdir]" tag_orange
        set space [namespace current]
        output "  eval in $space" tag_orange
        if {[info commands ::mastersetup::pre_read]!=""} {
          #output "::pre_read exists" tag_blue
          if [catch {::mastersetup::pre_read $space} mist] {
            output "mastersetup::pre_read: $mist" tag_red
            return -1
          }
        } else {
#          output_append "::pre_read does not exist" tag_blue
        }

        rename ::source ::source_orig
        rename ::source_new ::source
        global global_sourcelist
        variable setup_sources
        set global_sourcelist ${space}::setup_sources
        array unset setup_sources

        if [catch {source $global_setupdata(filename)} mist] {
          rename ::source ::source_new
          rename ::source_orig ::source
          output "read setupfile for $global_setupdata(spacename)\
            ($global_setupdata(filename)): $mist" tag_red
          return -1
        }
        rename ::source ::source_new
        rename ::source_orig ::source
        #output "setup_sources:"
        #foreach i [lsort -integer [array names setup_sources]] {
        #  output_append "$i: $setup_sources($i)"
        #}

        set filename $global_setupdata(filename)
#        set filetime $global_setupdata(filetime)
        if {![info exists description]} {set description "no Description"}
      }
    } else {
      output_append "  no need to read $global_setupdata(filename)"
    }
  }
  set global_init(data_read) 1
  return 0
}
