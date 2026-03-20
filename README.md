# Tris Multiplayer – Gioco Client‑Server

Un classico **Tris** (Tic‑Tac‑Toe) implementato con architettura client‑server.  
Il server multi‑thread gestisce più partite contemporaneamente, mentre i client si collegano tramite un protocollo binario personalizzato e offrono un’interfaccia testuale con supporto al mouse.

---

## Caratteristiche

- **Server multi‑thread** – ogni client è gestito in un thread separato; il `GameManager` garantisce accesso thread‑safe allo stato delle partite.
- **Protocollo binario TLV** – header fisso + payload, con conversione in network order per portabilità.
- **Client in ncurses** – interfaccia testuale completa, navigabile con tastiera (frecce, Invio, ESC) e mouse (click sulle celle).
- **Lobby dinamica** – i giocatori possono creare partite, vedere quelle disponibili, accettare o rifiutare richieste di join.
- **Opzioni post‑partita** – il vincitore può continuare come proprietario; in caso di pareggio entrambi decidono.
- **Spegnimento ordinato** – chiusura pulita di socket, thread e memoria (controllata con Valgrind).
- **Docker Compose ready** – il server può essere eseguito con un singolo comando.

---

## Architettura

Il progetto è suddiviso in tre moduli principali:

```
tris/
├── common/                 # codice condiviso (protocollo, serializzazione)
│   ├── include/            # protocol.h
│   └── src/                # protocol.c
├── server/                 # server di gioco
│   ├── include/            # server.h, game.h, client_handler.h
│   ├── src/                # implementazioni
│   ├── Makefile
│   └── Dockerfile          # per la build dell’immagine
├── client/                 # client con ncurses
│   ├── include/            # network, ui, utils
│   ├── src/
│   └── Makefile
└── docker-compose.yml      # per avviare il server con Docker Compose
```

- **common** – definisce i tipi di messaggio, le strutture dei payload e le funzioni di serializzazione (`send_message`, `receive_message`).
- **server** – gestisce la logica di gioco, le connessioni e il broadcast degli aggiornamenti della lobby.
- **client** – thread UI (ncurses) e thread network (code thread‑safe) per un’interfaccia reattiva.

---

## Prerequisiti

Per compilare ed eseguire il progetto occorrono:

- **Compilatore C** (gcc, supporto a C11)
- **make**
- **Librerie**:
  - `libncurses` (per il client)
  - `pthread` (server e client)
- **Docker** e **Docker Compose** (opzionali, per eseguire il server in container)

Su sistemi Debian/Ubuntu installare con:

```bash
sudo apt update
sudo apt install build-essential libncurses-dev docker.io docker-compose
```

---

## Compilazione

### Server

```bash
cd server
make release          # compila con ottimizzazioni e hardening
# oppure
make debug            # compila con debug symbols
```

L’eseguibile `tris_server` verrà creato nella cartella `server`.

### Client

```bash
cd client
make release          # compila con ottimizzazioni
# oppure
make debug            # compila con debug symbols
```

L’eseguibile `tris_client` verrà creato nella cartella `client`.

---

## Esecuzione

### Server nativo

```bash
cd server
./tris_server
```

Il server si mette in ascolto sulla porta `8080` (modificabile nel codice in `server.h`).

### Server con Docker Compose

Dalla directory principale del progetto (quella che contiene `docker-compose.yml`):

```bash
docker-compose up --build
```

Il container verrà costruito e avviato, e il server sarà esposto sulla porta `8080` dell’host.  
Per fermare il server:

```bash
docker-compose down
```

### Client

Prima di avviare il client, assicurarsi che il file di configurazione `client/config.conf` contenga l’indirizzo IP e la porta del server (di default `127.0.0.1:8080` se eseguito localmente).

```bash
cd client
./tris_client
```

Una volta avviato, l’utente può navigare nel menu con le frecce o con i tasti numerici.

---

## Utilizzo

- **Menu principale**:
  - `Create game` – crea una nuova partita e attende un avversario.
  - `Join game` – visualizza la lista delle partite disponibili e permette di unirsi.
  - `Settings` – modifica username, IP server e porta.
  - `Help` – mostra la guida.
  - `Quit` – esce.

- **Lobby**:
  - Scegliere una partita digitando il numero corrispondente.
  - `r` per aggiornare la lista.
  - `ESC` per tornare al menu.

- **Partita**:
  - Usare i **tasti freccia** per spostare il cursore, **Invio** o **Spazio** per piazzare il simbolo.
  - È anche possibile **cliccare con il mouse** sulla cella desiderata.
  - `ESC` abbandona la partita.

- **Fine partita**:
  - Il vincitore (o entrambi in caso di pareggio) può scegliere se giocare un’altra partita (`y`) o uscire (`n`). Il perdente viene automaticamente rimosso dalla partita.

---

## Protocollo di comunicazione

Ogni messaggio è composto da un header fisso di 16 byte:

```c
struct MessageHeader {
    uint32_t magic;    // 0x54524953 ("TRIS")
    uint32_t version;  // 1
    uint32_t type;     // MessageType
    uint32_t length;   // lunghezza payload (in byte)
};
```

I tipi di messaggio coprono tutte le azioni del gioco: creazione, join, mossa, stato, aggiornamento lobby, richieste di join, decisioni post‑partita, ecc. I dati vengono serializzati in network order per garantire portabilità.

---

## Struttura delle cartelle

```
tris/
├── common/
│   ├── include/
│   │   └── protocol.h
│   └── src/
│       └── protocol.c
├── server/
│   ├── include/
│   │   ├── server.h
│   │   ├── game.h
│   │   └── client_handler.h
│   ├── src/
│   │   ├── main.c
│   │   ├── server.c
│   │   ├── game.c
│   │   └── client_handler.c
│   ├── Makefile
│   └── Dockerfile
├── client/
│   ├── include/
│   │   ├── network/
│   │   │   └── network.h
│   │   ├── ui/
│   │   │   ├── ui.h
│   │   │   └── ui_events.h
│   │   └── utils/
│   │       ├── queue.h
│   │       ├── utils.h
│   │       └── log.h
│   ├── src/
│   │   ├── main.c
│   │   ├── network/
│   │   │   └── network.c
│   │   ├── ui/
│   │   │   └── ui.c
│   │   └── utils/
│   │       ├── queue.c
│   │       ├── utils.c
│   │       └── log.c
│   └── Makefile
├── docker-compose.yml
└── .gitignore
```

---

## Licenza

Distribuito sotto licenza MIT. Consultare il file `LICENSE` per i dettagli.
