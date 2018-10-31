//
// Created by Mario Marchand on 16-12-29.
//

#include "DonneesGTFS.h"

using namespace std;

//! \brief construit un objet GTFS
//! \param[in] p_date: la date utilisée par le GTFS
//! \param[in] p_now1: l'heure du début de l'intervalle considéré
//! \param[in] p_now2: l'heure de fin de l'intervalle considéré
//! \brief Ces deux heures définissent l'intervalle de temps du GTFS; seuls les moments de [p_now1, p_now2) sont considérés
DonneesGTFS::DonneesGTFS(const Date &p_date, const Heure &p_now1, const Heure &p_now2)
        : m_date(p_date), m_now1(p_now1), m_now2(p_now2), m_nbArrets(0), m_tousLesArretsPresents(false)
{
    this->m_transferts = {};
}


//! \brief partitionne un string en un vecteur de strings
//! \param[in] s: le string à être partitionner
//! \param[in] delim: le caractère utilisé pour le partitionnement
//! \return le vecteur de string sans le caractère utilisé pour le partitionnement
vector<string> DonneesGTFS::string_to_vector(const string &s, char delim)
{
    stringstream ss(s);
    string item;
    vector<string> elems;
    while (getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}


//! \brief ajoute les lignes dans l'objet GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les lignes
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterLignes(const std::string &p_nomFichier)
{
    this->traiterFichier(p_nomFichier, &DonneesGTFS::traitementLigne);
}


//! \brief ajoute les stations dans l'objet GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les station
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterStations(const std::string &p_nomFichier)
{
    this->traiterFichier(p_nomFichier, &DonneesGTFS::traitementStation);
}

//! \brief ajoute les services de la date du GTFS (m_date)
//! \param[in] p_nomFichier: le nom du fichier contenant les services
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterServices(const std::string &p_nomFichier)
{
    this->traiterFichier(p_nomFichier, &DonneesGTFS::traitementService);
}

//! \brief ajoute les voyages de la date
//! \brief seuls les voyages dont le service est présent dans l'objet GTFS sont ajoutés
//! \param[in] p_nomFichier: le nom du fichier contenant les voyages
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterVoyagesDeLaDate(const std::string &p_nomFichier)
{
    this->traiterFichier(p_nomFichier, &DonneesGTFS::traitementVoyage);
}

//! \brief ajoute les arrets aux voyages présents dans le GTFS si l'heure du voyage appartient à l'intervalle de temps du GTFS
//! \brief De plus, on enlève les voyages qui n'ont pas d'arrêts dans l'intervalle de temps du GTFS
//! \brief De plus, on enlève les stations qui n'ont pas d'arrets dans l'intervalle de temps du GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les arrets
//! \post assigne m_tousLesArretsPresents à true
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterArretsDesVoyagesDeLaDate(const std::string &p_nomFichier)
{
    this->traiterFichier(p_nomFichier, &DonneesGTFS::traitementArret);

    for (auto it = m_voyages.begin(); it != m_voyages.end();){
        if(it->second.getNbArrets() == 0){
            it = this->m_voyages.erase(it);
        }
        else{it++;}
    }

    for (auto it = m_stations.begin(); it != m_stations.end();){
        if(it->second.getNbArrets() == 0){
            it = this->m_stations.erase(it);
        }
        else{it++;}
    }
    m_tousLesArretsPresents = true;
}

//! \brief ajoute les transferts dans l'objet GTFS
//! \breif Cette méthode doit âtre utilisée uniquement après que tous les arrêts ont été ajoutés
//! \brief les transferts (entre stations) ajoutés sont uniquement ceux pour lesquelles les stations sont prensentes dans l'objet GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les station
//! \throws logic_error si un problème survient avec la lecture du fichier
//! \throws logic_error si tous les arrets de la date et de l'intervalle n'ont pas été ajoutés
void DonneesGTFS::ajouterTransferts(const std::string &p_nomFichier)
{
    this->traiterFichier(p_nomFichier, &DonneesGTFS::traitementTransfert);
}

Heure DonneesGTFS::stringToHeure(const std::string &p_heureEnString){
    vector<string> stringHeureVector = string_to_vector(p_heureEnString, ':');
    unsigned int heure = strtoul(stringHeureVector[0].c_str(), nullptr, 10);
    unsigned int minute = strtoul(stringHeureVector[1].c_str(), nullptr, 10);
    unsigned int seconde = strtoul(stringHeureVector[2].c_str(), nullptr, 10);

    return Heure(heure, minute, seconde);
}

unsigned int DonneesGTFS::getNbArrets() const
{
    return m_nbArrets;
}

size_t DonneesGTFS::getNbLignes() const
{
    return m_lignes.size();
}

size_t DonneesGTFS::getNbStations() const
{
    return m_stations.size();
}

size_t DonneesGTFS::getNbTransferts() const
{
    return m_transferts.size();
}

size_t DonneesGTFS::getNbServices() const
{
    return m_services.size();
}

size_t DonneesGTFS::getNbVoyages() const
{
    return m_voyages.size();
}

void DonneesGTFS::afficherLignes() const
{
    std::cout << "======================" << std::endl;
    std::cout << "   LIGNES GTFS   " << std::endl;
    std::cout << "   COMPTE = " << m_lignes.size() << "   " << std::endl;
    std::cout << "======================" << std::endl;
    for (const auto & ligneM : m_lignes_par_numero)
    {
        cout << ligneM.second;
    }
    std::cout << std::endl;
}

void DonneesGTFS::afficherStations() const
{
    std::cout << "========================" << std::endl;
    std::cout << "   STATIONS GTFS   " << std::endl;
    std::cout << "   COMPTE = " << m_stations.size() << "   " << std::endl;
    std::cout << "========================" << std::endl;
    for (const auto & stationM : m_stations)
    {
        std::cout << stationM.second << endl;
    }
    std::cout << std::endl;
}

void DonneesGTFS::afficherTransferts() const
{
    std::cout << "========================" << std::endl;
    std::cout << "   TRANSFERTS GTFS   " << std::endl;
    std::cout << "   COMPTE = " << m_transferts.size() << "   " << std::endl;
    std::cout << "========================" << std::endl;
    for (unsigned int i = 0; i < m_transferts.size(); ++i)
    {
        std::cout << "De la station " << get<0>(m_transferts.at(i)) << " vers la station " << get<1>(m_transferts.at(i))
        <<
        " en " << get<2>(m_transferts.at(i)) << " secondes" << endl;
        
    }
    std::cout << std::endl;
    
}


void DonneesGTFS::afficherArretsParVoyages() const
{
    std::cout << "=====================================" << std::endl;
    std::cout << "   VOYAGES DE LA JOURNÉE DU " << m_date << std::endl;
    std::cout << "   " << m_now1 << " - " << m_now2 << std::endl;
    std::cout << "   COMPTE = " << m_voyages.size() << "   " << std::endl;
    std::cout << "=====================================" << std::endl;
    
    for (const auto & voyageM : m_voyages)
    {
        unsigned int ligne_id = voyageM.second.getLigne();
        auto l_itr = m_lignes.find(ligne_id);
        cout << (l_itr->second).getNumero() << " ";
        cout << voyageM.second << endl;
        for (const auto & a: voyageM.second.getArrets())
        {
            unsigned int station_id = a->getStationId();
            auto s_itr = m_stations.find(station_id);
            std::cout << a->getHeureArrivee() << " station " << s_itr->second << endl;
        }
    }
    
    std::cout << std::endl;
}

void DonneesGTFS::afficherArretsParStations() const
{
    std::cout << "========================" << std::endl;
    std::cout << "   ARRETS PAR STATIONS   " << std::endl;
    std::cout << "   Nombre d'arrêts = " << m_nbArrets << std::endl;
    std::cout << "========================" << std::endl;
    for ( const auto & stationM : m_stations)
    {
        std::cout << "Station " << stationM.second << endl;
        for ( const auto & arretM : stationM.second.getArrets())
        {
            string voyage_id = arretM.second->getVoyageId();
            auto v_itr = m_voyages.find(voyage_id);
            unsigned int ligne_id = (v_itr->second).getLigne();
            auto l_itr = m_lignes.find(ligne_id);
            std::cout << arretM.first << " - " << (l_itr->second).getNumero() << " " << v_itr->second << std::endl;
        }
    }
    std::cout << std::endl;
}

const std::map<std::string, Voyage> &DonneesGTFS::getVoyages() const
{
    return m_voyages;
}

const std::map<unsigned int, Station> &DonneesGTFS::getStations() const
{
    return m_stations;
}

const std::vector<std::tuple<unsigned int, unsigned int, unsigned int> > &DonneesGTFS::getTransferts() const
{
    return m_transferts;
}

Heure DonneesGTFS::getTempsFin() const
{
    return m_now2;
}

Heure DonneesGTFS::getTempsDebut() const
{
    return m_now1;
}

const std::unordered_map<unsigned int, Ligne> &DonneesGTFS::getLignes() const
{
    return m_lignes;
}

void DonneesGTFS::traitementLigne(const std::vector<std::string> &vecteurString)
{
    unsigned int route_id = std::strtoul(vecteurString.at(0).c_str(), nullptr, 10);
    std::string route_short_name = vecteurString.at(2);
    std::string route_desc = vecteurString.at(4);
    CategorieBus route_color = Ligne::couleurToCategorie(vecteurString.at(7));

    Ligne ligneActuelle = Ligne(route_id, route_short_name, route_desc, route_color);

    this->m_lignes[route_id] = ligneActuelle;
    this->m_lignes_par_numero.insert(std::make_pair(route_short_name, ligneActuelle));
}

void DonneesGTFS::traitementStation(const std::vector<std::string> &vecteurString){
    unsigned int station_id = std::strtoul(vecteurString.at(0).c_str(), nullptr, 10);
    std::string station_name = vecteurString.at(1);
    std::string station_desc = vecteurString.at(2);
    Coordonnees station_coords = Coordonnees(std::strtod(vecteurString.at(3).c_str(), nullptr),
                                             std::strtod(vecteurString.at(4).c_str(), nullptr));

    Station stationActuelle = Station(station_id, station_name, station_desc, station_coords);

    this->m_stations[station_id] = stationActuelle;
}

void DonneesGTFS::traitementTransfert(const std::vector<std::string> &vecteurString){
    std::string from_station_idStr = vecteurString.at(0);
    unsigned int from_station_id = std::stoul(from_station_idStr, nullptr, 10);
    std::string to_station_idStr = vecteurString.at(1);
    unsigned int to_station_id = std::stoul(to_station_idStr, nullptr, 10);
    std::string min_transfer_timeStr = vecteurString.at(3);
    unsigned int min_transfer_time = std::stoul(min_transfer_timeStr, nullptr, 10);
    if(min_transfer_time == 0){
        min_transfer_time = 1;
    }

    if(m_stations.count(from_station_id) > 0 && m_stations.count(to_station_id) > 0){
        tuple<unsigned int, unsigned int, unsigned int> transfertActuel {from_station_id, to_station_id, min_transfer_time};
        this->m_transferts.push_back(transfertActuel);
    }
}

void DonneesGTFS::traitementService(const std::vector<std::string> &vecteurString)
{
    std::string service_id = vecteurString.at(0);
    std::string dateLigneStr = vecteurString.at(1);
    std::string exception_type = vecteurString.at(2);

    Date dateActuelle = this->m_date;
    std::string strAn = dateLigneStr.substr(0,4).c_str();
    unsigned int an = std::stoul(strAn, nullptr, 10);
    std::string strMois = dateLigneStr.substr(4,2);
    unsigned int mois = std::stoul(strMois, nullptr, 10);
    std::string strJour = dateLigneStr.substr(6,2);
    unsigned int jour = std::stoul(strJour, nullptr, 10);

    Date dateLigne = Date(an, mois, jour);
    if(dateLigne == dateActuelle && exception_type.compare("1") == 0){
        this->m_services.insert(service_id);
    }
}

void DonneesGTFS::traitementVoyage(const std::vector<std::string> &vecteurString) {
    std::string voyage_id = vecteurString.at(2);
    unsigned int voyage_ligne = std::strtoul(vecteurString.at(0).c_str(), nullptr, 10);
    std::string voyage_service_id = vecteurString.at(1);
    std::string voyage_destination = vecteurString.at(3);

    if (m_services.find(voyage_service_id) != m_services.end()) {
        Voyage voyageActuel = Voyage(voyage_id, voyage_ligne, voyage_service_id, voyage_destination);
        this->m_voyages[voyage_id] = voyageActuel;
    }
}

void DonneesGTFS::traitementArret(const std::vector<std::string> &vecteurString) {

    std::string stop_id = vecteurString.at(3);
    std::string arrival_time = vecteurString.at(1);
    std::string departure_time = vecteurString.at(2);
    std::string stop_sequence = vecteurString.at(4);
    std::string trip_id = vecteurString.at(0);

    unsigned int stopId = strtoul(stop_id.c_str(), nullptr, 10);
    Heure heureArriveeAutobus = stringToHeure(arrival_time);
    Heure heureDepartAutobus = stringToHeure(departure_time);
    unsigned int sequence = strtoul(stop_sequence.c_str(), nullptr, 10);

    Arret::Ptr a_ptr = make_shared<Arret>(stopId, heureArriveeAutobus, heureDepartAutobus, sequence, trip_id);

    if(heureDepartAutobus >= this->m_now1 && heureArriveeAutobus<this->m_now2 && m_voyages.find(trip_id) != m_voyages.end()){
        m_voyages[trip_id].ajouterArret(a_ptr);
        m_stations[stopId].addArret(a_ptr);
        m_nbArrets++;
    }
}

void DonneesGTFS::traiterFichier(const std::string &p_nomFichier, void (DonneesGTFS::*functionPointer)(const std::vector<std::string> &))
{
    std::vector<std::vector<std::string>> fichierTraite;
    ifstream fichierAOuvrir;
    string ligneDeFichier;

    fichierAOuvrir.open(p_nomFichier);

    if (!fichierAOuvrir.is_open()) {
        throw std::logic_error("Erreur lors de l'ouverture du fichier.");
    }
    //Passer la premiere ligne
    getline(fichierAOuvrir, ligneDeFichier);

    while (!fichierAOuvrir.eof()) {
        getline(fichierAOuvrir, ligneDeFichier);

        ligneDeFichier.erase(std::remove(ligneDeFichier.begin(), ligneDeFichier.end(), '\"'), ligneDeFichier.end());

        std::vector<std::string> vecteurLigneSeparee = this->string_to_vector(ligneDeFichier, ',');

        //Arrette si la ligne ne contient pas de valeur
        if (vecteurLigneSeparee.empty()) break;

        (this->*functionPointer)(vecteurLigneSeparee);


        ligneDeFichier = "";
    }
    fichierAOuvrir.close();
}





