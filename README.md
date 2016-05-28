arduino-WOL
==================

Ce programme permet d'allumer Un PC et ses périphériques depuis un appareil distant ou via un bouton grace au Wake-on-Lan (+ Secure On), et d'un signal radio.

La gestion radio se fait grace a des  commutateurs electriques radio-commandé, qui emmettent sur la frequence 433Mhz. Cela permet par exemple d'allumer automatiquement écran, enceinte, etc.

Par exemple, si le pc a réveiller possède comme adresse mac 0x22,0x45,0x45,0xF8,0xE3,0x42, alors via un logiciel de réveil, on rentre les informations suivante :
- Adresse MAC du PC a réveiller : 0x22,0x45,0x45,0xF8,0xE3,0x42
- mot de passe pour réveiller le PC : 0x74,0x34,0xF4,0x45,0x5E,0x12 ( a placer dans le champs secureOn)
- port a utiliser : 44596

Le reste des options sont a configurer directement dans le fichier arduino_WOL_dyndns.ino

A cela s'ajoute des information confidentiel, non synchroniser avec github, ecrite dans le fichier password.h.

contenu de password.h :

    #define ADRESSE_WOL     0x22,0x45,0x45,0xF8,0xE3,0x42	// Adresse du PC a réveiller
    #define PASSWORD_WOL 	0x74,0x34,0xF4,0x45,0x5E,0x12	// Password (a utiliser via apps SecureOn)
    #define PORT_DISTANT_WOL		44596							// Port écouté par Arduino
