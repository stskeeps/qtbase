Name:       qt5-qtbase-eglfs-hwcomposer
Summary:    Eglfs hwcomposer with 
Version:    0.0~git855.e5601d283c
Release:    1%{?dist}
Group:      Qt/Qt
License:    LGPLv2.1 with exception or GPLv3
URL:        http://qt.nokia.com
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5OpenGL)
BuildRequires:  pkgconfig(Qt5PlatformSupport)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Declarative)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5V8)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(udev)
BuildRequires:  libhybris-devel
BuildRequires:  libhybris-libEGL-devel
BuildRequires:  libhybris-libGLESv2-devel

BuildRequires:  fdupes

%description
Qt is a cross-platform application and UI framework. Using Qt, you can
write web-enabled applications once and deploy them across desktop,
mobile and embedded systems without rewriting the source code.
.

This contains a eglfs plugin for Droid hwcomposer

%prep
%setup -q -n %{name}-%{version}/qtbase

%build
export QTDIR=/usr/share/qt5
touch .git
cd src/plugins/platforms/eglfs
qmake -qt=5 

make %{?_smp_flags}

%install
rm -rf %{buildroot}
cd src/plugins/platforms/eglfs

%qmake5_install

#### Pre/Post section

#### File section

%files
%defattr(-,root,root,-)
%{_libdir}/qt5/plugins/platforms/libqeglfs_hwcomposer.so

### No changelog section, separate $pkg.changes contains the history
