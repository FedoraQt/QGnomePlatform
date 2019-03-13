QGnomePlatform
==========

QGnomePlatform is a Qt Platform Theme designed to use as many of the GNOME settings as possible in unmodified Qt applications. It allows Qt applications to fit into the environment as well as possible.

## How to compile

This library uses private Qt headers and will likely not be forward nor backward compatible. This library will have to be recompiled with every Qt update.

```
mkdir build
cd build
qmake-qt5 ..
make && make install
```

## Usage

This library is used automatically in Gtk based desktops such as Gnome, Cinnamon or Xfce.

This platform theme can also be used by setting the QT_QPA_PLATFORMTHEME environment variable to "gnome". For example, put the following command in `.bashrc`:

```
export QT_QPA_PLATFORMTHEME='gnome'
```

