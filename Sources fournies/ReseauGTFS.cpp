//
// Created by Mario Marchand on 16-12-30.
//

#include "ReseauGTFS.h"
#include <sys/time.h>
#include <tuple>


using namespace std;

//détermine le temps d'exécution (en microseconde) entre tv2 et tv2
long tempsExecution(const timeval &tv1, const timeval &tv2)
{
    const long unMillion = 1000000;
    long dt_usec = tv2.tv_usec - tv1.tv_usec;
    long dt_sec = tv2.tv_sec - tv1.tv_sec;
    long dtms = unMillion * dt_sec + dt_usec;
    if (dtms < 0) throw logic_error("ReaseauGTFS::tempsExecution(): dtms doit être non négatif");
    return dtms;
}

size_t ReseauGTFS::getNbArcsOrigineVersStations() const
{
    return m_nbArcsOrigineVersStations;
}

size_t ReseauGTFS::getNbArcsStationsVersDestination() const
{
    return m_nbArcsStationsVersDestination;
}

size_t ReseauGTFS::getNbArcs() const
{
    return m_leGraphe.getNbArcs();
}

double ReseauGTFS::getDistMaxMarche() const
{
    return distanceMaxMarche;
}

//! \brief construit le réseau GTFS à partir des données GTFS
//! \param[in] Un objet DonneesGTFS
//! \post constuit un réseau GTFS représenté par un graphe orienté pondéré avec poids non négatifs
//! \post initialise la variable m_origine_dest_ajoute à false car les points origine et destination ne font pas parti du graphe
//! \post insère les données requises dans m_arretDuSommet et m_sommetDeArret et construit le graphe m_leGraphe
ReseauGTFS::ReseauGTFS(const DonneesGTFS &p_gtfs)
        : m_leGraphe(p_gtfs.getNbArrets()), m_origine_dest_ajoute(false)
{
    //Le graphe possède p_gtfs.getNbArrets() sommets, mais il n'a pas encore d'arcs
    ajouterArcsVoyages(p_gtfs);
    ajouterArcsTransferts(p_gtfs);
}

//! \brief ajout des arcs dus aux voyages
//! \brief insère les arrêts (associés aux sommets) dans m_arretDuSommet et m_sommetDeArret
//! \throws logic_error si une incohérence est détecté lors de cette étape de construction du graphe
void ReseauGTFS::ajouterArcsVoyages(const DonneesGTFS & p_gtfs) {
    for (auto itVoyages = p_gtfs.getVoyages().begin(); itVoyages != p_gtfs.getVoyages().end(); itVoyages++) {
        Arret::Ptr arretPtrPrecedent = nullptr;
        for(const auto &itArrets : itVoyages->second.getArrets()){
            this->m_arretDuSommet.push_back(itArrets);
            this->m_sommetDeArret.emplace(itArrets, m_arretDuSommet.size()-1);
            if(arretPtrPrecedent != nullptr){
                unsigned int poidsSecondes = itArrets->getHeureArrivee() - arretPtrPrecedent->getHeureArrivee();
                m_leGraphe.ajouterArc(m_arretDuSommet.size()-2, m_arretDuSommet.size()-1, poidsSecondes);
            }
            arretPtrPrecedent = itArrets;
        }
    }
}


