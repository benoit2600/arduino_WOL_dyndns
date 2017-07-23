
/**
 * \file arduino_WOL_dyndns.ino
 * \brief serveur WOL.
 * \author benoit2600
 * \version 2
 * \date 28 mai 20016
 *
 * Serveur Arduino gérant les appel Wake-On-Lan
 * et l'allumage d'un PC via appel distant
 * ou via l'appui d'un bouton. Il gère aussi une télécommande
 * (via signal radio 433mHz.
 *
 */

#include <EtherCard.h>
#include <IPAddress.h>
#include <RCSwitch.h>
#include "password.h"


/* Paramètre utilisateur : */

/* - Paramètre généraux :  */
#define ADDRESSE_MAC_ARDUINO 	0x00,0xAA,0xBB,0xCC,0xDE,0x03 	// Adresse mac de la arduino
#define IP_LOCAL_ARDUINO		192,168,0,12			// IP de la arduino sur le réseaux local
#define IP_BROADCAST_ARDUINO	192,168,0,255					// IP de broadcast pour le WOL
#define MAGIC_PACKET_SIZE 		108								// Taille d'un paquet magique
#define WOL_PORT				9								// Port de transmission (local) du paquet magique
#define REQUEST_RATE 5000 // milliseconds

/*	-Parametre bouton*/
#define PINBUTTON1 6

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

/*variable global*/
String last_debug_msg;
byte Ethernet::buffer[500];   // a very small tcp/ip buffer is enough here


void setup () {
  static byte myip[] = {IP_LOCAL_ARDUINO};							// Adresse IP de la Arduino
  static byte gwip[] = {192, 168, 0, 1};
  static byte arduinoMAC[] = {ADDRESSE_MAC_ARDUINO};         // Adresse MAC de la Arduino

  pinMode(PINBUTTON1, INPUT);

#ifdef DEBUG
  Serial.begin(57600); // set up Serial library at 57600 bps
#endif

  DEBUGLN(("c'est parti !"));


  if (ether.begin(sizeof Ethernet::buffer, arduinoMAC) == 0)
    Serial.println( "Failed to access Ethernet controller");

  ether.staticSetup(myip, gwip);

  while (ether.clientWaitingGw())
    ether.packetLoop(ether.packetReceive());
  Serial.println("Gateway found");

  ether.udpServerListenOnPort(&check_wol_magic_packet, PORT_DISTANT_WOL);
  ether.udpServerListenOnPort(&check_udp_msg, 33663); // check msg extinction

  DEBUGLN(("On passe a la boucle principale \n"));
}

void loop() {
  ether.packetLoop(ether.packetReceive());
  delay(100);
  check_button(PINBUTTON1);
  //check_ethernet_msg();

}

void check_udp_msg(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len) {

  char password[] = "off";
  boolean passCheck = true;
  // si on a des données à lire, on va le traiter
  for ( unsigned char i = 0; i < len; i++ ) {
    if (data[i] != password[i]) {
      passCheck = false;
      break;
    }
  }
  if ( passCheck == true) {
    DEBUGLN(("extinction des ecrans"));
    delay(5000);
    Remote_OFF();  //On eteind les ecrans

  }
  else {
    DEBUGLN(("erreur msg udp inconnu"));
  }
  while (ether.packetReceive() > 0)
    DEBUGLN(("on vide la pile de paquets"));
}



/**
   \brief Verifie les paquet magique reçu, et reveil le PC si tout est bon. Une vérification par mot de passe est effectué.
*/
boolean wolState = false;
void check_wol_magic_packet(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len) {

  char password[] = {PASSWORD_WOL};
  boolean passCheck = true;
  // si on a des données à lire, on va le traiter
  for ( unsigned char i = 102; i < 108; i++ ) {
    if (data[i] != password[i - 102]) {
      passCheck = false;
      break;
    }
  }
  if ( passCheck == true) {
    DEBUGLN(("envoie packet magique"));

    byte wolMac[] = {ADRESSE_WOL};
    ether.sendWol(wolMac); // envoi du paquet magique recu.
    Remote_ON();  //On allume les ecrans

  }
  else {
    DEBUGLN(("Mauvais mot de passe "));
  }
  while (ether.packetReceive() > 0)
    DEBUGLN(("on vide la pile de paquets"));
}


/**
   \brief Si un bouton est appuyé, on envoie un paquet magique.
   \param inPin Pin du bouton.
*/

void check_button(int inPin) {
  unsigned char  val = digitalRead(inPin);  // read input value
  if (val == HIGH) {         // check if the input is HIGH (button released)
    delay(50);
    val = digitalRead(inPin);
    if (val == HIGH) {        // check false positive
      while (val == HIGH) {
        val = digitalRead(inPin);
        DEBUGLN(("boucle bouton"));

        delay(50);
      }
      DEBUG(("Bouton pin "));
      DEBUG((inPin));
      DEBUGLN((" appuyer"));
      byte wolMac[] = {ADRESSE_WOL};
      ether.sendWol(wolMac); // envoi du paquet magique recu.
      Remote_ON();  //On allume les ecrans
      DEBUGLN(("fin check bouton"));
      last_debug_msg = "Bouton appuyé : ";
      last_debug_msg += inPin ;

    }
  }
}



/* Séquence perso d'extinction des écrans + enceinte*/
void Remote_OFF(void) { 
  RCSwitch mySwitch = RCSwitch();								// controle l'emetteur 433Mhz


  mySwitch.enableTransmit(PIN_TRANSMITTER);
  delay(3000);
  mySwitch.switchOff(2, 3);
  delay(2000);
  mySwitch.switchOff(2, 2);
  delay(2000);
  mySwitch.switchOff(2, 1);
  delay(2000);
  mySwitch.disableTransmit();
}

/* Extinction d'un interrupteur*/
void switch_OFF(char num) {
  RCSwitch mySwitch = RCSwitch();								// controle l'emetteur 433Mhz
  mySwitch.enableTransmit(PIN_TRANSMITTER);

  mySwitch.switchOff(2, num);
  delay(2000);
  mySwitch.disableTransmit();
}

/*Séquence perso d'allumage des écrans + enceinte*/
void Remote_ON(void) {
  RCSwitch mySwitch = RCSwitch();								// controle l'emetteur 433Mhz
  mySwitch.enableTransmit(PIN_TRANSMITTER);

  mySwitch.switchOn(2, 1);
  delay(500);
  mySwitch.switchOn(2, 2);
  delay(1000);
  mySwitch.switchOn(2, 3);
  delay(1000);
  mySwitch.disableTransmit();



}

/*allumage d'un interrupteur*/
void switch_ON(char num) {
  RCSwitch mySwitch = RCSwitch();								// controle l'emetteur 433Mhz
  mySwitch.enableTransmit(PIN_TRANSMITTER);

  mySwitch.switchOn(2, num);
  delay(1000);
  mySwitch.disableTransmit();
}
