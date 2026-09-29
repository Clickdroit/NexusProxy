# ⚡ NexusProxy — Lightweight C++ Reverse Proxy & Load Balancer

**NexusProxy** est un répartiteur de charge (**Load Balancer**) et reverse-proxy haute performance développé en **C++20**.

Il reçoit les requêtes réseau entrantes sur un port d'écoute et les distribue intelligemment vers un ensemble de serveurs backend en utilisant des algorithmes d'équilibrage configurables (**Round-Robin**, **Least Connections**), tout en surveillant la santé des serveurs via des **Health Checks** asynchrones.

---

## 🏗️ Architecture du Système

```
                       +----------------------+
                       |  Clients / Navigateurs|
                       +-----------+----------+
                                   | Requêtes HTTP / TCP
                                   v
                       +----------------------+
                       |      NexusProxy      |
                       |  (Port d'écoute 8080)|
                       +-----------+----------+
                                   |
         +-------------------------+-------------------------+
         |                                                   |
         | [Algorithme Round-Robin ou Least-Connections]     |
         |                                                   |
         v                                                   v
+------------------+                               +------------------+
| Backend 1 (Actif)|                               | Backend 2 (Actif)|
| (127.0.0.1:3000) |                               | (127.0.0.1:3001) |
+--------+---------+                               +--------+---------+
         ^                                                  ^
         |                    Health Checker                |
         +------------------ [Pings réguliers] -------------+
```

---

## ✨ Fonctionnalités clés

- **⚡ Cœur Réseau Haute Performance en C++20 :**
  - Architecture non-bloquante ou multi-threadée avec `std::jthread`.
  - Prise en charge multi-plateforme (Linux POSIX Sockets & Windows Winsock).

- **⚖️ Algorithmes de Load Balancing :**
  - **Round-Robin :** Distribution circulaire équitable de chaque requête entrante.
  - **Least Connections :** Routage préférentiel vers le backend ayant le plus faible nombre de connexions actives.
  - **IP Hash (Optionnel) :** Persistance de session basée sur le hash de l'IP cliente.

- **💓 Health Checking & Tolérance aux pannes :**
  - Thread d'arrière-plan surveillant périodiquement l'état des serveurs backend.
  - Éviction automatique et immédiate d'un backend défaillant (Timeout, connexion refusée).
  - Réintégration automatique dès que le serveur redevient joignable.

- **📑 Injection d'en-têtes Proxy :**
  - Ajout transparent des en-têtes standard `X-Forwarded-For`, `X-Forwarded-Proto` et `X-Real-IP`.

- **⚙️ Configuration JSON Dynamique :**
  - Fichier de configuration simple (`config.json`) définissant le port d'écoute, la liste des serveurs cibles, le timeout et les intervalles de vérification.

---

## 🛠️ Stack Technique

- **Langage :** C++ moderne (**C++20**)
- **Build System :** CMake 3.20+
- **Réseau :** Sockets TCP (`sys/socket.h` sous Linux / `winsock2.h` sous Windows)
- **Concurrence :** `std::jthread`, `std::mutex`, `std::atomic`

---

## 🚀 Compilation & Lancement

### Prérequis
- Compilateur compatible C++20 (GCC 11+, Clang 13+, ou MSVC 2022).
- CMake 3.20 ou supérieur.

### Compilation
```bash
# Générer les fichiers de build
cmake -B build -S .

# Compiler le projet
cmake --build build --config Release
```

### Lancement
```bash
# Lancer avec la configuration par défaut
./build/NexusProxy
```

---

## 📄 Configuration (`config.json`)
```json
{
  "listen_port": 8080,
  "strategy": "round_robin",
  "health_check_interval_ms": 5000,
  "backends": [
    { "host": "127.0.0.1", "port": 3000, "weight": 1 },
    { "host": "127.0.0.1", "port": 3001, "weight": 1 }
  ]
}
```

---

## 📄 Licence
Ce projet est sous licence MIT - voir le fichier [LICENSE](LICENSE) pour plus d'informations.
