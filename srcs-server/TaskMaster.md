# TaskMaster and co

Ce document décrit les étapes que nous allons effectuer pour faire le projet `TaskMaster` en prenant en compte les besoins de projets futurs tels que `Matt_Daemon` et `ft_shield`.    

# Description

# Besoins

`Matt Daemon` necessitant d'être en C++ et `ft_Shield` necessitant d'être en `C` nous allons donc faire `TaskMaster` en `C++`.    


## TM

### Client
> Must run with a tty terminal
Must connect to the server through internet.

#### Provide a ControlShell
Le programme devra fournir un shell.
> All minishell capabilities
> An interface specific for jobcontrols
> Must be able to send commands to the server

#### Secure connection
Connection to the server must be encrypted by TLS.

#### Remote administration
On doit pouvoir attacher un des process supervisé au terminal du client.

## Server

#### Demarrage
Le program devra obligatoirement être lancé en tant que ROOT.
> Check if `EUID` is equal to `0`
Le programme devra être un `daemon`.    
> at start, daemonize the process.
Le programme doit écouter sur un port.

Le programme doit rendre rendre les privilèges pour chacun de ses childs. Ce doit être le seul programme


#### Configuration file
Nous avons le droit d'utiliser une librairie de parsing pour lire les fichiers de configuration.
A la reception d'un signal `SIGHUP` le serveur doit recharger le fichier de config et appliquer uniquement les differences entre les configurations.    
> Load a new config file    
> Check diff between new and old configs     
> Apply this list of differences to update the server     
> Encrypted when stored on the victim? 

Le fichier de config doit définir l'ensemble des JOB qui seront lancé mais on peut aussi l'utiliser pour parametrer le serveur lui-même.

Voici un exemple d'un fichier de configuration minimal:
```YAML
programs:
    nginx:
        cmd: "/usr/local/bin/nginx -c /etc/nginx/test.conf"
        numprocs: 1
        umask: 022
        workingdir: /tmp
        autostart: true
        autorestart: unexpected
        exitcodes:
            - 0
            - 2
        startretries: 3
        starttime: 5
        stopsignal: TERM
        stoptime: 10
        stdout: /tmp/nginx.stdout
        stderr: /tmp/nginx.stderr
        env:
            STARTED_BY: taskmaster
            ANSWER: 42
    vogsphere:
        cmd: "/usr/local/bin/vogsphere-worker --no-prefork"
        numprocs: 8
        umask: 077
        workingdir: /tmp
        autostart: true
        autorestart: unexpected
        exitcodes: 0
        startretries: 3
        starttime: 5
        stopsignal: USR1
        stoptime: 10
        stdout: /tmp/vgsworker.stdout
        stderr: /tmp/vgsworker.stderr
```

#### Logging
Nous devons aussi enregistrer tout les évenements notables du gestionnaire de taches dans un fichier de log.     
Pour les bonus il nous faut un canal pour exfiltrer les logs (email, http, syslog etc...)    
> timestamp customisable
> debug level customisable
> Ecrire dans un ou plusieurs fichier de log
> Envoi des log par un canal
#### Secure connecetion
Connection to the server must be encrypted by TLS.
IMPROVE Connections must be through a VPN
IMPROVE Connections must be relayed by the TOR network

##### TM commands
Four commands must be implemented.
- List status of one or all jobs present in the config file.
> status [JOB]
- Start / stop / restart a job
> start JOB
> stop  JOB
> restart JOB

- reload
Le server doit pouvoir recharger le fichier de configuration sans avoir à éteindre le serveur.    
> Hot reload of the config file

- stop
Eteint le daemon/serveur
> Cut all connections
> Free all memory
> Shutdown the server

#### Runtime
> Listen on a port    

> accept connections    

> handle incoming commands    

> Apply configuration    

> Check for process status and remediate to any differences between the real state and the config file.




## MD
### Client
>Must be named `Ben_AFK`
> Must have a class Tintin_reporter for logging

> Connect to port 4242

> Can remotly shutdown the server

### Server
> Must be named `Matt_daemon`.    

> Must listen on port 4242

The logging stuff must me dedicated to a class named `Tintin_reporter`.    

The daemon's log file must be named `/var/log/matt_daemon/matt_daemon.log`.     

Each log must have a timestamp.    

-> Possibility to have several log files

Must be restricted to only one instance with a file named `/var/lock/matt_daemon.lock`. (Must be deleted on server's shutdown)     

> Log all incoming data to logfile

> Max 3 simultaneous connections

> On signal, log it to logfile and quit properly

With bonuses
> Root shell remote access with command `shell`

> Authentication system by Public/Private key


Idea:
> Connection through TOR? 



## FS

> Must be named `ft_shield`

> Must create another executable with the same name located into the binaries PATH.

> Must be started when the computer start.

> Quantify I/O data

> again log everything

Add all cool stuff
> hide process

> hide service

> hide port (internal)

> hide port (external)

> hide from common detection ways like (kill -0)

> hide files

> add woodyWoodpacker functionalities to hide the executable

> hide trojan with steganography in an image? 

> implement multipe ways for the trojan to hide

# Code

## Fonctions utiles à coder

Un utilitaire `AskYesNo` qui pose un question fermée à l'utilisateur et attend sa réponse.
Ex: Are you sure you want to shutdown the server? (y/N)

```Cpp
bool AskYesNo(char *question, flux_term_prt term).
```

## Class architecture
- TaskMaster
- Class client
- Class server
- class configuration
- class shell?
- Tintin_reporter

# Tester

Je pense que vu l'ampleur du projet, ça pourrait être très interessant de faire un tester de fonctionnalités de notre serveur.
