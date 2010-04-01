# Installs in /usr/X11R6/bin
Summary: Video4Linux Stream Capture Viewer
Name: xawtv
Version: 3.07
Release: 1
Source0: xawtv-%{version}.tar.gz
Group: X11/Applications
Copyright: GNU GENERAL PUBLIC LICENSE
URL: http://www.in-berlin.de/User/kraxel/v4l/xawtv-%{version}.tar.gz
Vendor: Gerd Knorr <kraxel@goldbach.in-berlin.de>
BuildRoot: /var/tmp/xawtv-%{version}.root

%package radio
Summary: radio
Group: Applications/Sound

%package misc
Summary: misc
Group: X11/Applications

%description
A collection tools for video4linux:
 * xawtv    - X11 TV application
 * fbtv     - console TV application
 * streamer - capture tool (images / movies)
 * v4lctl   - command line tool to control v4l devices

%description radio
This is a ncurses-based radio application

%description misc
This package has a few tools you might find useful.  They
have not to do very much to do with xawtv.  I've used/wrote
them for debugging:
 * propwatch   - monitors properties of X11 windows.  If you
                 want to know how to keep track of xawtv's
                 _XAWTV_STATION property, look at this.
 * dump-mixers - dump mixer settings to stdout
 * record      - console sound recorder.  Has a simple input
                 level meter which might be useful to trouble
                 shoot sound problems.
 * showriff    - display the structure of RIFF files (avi, wav).

%prep
%setup

%build
mkdir build
cd build
CFLAGS="$RPM_OPT_FLAGS" ../configure --prefix=/usr/X11R6
make

%install
cd build
make ROOT="$RPM_BUILD_ROOT" SUID_ROOT="" install

%files
%attr(4711,root,root) /usr/X11R6/bin/v4l-conf
%defattr(-,root,root)
/usr/X11R6/bin/fbtv
/usr/X11R6/bin/streamer
/usr/X11R6/bin/xawtv-remote
/usr/X11R6/bin/xawtv
/usr/X11R6/bin/v4lctl
/usr/X11R6/man/man1/fbtv.1
/usr/X11R6/man/man1/v4l-conf.1
/usr/X11R6/man/man1/v4lctl.1
/usr/X11R6/man/man1/xawtv-remote.1
/usr/X11R6/man/man1/xawtv.1
/usr/X11R6/lib
%doc README Changes COPYING Programming-FAQ Trouble-Shooting Sound-FAQ
%doc README.lirc README.bttv UPDATE_TO_v3.0

%files radio
%defattr(-,root,root)
/usr/X11R6/bin/radio
/usr/X11R6/man/man1/radio.1

%files misc
%defattr(-,root,root)
/usr/X11R6/bin/dump-mixers
/usr/X11R6/bin/propwatch
/usr/X11R6/bin/record
/usr/X11R6/bin/showriff
/usr/X11R6/man/man1/propwatch.1
%doc tools/README

%clean
rm -rf $RPM_BUILD_ROOT

%post
cd /usr/X11R6/lib/X11/fonts/misc
mkfontdir
xset fp rehash || true

