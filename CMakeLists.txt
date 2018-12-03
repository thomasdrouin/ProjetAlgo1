cmake_minimum_required(VERSION 3.12)
project(ProjetAlgo1)

set(CMAKE_CXX_STANDARD 14)

include_directories("Sources fournies")

add_executable(ProjetAlgo1
        RTC-1aout-30nov/__licence.txt
        RTC-1aout-30nov/__lisez-moi.txt
        RTC-1aout-30nov/agency.txt
        RTC-1aout-30nov/calendar_dates.txt
        RTC-1aout-30nov/feed_info.txt
        RTC-1aout-30nov/Horaire_Boucle_Partage.txt
        RTC-1aout-30nov/routes.txt
        RTC-1aout-30nov/shapes.txt
        RTC-1aout-30nov/stop_times.txt
        RTC-1aout-30nov/stops.txt
        RTC-1aout-30nov/transfers.txt
        RTC-1aout-30nov/trips.txt
        "Sources fournies/cmake-build-debug/cmake_install.cmake"
        "Sources fournies/cmake-build-debug/CMakeCache.txt"
        "Sources fournies/cmake-build-debug/Makefile"
        "Sources fournies/cmake-build-debug/TP1"
        "Sources fournies/cmake-build-debug/TP1.cbp"
        "Sources fournies/arret.cpp"
        "Sources fournies/arret.h"
        "Sources fournies/auxiliaires.cpp"
        "Sources fournies/auxiliaires.h"
        "Sources fournies/CMakeLists.txt"
        "Sources fournies/coordonnees.cpp"
        "Sources fournies/coordonnees.h"
        "Sources fournies/DonneesGTFS.cpp"
        "Sources fournies/DonneesGTFS.h"
        "Sources fournies/graphe.cpp"
        "Sources fournies/graphe.h"
        "Sources fournies/libTP1.a"
        "Sources fournies/ligne.cpp"
        "Sources fournies/ligne.h"
        "Sources fournies/main.cpp"
        "Sources fournies/ReseauGTFS.cpp"
        "Sources fournies/ReseauGTFS.h"
        "Sources fournies/station.cpp"
        "Sources fournies/station.h"
        "Sources fournies/voyage.cpp"
        "Sources fournies/voyage.h"
        )
