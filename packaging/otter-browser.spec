#
# Spec file for package Otter Browser
#
# Copyright © 2014 George Machitidze <giomac@gmail.com>
# Copyright © 2014 David Eder <david@eder.us>
# Copyright © 2014 Martin Gansser <martinkg@fedoraproject.org>
# Copyright © 2015 Mateusz Mielczarek <sysek@quebec.zapto.org>
# Copyright © 2015 Alexei Sorokin <sor.alexei@meowr.ru>
# Copyright © 2015 Markus S. <kamikazow@web.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

Name:           otter-browser
Summary:        Modern web browser inspired by Opera 12
Version:        0.9.04
Release:        0%{?dist}
Group:          Applications/Internet
License:        GPL-3.0+
Url:            http://otter-browser.org/
Source:         https://github.com/OtterBrowser/%{name}/archive/v%{version}.tar.gz#/%{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  desktop-file-utils
BuildRequires:  hicolor-icon-theme
BuildRequires:  pkgconfig(appstream-glib)
BuildRequires:  pkgconfig(Qt5Concurrent) >= 5.2
BuildRequires:  pkgconfig(Qt5Core) >= 5.2
BuildRequires:  pkgconfig(Qt5Gui) >= 5.2
BuildRequires:  pkgconfig(Qt5Multimedia) >= 5.2
BuildRequires:  pkgconfig(Qt5Network) >= 5.2
BuildRequires:  pkgconfig(Qt5PrintSupport) >= 5.2
BuildRequires:  pkgconfig(Qt5Script) >= 5.2
BuildRequires:  pkgconfig(Qt5WebKit) >= 5.2
BuildRequires:  pkgconfig(Qt5WebKitWidgets) >= 5.2
BuildRequires:  pkgconfig(Qt5Widgets) >= 5.2
BuildRequires:  pkgconfig(Qt5XmlPatterns) >= 5.2

%description
Otter Browser aims to recreate classic Opera (Presto) UI using Qt
and designed to give the user full control.

%prep
%setup -q

%build
mkdir build
pushd build
cmake ../ -DCMAKE_INSTALL_PREFIX=%{_prefix}
make %{?_smp_mflags}
popd

%install
pushd build
make install DESTDIR=%{buildroot}
popd
install -Dm 0644 packaging/%{name}.appdata.xml %{buildroot}%{_datadir}/appdata/%{name}.appdata.xml

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/*.desktop

# Exclude openSUSE. The appstream-glib package is broken there.
%if !0%{?suse_version}
appstream-util validate-relax --nonet %{buildroot}%{_datadir}/appdata/*.appdata.xml
%endif

%post
%desktop_database_post
%icon_theme_cache_post

%postun
%desktop_database_postun
%icon_theme_cache_postun

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc CHANGELOG COPYING README.md TODO
%{_bindir}/%{name}
%{_datadir}/%{name}/
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/appdata/%{name}.appdata.xml

%changelog
* Fri Apr  3 2015 Markus S. <kamikazow@web.de>
- Added license header
- Ported changes from Fedora's spec file by Martin Gansser <martinkg@fedoraproject.org>
- Ported changes from openSUSE's spec file by Alexei Sorokin <sor.alexei@meowr.ru>
- Made %description a bit longer.

* Sat Jan 10 2015 Mateusz Mielczarek <sysek@quebec.zapto.org>
- Update BuildRequires
- Bump to newest version

* Tue Jun 10 2014 David Eder <david@eder.us>
- Updated %files for icons and locale

* Sat Mar 22 2014 George Machitidze <giomac@gmail.com> - 0.3.01
- Initial RPM build