//! \brief ajouts des arcs dus aux transferts entre stations
//! \throws logic_error si une incohérence est détecté lors de cette étape de construction du graphe
void ReseauGTFS::ajouterArcsTransferts(const DonneesGTFS & p_gtfs) {
    auto voyages = p_gtfs.getVoyages();
    auto lignes = p_gtfs.getLignes();

    for (std::tuple<unsigned int, unsigned int, unsigned int> transfertTuple : p_gtfs.getTransferts()) {
        unsigned int station1 = get<0>(transfertTuple);
        unsigned int station2 = get<1>(transfertTuple);
        Station vraieStation1 = p_gtfs.getStations().find(station1)->second;
        Station vraieStation2 = p_gtfs.getStations().find(station2)->second;
        unsigned int minTime = get<2>(transfertTuple);


        for (auto arret1 : vraieStation1.getArrets()) {
            //Map contenant le poids de l'arret de la ligne contenant le plus petit poids
            std::map<string, unsigned int> map_LignePoids;
            //Map contenant le pointeur de l'arret de la ligne contenant le plus petit poids
            std::map<string, Arret::Ptr> map_LigneArretPtr;
            //on prend la ligne de l'arret 1
            string voyageId = arret1.second->getVoyageId();
            Voyage voyage1 = voyages[voyageId];
            unsigned int ligneId1 = voyage1.getLigne();
            Ligne ligne1 = lignes[ligneId1];
            string ligneNum1 = ligne1.getNumero();
            //on met dans les map les deuxiemes arrets dont l'heure d'arrivee est plus grande que l'heure d'arrivee de l'arret 1
            for (auto arret2 = lower_bound(vraieStation2.getArrets().begin(), vraieStation2.getArrets().end(), arret1);
                 arret2 != vraieStation2.getArrets().end(); arret2++) {
                //on prend le num de ligne de l'arret 2
                string voyageId = arret2->second->getVoyageId();
                Voyage voyage2 = voyages[voyageId];
                unsigned int ligneId2 = voyage2.getLigne();
                Ligne ligne2 = lignes[ligneId2];
                string ligneNum2 = ligne2.getNumero();

                if (ligneNum1 != ligneNum2) {
                    unsigned int poidsSecondes = arret2->first - arret1.first;
                    if (poidsSecondes >= minTime) {
                        map_LignePoids[ligneNum2] = poidsSecondes;
                        map_LigneArretPtr[ligneNum2] = arret2->second;
                    }
                }
            }
            for (auto itLignePoids = map_LignePoids.begin();
                 itLignePoids != map_LignePoids.end(); itLignePoids++) {
                string ligneId2 = itLignePoids->first;
                unsigned int poidsSecondes = itLignePoids->second;
                Arret::Ptr arret2 = map_LigneArretPtr[ligneId2];
                m_leGraphe.ajouterArc(m_sommetDeArret[arret1.second],
                                      m_sommetDeArret[arret2],
                                      poidsSecondes);
            }
        }
    }
}


