
#ifndef UNION_NO_WARNINGS
#ifndef USED_LANG
#define RU 0
#define EN 1
#define USED_LANG EN
#define USED_LANG_AUTO_DEFINITION
#endif

#if USED_LANG == EN

#ifdef _Win64
#error The plugin can't be compiled as a x64 application. Change to x32 and try to compile again.
#endif

#ifdef _DEBUG
#pragma message( "\nProject building is in DEBUG. Final building must be in RELEASE.\n" )
#endif

#if _MSC_VER > 1600
#pragma message( \
"\n---------------------------------------------------------------------------------\n\
!!! WARNING !!!\n\
Plugins compiled higher than 2010 version may work unstable!\n\
You should change the compiler version to v100.\n\
If you don't have this version download this compiler set or\n\
Microsoft Visual Studio 2010, then go to project settings \n\
and choose v100 version.\n"\
)
#pragma message( "Current platform version: " )

#if _MSC_VER <= 1200
#pragma message( "Visual Studio 6.0 (v60)" )
#elif _MSC_VER <= 1300
#pragma message( "Visual Studio 2002 (v70)" )
#elif _MSC_VER <= 1310
#pragma message( "Visual Studio 2003 (v71)" )
#elif _MSC_VER <= 1400
#pragma message( "Visual Studio 2005 (v80)" )
#elif _MSC_VER <= 1500
#pragma message( "Visual Studio 2008 (v90)" )
#elif _MSC_VER <= 1600
#pragma message( "Visual Studio 2010 (v100)" )
#elif _MSC_VER <= 1700
#pragma message( "Visual Studio 2012 (v110)" )
#elif _MSC_VER <= 1800
#pragma message( "Visual Studio 2013 (v120)" )
#elif _MSC_VER <= 1900
#pragma message( "Visual Studio 2015 (v140)" )
#elif _MSC_VER <= 1910
#pragma message( "Visual Studio 2017 (v150)" )
#elif _MSC_VER <= 1920
#pragma message( "Visual Studio 2019 (v160)" )
#else
#pragma message( "Higher than Visual Studio 2019 (v160)" )
#endif

#pragma message( "\n\
Current building process will continue.\
\n---------------------------------------------------------------------------------\n\n" \
)
#endif

#endif

#ifdef USED_LANG_AUTO_DEFINITION
#undef USED_LANG
#undef EN
#undef RU
#endif
#endif

#pragma warning(disable:4005)
#pragma warning(disable:4805)
#pragma warning(disable:4800)
#pragma warning(disable:4482)
#pragma warning(disable:4234)
#pragma warning(disable:4099)
#pragma warning(disable:4275)
#pragma warning(disable:4715)
#pragma warning(disable:4244)
#pragma warning(disable:4251)
#pragma warning(disable:4731)
#pragma warning(disable:4166)
#pragma warning(disable:4200)
#pragma warning(disable:4305)
#pragma warning(disable:4595)
#define _CRT_SECURE_NO_WARNINGS
