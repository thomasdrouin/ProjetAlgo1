//
// Created by Mario Marchand on 16-12-30.
//

#include "ReseauGTFS.h"
#include <sys/time.h>

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
void ReseauGTFS::ajouterArcsVoyages(const DonneesGTFS & p_gtfs)
{
    try {
        std::map<std::string, Voyage> voyages = p_gtfs.getVoyages();
        unsigned int poids;
        size_t i = 0;
        for (auto &itr:voyages) {
            auto arrets = itr.second.getArrets();
            for (auto &itrArret:arrets) {
                m_arretDuSommet.push_back(itrArret);
                m_sommetDeArret.emplace(itrArret, i);
                if (itrArret != *arrets.begin()) {
                    poids = itrArret->getHeureArrivee() - m_arretDuSommet[i - 1]->getHeureArrivee();
                    m_leGraphe.ajouterArc(i - 1, i, poids);
                }
                i++;
            }
        }
    }
    catch (exception e){
        throw logic_error("ReseauGTFS::ajouterArcsVoyages(const DonneesGTFS & p_gtfs): Incohérence détectée.");
    }
}


//! \brief ajouts des arcs dus aux transferts entre stations
//! \throws logic_error si une incohérence est détecté lors de cette étape de construction du graphe
void ReseauGTFS::ajouterArcsTransferts(const DonneesGTFS & p_gtfs)
{
    try {
        auto transferts = p_gtfs.getTransferts();
        auto stations = p_gtfs.getStations();
        auto voyages = p_gtfs.getVoyages();
        for (auto &transfert:transferts) {
            unsigned int id_station1 = std::get<0>(transfert);
            Station station1 = stations.at(id_station1);
            unsigned int id_station2 = std::get<1>(transfert);
            Station station2 = stations.at(id_station2);
            unsigned int temps_minimal = std::get<2>(transfert);
            auto arretsStation1 = station1.getArrets();
            auto arretsStation2 = station2.getArrets();
            for (auto &arret1:arretsStation1) {
                size_t i = m_sommetDeArret.at(arret1.second);

                std::set<std::string> lignes_ajoutees;
                std::string id = arret1.second->getVoyageId();

                auto ligne1_id = voyages.at(id).getLigne();
                auto ligne1 = p_gtfs.getLignes().at(ligne1_id).getNumero();
                lignes_ajoutees.emplace(ligne1);
                for (auto arret2 = arretsStation2.lower_bound(arret1.first); arret2 != arretsStation2.end(); ++arret2) {
                    string id2 = arret2->second->getVoyageId();
                    auto ligne2_id = voyages.at(id2).getLigne();
                    auto ligne2 = p_gtfs.getLignes().at(ligne2_id).getNumero();
                    unsigned int poids = arret2->first - arret1.first;
                    if (temps_minimal <= poids) {
                        auto res = lignes_ajoutees.emplace(ligne2);
                        if (res.second) {
                            size_t j = m_sommetDeArret.at(arret2->second);

                            m_leGraphe.ajouterArc(i, j, poids);
                        }
                    }
                }
            }
        }
    }
    catch(exception e){
        throw logic_error("ReseauGTFS::ajouterArcsTransferts(const DonneesGTFS & p_gtfs): Incohérence détectée.");
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
    try{
        Arret::Ptr arretOrigine = make_shared<Arret>(stationIdOrigine, Heure(6,0,0), Heure(6,0,0), 1, "1");
        Arret::Ptr arretDestination = make_shared<Arret>(stationIdDestination, Heure(6,0,0), Heure(6,0,0),1,"1");
        size_t i = m_arretDuSommet.size();
        m_arretDuSommet.push_back(arretOrigine);
        m_sommetDeArret.emplace(arretOrigine, i);
        m_sommetOrigine = i;
        ++i;
        m_arretDuSommet.push_back(arretDestination);
        m_sommetDeArret.emplace(arretDestination, i);
        m_sommetDestination = i;


        m_leGraphe.resize(m_arretDuSommet.size());
        Heure heureDepart = p_gtfs.getTempsDebut();
        auto stations = p_gtfs.getStations();
        auto voyages = p_gtfs.getVoyages();

        m_nbArcsStationsVersDestination = 0;
        m_nbArcsOrigineVersStations = 0;
        for(auto &station:stations){
            Coordonnees coord = station.second.getCoords();
            double distanceOrigine = coord - p_pointOrigine;
            double distanceDestination = coord - p_pointDestination;
            auto arrets = station.second.getArrets();

            set<string> lignesOrigineAjoutees;

            for(auto arret = arrets.lower_bound(p_gtfs.getTempsDebut());arret != arrets.end(); ++arret){
                size_t j = m_sommetDeArret.at(arret->second);
                if(distanceOrigine <= distanceMaxMarche){
                    double tempsMarcheOrigine = distanceOrigine / vitesseDeMarche * 3600;
                    unsigned int poids = arret->first - p_gtfs.getTempsDebut();
                    if(tempsMarcheOrigine <= poids){
                        auto ligne_id = voyages.at(arret->second->getVoyageId()).getLigne();
                        auto ligne = p_gtfs.getLignes().at(ligne_id).getNumero();
                        if(lignesOrigineAjoutees.emplace(ligne).second) {
                            m_leGraphe.ajouterArc(m_sommetOrigine, j, poids);
                            m_nbArcsOrigineVersStations++;
                        }
                    }
                }
                if(distanceDestination <= distanceMaxMarche){
                    auto ligne2_id = voyages.at(arret->second->getVoyageId()).getLigne();
                    auto ligne2 = p_gtfs.getLignes().at(ligne2_id).getNumero();
                    unsigned int poids = distanceDestination / vitesseDeMarche * 3600;
                    m_leGraphe.ajouterArc(j, m_sommetDestination, poids);
                    m_sommetsVersDestination.push_back(j);
                    m_nbArcsStationsVersDestination++;
                }
            }


        }
        m_origine_dest_ajoute = true;
    }
    catch (exception e){
        throw logic_error("ReseauGTFS::ajouterArcsOrigineDestination(const DonneesGTFS &p_gtfs, const Coordonnees &p_pointOrigine, const Coordonnees &p_pointDestination)");
    }
}

//! \brief Remet ReseauGTFS dans l'était qu'il était avant l'exécution de ReseauGTFS::ajouterArcsOrigineDestination()
//! \param[in] p_gtfs: un objet DonneesGTFS
//! \throws logic_error si une incohérence est détecté lors de la modification du graphe
//! \post Enlève de ReaseauGTFS tous les arcs allant du point source vers un arrêt de station et ceux allant d'un arrêt de station vers la destination
//! \post assigne la variable m_origine_dest_ajoute à false (les points orignine et destination sont enlevés du graphe)
//! \post enlève les données de m_sommetsVersDestination
void ReseauGTFS::enleverArcsOrigineDestination()
{
    try {
        for (size_t &j:m_sommetsVersDestination) {
            m_leGraphe.enleverArc(j, m_sommetDestination);
        }

        size_t graphSize = m_arretDuSommet.size() - 2;
        m_leGraphe.resize(graphSize);

        Arret::Ptr origine = m_arretDuSommet.at(m_sommetOrigine);
        Arret::Ptr dest = m_arretDuSommet.at(m_sommetDestination);

        m_sommetDeArret.erase(origine);
        m_sommetDeArret.erase(dest);

        m_arretDuSommet.erase(m_arretDuSommet.begin() + m_sommetDestination);
        m_arretDuSommet.erase(m_arretDuSommet.begin() + m_sommetOrigine);

        m_nbArcsOrigineVersStations = 0;
        m_nbArcsStationsVersDestination = 0;

        m_sommetsVersDestination.clear();

        m_origine_dest_ajoute = false;
    }
    catch(exception e){
        throw logic_error("ReseauGTFS::enleverArcsOrgineDestination(): Incohérence détectée.");
    }
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


