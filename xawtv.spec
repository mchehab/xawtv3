Name:         xawtv
Group:        Applications/Multimedia
Autoreqprov:  on
Version:      3.105
Release:      0
License:      GPL
Summary:      v4l applications
Source:       http://bytesex.org/xawtv/%{name}_%{version}.tar.gz
Buildroot:    /var/tmp/root.%{name}-%{version}

%description
fixme

%prep
%setup -q

%build
mkdir build
cd build
CFLAGS="$RPM_OPT_FLAGS" ../configure --prefix=/usr/X11R6
make

%install
test "%{buildroot}" != "" && rm -rf "%{buildroot}"
(cd build; make DESTDIR="%{buildroot}" SUID_ROOT="" install)
gzip -v %{buildroot}/usr/X11R6/man/man*/*.[158]
gzip -v %{buildroot}/usr/X11R6/man/*/man*/*.[158]
find %{buildroot} -type f -print	\
	| sed -e 's|%{buildroot}||'	\
	| grep -v -e %{docdir}		\
	| grep -v -e bin/v4l-conf	\
	> filelist

%files -f filelist
%defattr(-,root,root)
%doc COPYING Changes TODO README README.* contrib/frequencies*
%attr(4711,root,root) /usr/X11R6/bin/v4l-conf

%clean
test "%{buildroot}" != "" && rm -rf "%{buildroot}"
