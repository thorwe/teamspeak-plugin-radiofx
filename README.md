# teamspeak-plugin-radiofx

Walkie Talkie? Walkie Talkie!

The RadioFX plugin enables you to hear clients as if they'd be talking through a walkie talkie. Independent settings are possible for Current ServerTab, Whispers, Other Tabs and channel specific.

The RadioFX plugin isn't tied to any game and thereby fills this gap as a generic solution.

![Radio FX](https://github.com/thorwe/teamspeak-plugin-radiofx/raw/master/misc/screenie.png "Radio FX")

## Installation

It is recommended to install the plugin directly from within the client, so that it gets automatically updated.
In you TeamSpeak client, go to Tools->Options->Addons->Browse, search for the "RadioFX" plugin and install.

## Compiling yourself
After cloning, you'll have to manually initialize the submodules:
```
git submodule update --init --recursive
```

Qt in the minor version of the client is required, e.g.

```
mkdir build32 & pushd build32
cmake -G "Visual Studio 15 2017" -DCMAKE_PREFIX_PATH="path_to/Qt/5.6/msvc2015" ..
popd
mkdir build64 & pushd build64
cmake -G "Visual Studio 15 2017 Win64"  -DCMAKE_PREFIX_PATH="path_to/Qt/5.6/msvc2015_64" ..
```
