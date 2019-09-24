TEMPLATE = subdirs

CONFIG += debug

SUBDIRS += common decoration theme

decoration.depends = common
theme.depends = common
