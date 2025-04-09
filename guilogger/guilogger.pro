SUBDIRS += src
TEMPLATE = subdirs
QT += core gui widgets
CONFIG += warn_on \
          qt \
          thread \
          console

#CONFIG += debug

CONFIG -= app_bundle
