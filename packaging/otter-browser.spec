Name:          otter
Summary:       Otter Browser
Version:       0.9.04
Release:       0%{dist}
Group:         Applications/Internet
License:       GPLv3
URL:           https://github.com/Emdek/otter
Source:        https://github.com/Emdek/otter/archive/v%{version}.tar.gz
BuildRoot:     %{_tmppath}/v%{version}-build
Packager:      George Machitidze <giomac@gmail.com>
BuildRequires: cmake
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-qtmultimedia-devel
BuildRequires: qt5-qtscript-devel
BuildRequires: qt5-qtwebkit-devel

%description
Project aiming to recreate classic Opera (12.x) UI using Qt5

%prep
%autosetup -n %{name}-%{version}

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
%{_datadir}/icons/hicolor/*/apps/otter-browser.png
%{_datadir}/otter-browser/locale/*.qm

%changelog
* Sat Jan 10 2015 Mateusz Mielczarek <sysek@quebec.zapto.org> - Update BuildRequires, bump to newest version
* Tue Jun 10 2014 David Eder <david@eder.us> - Updated %files for icons and locale
* Sat Mar 22 2014 George Machitidze <giomac@gmail.com> - 0.3.01
- Initial RPM build
