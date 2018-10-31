cmake_minimum_required(VERSION 3.12)
project(TP1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(TP1
        arret.cpp
        arret.h
        auxiliaires.cpp
        auxiliaires.h
        coordonnees.cpp
        coordonnees.h
        DonneesGTFS.cpp
        DonneesGTFS.h
        ligne.cpp
        ligne.h
        main.cpp
        station.cpp
        station.h
        voyage.cpp
        voyage.h)
