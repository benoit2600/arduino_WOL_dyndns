
/**
 * \file main.ino
 * \brief serveur WOL + MAJ DNS auto.
 * \author benoit2600
 * \version 0.1
 * \date 1 fevrier 2014
 *
 * Serveur Arduino gérant les appel Wake-On-Lan, 
 * la maj auto de DNS (via Dyndns), et l'allumage d'un PC via appel distant 
 * ou via l'appui d'un bouton. Il gère aussi une télécommande 
 * (via simulation d'appui sur les boutons grace a des moteurs.
 *
 */
#include <SPI.h>
#include <Ethernet.h>
#include <Dns.h>
#include <EthernetUdp.h>
#include <RCSwitch.h>
#include "password.h"
/* Paramètre utilisateur : */

/* - Paramètre généraux :  */
#define ADRESSE_WOL				0x30,0x85,0xA9,0xA0,0xE0,0x38	// Adresse du PC a réveiller
#define PASSWORD_WOL 			0x00,0xAC,0x1B,0xA3,0xDE,0x53	// Password (a utiliser via apps SecureOn)

#define ADDRESSE_MAC_ARDUINO 	0x00,0xAA,0xBB,0xCC,0xDE,0x03 	// Adresse mac de la arduino 
#define PORT_DISTANT_WOL		33664							// Port écouté par Arduino
#define IP_LOCAL_ARDUINO		192,168,0,180					// IP de la arduino sur le réseaux local
#define IP_BROADCAST_ARDUINO	192,168,0,255					// IP de broadcast pour le WOL
#define MAGIC_PACKET_SIZE 		108								// Taille d'un paquet magique
#define WOL_PORT				9								// Port de transmission (local) du paquet magique

/*	- Paramètre Dyndns*/
#define HOST_DYNDNS				benoit2600.dyndns.org		// host dyndns a mettre a jour.
#define DNS_IP 					208,67,222,222  			// ici, OpenDNS (ex DNS google : 8,8,8,8)
#define USER_PASS64				PASSWORD	 // user:pass en base 64.

/*	-Parametre bouton*/
#define PINBUTTON1 6
#define PINBUTTON2 7

/* Paramètre télécommande, écran*/
#define PORT_EXTINCTION_SERVER 4243							// Port du serveur d'extinction du PC
#define PIN_TRANSMITTER	9 									// Pin du transmetteur 433Mhz
/*	- Paramètre de débugage*/
#define DEBUG 1

#ifdef DEBUG
  #define DEBUGLN(x) Serial.println x
  #define DEBUG(x) Serial.print x
#else
  #define DEBUGLN(x)
  #define DEBUG(x)
#endif


unsigned int timer =0;
byte arduinoMAC[] = {ADDRESSE_MAC_ARDUINO}; 				// Adresse MAC Arduino sticker
IPAddress ipLocal(IP_LOCAL_ARDUINO);							// Adresse IP de l'arduino 
IPAddress broadcastIP(IP_BROADCAST_ARDUINO);					// Adresse IP de Broadcast LAN
EthernetUDP udp; 											// Création d'Un objet de classe EthernetUDP
EthernetServer tcpServer(PORT_EXTINCTION_SERVER);

void setup() {
	pinMode(PINBUTTON1, INPUT);
	pinMode(PINBUTTON2, INPUT);

	delay(1000);
	Serial.begin(9600); // set up Serial library at 9600 bps
	DEBUGLN(("c'est parti !"));
	Ethernet.begin(arduinoMAC,ipLocal);
	DEBUGLN(("Ethernet connecter !"));
	udp.begin(PORT_DISTANT_WOL);
	tcpServer.begin();
	DEBUGLN(("On passe a la boucle principale \n"));
}

void loop() {
	if(timer == 72000){ // On s'occupe des DNS toutes les 2h
		findAndSetIPPublic();
		timer = 0;
	}
	if(timer % 10 == 0){// On s'occupe du wol reseaux  toutes les 1 seconde
		check_wol_magic_packet();
	}
	check_button(PINBUTTON1);
	check_button(PINBUTTON2);
	check_extinction();
	delay(100);
	timer++;
}
/**
* \brief vérifie une connection réseau, et regarde si elle correspond à l'extinction de l'écran.
*/
void check_extinction(void){
	EthernetClient client = tcpServer.available();
  	if (client) {
  		DEBUGLN(("y a un client !"));
  		char c;
  		char * str = (char* )calloc(sizeof(char),12);
		char * strP = str; // pointeur sur la chaine d'entrée str

    	while (client.connected()) {
      		if (client.available()) {

       			c = client.read();
          		*strP = c;
				strP++;
        	} 
		}
		DEBUG(("msg recu : "));
		DEBUGLN((str));
		if(strcmp(str, "extinction")==0){
			delay(5000); //on attend pour verifier la bonne exctinction de windows
			Remote_OFF();
			Remote_OFF();
		}
		free(str);
	}
}

