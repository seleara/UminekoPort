# UminekoPort
A port of the Umineko no Naku Koro ni PS3 games. Currently in a very, very early state, since I'm still deciphering the bytecode format.

## Setup
I'll try to switch over to CMake ASAP, in the meantime you can replace include/library paths of the dependencies yourself in the project settings of the Visual Studio project.

In order to run the game, you will need to create a `data` folder in the project folder `UminekoPort`, then either copy the `DATA.ROM` file of one of the original games (can be found in `PS3_GAME/USRDIR`, the first game is what is being used for development at the moment, so I recommend that one) there or create a symbolic link to it.

## License
UminekoPort is licensed under the MIT license.

## Dependencies
- GLFW (http://www.glfw.org/)
- GLEW (http://glew.sourceforge.net/)
- GLM (http://glm.g-truc.net)
