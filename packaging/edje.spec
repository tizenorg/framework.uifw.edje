#sbs-git:slp/pkgs/e/edje edje 1.1.0+svn.69011slp2+build03 96cd9783918ce594c786d12a5107be27aec4d34b
Name:       edje
Summary:    Complex Graphical Design/Layout Engine
Version:    1.7.2
Release:    1
Group:      System/Libraries
License:    BSD 2-clause and GPL-2.0+
URL:        http://www.enlightenment.org/
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-evas)
BuildRequires:  pkgconfig(ecore-file)
BuildRequires:  pkgconfig(ecore-imf)
BuildRequires:  pkgconfig(ecore-imf-evas)
BuildRequires:  pkgconfig(eet)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(embryo)
BuildRequires:  embryo-tools
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(lua)
BuildRequires:  pkgconfig(remix)
BuildRequires:  pkgconfig(sndfile)
BuildRequires:  pkgconfig(libpulse)
BuildRequires:  pkgconfig(libpng)

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
Requires:   embryo-tools
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
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/%{_datadir}/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}-tools


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_libdir}/libedje.so.*
%{_datadir}/mime/packages/edje.xml
%{_datadir}/%{name}/images/*
%{_libdir}/edje/modules/multisense_factory/*/module.so
%{_libdir}/remix/*.so*
%{_datadir}/license/%{name}
%manifest %{name}.manifest


%files devel
%defattr(-,root,root,-)
%{_includedir}/edje-1/*.h
%{_libdir}/libedje.so
%{_libdir}/pkgconfig/edje.pc
%{_datadir}/edje/examples/*


%files tools
%defattr(-,root,root,-)
%{_bindir}/*
%{_libdir}/%{name}/utils/epp
%{_datadir}/%{name}/include/edje.inc
%{_datadir}/license/%{name}-tools
%manifest %{name}-tools.manifest
