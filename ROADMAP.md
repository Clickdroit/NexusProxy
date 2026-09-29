# 🗺️ Feuille de Route (Roadmap) — NexusProxy

Ce document décrit les étapes de développement pour construire **NexusProxy**, du simple tunnel TCP multi-threadé jusqu'à un répartiteur de charge HTTP complet avec failover automatique.

---

## 📌 Phase 1 : Tunnel TCP & Sockets Multi-Plateforme
- [ ] Initialiser le projet CMake (`CMakeLists.txt`) avec le standard C++20.
- [ ] Créer une couche d'abstraction des sockets TCP supportant à la fois **Winsock2** (Windows) et **POSIX Sockets** (Linux).
- [ ] Mettre en place la boucle d'écoute maître (`bind`, `listen`, `accept`).
- [ ] Gérer chaque client entrant dans un worker thread dédié (`std::jthread`).
- [ ] Valider le transfert de flux bidirectionnel entre un client et un unique serveur backend cible (tunnel TCP brut).

---

## 📌 Phase 2 : Parseur HTTP/1.1 & En-têtes Proxy
- [ ] Implémenter un parseur de requête HTTP/1.1 léger (extraction méthode, URI, version, en-têtes).
- [ ] Ajouter les en-têtes essentiels de reverse-proxying :
  - `X-Forwarded-For: <client_ip>`
  - `X-Forwarded-Proto: http`
  - `X-Real-IP: <client_ip>`
- [ ] Réécrire l'en-tête `Host` pour cibler le backend sélectionné.
- [ ] Transmettre la réponse du backend au client sans altération.

---

## 📌 Phase 3 : Algorithmes de Load Balancing
- [ ] Créer la structure `BackendServer` (`host`, `port`, `is_alive`, `active_connections`).
- [ ] Créer l'interface abstraite `ILoadBalancerStrategy`.
- [ ] Implémenter la stratégie **Round-Robin** (compteur atomique thread-safe pour alterner entre les serveurs sains).
- [ ] Implémenter la stratégie **Least-Connections** (recherche du serveur sain ayant le minimum de connexions actives en cours).
- [ ] Charger les backends depuis un fichier `config.json`.

---

## 📌 Phase 4 : Health Checking Actif & Failover
- [ ] Créer un thread d'arrière-plan autonome (`HealthChecker`).
- [ ] À intervalle régulier (ex: toutes les 3 ou 5 secondes) :
  - Tenter une connexion TCP ou une requête `HEAD /` sur chaque backend.
  - Marquer le serveur comme `INACTIF` si la connexion échoue ou timeout.
  - Marquer le serveur comme `ACTIF` dès qu'il répond avec succès.
- [ ] Protéger la liste des serveurs sains avec un `std::shared_mutex` (optimisation lectures fréquentes / écritures rares).
- [ ] Renvoyer une page d'erreur propre `502 Bad Gateway` si aucun backend n'est disponible.

---

## 📌 Phase 5 : Métriques & Monitoring Console
- [ ] Calculer les statistiques en direct : requêtes totales, requêtes/seconde, temps moyen de réponse du backend, octets transférés.
- [ ] Afficher un mini tableau de bord dans le terminal avec l'état de santé de chaque backend en vert/rouge.
- [ ] Tester les performances et la latence sous charge avec un outil de benchmark (`wrk` ou `ab`).
