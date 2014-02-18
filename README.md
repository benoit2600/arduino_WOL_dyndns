arduino-WOL-dyndns
==================

Ce programme permet d'allumer Un PC depuis un appareil distant, par l'intermédiaire d'une adresse Wake-on-Lan, et d'une adresse mac Secure On, le tout via une adresse host (ici gérer par dyndns)

En plus de cela, le logiciel met automatiquement a jour l'adresse DynDNS en fonction de l'adresse IP Public du routeur derière lequel se trouve le PC.
On peut également reveiller le le PC via des boutons. Il y a aussi une gestion de commutateur electrique radio-commandé, qui emmettent sur la frequence 433Mhz. Cela permet par exemple d'allumer automatiquement écran, enceinte, etc.

Par exemple, si le pc a réveiller possède comme adresse mac 0x22,0x45,0x45,0xF8,0xE3,0x42, alors via un logiciel de réveil, on rentre les informations suivante :
- Adresse MAC du PC a réveiller : 0x22,0x45,0x45,0xF8,0xE3,0x42
- mot de passe pour réveiller le PC : 0x74,0x34,0xF4,0x45,0x5E,0x12 ( a placer dans le champs secureOn)
- adresse du routeur : "host".dyndns.org
- port a utiliser : 44596

Le reste des options sont a configurer directement dans le fichier arduino_WOL_dyndns.ino
contenu de password.h :

#define USER_PASS64 dXNlcjpwYXNz            // user:pass en base 64.
#define ADRESSE_WOL     0x22,0x45,0x45,0xF8,0xE3,0x42	// Adresse du PC a réveiller
#define PASSWORD_WOL 	0x74,0x34,0xF4,0x45,0x5E,0x12	// Password (a utiliser via apps SecureOn)
#define HOST_DYNDNS				"host".dyndns.org		// host dyndns a mettre a jour.
#define PORT_DISTANT_WOL		44596							// Port écouté par Arduino