//! \brief ajoute des arcs au réseau GTFS à partir des données GTFS
//! \brief Il s'agit des arcs allant du point origine vers une station si celle-ci est accessible à pieds et des arcs allant d'une station vers le point destination
//! \param[in] p_gtfs: un objet DonneesGTFS
//! \param[in] p_pointOrigine: les coordonnées GPS du point origine
//! \param[in] p_pointDestination: les coordonnées GPS du point destination
//! \throws logic_error si une incohérence est détecté lors de la construction du graphe
//! \post constuit un réseau GTFS représenté par un graphe orienté pondéré avec poids non négatifs
//! \post assigne la variable m_origine_dest_ajoute à true (car les points orignine et destination font parti du graphe)
//! \post insère dans m_sommetsVersDestination les numéros de sommets connctés au point destination
void ReseauGTFS::ajouterArcsOrigineDestination(const DonneesGTFS &p_gtfs, const Coordonnees &p_pointOrigine,
                                               const Coordonnees &p_pointDestination)
{
    auto voyages = p_gtfs.getVoyages();
    auto lignes = p_gtfs.getLignes();
    Heure heure = Heure();
    string voyageId = "VoyageId";
    unsigned int numSequence = 1;
    shared_ptr<Arret> arretPtrOrigine = make_shared<Arret>(this->stationIdOrigine, heure, heure, numSequence, voyageId);
    shared_ptr<Arret> arretPtrDestination = make_shared<Arret>(this->stationIdDestination, heure, heure, numSequence, voyageId);
    this->m_sommetOrigine = m_leGraphe.getNbArcs();
    this->m_sommetDestination = m_leGraphe.getNbArcs()+1;
    this->m_arretDuSommet.push_back(arretPtrOrigine);
    this->m_arretDuSommet.push_back(arretPtrDestination);
    this->m_sommetDeArret.emplace(arretPtrOrigine, m_sommetOrigine);
    this->m_sommetDeArret.emplace(arretPtrDestination, m_sommetDestination);

    m_leGraphe.resize(m_leGraphe.getNbArcs()+2);

    //map getstation = <idStation, Station>
    std::map<Station, unsigned int> map_Origine_StationPotentiellePoids;
    std::map<Station, unsigned int> map_Destination_StationPotentiellePoids;
    for(auto itStation = p_gtfs.getStations().begin(); itStation != p_gtfs.getStations().end(); itStation++){
        Coordonnees coordsStation = itStation->second.getCoords();
        double distanceMarcheOrigine = p_pointOrigine - coordsStation;
        double distanceMarcheDestination = p_pointDestination - coordsStation;
        if(distanceMarcheOrigine <= distanceMaxMarche){
            unsigned int poidsSecondes = distanceMarcheOrigine/vitesseDeMarche*3600;
            map_Origine_StationPotentiellePoids[itStation->second] = poidsSecondes;
        }
        if(distanceMarcheDestination <= distanceMaxMarche){
            unsigned int poidsSecondes = distanceMarcheOrigine/vitesseDeMarche*3600;
            map_Destination_StationPotentiellePoids[itStation->second] = poidsSecondes;
        }
    }
    for(auto mapStationPoids : map_Origine_StationPotentiellePoids){
        Station station = mapStationPoids.first;
        unsigned int poidsSecondesToStation = mapStationPoids.second;
        //Map contenant le poids de l'arret de la ligne contenant le plus petit poids
        std::map<string, unsigned int> map_LignePoids;
        //Map contenant le pointeur de l'arret de la ligne contenant le plus petit poids
        std::map<string, Arret::Ptr> map_LigneArretPtr;
        for (   auto itMapHeureArriveeArret = lower_bound(station.getArrets().begin(), station.getArrets().end(), poidsSecondesToStation);
                itMapHeureArriveeArret != station.getArrets().end(); itMapHeureArriveeArret++) {
            Arret::Ptr arretPtr = itMapHeureArriveeArret->second;
            if (p_gtfs.getTempsDebut().add_secondes(poidsSecondesToStation) <= arretPtr->getHeureArrivee()) {
                string voyageId = arretPtr->getVoyageId();
                Voyage voyage = voyages[voyageId];
                unsigned int ligneId = voyage.getLigne();
                Ligne ligne = lignes[ligneId];
                string ligneNum = ligne.getNumero();
                map_LignePoids[ligneNum] = poidsSecondesToStation;
                map_LigneArretPtr[ligneNum] = arretPtr;
            }
        }
        for (auto itLignePoids = map_LignePoids.begin();itLignePoids != map_LignePoids.end(); itLignePoids++) {
            string ligneId = itLignePoids->first;
            unsigned int poidsSecondes = itLignePoids->second;
            Arret::Ptr arret = map_LigneArretPtr[ligneId];
            m_leGraphe.ajouterArc(m_sommetOrigine,
                                  m_sommetDeArret[arret],
                                  poidsSecondes);
        }
    }
    for(auto mapStationPoids : map_Destination_StationPotentiellePoids){
        Station station = mapStationPoids.first;
        unsigned int poidsSecondesToDestination = mapStationPoids.second;
        //Map contenant le poids de l'arret de la ligne contenant le plus petit poids
        std::map<string, unsigned int> map_LignePoids;
        //Map contenant le pointeur de l'arret de la ligne contenant le plus petit poids
        std::map<string, Arret::Ptr> map_LigneArretPtr;
        for (   auto itMapHeureArriveeArret = lower_bound(station.getArrets().begin(), station.getArrets().end(), poidsSecondesToDestination);
                itMapHeureArriveeArret != station.getArrets().end(); itMapHeureArriveeArret++) {
            Arret::Ptr arretPtr = itMapHeureArriveeArret->second;
            if (p_gtfs.getTempsDebut().add_secondes(poidsSecondesToDestination) <= arretPtr->getHeureArrivee()) {
                string voyageId = arretPtr->getVoyageId();
                Voyage voyage = voyages[voyageId];
                unsigned int ligneId = voyage.getLigne();
                Ligne ligne = lignes[ligneId];
                string ligneNum = ligne.getNumero();
                map_LignePoids[ligneNum] = poidsSecondesToDestination;
                map_LigneArretPtr[ligneNum] = arretPtr;
            }
        }
        for (auto itLignePoids = map_LignePoids.begin();itLignePoids != map_LignePoids.end(); itLignePoids++) {
            string ligneId = itLignePoids->first;
            unsigned int poidsSecondes = itLignePoids->second;
            Arret::Ptr arret = map_LigneArretPtr[ligneId];
            m_leGraphe.ajouterArc(m_sommetDeArret[arret],
                                  m_sommetDestination,
                                  poidsSecondes);
        }
    }




}

