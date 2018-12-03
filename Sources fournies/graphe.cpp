//
//  Graphe.cpp
//  Classe pour graphes orientés pondérés (non négativement) avec listes d'adjacence
//
//  Mario Marchand automne 2016.
//

#include "graphe.h"

using namespace std;

//! \brief Constructeur avec paramètre du nombre de sommets désiré
//! \param[in] p_nbSommets indique le nombre de sommets désiré
//! \post crée le vecteur de p_nbSommets de listes d'adjacence vides avec nbArcs=0
Graphe::Graphe(size_t p_nbSommets)
        : m_listesAdj(p_nbSommets), m_nbArcs(0)
{
}

//! \brief change le nombre de sommets du graphe
//! \param[in] p_nouvelleTaille indique le nouveau nombre de sommet
//! \post le graphe est un vecteur de p_nouvelleTaille de listes d'adjacence
//! \post les anciennes listes d'adjacence sont toujours présentes lorsque p_nouvelleTaille >= à l'ancienne taille
//! \post les dernières listes d'adjacence sont enlevées lorsque p_nouvelleTaille < à l'ancienne taille
//! \post nbArcs est diminué par le nombre d'arcs sortant des sommets à enlever si certaines listes d'adgacence sont supprimées
void Graphe::resize(size_t p_nouvelleTaille)
{
    if (p_nouvelleTaille < m_listesAdj.size()) //certaines listes d'adj seront supprimées
    {
        //diminuer nbArcs par le nb d'arcs sortant des sommets à enlever
        for(size_t i = p_nouvelleTaille; i < m_listesAdj.size(); ++i)
        {
            m_nbArcs -= m_listesAdj[i].size();
        }
    }
    m_listesAdj.resize(p_nouvelleTaille);
}

size_t Graphe::getNbSommets() const
{
    return m_listesAdj.size();
}

size_t Graphe::getNbArcs() const
{
    return m_nbArcs;
}

//! \brief ajoute un arc d'un poids donné dans le graphe
//! \param[in] i: le sommet origine de l'arc
//! \param[in] j: le sommet destination de l'arc
//! \param[in] poids: le poids de l'arc
//! \pre les sommets i et j doivent exister
//! \throws logic_error lorsque le sommet i ou le sommet j n'existe pas
//! \throws logic_error lorsque le poids == numeric_limits<unsigned int>::max()
void Graphe::ajouterArc(size_t i, size_t j, unsigned int poids)
{
    if (i >= m_listesAdj.size())
        throw logic_error("Graphe::ajouterArc(): tentative d'ajouter l'arc(i,j) avec un sommet i inexistant");
    if (j >= m_listesAdj.size())
        throw logic_error("Graphe::ajouterArc(): tentative d'ajouter l'arc(i,j) avec un sommet j inexistant");
    if (poids == numeric_limits<unsigned int>::max())
        throw logic_error("Graphe::ajouterArc(): valeur de poids interdite");
    m_listesAdj[i].emplace_back(Arc(j, poids));
    ++m_nbArcs;
}

//! \brief enlève un arc dans le graphe
//! \param[in] i: le sommet origine de l'arc
//! \param[in] j: le sommet destination de l'arc
//! \pre l'arc (i,j) et les sommets i et j dovent exister
//! \post enlève l'arc mais n'enlève jamais le sommet i
//! \throws logic_error lorsque le sommet i ou le sommet j n'existe pas
//! \throws logic_error lorsque l'arc n'existe pas
void Graphe::enleverArc(size_t i, size_t j)
{
    if (i >= m_listesAdj.size())
        throw logic_error("Graphe::enleverArc(): tentative d'enlever l'arc(i,j) avec un sommet i inexistant");
    if (j >= m_listesAdj.size())
        throw logic_error("Graphe::enleverArc(): tentative d'enlever l'arc(i,j) avec un sommet j inexistant");
    auto &liste = m_listesAdj[i];
    bool arc_enleve = false;
    if(liste.empty()) throw logic_error("Graphe:enleverArc(): m_listesAdj[i] est vide");
    for (auto itr = liste.end(); itr != liste.begin();) //on débute par la fin par choix
    {
        if ((--itr)->destination == j)
        {
            liste.erase(itr);
            arc_enleve = true;
            break;
        }
    }
    if (!arc_enleve)
        throw logic_error("Graphe::enleverArc: cet arc n'existe pas; donc impossible de l'enlever");
    --m_nbArcs;
}


unsigned int Graphe::getPoids(size_t i, size_t j) const
{
    if (i >= m_listesAdj.size()) throw logic_error("Graphe::getPoids(): l'incice i n,est pas un sommet existant");
    for (auto & arc : m_listesAdj[i])
    {
        if (arc.destination == j) return arc.poids;
    }
    throw logic_error("Graphe::getPoids(): l'arc(i,j) est inexistant");
}


