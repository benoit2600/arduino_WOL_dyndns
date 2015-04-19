
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
#define ADDRESSE_MAC_ARDUINO 	0x00,0xAA,0xBB,0xCC,0xDE,0x03 	// Adresse mac de la arduino 
#define IP_LOCAL_ARDUINO		192,168,0,180					// IP de la arduino sur le réseaux local
#define IP_BROADCAST_ARDUINO	192,168,0,255					// IP de broadcast pour le WOL
#define MAGIC_PACKET_SIZE 		108								// Taille d'un paquet magique
#define WOL_PORT				9								// Port de transmission (local) du paquet magique

/*	- Paramètre Dyndns*/
#define DNS_IP 					208,67,222,222  			// ici, OpenDNS (ex DNS google : 8,8,8,8)

/*	-Parametre bouton*/
#define PINBUTTON1 6
#define PINBUTTON2 7

/* Paramètre télécommande, écran*/
#define PORT_EXTINCTION_SERVER 4243							// Port du serveur d'extinction du PC
#define PIN_TRANSMITTER	9 									// Pin du transmetteur 433Mhz
/*	- Paramètre de débugage*/
//#define DEBUG 1

#ifdef DEBUG
  #define DEBUGLN(x) Serial.println x
  #define DEBUG(x) Serial.print x
#else
  #define DEBUGLN(x)
  #define DEBUG(x)
#endif

/*variable global*/
EthernetUDP udp; 											// Création d'Un objet de classe EthernetUDP
EthernetServer tcpServer(PORT_EXTINCTION_SERVER);
String last_debug_msg; 

void setup() {
	byte arduinoMAC[] = {ADDRESSE_MAC_ARDUINO}; 				// Adresse MAC de la Arduino
    IPAddress ipLocal(IP_LOCAL_ARDUINO);							// Adresse IP de la Arduino 
	pinMode(PINBUTTON1, INPUT);
	pinMode(PINBUTTON2, INPUT);
	//delay(1000);

	#ifdef DEBUG
		Serial.begin(9600); // set up Serial library at 9600 bps
	#endif
	DEBUGLN(("c'est parti !"));
	Ethernet.begin(arduinoMAC,ipLocal);
	DEBUGLN(("TCP connecter !"));
	udp.begin(PORT_DISTANT_WOL);
	tcpServer.begin();
	DEBUGLN(("On passe a la boucle principale \n"));
}

void loop() {
    static unsigned int timer =0;

	if(timer % 50 == 0){			// On s'occupe du wol reseaux  toutes les 1 secondes
		check_wol_magic_packet();
		timer = 0;
	}
	check_button(PINBUTTON1);
	//check_button(PINBUTTON2);
	check_ethernet_msg();
	delay(10);
	timer++;
}
/**
* \brief vérifie une connection réseau, et regarde si elle correspond à l'extinction de l'écran.
*/
void check_ethernet_msg(void){
	EthernetClient client = tcpServer.available();
	String stringstr;

  	if (client) {
  		DEBUGLN(("y a un client !"));
  		char c;
    	if (client.connected()) {
      		while (client.available()) {

       			c = client.read();
          		stringstr += c;
				DEBUGLN((stringstr));
        	} 
		}
		stringstr.trim();
		DEBUG(("msg recu :_"));
		DEBUGLN((stringstr));
		test_msg(stringstr, &client);
		//client.stop();
	}
}

void test_msg(String str,EthernetClient *client){
	char otherString[50];
	unsigned long time;
	time = millis();

	DEBUG(("msg dans test_msg : "));
	DEBUGLN((str));

	if( str == "extinction\n" || str == "extinction"){
		client->write("ok pour l'extinction\n");
		delay(5000); //on attend pour verifier la bonne extinction de windows
		Remote_OFF();
		Remote_OFF();
		last_debug_msg = "Extinction ecran";

	}
	if( str == "debug" || str == "debug\n"){
		last_debug_msg+="\nuptime :";
		last_debug_msg+=time;

		last_debug_msg.toCharArray(otherString, last_debug_msg.length() + 1); 
		client->write(otherString);

		DEBUGLN(("msg de debug sans RaL "));
		DEBUGLN((otherString));
		last_debug_msg=""; 	// fix overflow
	}	
}
/**
 * \brief Si un bouton est appuyé, on envoie un paquet magique.
 * \param inPin Pin du bouton.
 */
void check_button(int inPin){
	unsigned char  val = digitalRead(inPin);  // read input value
	if (val == HIGH) {         // check if the input is HIGH (button released)
		delay(20);
		val = digitalRead(inPin); 
		if (val == HIGH) {        // check false positive
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
			last_debug_msg = "Bouton appuyé : ";
			last_debug_msg += inPin ;

		} 
	}
}


/**
 * \brief Envoi un packet magique a l'adresse passé en paramètre.
 * \param packetBuffer Adresse mac du PC a réveiller.
 */
void wol_send_packet(byte * packetBuffer){
	IPAddress broadcastIP(IP_BROADCAST_ARDUINO);					// Adresse IP de Broadcast LAN

	last_debug_msg = "allumage via WOL";


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
		// envoi du paquet sur l'adresse à l'adresse packetBuffer
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
	
	mySwitch.switchOff(2, 1);	// On essaie d'éteindre directement toutes les prises.
	delay(1000);
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

