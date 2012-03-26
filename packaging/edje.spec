#sbs-git:slp/pkgs/e/edje edje 1.1.0+svn.69011slp2+build01 96cd9783918ce594c786d12a5107be27aec4d34b
Name:       edje
Summary:    Complex Graphical Design/Layout Engine
Version:    1.1.0+svn.69356slp2+build02
Release:    1
Group:      System/Libraries
License:    BSD
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
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(lua)


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
%{_bindir}/inkscape2edc
%{_bindir}/edje_external_inspector
%{_bindir}/edje_inspector
%{_libdir}/%{name}/utils/epp
%{_bindir}/edje_player
%{_bindir}/edje_cc
%{_bindir}/edje_decc
%{_bindir}/edje_recc
%{_datadir}/%{name}/include/edje.inc