//! \brief Algorithme de Dijkstra permettant de trouver le plus court chemin entre p_origine et p_destination
//! \pre p_origine et p_destination doivent être des sommets du graphe
//! \return la longueur du plus court chemin est retournée
//! \param[out] le chemin est retourné (un seul noeud si p_destination == p_origine ou si p_destination est inatteignable)
//! \return la longueur du chemin (= numeric_limits<unsigned int>::max() si p_destination n'est pas atteignable)
//! \throws logic_error lorsque p_origine ou p_destination n'existe pas
unsigned int Graphe::plusCourtChemin(size_t p_origine, size_t p_destination, std::vector<size_t> &p_chemin) const
{
    if (p_origine >= m_listesAdj.size() || p_destination >= m_listesAdj.size())
        throw logic_error("Graphe::dijkstra(): p_origine ou p_destination n'existe pas");

    p_chemin.clear();

    if (p_origine == p_destination)
    {
        p_chemin.push_back(p_destination);
        return 0;
    }

    vector<unsigned int> distance(m_listesAdj.size(), numeric_limits<unsigned int>::max());
    vector<size_t> predecesseur(m_listesAdj.size(), numeric_limits<size_t>::max());
    size_t sommet;

    //Comparateur de distances des pairs dans la heap
    struct CompareByFirst {
        constexpr bool operator()(pair<int, size_t> const & a,
                                  pair<int, size_t> const & b) const noexcept
        { return a.first < b.first; }
    };

    std::priority_queue<	std::pair<unsigned int, size_t>,
            std::vector<std::pair<unsigned int, size_t>>,
            CompareByFirst
    > heap;

    //distance de depart
    distance[p_origine] = 0;

    //On met la distance 0 de depart avec le point d'origine
    heap.push(pair<unsigned int, size_t>(distance[p_origine],p_origine));

    //c'est la distance du sommet actuel plus la distance vers le prochain sommet
    unsigned int distanceMinimePotentielle;

    //Tant que notre map distance Noaud n'est pas vide
    while (!heap.empty())
    {
        //on itere sur le heap des noeuds, tout en pouvant ajouter des noeuds en faisant de la recursion
        //ici, on sort le sommet de la map avant de peut-etre l'effacer
        sommet = heap.top().second;

        //si le sommet actuel correspond a la destination, on peut arreter
        if (sommet == p_destination) break;

        //sinon on enleve le noeud actuel de la map, car on sait que ce n'est pas un chemin final,
        //mais l'info est encore dans la variable sommet
        heap.pop();

        //on itere grace a la liste d'adjacence du noeud actuel
        //m_listesAdj est un vector<list<Arc>>, donc on itere sur les arcs directs du noeud, non tries

        for (auto sommetAdjacent = m_listesAdj[sommet].begin(); sommetAdjacent != m_listesAdj[sommet].end(); ++sommetAdjacent)
        {
            //distance totale vers le prochain sommet
            distanceMinimePotentielle = distance[sommet] + sommetAdjacent->poids;

            //si on trouve une plus petite distance que celle trouvee auparavant (obligatoire a la decouverte du noeud)
            if (distanceMinimePotentielle < distance[sommetAdjacent->destination])
            {
                //on met la nouvelle distance plus petite dans le prochain sommet
                distance[sommetAdjacent->destination] = distanceMinimePotentielle;
                //on met le nouveau predecesseur
                predecesseur[sommetAdjacent->destination] = sommet;
                //On mettra le nouveau noeud dans le heap pour trouver des potentiels plus petits chemins avec la nouvelle valeur
                heap.push(pair<unsigned int , size_t>(distanceMinimePotentielle,sommetAdjacent->destination));
            }
        }
    }

    //Si pas de solution
    if (predecesseur[p_destination] == numeric_limits<unsigned int>::max())
    {
        p_chemin.push_back(p_destination);
        return numeric_limits<unsigned int>::max();
    }

    //On refera le chemin a partir du dernier chemin avec une pile
    stack<size_t> pileDuChemin;
    size_t sommetActuel = p_destination;
    //on met la destination
    pileDuChemin.push(sommetActuel);

    //tant qu'il y a un predecesseur, on l'ajoute dans la pile
    while (predecesseur[sommetActuel] != numeric_limits<size_t>::max())
    {
        sommetActuel = predecesseur[sommetActuel];
        pileDuChemin.push(sommetActuel);
    }
    //on vide la pile dans le chemin dans le bon ordre
    while (!pileDuChemin.empty())
    {
        p_chemin.push_back(pileDuChemin.top());
        pileDuChemin.pop();
    }
    //on retourne la distance de la destination
    return distance[p_destination];
}
