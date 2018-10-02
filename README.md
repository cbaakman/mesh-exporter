# XML Mesh 3.3.1
An xml mesh format with an exporter for blender 2.79b and a C++ import library.
It supports:
* Texture coordinates (UV) per face
* Subsets for grouping faces
* Skeletal animation

The import library does NOT handle rendering. That's up to the executable that imports it.
The visual test is an example of such an executable. There are however, many other ways to render the imported meshes.

See also the wiki at https://github.com/cbaakman/xml-mesh/wiki to learn how to work with this mesh format.

## Contents
* 'xml_exporter.py': the mesh exporter to include in blender
* An include dir with headers for the imiport library
* A src dir with the implementation of the import library
* A test dir, containing a visual test

## Dependencies
The exporter requires Blender 2.79b or higher. (https://www.blender.org/)

The importer requires:
* GNU/MinGW C++ compiler 4.7 or higher.
* GLM 0.9.9.2 or higher: https://glm.g-truc.net/0.9.9/index.html
* LibXML 2.0 or higher: http://xmlsoft.org/

For the visual test, the following is also required:
* Boost 1.62.0 or higher: https://www.boost.org/doc/libs/1_62_0/more/getting_started/windows.html
* LibPNG 1.6.35 or higher: http://www.libpng.org/pub/png/libpng.html
* OpenGL 3.2 or higher, should be installed on your OS by default, if the hardware supports it.
* GLEW 2.1.0 or higher: http://glew.sourceforge.net/
* LibSDL 2.0.8 or higher: https://www.libsdl.org/download-2.0.php

The 'build.cmd' and 'Makefile' builder scripts assume that these dependencies are located in the build path.
If not, then you might need to modify these scripts yourself or add/change environment variables.

## Building the import library
On linux, run 'make'. It will generate a .so file under 'lib'.

On Windows run 'build.cmd'. It will generate a .dll file under 'bin' and an import .a under 'lib'.

## Running the visual test
For the visual test, the blender exporter will be used to generate an xml mesh file.
This file will then be imported into an executable that shows the 3d model in a window.

On Linux, run 'make test'.

On Windows, the test is executed automatically when you build the library.

## Installing

### Installing the Exporter
Run blender and open a "user preferences" window. Go to Add-ons > Install Add-on from file. Then select the file and enable it.
Click "Save user settings" to save.

### Installing the import Library
On Linux, run 'make install'. Or run 'make uninstall' to undo the installation.

On Windows, add the .dll under 'bin', the import .a under 'lib' and the headers under 'include' to your build path.

## Using the Exporter
Simply select the exporter in blender under File > export > xml mesh(.xml)
This will generate xml mesh files, that can be imported using the import library.
