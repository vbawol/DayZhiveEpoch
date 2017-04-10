
# Dependincies
* The instructions for buiulding dependincies are based on VS 2017, modify MSVC versions according to your VS version
* Unless otherwise noted this guide is performed from the VS tools command prompt
* This project reuires visual studio to be run as an administrator!
1. Copy the HiveDeps folder into your C:\ drive

2. Install Python (any version works, use latest 3.x), and check add to PATH. click disable PATH limit then close installer https://www.python.org/downloads/

3. Download and build boost http://www.boost.org/ in the HiveDeps folder (We need the libs, prebuilt packages are not acceptable)
	* download the latest version of boost (this guide was made using 1.63)
	* Extract the files (not the root folder) to your C:\HiveDeps\boost directory

  #### Building Boost
  1. open up your visual studio command prompt
  2. ```CD C:\HiveDeps\boost```
  3. ```bootstrap.bat```
  4. ```b2.exe toolset=msvc-14.0 --build-type=complete variant=release,debug runtime-link=shared,static link=static threading=multi address-model=32 --without-log --stagedir=lib\x86\v141 --build-dir=out\x86\v141```
	  * NOTE: boost 1.63 has no build out of the box for VS2017, you must edit project-config.jam
	  ```using msvc ;``` -----> ```using msvc : 14.0 : "c:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.10.24728\bin\HostX64\x64\cl.exe";```

#### Building POCO
1. Download Poco here (We use 1.7.8): https://pocoproject.org/download/index.html and extract files to C:\HiveDeps\poco
2. POCO requires OpenSSL, MySQL Connector and ODBC (part of Windows SDK)
3. install Windows 8.1 SDK from this link if building fails https://developer.microsoft.com/en-us/windows/downloads/windows-8-1-sdk
4. download OpenSSL https://www.openssl.org/source/ and extract to C;\HiveDeps\openssl, rename openssl-VER to openssl, we need to configure OpenSSL
    * Download and install NASM http://www.nasm.us/
    * download and install PERL for windows, I used Strawberry PERL http://strawberryperl.com/releases.html
    * We need to build both MT and MD libraries
    * ```@set path=C:\Users\Nick-PC\AppData\Local\NASM;%path%```
    * ```@set path=C:\Strawberry\perl\bin;%path%```
    * ```CD C:\openssl```
    * ```perl Configure VC-WIN32```
    * ```nmake```
    * ```nmake test```
    * We use a buiuld script from https://www.npcglib.org/~stathis/blog/precompiled-openssl/ to build both static and dynamic (MT/MD) libraries for POCO.
    * alternatively you may just want to download the compiled version and copy over the libs, but there isn't a precompiled MSVC 2017 version yet
    * Download and install CygWin https://www.cygwin.com/ we need DOS2UNIX, PATCH, and 7Z (use view: FULL and search feature)
    * Download the applicable version of the openssl build script, I used build-openssl-1.1.0e.bat AND PATCH openssl-1.1.0e.patch then place in your C:\openssl folder
    * Download perlEnv.bat here: https://www.npcglib.org/~stathis/downloads/perlenv.bat and place in your perl root director
    * edit the build script, find and replace D:\ with C:\ or your applicable drive. Edit \opensource\ -> \ and in the buildpath variable remove ```-!PROJECT_VERSION!``` be sure to verify your CygWin and PERL paths!
    * Edit VS_VERSION to your version, and VISUAL_STUDIO_VC to the correct path where your VS install has vcvarsall.bat i.e. mine is ```C:\Program Files (x86)\Microsoft Visual Studio\!VS_VERSION!\Community\VC\Auxiliary\Build```
    * ```CD C:\openssl```
    * ```build-openssl-1.1.0e.bat patch build x86 static release```
    * ```build-openssl-1.1.0e.bat build all```
    * applicable files will be in the C:\Openssl-build-VER-VER and C:\Openssl-dist-VER-VER folders MD Release (C:\openssl-VER-VER\openssl-x86-shared-release-VER) MT Release (C:\openssl-VER-VER\openssl-x86-static-release-VER)
    * find libsslmd.lib and create a copy, rename to ssleay32, and libcrypto -> libeay32. same again for the .exp files (do the same for both MT and MD files) BE SURE TO CHANGE MT AND MD TO LOWERCASE!
    * Copy .lib and .exp files from the openssl folder to C:\openssl\include\lib (you may need to make the lib folder)