//L’autre conteneur, qui est membre privé de ReseauGTFS, est le vecteur m_sommetsVersDestination qui contient
//les sommets possédant un arc vers le point destination. Ces sommets sont associés à des arrêts de stations
//où chacune de ces stations possède la propriété d’être à distance de marche du point destination. Le membre
//privé constant distanceMaxMarche fixe cette distance maximale de marche à 1.5 km. Donc toute station se
//trouvant, à « vol d’oiseau », à moins de 1.5 km du point destination pourra être utilisée et faire parti
//du trajet pour aller au point destination. L’utilisateur aura donc la possibilité de débarquer à n’importe
//quelle station se trouvant à moins de 1.5 km du point destination pour aller rejoindre à pieds le point
//destination. Le temps, en secondes, de ces trajets à pieds est donné par la distance parcourue (à vol d’oiseau)
//en km divisée par le vitesse de marche (donnée par l’attribut constant, vitesseDeMarche) fixée à 5 km/heure.


//! \brief Remet ReseauGTFS dans l'était qu'il était avant l'exécution de ReseauGTFS::ajouterArcsOrigineDestination()
//! \param[in] p_gtfs: un objet DonneesGTFS
//! \throws logic_error si une incohérence est détecté lors de la modification du graphe
//! \post Enlève de ReaseauGTFS tous les arcs allant du point source vers un arrêt de station et ceux allant d'un arrêt de station vers la destination
//! \post assigne la variable m_origine_dest_ajoute à false (les points orignine et destination sont enlevés du graphe)
//! \post enlève les données de m_sommetsVersDestination
void ReseauGTFS::enleverArcsOrigineDestination()
{
    //ajouter votre code ici
}


