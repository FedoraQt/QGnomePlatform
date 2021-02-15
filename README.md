QGnomePlatform
==========

QGnomePlatform is a Qt Platform Theme designed to use as many of the GNOME settings as possible in unmodified Qt applications. It allows Qt applications to fit into the environment as well as possible.

## How to compile

This library uses private Qt headers and will likely not be forward nor backward compatible. This library will have to be recompiled with every Qt update.

```
mkdir build
cd build
cmake [OPTIONS] ..
make && make install
```

## Usage

This library is used automatically in Gtk based desktops such as Gnome, Cinnamon or Xfce.

This platform theme can also be used by setting the QT_QPA_PLATFORMTHEME environment variable to "gnome". For example, put the following command in `.bashrc`:

```
export QT_QPA_PLATFORMTHEME='gnome'
```

## License
Most code is under [LGPL 2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html) with the "or any later version" clause. New code should be contributed under this license.

This project also incorporates some code from the Qt Project. Because of that the so-called combined work is licensed under [LGPL 3.0-only](https://www.gnu.org/licenses/lgpl-3.0), [GPL 2.0](https://www.gnu.org/licenses/old-licenses/gpl-2.0), [GPL 3.0](https://www.gnu.org/licenses/gpl-3.0), or any later GPL version approved by the [KDE Free Qt Foundation](https://kde.org/community/whatiskde/kdefreeqtfoundation/).
