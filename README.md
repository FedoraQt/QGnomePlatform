QGnomePlatform
==========

QGnomePlatform is a Qt Platform Theme aimed to accommodate as much of GNOME settings as possible and utilize them in Qt applications without modifying them - making them fit into the environment as well as possible.

## How to compile

Please note this library uses private Qt headers. It's most likely won't be forward nor backward compatible. This means you'll have to recompile it with every Qt update.


```
mkdir build
cd build
qmake-qt5 ..
make && make install
```

## Usage

You can use this platform theme by setting the QT_QPA_PLATFORMTHEME to "qgnomeplatform" - so for example putting the following command in your `.bashrc` works well:

```
export QT_QPA_PLATFORMTHEME='qgnomeplatform'
```

