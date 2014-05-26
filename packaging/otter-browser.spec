Name:          otter-browser
Summary:       Otter Browser
Version:       0.9.01-dev
Release:       0%{dist}
Group:         Applications/Internet
License:       GPLv3
URL:           https://github.com/Emdek/otter
Source:        %{name}-%{version}.tar.gz
BuildRoot:     %{_tmppath}/%{name}-%{version}-build
Packager:      George Machitidze <giomac@gmail.com>
BuildRequires: cmake

%description
Project aiming to recreate classic Opera (12.x) UI using Qt5

%prep
%setup -q

%install
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT
mkdir build
pushd build
cmake ../ -DCMAKE_INSTALL_PREFIX=%{_prefix}
make
make install DESTDIR=%{buildroot}
popd

install -d $RPM_BUILD_ROOT%{_datadir}/applications/
install -p -m 0644 *.desktop $RPM_BUILD_ROOT%{_datadir}/applications/

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc CHANGELOG COPYING README.md
%defattr(-,root,root)
%{_bindir}/*
%{_datadir}/applications/*.desktop

%changelog
* Sat Mar 22 2014 George Machitidze <giomac@gmail.com> - 0.3.01
- Initial RPM build