5. Download and build the MySQL client connector in the C:\HiveDeps\MySQL folder
[REQUIRES CMAKE]
    * https://dev.mysql.com/downloads/connector/c/ download the source code OS
    * Edit the Poco buildwin.cmd file to set MYSQL_DIR=C:\HiveDeps\MySQL
    * To build:
    * (if your CMake path is not set) @set path=C:\Program Files (x86)\CMake\bin;%path%
    * ```CD C:\HiveDeps\MySQL```
    * ```cmake```
    * Open ALL_BUILD.vcxproj and build the RELEASE VERSION of the solution
    * Copy the files in  C:\HiveDeps\MySQL\libmysql\Release to your C:\HiveDeps\poco\lib folder
    * in the C:\HiveDeps\MySQL folder create a new lib folder
    * Copy the files in the following directors to C:\HiveDeps\MySQL\
    1. C:\HiveDeps\MySQL\libmysql\Release + C:\HiveDeps\MySQL\libmysql\Debug
    * download this repo, https://github.com/rajkosto/deps-mysql and merge the include directories. DO NOT OVERWRITE ANY FILES
    * NOTE you need to clean and rebuild a Dynamically linked version of MySQL for the hive AFTER building poco! This is not covered, since it's actually not necessary unless you change the DatabaseMySQL.dll code
    3. ```CD C:\HiveDeps\poco```
    4. ```buildwin 150 build all both Win32 samples```
    4a. Replace 150 with your VS MSVC version

#### Building intel TBB
1. download from https://www.threadingbuildingblocks.org/download#stable-releases (guide is using 4.4 u5)
2. Extract to C:\HiveDeps\TBB
3. open C:\HiveDeps\TBB\build\vs2012\makefile.sln and update the solutions versions if prompted
4. Save all and close visual studio
5. CD C:\HiveDeps\TBB
6. gmake
7. Your output will be: C:\HiveDeps\TBB\build\windows_ia32_cl_vc14_release

#### Building detours
1. download the latest version of Microsoft Detours https://www.microsoft.com/en-us/download/details.aspx?id=52586 (extract to C:\kfw-2.6.5),
2. Extract files to the C:\HiveDeps\Detours folder
3. CD C:\HiveDeps\Detours\src
4. Overwrite your Makefile with the one in the root of the Detours folder in this Github (LINK)
4. Unset the Read Only property on MAKEFILE and edit the CFLAGS line to read ```CFLAGS=/W4 /WX /Zi /MTd /Gy /Gm- /Zl /Od /DDETOURS_BITS=$(DETOURS_BITS)```
5. ```nmake```

#### Building PostgreSQL
** PostgreSQL compilation is not covered in this tutorial as many users do not use it. You may use Rajkosto's Poco-Deps repository on Github**
** IN VS Right click on the PostgreSQL project and unload it **

#### Finally building the Hive
1. resolve your lib and include paths
    * if you are using the windows tooal and apps SDK version 10 you will need to add an include path ```$(UniversalCRT_IncludePath)``` to all projects
    * right click each project > properties > C/C++ > General > add ```$(UniversalCRT_IncludePath)``` to your additional includes directory (replace [VER] with your SDK version path)
    * replace text in the additional includes field with: ```%(AdditionalIncludeDirectories)$(UniversalCRT_IncludePath)C:\HiveDeps\boost;C:\Program Files (x86)\Windows Kits\10\Include\[VER]\ucrt```
    1. you need to edit the boost lib directory to the Library Directories field of project properties -> VC++ Directories. where VER is your MSVC VER
        * AND Add the ucrt libs directory to the field if you are using 10 SDK
        * add first to the input field, keep any text already in the box ```C:\HiveDeps\boost\lib\x86\VER\lib;```
        * FOR SDK 10 USERS ONLY! ```C:\HiveDeps\boost\lib\x86\VER\lib;C:\Program Files (x86)\Windows Kits\10\Lib\VER\ucrt\x86;```
    2. Edit TBB.props file if it does not reflect you current TBB lib path
        * ```$(MSBuildThisFileDirectory)build\windows_ia32_cl_vc14_release;``` ----> ```$(MSBuildThisFileDirectory)build\windows_ia32_cl_[YOUR VERSION]_release;```
2. if you recieve any missing include errors, google them, chances are the VS installation requires additional components

# Common Errors
1. Double check your include and library paths if you are missing any .lib .h files, or links. You need to change paths in this guide to YOUR actual path and version after building the dependincies
	* Also make sure your .props files are in the correct places and are up to date
2. ```:VCEnd exited with error code #```
	* Run VS as administrator, make sure you have python! Clean solution and rebuild
	* Ensure git is part of your environment variables (this project requires the git command for versioning) see the instructions here for directions: http://stackoverflow.com/questions/26620312/installing-git-in-path-with-github-client-for-windows
	* Environment variable edits require application restart!
	* You must be working out of a cloned repository using the github app for windows
	* The file ./source/HiveLib/version_gen.sh and version_gen.py may need to be edited ```gitVer=`git rev-list HEAD | head -n 1``` ---> ```gitVer=`git rev-list HEAD ^origin | head -n 1```
3. If you are building a debug build, you will need to change all of the static paths to the debug libraries, since I linked to the release libs in the props, and guide above
4. If your build is failing despite following the directions, ensure you select the release|Win32 Active config setting