//! \brief Trouve le plus court chemin menant du point d'origine au point destination préalablement choisis
//! \brief Permet également d'affichier l'itinéraire du voyage et retourne le temps d'exécution de l'algorithme de plus court chemin utilisé
//! \param[in] p_afficherItineraire: true si on désire afficher l'itinéraire et false autrement
//! \param[out] p_tempsExecution: le temps d'exécution de l'algorithme de plus court chemin utilisé
//! \returns le temps du trajet en secondes (= numeric_limits<unsigned_int>::max() si la destination n'est pas atteignable)
//! \throws logic_error si un problème survient durant l'exécution de la méthode
unsigned int ReseauGTFS::itineraire(const DonneesGTFS &p_gtfs, bool p_afficherItineraire, long &p_tempsExecution) const
{
    if (!m_origine_dest_ajoute)
        throw logic_error(
                "ReseauGTFS::afficherItineraire(): il faut ajouter un point origine et un point destination avant d'obtenir un itinéraire");

    vector<size_t> chemin;

    timeval tv1;
    timeval tv2;
    if (gettimeofday(&tv1, 0) != 0)
        throw logic_error("ReseauGTFS::afficherItineraire(): gettimeofday() a échoué pour tv1");
    unsigned int tempsDuTrajet = m_leGraphe.plusCourtChemin(m_sommetOrigine, m_sommetDestination, chemin);
    if (gettimeofday(&tv2, 0) != 0)
        throw logic_error("ReseauGTFS::afficherItineraire(): gettimeofday() a échoué pour tv2");
    p_tempsExecution = tempsExecution(tv1, tv2);

    if (tempsDuTrajet == numeric_limits<unsigned int>::max())
    {
        if (p_afficherItineraire)
            cout << "La destination n'est pas atteignable de l'orignine avec cette distance maximale de marche" << endl;
        return tempsDuTrajet;
    }

    if (tempsDuTrajet == 0)
    {
        if (p_afficherItineraire) cout << "Vous êtes déjà situé à la destination demandée" << endl;
        return tempsDuTrajet;
    }

    //un chemin non trivial a été trouvé
    if (chemin.size() <= 2)
        throw logic_error("ReseauGTFS::afficherItineraire(): un chemin non trivial doit contenir au moins 3 sommets");

    if (m_arretDuSommet[chemin[0]]->getStationId() != stationIdOrigine)
    {
        cout << "m_arretDuSommet[chemin[0]]->getStationId() = " << m_arretDuSommet[chemin[0]]->getStationId() << endl;
        cout << "chemin[0] = " << chemin[0] << " m_sommetOrigine = " << m_sommetOrigine << endl;
        for (const auto & i : chemin)
        {
            cout << "chemin[" << i << "] = " << chemin[i] << endl;
        }
        throw logic_error("ReseauGTFS::afficherItineraire(): le premier noeud du chemin doit être le point origine");
    }
    if (m_arretDuSommet[chemin[chemin.size() - 1]]->getStationId() != stationIdDestination)
        throw logic_error(
                "ReseauGTFS::afficherItineraire(): le dernier noeud du chemin doit être le point destination");


    if (p_afficherItineraire)
    {
        std::cout << std::endl;
        std::cout << "=====================" << std::endl;
        std::cout << "     ITINÉRAIRE      " << std::endl;
        std::cout << "=====================" << std::endl;
        std::cout << std::endl;
    }

    if (p_afficherItineraire) cout << "Heure de départ du point d'origine: "  << p_gtfs.getTempsDebut() << endl;
    Arret::Ptr ptr_a = m_arretDuSommet.at(chemin[0]);
    Arret::Ptr ptr_b = m_arretDuSommet.at(chemin[1]);
    if (p_afficherItineraire)
        cout << "Rendez vous à la station " << p_gtfs.getStations().at(ptr_b->getStationId()) << endl;

    unsigned int sommet = 1;

    while (sommet < chemin.size() - 1)
    {
        ptr_a = ptr_b;
        ++sommet;
        ptr_b = m_arretDuSommet.at(chemin[sommet]);
        while (ptr_b->getStationId() == ptr_a->getStationId())
        {
            ptr_a = ptr_b;
            ++sommet;
            ptr_b = m_arretDuSommet.at(chemin[sommet]);
        }
        //on a changé de station
        if (ptr_b->getStationId() == stationIdDestination) //cas où on est arrivé à la destination
        {
            if (sommet != chemin.size() - 1)
                throw logic_error(
                        "ReseauGTFS::afficherItineraire(): incohérence de fin de chemin lors d'un changement de station");
            break;
        }
        if (sommet == chemin.size() - 1)
            throw logic_error("ReseauGTFS::afficherItineraire(): on ne devrait pas être arrivé à destination");
        //on a changé de station mais sommet n'est pas le noeud destination
        string voyage_id_a = ptr_a->getVoyageId();
        string voyage_id_b = ptr_b->getVoyageId();
        if (voyage_id_a != voyage_id_b) //on a changé de station à pieds
        {
            if (p_afficherItineraire)
                cout << "De cette station, rendez-vous à pieds à la station " << p_gtfs.getStations().at(ptr_b->getStationId()) << endl;
        }
        else //on a changé de station avec un voyage
        {
            Heure heure = ptr_a->getHeureArrivee();
            unsigned int ligne_id = p_gtfs.getVoyages().at(voyage_id_a).getLigne();
            string ligne_numero = p_gtfs.getLignes().at(ligne_id).getNumero();
            if (p_afficherItineraire)
                cout << "De cette station, prenez l'autobus numéro " << ligne_numero << " à l'heure " << heure << " "
                     << p_gtfs.getVoyages().at(voyage_id_a) << endl;
            //maintenant allons à la dernière station de ce voyage
            ptr_a = ptr_b;
            ++sommet;
            ptr_b = m_arretDuSommet.at(chemin[sommet]);
            while (ptr_b->getVoyageId() == ptr_a->getVoyageId())
            {
                ptr_a = ptr_b;
                ++sommet;
                ptr_b = m_arretDuSommet.at(chemin[sommet]);
            }
            //on a changé de voyage
            if (p_afficherItineraire)
                cout << "et arrêtez-vous à la station " << p_gtfs.getStations().at(ptr_a->getStationId()) << " à l'heure "
                     << ptr_a->getHeureArrivee() << endl;
            if (ptr_b->getStationId() == stationIdDestination) //cas où on est arrivé à la destination
            {
                if (sommet != chemin.size() - 1)
                    throw logic_error(
                            "ReseauGTFS::afficherItineraire(): incohérence de fin de chemin lors d'u changement de voyage");
                break;
            }
            if (ptr_a->getStationId() != ptr_b->getStationId()) //alors on s'est rendu à pieds à l'autre station
                if (p_afficherItineraire)
                    cout << "De cette station, rendez-vous à pieds à la station " << p_gtfs.getStations().at(ptr_b->getStationId()) << endl;
        }
    }

    if (p_afficherItineraire)
    {
        cout << "Déplacez-vous à pieds de cette station au point destination" << endl;
        cout << "Heure d'arrivée à la destination: " << p_gtfs.getTempsDebut().add_secondes(tempsDuTrajet) << endl;
    }
    unsigned int h = tempsDuTrajet / 3600;
    unsigned int reste_sec = tempsDuTrajet % 3600;
    unsigned int m = reste_sec / 60;
    unsigned int s = reste_sec % 60;
    if (p_afficherItineraire)
    {
        cout << "Durée du trajet: " << h << " heures, " << m << " minutes, " << s << " secondes" << endl;
    }

    return tempsDuTrajet;
}


