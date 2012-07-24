#sbs-git:slp/pkgs/e/edje edje 1.1.0+svn.69011slp2+build03 96cd9783918ce594c786d12a5107be27aec4d34b
Name:       edje
Summary:    Complex Graphical Design/Layout Engine
Version:    1.2.0+svn.73008slp2+build03
Release:    1
Group:      System/Libraries
License:    BSD
URL:        http://www.enlightenment.org/
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  eina-devel
BuildRequires:  eet-devel
BuildRequires:  embryo-devel
BuildRequires:  evas-devel
BuildRequires:  ecore-devel
BuildRequires:  liblua-devel
BuildRequires:  libremix-devel
BuildRequires:  libflac-devel
BuildRequires:  libsndfile-devel


%description
Various binaries for use with libedje
Edje is a graphical layout and animation library for animated resizable,
 compressed and scalable themes. It is the theming engine behind
 Enlightenment DR 0.17.
 .
 This package contains the following binaries:
  - edje_cc: Compiles EDC files.
  - edje_decc: Used to decompile compiled edje files.
  - edje_recc: A convenience script to recompile EDC files.

%package devel
Summary:    Complex Graphical Design/Layout Engine (devel)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Edje is a graphical layout and animation library (devel)

%package tools
Summary:    Complex Graphical Design/Layout Engine (tools)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Provides:   %{name}-bin
Obsoletes:  %{name}-bin

%description tools
Edje is a graphical layout and animation library (tools)

%prep
%setup -q

%build
export CFLAGS+=" -fvisibility=hidden -ffast-math -fPIC"
export LDFLAGS+=" -fvisibility=hidden -Wl,--hash-style=both -Wl,--as-needed"

%autogen --disable-static
%configure --disable-static
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libedje.so.*
%{_libdir}/remix/*.so
%{_datadir}/mime/packages/edje.xml
%{_libdir}/edje/modules/multisense_factory/*/module.so

%files devel
%defattr(-,root,root,-)
%{_includedir}/edje-1/*.h
%{_libdir}/libedje.so
%{_libdir}/pkgconfig/edje.pc
%exclude /usr/share/edje/examples/*

%files tools
%defattr(-,root,root,-)
%{_bindir}/*
%{_libdir}/%{name}/utils/epp
%{_datadir}/%{name}/include/edje.inc
