set CXX=g++
set CFLAGS=-std=c++17
set VERSION=3.2.0
set LIB_NAME=xml-mesh
set BLENDER="C:\Program Files\Blender Foundation\Blender\blender.exe"


if not exist obj (mkdir obj)
if not exist bin (mkdir bin)
if not exist lib (mkdir lib)


del /Q /F /S obj\* bin\%LIB_NAME%-%VERSION%.dll lib\lib%LIB_NAME%.a data\dummy.xml

:: Export the test mesh.
%BLENDER% data\dummy.blend --background --python xml_exporter.py -- dummy data\dummy.xml
@if %ERRORLEVEL% neq 0 (
    goto end
)

:: Make the library.

@for %%m in (parse iter math animate) do (
    %CXX% %CFLAGS% -I include\xml-mesh -c src\%%m.cpp -o obj\%%m.o -fPIC

    @if %ERRORLEVEL% neq 0 (
        goto end
    )
)

%CXX% obj\parse.o obj\iter.o obj\math.o obj\animate.o -lxml2 -llinear-gl ^
-o bin\%LIB_NAME%-%VERSION%.dll -shared -fPIC -Wl,--out-implib,lib\lib%LIB_NAME%.a
@if %ERRORLEVEL% neq 0 (
    goto end
)

:: Run the visual test.

%CXX% %CFLAGS% -I include\xml-mesh -L lib tests\visual.cpp ^
-l%LIB_NAME% -lopengl32 -lglew32 -lmingw32 -lSDL2main -lSDL2 -lpng -lboost_filesystem -lboost_system -llinear-gl ^
-o bin\visual.exe && bin\visual.exe data\dummy.xml data\dummy.png run

@if %ERRORLEVEL% neq 0 (
    echo Test failed with exit code 0x%=ExitCode%
)

:end
