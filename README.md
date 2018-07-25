# janus-vr

Janus VR source code is released with a GPLv3 license:

https://www.gnu.org/licenses/gpl.html

Additional deployment notes:

- Before building, copy and rename the following: riftid.txt.sample
to riftid.txt and gearid.txt.sample to gearid.txt. Update these files
with specific app IDs where necessary (for publishing to platforms
like Oculus Home or Android/Google Play storefronts).
- In order to build a signed version for Android, a key store file
must be generated. Copy and rename the following:
android/keystore.properties.sample to android/keystore.properties.
Update this file with the correct values of the generated keystore.

Building on windows
===================

1) Download and install MSVC 2017 community version (64-bit).  
   Do not forget to check the "C++ development box".

2) Download and install Qt 5.9.4 
   https://download.qt.io/official_releases/qt/5.9/5.9.4/

   Make sure to select the release compatible with 64-bit MSVC 2017,
   and check QtScript.

   During the installation, make sure msvc2017-64bit is selected
   in the "Select Components" screen.

3) Clone the janus-vr repo

4) Open the project in Qt Creator by pointing it towards
   $CONTAINING_DIR\janus-vr\Firebox.pro

   (At this point, the project should build successfully.)

5) Download and install the latest Janus release from janusvr.com.
   Go to the install location (e.g. C:\Program Files\Janus VR) and
   copy the assets and other plugin folders. Copy the .dll files in 
   the janus-vr repo in dependencies\windows. Also copy 
   janusvr_websurface.exe.  Paste the things you copied in your build 
   folder (where the janusvr.exe that you built is located). 

   - If your build is a release build, you should now be able to run your 
     executable.
   - If your build is a debug build, copy and paste the Qt debug dlls 
   	 to the same location. Those can be found where you installed Qt
     (e.g. C:\Qt\5.9.4\msvc2017\bin)

   Qt Creator will look for those files in a different location, so in 
   order to run from inside Qt Creator, you will also need to paste 
   those things in the folder containing janusvr.exe (one level up from 
   where you just pasted).

6. For debugging on Windows when compiling with MSVC 2017, you will need 
   to install additional Debugging Tools for Windows, which are packaged 
   with the Windows Driver Kit: 

https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit

Building on linux (Ubuntu 18.04 LTS)
================================================

=== Automated ===

1) Clone the repository using Git:
	`git clone https://github.com/janusvr/janus`

2) Change to the source directory (`cd janus`) and run the automated build script:
	`./build-janusvr-linux.sh`

3) Once completed, your new JanusVR build can be found inside `dist/linux/` in the root of your source repo.
   To run Janus, just type `dist/linux/janusvr -render 2d`

=== Manual ===

For now, to compile manually please read the instructions found in the automated build script that
is located in the root of the source repository (`build-janusvr-linux.sh`).

(Optional, for reference) Build and install OpenEXR:

   - cd ./resources/openexr-2.2.0
   - ./configure
   - make
   - sudo make install

  Note: if updating cmft, edit main.lua and comment out strip(), 
  otherwise the generated libcmftRelease.a will not work

Building on OS X
================

1) Install Qt 5.5 and Qt Creator (http://download.qt.io/archive/qt/5.5);
   during installation make sure clang 64-bit is selected in the
   "Select Components" screen 

2) Clone the janus-vr repo

3) Install remaining library dependencies: 

   - brew install opus
   - brew install bullet
   - brew install libvorbis
   - brew install mpg123
   - brew install assimp
   
   If brew is not installed, copy and paste the following into the
   terminal:
   
   - /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
   
4) Build Janus from inside QtCreator. Download and install the latest
   Janus release from janusvr.com. Go to the install location and view
   the Contents of the app. Copy the assets folder and .dylib (e.g. libLeap
   and libopenvr_api) files (in Applications/janusvr.app/Contents/MacOS/)
   and paste them to the built .app (e.g. in $BUILD_DIR/janusvr.app/Contents/MacOS/)

5) Sign the app with the following (if you have the correct certificate):

   - codesign --deep janusvr.app -s "Janus VR Inc."

6) Use macdeployqt to deploy Janus as a .dmg file. From the terminal, go
   to the build directory and enter the following comamand (replacing the
   user name):

   - /Users/user/Qt/5.5/clang_64/bin/macdeployqt ./janusvr.app -always-overwrite -dmg janusvr.dmg
	
   The .app should now be executable.
   
   Note: If the macdeployqt does not work, you may need to install mysql55 and
   libpq (and correctly link the files) as described here:
   
   - https://v-play.net/developers/forums/t/macdeployqt

Building for Android (on Mac/Windows)
=====================================

1) Download and install the stuff mentioned here: http://doc.qt.io/qt-5/androidgs.html

	- Make sure to use a copy of SDK tools online 25.2.2 (paste and overwrite the tools in the sdk folder, $SDK_DIR) and NDK 10e (the most current SDK/NDK do not work with QtCreator)
	
		- Use: http://stackoverflow.com/questions/27043522/where-can-i-download-an-older-version-of-the-android-sdk and http://stackoverflow.com/questions/6849981/where-do-i-find-old-versions-of-android-ndk for reference
		
2) Make sure JDK/bin is higher in the PATH environment variable; Make sure stuff in the PATH environment variable does not contain sh.exe (e.g. Git; if so remove it from the PATH)

3) Install system images of desired OS that are needed through $SDK_DIR/tools/android.bat

4) Create AVDs through $SDK_DIR/tools/android.bat with the argument "avd"

5) In QtCreator, go to Tools>Options>Android; set the JDK location, Android SDK location, Android NDK location, Ant executable (location of ant.bat)

6) For Windows, make sure emulator is on while building/installing/running an application. Apps need to be built from command line or the build steps in QtCreator must be changed:

	- After running QMake either from the command line or QtCreator, run the following in the directory with the makefile:
	
		- C:\Qt\Tools\mingw492_32\bin\mingw32-make install INSTALL_ROOT=.\Deploy
		- androiddeployqt --output .\Deploy --install --input android-lib2dpainting.so-deployment-settings.json

Note: For the Android build, libxul is not in the repo (due to file size limits). You will need to have a local copy of it in the dependencies folder in order to build the app.

ADB Tool Tips
=====================================

ADB is a tool used for communicating with Android devices. Some useful commands:

    - For obtaining debug logs:
                - adb logcat
    - Clearing debug log
                - adb logcat -c
    - For perf info for the Go:
                - adb logcat | grep -e UtilPoller -e VrApi
    - For installing/uninstalling Janus APKs:
                - adb install <APKNAME>
                - adb uninstall org.janusvr
    - For starting and stopping the Janus app
                - adb shell am start org.janusvr/org.janus.JanusActivity
                - adb shell am force-stop org.janusvr
    - For setting properties used by the Go:
                - adb shell setprop debug.oculus.foveation.level 3
                - adb shell setprop debug.oculus.adaclocks.force 0


Common issues and fixes
=======================

- If you encounter mysterious linking errors after having just added
  a class, try cleaning the project, running qmake, and rebuilding.
   
