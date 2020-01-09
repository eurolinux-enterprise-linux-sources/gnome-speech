Summary: GNOME Desktop text-to-speech services
Name: gnome-speech
Version: 0.4.25
Release: 1
License: LGPL
Group: Desktop/Accessibility
URL: http://www.gnome.org/
Source0: gnome-speech-%{version}.tar.gz
BuildRoot: %{_tmppath}/gnome-speech-%{version}-%{release}-buildroot

%description
GNOME Speech

%package devel
Summary:	The files needed for developing a gnome-speech driver
Group:		Development/Libraries
Requires:	%name = %{PACKAGE_VERSION}
Requires:	libbonobo-devel

%description devel
The files needed for developing a gnome-speech driver

%package festival
Summary:	The gnome-speech driver for Festival
Group:		Desktop/Accessibility
Requires:	%name = %{PACKAGE_VERSION}
Requires:	festival

%description festival
The gnome-speech Festival driver

%package dectalk
Summary:	The gnome-speech driver for DECtalk
Group:		Desktop/Accessibility
Requires:	%name = %{PACKAGE_VERSION}
AutoReqProv: off

%description dectalk
The gnome-speech driver for DECtalk

%package swift
Summary:	The gnome-speech driver for the Cepstral Swift engine
Group:		Desktop/Accessibility
Requires:	%name = %{PACKAGE_VERSION}
AutoReqProv: off

%description swift
The gnome-speech for the Cepstral Swift engine

%package theta
Summary:	The gnome-speech driver for the Cepstral Theta engine
Group:		Desktop/Accessibility
Requires:	%name = %{PACKAGE_VERSION}
AutoReqProv: off

%description theta
The gnome-speech Theta driver

%package loquendo
Summary:	The gnome-speech driver for Loquendo
Group:		Desktop/Accessibility
Requires:	%name = %{PACKAGE_VERSION}
AutoReqProv: off

%description loquendo
The gnome-speech driver for Loquendo

%package ibmtts
Summary:	The gnome-speech driver for IBM ViaVoice Text-to-Speech
Group:		Desktop/Accessibility
Requires:	%name = %{PACKAGE_VERSION}
AutoReqProv: off

%description ibmtts
The gnome-speech driver for IBM ViaVoice Text-to-Speech

%package espeak
Summary:	The gnome-speech driver for eSpeak
Group:		Desktop/Accessibility
Requires:	%name = %{PACKAGE_VERSION}
AutoReqProv: off

%description espeak
The gnome-speech driver for eSpeak



%prep
%setup -q

%build
%configure
make

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc
%{_libdir}/lib*.so.*

%files devel
%defattr(-,root,root,-)
%{_libdir}/lib*.so
%{_includedir}/gnome-speech-1.0
%{_libdir}/pkgconfig/*pc
%{_datadir}/idl/gnome-speech-1.0
%{_bindir}/test-speech

%files festival
%{_bindir}/festival-synthesis-driver
%{_libdir}/bonobo/servers/GNOME_Speech_SynthesisDriver_Festival.server

%files dectalk
%{_bindir}/dectalk-synthesis-driver
%{_libdir}/bonobo/servers/GNOME_Speech_SynthesisDriver_Dectalk.server

%files swift
%{_bindir}/swift-synthesis-driver
%{_libdir}/bonobo/servers/GNOME_Speech_SynthesisDriver_Swift.server

%files theta
%{_bindir}/theta-synthesis-driver
%{_libdir}/bonobo/servers/GNOME_Speech_SynthesisDriver_Theta.server

%files loquendo
%{_bindir}/loquendo-synthesis-driver
%{_libdir}/bonobo/servers/GNOME_Speech_SynthesisDriver_Loquendo.server

%files ibmtts
%{_bindir}/viavoice-synthesis-driver
%{_libdir}/bonobo/servers/GNOME_Speech_SynthesisDriver_Viavoice.server

%files espeak
%{_bindir}/espeak-synthesis-driver
%{_libdir}/bonobo/servers/GNOME_Speech_SynthesisDriver_Espeak.server

%changelog
* Wed Jan 10 2007 Gilles Casse <gcasse@oralux.org>:
  Add eSpeak driver.
* Sun Dec 13 2006 Willie Walker <william.walker@sun.com>:
  Add viavoice driver and update wording.
* Sun Dec  3 2006 Willie Walker <william.walker@sun.com>:
  Add swift driver.
* Mon Jun  7 2004 Marc Mulcahy <marc.mulcahy@sun.com>:
  Split each driver into its own package.
  Re-added the ORBit typelibs for use with Orca.
* Fri May  9 2003 Jonathan Blandford <jrb@redhat.com> speech-1
- Initial build.


