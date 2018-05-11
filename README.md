# UminekoPort
A port of the Umineko no Naku Koro ni PS3 games (and Higurashi Sui too in the future!). Currently in a very, very early state, since I'm still deciphering the bytecode format.

## Setup
I'll try to switch over to CMake ASAP, in the meantime you can replace include/library paths of the dependencies yourself in the project settings of the Visual Studio project.

In order to run the game, you will need to create a `data`/`chiru_data`/`higu_data` folder in the project folder `UminekoPort` (depending on which game you wish to play), then either copy the `DATA.ROM` file of one of the original games (can be found in `PS3_GAME/USRDIR`, the first game is what is being used for development at the moment, so I recommend that one) there or create a symbolic link to it.

At this point in time neither Chiru nor Higurashi Sui get very far after booting. If you want to give them a try anyway, change the static `game` variable at the top of `src/engine/engine.cc` to either `"chiru"` or `"higu"` and recompile.

## License
UminekoPort is licensed under the MIT license.

## Dependencies
- GLFW (http://www.glfw.org/)
- GLEW (http://glew.sourceforge.net/)
- GLM (http://glm.g-truc.net)
