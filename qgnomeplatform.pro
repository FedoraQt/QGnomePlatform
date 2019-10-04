TEMPLATE = subdirs

SUBDIRS += common decoration theme

decoration.depends = common
theme.depends = common