/**
 * \brief Si un bouton est appuyé, on envoie un paquet magique.
 * \param inPin Pin du bouton.
 */
void check_button(int inPin){
	unsigned char  val = digitalRead(inPin);  // read input value
	if (val == HIGH) {         // check if the input is HIGH (button released)
		while(val == HIGH){
			val = digitalRead(inPin);
			DEBUGLN(("boucle bouton"));

			delay(100);
		}
		DEBUG(("Bouton pin "));
		DEBUG((inPin));
		DEBUGLN((" appuyer"));
		wol_send_packet(NULL);  // Allume le PC
		DEBUGLN(("fin check bouton"));
	} 
}

/**
 * \brief met a jour l'adresse IP public si elle a changé depuis la dernière vérif.
 */
void findAndSetIPPublic(void){
		static char * IPPublicNew;
		static char * IPPublic = (char*) calloc(sizeof(char), 15); // Adresse IP public actuelle. (taille max : 15. min : 11)

		IPPublicNew = IpAdressPublic();
		DEBUG(("\nIPPublicNew :"));
		DEBUGLN((IPPublicNew));
		DEBUG(("IPPublic :"));
		DEBUGLN((IPPublic));
		if(strcmp(IPPublicNew, IPPublic) != 0)
		{
			   setIpAdressPublic(IPPublicNew);
		}
		free(IPPublic);
		IPPublic = IPPublicNew;
}

/**
 * \brief Transmet une adresse IP au service DynDns en fonction de l'host.
 *
 * \param IPpublic nouvelle IP a joindre a l'host.
 * \return retourne 1 si tout va bien, et 0 en cas d'erreur de connection.
 */
int setIpAdressPublic(char * IPpublic){
	EthernetClient tcpClient; 									// Client TCP
	char dynMember[] = "members.dyndns.org";
	char hostname[] = "HOST_DYNDNS";
	DNSClient dns1;
	IPAddress dns_ip(DNS_IP); // OPENDNS
	IPAddress out_ip;
	dns1.begin(dns_ip);
	dns1.getHostByName(dynMember, out_ip);
	// création chaine getLine:
	
	if (tcpClient.connect(out_ip, 80)) {
		tcpClient.println("GET /nic/update?hostname=" + (String)hostname + "&myip=" + (String)IPpublic + " HTTP/1.0"); // Make a HTTP request:
		tcpClient.println("Host: members.dyndns.org");
		tcpClient.println("Authorization: Basic USER_PASS64");
		tcpClient.println("User-Agent: Benoit - arduino - 1");
		tcpClient.println("Connection: close");
		tcpClient.println();
	}
	else {
		// if you didn't get a connection to the server:
		DEBUGLN(("connection failed"));
		return 0;
	}
	while(tcpClient.connected())
	{
		if (tcpClient.available())
		{
			char c = tcpClient.read();
			//DEBUG((c));
		}
		check_button(PINBUTTON1);
		check_button(PINBUTTON2);
	}
	tcpClient.stop();
	return 1;
}

/**
 * \brief Va chercher l'adresse P Public du routeur depuis le site checkip.dyndns.com
 * \return retourne l'adresse IP sous la forme : "xxx.xxx.xxx.xxx"
 */
