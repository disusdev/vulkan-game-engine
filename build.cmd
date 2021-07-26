
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

msbuild simple-game-engine.sln /p:Configuration=Debug /p:Platform=Win64 /p:applicationdatadir=./data/ /target:engine:rebuild /p:OutputPath=.bin/Debug/