char * IpAdressPublic(void){
	EthernetClient tcpClient;	// Client TCP

	boolean ligne = false; // permet de recuperer la ligne contenant le HTML, en ne prenant pas le header
	char * str = (char* )calloc(sizeof(char),256);
	char * strP = str; // pointeur sur la chaine d'entrée str
	char DynIP[] = "IP Address: "; //phrase après laquelle ce trouve l'adresse IP"
	char DynFinIp = '<'; // symbole se trouvant juste après l'adresse IP
	char * IP = (char*) calloc(sizeof(char), 15); // Adresse IP finale, a placer dans str a la fin de la fonction. (taille max : 15. min : 11)
	char * IPPointeur = IP;
	char dynCheckIp[] = "checkip.dyndns.com";
	DNSClient dns;
	IPAddress dns_ip(DNS_IP);
	IPAddress out_ip;
	dns.begin(dns_ip);
	dns.getHostByName(dynCheckIp, out_ip);
	if (tcpClient.connect(out_ip, 80)) {
		tcpClient.println("GET / HTTP/1.0"); // Make a HTTP request:
		tcpClient.println("HOST: "+ (String)dynCheckIp);
		tcpClient.println("Connection: close");
		tcpClient.println();
	}
	else {
		// if you didn't get a connection to the server:
		DEBUGLN(("connection failed"));
		return NULL;
	}
	while(tcpClient.connected())
	{
		delay(10);
		if (tcpClient.available())
		{
			char c = tcpClient.read();
			if((c == '<') || (ligne == true))
			{
				ligne = true;
				//DEBUG((c));
				*strP = c;
				strP++;
			}
		}
	}
	tcpClient.stop();
	strP = strstr(str, DynIP);
	strP += (int) strlen(DynIP); // on se place au niveau de l'adresse IP
	while(*strP != DynFinIp) // On recopie l'adresse IP dans IP
	{
		*IPPointeur = *strP;
		IPPointeur++;
		strP++;
	}
	IP =(char*) realloc( IP, strlen(IP)*sizeof(char));
	if(IP == NULL) {
		DEBUG(("Erreur reallocation memoire de IP"));
		while(true);
	}
	free(str);
	return IP;
}
/**
 * \brief Envoi un packet magique a l'adresse passé en paramètre.
 * \param packetBuffer Adresse mac du PC a réveiller.
 */
void wol_send_packet(byte * packetBuffer){
	if(packetBuffer == NULL){
 		byte adresseMacWOL[] = {ADRESSE_WOL};
 		byte packetBufferDefaut[MAGIC_PACKET_SIZE];
 		for( int i = 0; i < 6; i++ )
			packetBufferDefaut[i] = 0xFF;
		for(int i = 6; i < MAGIC_PACKET_SIZE; i++){
			packetBufferDefaut[i] = adresseMacWOL[i%6];
		}
		DEBUGLN(("envoi du paquet magique par defaut."));
		udp.beginPacket(broadcastIP, WOL_PORT);
		udp.write(packetBufferDefaut, MAGIC_PACKET_SIZE);
		udp.endPacket();
	}
	else{
		// envoi du paquet sur l'adresse de broadcast
		DEBUGLN(("envoi du paquet magique."));
		udp.beginPacket(broadcastIP, WOL_PORT);
		udp.write(packetBuffer, MAGIC_PACKET_SIZE);
		udp.endPacket();
	}
	DEBUGLN(("Allumage ecrans."));
	Remote_ON();	//On allume les ecrans
	Remote_ON();	//On relance au cas ou

}
/**
 * \brief Verifie les paquet magique reçu, et reveil le PC si tout est bon. Une vérification par mot de passe est effectué.
 */
void check_wol_magic_packet(void){
	byte password[] = {PASSWORD_WOL};
	byte packetBuffer[MAGIC_PACKET_SIZE];
	boolean passCheck= true;
	// si on a des données à lire, on va le traiter
	int packetSize = udp.parsePacket();
	if(packetSize)
	{
		// lecture des données et stockage dans packetBufffer
		udp.read(packetBuffer, MAGIC_PACKET_SIZE +6);
		for( int i = 102; i < 108; i++ ){
			if(packetBuffer[i] != password[i-102]){
				passCheck = false;
				break;
			}
		}
		if( passCheck== false){
			DEBUGLN(("Mauvais mot de passe "));
		}
		else{
			wol_send_packet(packetBuffer); // envoi du paquet magique recu.
		}
		do{									// On vide la pile de paquet reçu
			packetSize = udp.parsePacket();
			DEBUG(("packetSize :  "));
			DEBUGLN((packetSize));
			delay(10);
		}while(packetSize != 0);
	}
}

/* Séquence perso d'ectinction des écrans + enceinte*/
void Remote_OFF(void){
	RCSwitch mySwitch = RCSwitch();								// controle l'emetteur 433Mhz
	mySwitch.enableTransmit(PIN_TRANSMITTER);

	mySwitch.switchOff(2, 3);
	delay(1000);
	mySwitch.switchOff(2, 2);
	delay(1000);
	mySwitch.switchOff(2, 1);
	delay(1000);
  	mySwitch.disableTransmit();
}

/*Séquence perso d'allumage des écrans + enceinte*/
void Remote_ON(void){
	RCSwitch mySwitch = RCSwitch();								// controle l'emetteur 433Mhz
	mySwitch.enableTransmit(PIN_TRANSMITTER);

  	mySwitch.switchOn(2, 1);
  	delay(750);
  	mySwitch.switchOn(2, 2);
  	delay(750);  
  	mySwitch.switchOn(2, 3);
  	delay(750);  
  	mySwitch.disableTransmit();
}

