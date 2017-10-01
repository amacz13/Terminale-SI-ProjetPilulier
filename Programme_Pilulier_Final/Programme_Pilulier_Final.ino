/*
Programme du Pilulier Electronique
Date de denière modification : 15/05/2015
*/

//Initialisation des extensions :
#include <LiquidCrystal.h> //Ajout de l'extension de contrôle de l'écran LCD
#include "Wire.h" //Ajout de l'extension de liaison I2C
#define DS1307_I2C_ADDRESS 0x68 //Définition du type de carte horloge temps-réel utilisée
#include <Servo.h> //Ajout de l'extension de contrôle des servomoteurs


//Définition des Variables Utilisées
int Menu = 0;
int Chiffre = 0;
char Code_1 = 0; //1er Chiffre saisi lors de la saisie du code confidentiel
char Code_2 = 0; //2eme Chiffre saisi lors de la saisie du code confidentiel
char Code_3 = 0; //3eme Chiffre saisi lors de la saisie du code confidentiel
char Code_4 = 0; //4eme Chiffre saisi lors de la saisie du code confidentiel
char C1 = 49; //Code enregistré dans la machine : 1er caractère
char C2 = 50; //Code enregistré dans la machine : 2eme caractère
char C3 = 51; //Code enregistré dans la machine : 3eme caractère
char C4 = 52; //Code enregistré dans la machine : 4eme caractère
byte HeureP1 = 0x07; //Heure de la 1ère prise de médicaments
byte MinP1 = 0x00; //Minute de la 1ère prise de médicaments
byte HeureP2 = 0x0C; //Heure de la 2ème prise de médicaments
byte MinP2 = 0x00; //Minute de la 2ème prise de médicaments
byte HeureP3 = 0x13; //Heure de la 3ème prise de médicaments
byte MinP3 = 0x00; //Minute de la 3ème prise de médicaments
int Plus = 0;
Servo myservo; //Nom du servomoteur linéaire
int val;
int buzzer =0; //Broche du buzzer
const int fourchePin = 1; //Sortie de la fourche pin5
int moteur = 12; //Broche du moteur
//Variables utilisées pour la lecture des boutons
int lcd_key     = 0;
int adc_key_in  = 0;
int LED = 10;
int HeureChgt;
int MinuteChgt;
byte Heure;

//Définition des broches utilisées par l'écran LCD
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//Convertir les variables décimales en variables binaires
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

//Convertir les variables binaires en variables décimales
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

// Fonction de modification de la date et de l'heure de l'horloge temps-réel :
void setDateDs1307(byte second,        // 0-59
                   byte minute,        // 0-59
                   byte hour,          // 1-23
                   byte dayOfWeek,     // 1-7
                   byte dayOfMonth,    // 1-28/29/30/31
                   byte month,         // 1-12
                   byte year)          // 0-99
{
   Wire.beginTransmission(DS1307_I2C_ADDRESS);
   Wire.write(0);
   Wire.write(decToBcd(second));
   Wire.write(decToBcd(minute));
   Wire.write(decToBcd(hour));
   Wire.write(decToBcd(dayOfWeek));
   Wire.write(decToBcd(dayOfMonth));
   Wire.write(decToBcd(month));
   Wire.write(decToBcd(year));
   Wire.endTransmission();
}

// Fonction de récupération de la date et de l'heure de l'horloge temps-réel
void getDateDs1307(byte *second,
          byte *minute,
          byte *hour,
          byte *dayOfWeek,
          byte *dayOfMonth,
          byte *month,
          byte *year)
{
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  // A few of these need masks because certain bits are control bits
  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f);  // Need to change this if 12 hour am/pm
  *dayOfWeek  = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month      = bcdToDec(Wire.read());
  *year       = bcdToDec(Wire.read());
}

//On définit les variables des boutons :
#define S1 0
#define S2 1
#define S3 2
#define S4 3
#define S5 4
#define NONE 5

//Fonction de lecture des boutons
int read_LCD_buttons(){
    adc_key_in = analogRead(0);       //Lecture de la valeur analogique émise par le clavier à touches

    if (adc_key_in > 1000) return NONE; 

    if (adc_key_in < 50)   return S1;  
    if (adc_key_in < 450)  return S2; 
    if (adc_key_in < 650)  return S3; 
    if (adc_key_in < 850)  return S4; 
    if (adc_key_in < 950)  return S5; 

    return NONE;                //Si la valeur est invalide, alors on considère qu'aucun bouton n'est actionné
}

void i2c_eeprom_write_byte( int deviceaddress, unsigned int eeaddress, byte data ) {
    int rdata = data;
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(rdata);
    Wire.endTransmission();
  }

  // WARNING: address is a page address, 6-bit end will wrap around
  // also, data can be maximum of about 30 bytes, because the Wire library has a buffer of 32 bytes
  void i2c_eeprom_write_page( int deviceaddress, unsigned int eeaddresspage, byte* data, byte length ) {
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddresspage >> 8)); // MSB
    Wire.write((int)(eeaddresspage & 0xFF)); // LSB
    byte c;
    for ( c = 0; c < length; c++)
      Wire.write(data[c]);
    Wire.endTransmission();
  }

  byte i2c_eeprom_read_byte( int deviceaddress, unsigned int eeaddress ) {
    byte rdata = 0xFF;
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(deviceaddress,1);
    if (Wire.available()) rdata = Wire.read();
    return rdata;
  }

  // maybe let's not read more than 30 or 32 bytes at a time!
  void i2c_eeprom_read_buffer( int deviceaddress, unsigned int eeaddress, byte *buffer, int length ) {
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(deviceaddress,length);
    int c = 0;
    for ( c = 0; c < length; c++ )
      if (Wire.available()) buffer[c] = Wire.read();
  }

//Fonction d'écran de connexion
void Connexion(){
  lcd.clear();
  while (Menu == 1) {
  lcd.setCursor(0,0);
  lcd.print("Connexion :");
  //On réalise une pause de 0.5 seconde pour éviter que le précédent bouton soit comptabilisé
  delay(500);
  Chiffre = 0;
  //Saisie du premier chiffre du code
  while (Chiffre != 1){
     lcd_key = read_LCD_buttons();
     switch (lcd_key){
       case S1:{
            Code_1 = 49;
            Chiffre = 1;
            lcd.setCursor(12,0);
            lcd.print("1");
            break;
       }
       case S2:{
             Code_1 = 50;
             Chiffre = 1;
             lcd.setCursor(12,0);
             lcd.print("2");
             break;
       }    
       case S3:{
             Code_1 = 51;
             Chiffre = 1;
             lcd.setCursor(12,0);
             lcd.print("3");
             break;
       }
       case S4:{
             Code_1 = 52;
             Chiffre = 1;
             lcd.setCursor(12,0);
             lcd.print("4");
             break;
       }
       case S5:{
             Code_1 = 53;
             Chiffre = 1;
             lcd.setCursor(12,0);
             lcd.print("5");
             break;
       }
  }
  }
  
  //On réalise une pause de 0.5 seconde pour éviter que le précédent bouton soit comptabilisé
  delay(500);
  Chiffre = 0;
  //Saisie du deuxième chiffre du code
  while (Chiffre != 1){
     lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S1:{
            Code_2 = 49;
            Chiffre = 1;
            lcd.setCursor(13,0);
            lcd.print("1");
            break;
       }
       case S2:{
             Code_2 = 50;
             Chiffre = 1;
             lcd.setCursor(13,0);
             lcd.print("2");
             break;
       }    
       case S3:{
             Code_2 = 51;
             Chiffre = 1;
             lcd.setCursor(13,0);
             lcd.print("3");
             break;
       }
       case S4:{
             Code_2 = 52;
             Chiffre = 1;
             lcd.setCursor(13,0);
             lcd.print("4");
             break;
       }
       case S5:{
             Code_2 = 53;
             Chiffre = 1;
             lcd.setCursor(13,0);
             lcd.print("5");
             break;
       }
}
  }
  //On réalise une pause de 0.5 seconde pour éviter que le précédent bouton soit comptabilisé
  delay(500);
  Chiffre = 0;
  //Saisie du troisième chiffre du code
  while (Chiffre != 1){
     lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S1:{
            Code_3 = 49;
            Chiffre = 1;
            lcd.setCursor(14,0);
            lcd.print("1");
            break;
       }
       case S2:{
             Code_3 = 50;
             Chiffre = 1;
             lcd.setCursor(14,0);
             lcd.print("2");
             break;
       }    
       case S3:{
             Code_3 = 51;
             Chiffre = 1;
             lcd.setCursor(14,0);
             lcd.print("3");
             break;
       }
       case S4:{
             Code_3 = 52;
             Chiffre = 1;
             lcd.setCursor(14,0);
             lcd.print("4");
             break;
       }
       case S5:{
             Code_3 = 53;
             Chiffre = 1;
             lcd.setCursor(14,0);
             lcd.print("5");
             break;
       }
}
}
  //On réalise une pause de 0.5 seconde pour éviter que le précédent bouton soit comptabilisé
  delay(500);
  Chiffre = 0;
  //Saisie du quatrième chiffre du code
  while (Chiffre != 1){
     lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S1:{
            Code_4 = 49;
            Chiffre = 1;
            lcd.setCursor(15,0);
            lcd.print("1");
            break;
       }
       case S2:{
             Code_4 = 50;
             Chiffre = 1;
             lcd.setCursor(15,0);
             lcd.print("2");
             break;
       }    
       case S3:{
             Code_4 = 51;
             Chiffre = 1;
             lcd.setCursor(15,0);
             lcd.print("3");
             break;
       }
       case S4:{
             Code_4 = 52;
             Chiffre = 1;
             lcd.setCursor(15,0);
             lcd.print("4");
             break;
       }
       case S5:{
             Code_4 = 53;
             Chiffre = 1;
             lcd.setCursor(15,0);
             lcd.print("5");
             break;
       }
}
}
  Chiffre = 0;
  delay(500);
  //On vérifie si le code saisi est valide
  if (Code_1 == C1 && Code_2 == C2 && Code_3 == C3 && Code_4 == C4){
      //Le code est valide, on affiche le menu
      lcd.clear();
      MenuPage1();
    }
else {  
      //Le code est invalide, on affiche l'écran principal
      lcd.clear();
      
      //On réinitialise les variables utilisées
      Code_1 = 0;
      Code_2 = 0;
      Code_3 = 0;
      Code_4 = 0;
      Menu = 0;
      
      //On avertit l'utilisateur que le code est incorrect
      lcd.setCursor(0,0);
      lcd.print("Code Incorrect");
      delay(2000);
      lcd.clear();
      loop();
}
}
}

//Première page du menu
void MenuPage1(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Menu 1/5");
  lcd.setCursor(0,1);
  lcd.print("Reg. 1e prise");
  delay(500);
  while (1){
  
  //Navigation entre les pages du menu
  lcd_key = read_LCD_buttons();
     switch (lcd_key){
       case S1:{
            MenuPage5();
            break;
       }
       case S4:{
            ChgtHeureP1();
            break;
       }
       case S3:{
            MenuPage2();
            break;
       }
    }
  }
  MenuPage1();
     }

//Deuxième page du menu
void MenuPage2(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Menu 2/5");
  lcd.setCursor(0,1);
  lcd.print("Reg. 2e prise");
  delay(500);
  int Test = 0;
  while (Test ==0){
    
  //Navigation entre les pages du menu
  lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S1:{
            MenuPage1();
            break;
       }
       case S4:{
            ChgtHeureP2();
            break;
       }
       case S3:{
            MenuPage3();
            break;
       }
    }
  }
  MenuPage2();
}

//Troisième page du menu
void MenuPage3(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Menu 3/5");
  lcd.setCursor(0,1);
  lcd.print("Reg. 3e prise");
  delay(500);
  int Test = 0;
  while (Test ==0){
  lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S1:{
            MenuPage2();
            break;
       }
       case S4:{
            ChgtHeureP3();
            break;
       }
       case S3:{
            MenuPage4();
            break;
       }
    }
  }
  MenuPage3();
}

//Cinquième page du menu
void MenuPage4(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Menu 4/5");
  lcd.setCursor(0,1);
  lcd.print("Reglage Heure");
  delay(500);
  int Test = 0;
  while (Test ==0){
  lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S1:{
            MenuPage3();
            break;
       }
       case S3:{
            MenuPage5();
            break;
       }
       case S4:{
            ReglageHeure();
            break;
       }
    }
  }
  MenuPage4();
}

//Septième page du menu
void MenuPage5(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Menu 5/5");
  lcd.setCursor(0,1);
  lcd.print("Quitter");
  delay(500);
  int Test = 0;
  while (Test ==0){
  lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S1:{
            MenuPage4();
            break;
       }
       case S4:{
            Quitter();
            break;
       }
       case S3:{
            MenuPage1();
            break;
       }
    }
  }
  MenuPage5();
}

/*
Fonction de changement de la date
*/
void ReglageHeure(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reglage Heures");
  lcd.setCursor(2,1);
  lcd.print(":");
  lcd.setCursor(0,1);
  lcd.print(Plus);
  delay(200);
  lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S5:{
            Plus++;
            break;
       }
       case S4:{ 
            HeureChgt = Plus;
            Plus = 0;
            ReglageMinute();
            break;
       }
       case S2:{
            Plus--;
            break;
       }
}
  if (Plus == 24){
   Plus = 0; 
  }
  else if (Plus == -1){
   Plus = 23; 
  }
  ReglageHeure();
}

void ReglageMinute(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reglage Minutes");
  lcd.setCursor(0,1);
  lcd.print(HeureChgt);
  lcd.setCursor(2,1);
  lcd.print(":");
  lcd.setCursor(3,1);
  lcd.print(Plus);
  delay(200);
  lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S5:{
            Plus++;
            break;
       }
       case S4:{
            MinuteChgt = Plus;
            Plus = 0;
            InscriptionDateHeure();
            break;
       }
       case S2:{
            Plus--;
            break;
       }
}
  if (Plus == 60){
   Plus = 0; 
  }
  else if (Plus == -1){
   Plus = 60; 
  }
  ReglageMinute();
}

void InscriptionDateHeure(){
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  second = 00;
  minute = MinuteChgt;
  hour = HeureChgt;
  setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
  Quitter();
  }
  
void ChgtHeureP1(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reglage Prise 1");
  lcd.setCursor(2,1);
  lcd.print(":00");
  lcd.setCursor(0,1);
  lcd.print(Plus);
  delay(200);
  lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S5:{
            Plus++;
            break;
       }
       case S4:{ 
            HeureChgt = Plus;
            Plus = 0;
            Wire.begin(); // initialise the connection
            Serial.begin(9600);
            i2c_eeprom_write_byte(0x50, 2, byte(HeureChgt)); // write to EEPROM 
            Quitter();
            break;
       }
       case S2:{
            Plus--;
            break;
       }
}
  if (Plus == 24){
   Plus = 0; 
  }
  else if (Plus == -1){
   Plus = 23; 
  }
  ChgtHeureP1();
  }
  
void ChgtHeureP2(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reglage Prise 2");
  lcd.setCursor(2,1);
  lcd.print(":00");
  lcd.setCursor(0,1);
  lcd.print(Plus);
  delay(200);
  lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S5:{
            Plus++;
            break;
       }
       case S4:{ 
            HeureChgt = Plus;
            Plus = 0;
            Wire.begin(); // initialise the connection
            Serial.begin(9600);
            i2c_eeprom_write_byte(0x50, 4, byte(HeureChgt)); // write to EEPROM 
            Quitter();
            break;
       }
       case S2:{
            Plus--;
            break;
       }
}
  if (Plus == 24){
   Plus = 0; 
  }
  else if (Plus == -1){
   Plus = 23; 
  }
  ChgtHeureP2();
  }
  
void ChgtHeureP3(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reglage Prise 3");
  lcd.setCursor(2,1);
  lcd.print(":00");
  lcd.setCursor(0,1);
  lcd.print(Plus);
  delay(200);
  lcd_key = read_LCD_buttons();
     switch (lcd_key){               // depending on which button was pushed, we perform an action
       case S5:{
            Plus++;
            break;
       }
       case S4:{ 
            HeureChgt = Plus;
            Plus = 0;
            Wire.begin(); // initialise the connection
            Serial.begin(9600);
            i2c_eeprom_write_byte(0x50, 6, byte(HeureChgt)); // write to EEPROM 
            Quitter();
            break;
       }
       case S2:{
            Plus--;
            break;
       }
}
  if (Plus == 24){
   Plus = 0; 
  }
  else if (Plus == -1){
   Plus = 23; 
  }
  ChgtHeureP3();
  }  
  
//Fonction de déconnexion
void Quitter(){
  lcd.clear();
  while (Code_1 == C1 && Code_2 == C2 && Code_3 == C3 && Code_4 == C4){
  Code_1 = 48;
  Code_2 = 48;
  Code_3 = 48;
  Code_4 = 48;
  lcd.setCursor(0,0);
  lcd.print("Deconnexion...");
  delay(2000);
  asm volatile ("  jmp 0"); 
  }
  lcd.clear();
  Menu = 0;
  loop();
}

void PrisePilules(){
     delay(2000);
     byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
     int Fourche = LOW;
     digitalWrite(moteur,1);
     do {
       Fourche = digitalRead(fourchePin);
       lcd.setCursor(0,0);
       lcd.print("Test");
     }
     while(Fourche != HIGH);
     digitalWrite(moteur,0);
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("123");
     delay(5000);
     while(Heure == hour){
     //On ouvre la trappe avec le servomoteur
     val = map(125, 0, 1023, 2, 128);
     myservo.write(val);
     
     //On affiche un message sur l'écran
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Vos pilules sont");
     lcd.setCursor(0,1);
     lcd.print("disponibles");
     
     //On fait sonner le buzzer
     if(1){
     digitalWrite(LED,LOW);
     digitalWrite(buzzer,HIGH);
     delay(1000);
     digitalWrite(LED,HIGH);
     digitalWrite(buzzer,LOW);
     delay(1000);  
     }
     lcd.clear();
     getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
     }
  }

//Fonction d'initialisation
void setup(){
  //Initialisation de l'écran LCD
  lcd.clear();
  lcd.begin(16, 2);
  
  //Lecture de la date et de l'heure
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  Wire.begin();
  Serial.begin(9600);
  
  int addr=0; //first address
  byte HP1 = i2c_eeprom_read_byte(0x50, 2);
  byte HP2 = i2c_eeprom_read_byte(0x50, 4);
  byte HP3 = i2c_eeprom_read_byte(0x50, 6);
  HeureP1 = HP1;
  HeureP2 = HP2;
  HeureP3 = HP3;
  
  myservo.attach(13);
  
  second = 00;
  minute = 59;
  hour = 16;
  dayOfWeek = 1;
  dayOfMonth = 1;
  month = 6;
  year = 15;
  //Si on souhaite changer l'heure manuellement, décommenter la ligne
  //setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
  
  //Définition des bornes utilisées : entrée (INPUT) ou sortie (OUTPUT)
  pinMode(moteur, OUTPUT);
  pinMode(buzzer,OUTPUT);
  pinMode(LED,OUTPUT);
  pinMode(fourchePin, INPUT);
  digitalWrite(LED, HIGH);
}

//Fonction principale
void loop(){
   //On récupère la date et l'heure de l'horloge temps-réel
   byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
   getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
   
   //Affichage de la date et de l'heure
   if (dayOfMonth <= 0x09)
   {
     
   lcd.setCursor(4,0);
   lcd.print("0");
   lcd.setCursor(5,0);
   lcd.print(dayOfMonth);
   }
   else
   {
     lcd.setCursor(4,0);
     lcd.print(dayOfMonth);
   }
   if (hour <= 0x09)
   {
     
   lcd.setCursor(4,1);             // move to the begining of the second line
   lcd.print("0");
   lcd.setCursor(5,1);             // move to the begining of the second line
   lcd.print(hour);
   }
   else
   {
     lcd.setCursor(4,1);             // move to the begining of the second line
     lcd.print(hour);
   }
   if (second <= 0x09)
   {
     
   lcd.setCursor(10,1);             // move to the begining of the second line
   lcd.print("0");
   lcd.setCursor(11,1);             // move to the begining of the second line
   lcd.print(second);
   }
   else
   {
     lcd.setCursor(10,1);             // move to the begining of the second line
     lcd.print(second);
   }
   if (minute <= 0x09)
   {
     
   lcd.setCursor(7,1);             // move to the begining of the second line
   lcd.print("0");
   lcd.setCursor(8,1);             // move to the begining of the second line
   lcd.print(minute);
   }
   else
   {
     lcd.setCursor(7,1);             // move to the begining of the second line
     lcd.print(minute);
   }
   lcd.setCursor(6,1);             // move to the begining of the second line
   lcd.print(":");
   lcd.setCursor(9,1);             // move to the begining of the second line
   lcd.print(":");
   
   if (dayOfWeek == 1)
   {
     lcd.setCursor(0,0);
     lcd.print("Lun");
   }
   else if (dayOfWeek == 2)
   {
     lcd.setCursor(0,0);
     lcd.print("Mar");
   }
   else if (dayOfWeek == 3)
   {
     lcd.setCursor(0,0);
     lcd.print("Mer");
   }
   else if (dayOfWeek == 4)
   {
     lcd.setCursor(0,0);
     lcd.print("Jeu");
   }
   else if (dayOfWeek == 5)
   {
     lcd.setCursor(0,0);
     lcd.print("Ven");
   }
   else if (dayOfWeek == 6)
   {
     lcd.setCursor(0,0);
     lcd.print("Sam");
   }
   else if (dayOfWeek == 7)
   {
     lcd.setCursor(0,0);
     lcd.print("Dim");
   }
   if (month == 0x01)
   {
     lcd.setCursor(7,0);
     lcd.print("Jan.");
   }
   else if (month == 0x02)
   {
     lcd.setCursor(7,0);
     lcd.print("Fev.");
   }
   else if (month == 0x03)
   {
     lcd.setCursor(7,0);
     lcd.print("Mars");
   }
   else if (month == 0x04)
   {
     lcd.setCursor(7,0);
     lcd.print("Avr");
   }
   else if (month == 0x05)
   {
     lcd.setCursor(7,0);
     lcd.print("Mai");
   }
   else if (month == 0x06)
   {
     lcd.setCursor(7,0);
     lcd.print("Juin");
   }
   else if (month == 0x07)
   {
     lcd.setCursor(7,0);
     lcd.print("Juil");
   }
   else if (month == 0x08)
   {
     lcd.setCursor(7,0);
     lcd.print("Aout");
   }
   else if (month == 0x09)
   {
     lcd.setCursor(7,0);
     lcd.print("Sept");
   }
   else if (month == 0x0A)
   {
     lcd.setCursor(7,0);
     lcd.print("Oct.");
   }
   else if (month == 0x0B)
   {
     lcd.setCursor(7,0);
     lcd.print("Nov.");
   }
   else if (month == 0x0C)
   {
     lcd.setCursor(7,0);
     lcd.print("Dec.");
   }
   lcd.setCursor(12,0);
   lcd.print("20");
   lcd.setCursor(14,0);
   lcd.print(year);
   
   //Si un bouton est appuyé, on affiche l'écran de connexion
   lcd_key = read_LCD_buttons();
   lcd.setCursor(0,0);
   switch (lcd_key){
       case S1:{
            Menu++;
            Connexion();
            break;
       }
       case S2:{
             Menu++;
             Connexion();
             break;
       }    
       case S3:{
             Menu++;
             Connexion();
             break;
       }
       case S4:{
             Menu++;
             Connexion();
             break;
       }
       case S5:{
             Menu++;
             Connexion();
             break;
       }
   }
   
   //Première prise :
   if (hour == HeureP1) {
     delay(2000);
     byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
     int Fourche = LOW;
     digitalWrite(moteur,1);
     delay(200);
       Fourche = digitalRead(fourchePin);
     while(Fourche != HIGH){
       Fourche = digitalRead(fourchePin);
       }
     digitalWrite(moteur,0);
     //On ouvre la trappe avec le servomoteur
     val = map(125, 0, 1023, 2, 128);
     myservo.write(val);
     
     //On affiche un message sur l'écran
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Vos pilules sont");
     lcd.setCursor(0,1);
     lcd.print("disponibles");
     
     //On fait sonner le buzzer
     if(1){
     digitalWrite(LED,LOW);
     digitalWrite(buzzer,HIGH);
     delay(1000);
     digitalWrite(LED,HIGH);
     digitalWrite(buzzer,LOW);
     delay(1000);  
     }
     lcd.clear();
     getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
     while(HeureP1 == hour){
        //On affiche un message sur l'écran
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Vos pilules sont");
     lcd.setCursor(0,1);
     lcd.print("disponibles");
     
     //On fait sonner le buzzer
     if(1){
     digitalWrite(LED,LOW);
     digitalWrite(buzzer,HIGH);
     delay(1000);
     digitalWrite(LED,HIGH);
     digitalWrite(buzzer,LOW);
     delay(1000);  
     }
     lcd.clear();
     getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
     lcd_key = read_LCD_buttons();
   switch (lcd_key){
       case S1:{
            Menu++;
            lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
              hour = 9;
              //Si on souhaite changer l'heure manuellement, décommenter la ligne
              setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
            while(1){
              }
            break;
       }
       case S2:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
              hour = 9;
              //Si on souhaite changer l'heure manuellement, décommenter la ligne
              setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
            while(1){
              }
             break;
       }    
       case S3:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
              hour = 9;
              //Si on souhaite changer l'heure manuellement, décommenter la ligne
              setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
            while(1){
              }
             break;
       }
       case S4:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
              hour = 9;
              //Si on souhaite changer l'heure manuellement, décommenter la ligne
              setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
            while(1){
              }
             break;
       }
       case S5:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
              hour = 9;
              //Si on souhaite changer l'heure manuellement, décommenter la ligne
              setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
            while(1){
              }
             break;
       }
   }
     }
   }
   
   if (hour == HeureP2) {
    delay(2000);
     byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
     int Fourche = LOW;
     digitalWrite(moteur,1);
     delay(200);
       Fourche = digitalRead(fourchePin);
     while(Fourche != HIGH){
       Fourche = digitalRead(fourchePin);
       }
     digitalWrite(moteur,0);
     //On ouvre la trappe avec le servomoteur
     val = map(125, 0, 1023, 2, 128);
     myservo.write(val);
     
     //On affiche un message sur l'écran
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Vos pilules sont");
     lcd.setCursor(0,1);
     lcd.print("disponibles");
     
     //On fait sonner le buzzer
     if(1){
     digitalWrite(LED,LOW);
     digitalWrite(buzzer,HIGH);
     delay(1000);
     digitalWrite(LED,HIGH);
     digitalWrite(buzzer,LOW);
     delay(1000);  
     }
     lcd.clear();
     getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
     while(HeureP2 == hour){
        //On affiche un message sur l'écran
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Vos pilules sont");
     lcd.setCursor(0,1);
     lcd.print("disponibles");
     
     //On fait sonner le buzzer
     if(1){
     digitalWrite(LED,LOW);
     digitalWrite(buzzer,HIGH);
     delay(1000);
     digitalWrite(LED,HIGH);
     digitalWrite(buzzer,LOW);
     delay(1000);  
     }
     lcd.clear();
     getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
     lcd_key = read_LCD_buttons();
   switch (lcd_key){
       case S1:{
            Menu++;
            lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
              hour = 18;
              //Si on souhaite changer l'heure manuellement, décommenter la ligne
              setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
            while(1){
              }
            break;
       }
       case S2:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
              hour = 18;
              //Si on souhaite changer l'heure manuellement, décommenter la ligne
              setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
            while(1){
              }
             break;
       }    
       case S3:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
              hour = 18;
              //Si on souhaite changer l'heure manuellement, décommenter la ligne
              setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
            while(1){
              }
             break;
       }
       case S4:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
            while(1){
              }
             break;
       }
       case S5:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
            while(1){
              }
             break;
       }
   }
     }
   }
   if (hour == HeureP3) {
     delay(2000);
     byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
     int Fourche = LOW;
     digitalWrite(moteur,1);
     delay(200);
       Fourche = digitalRead(fourchePin);
     while(Fourche != HIGH){
       Fourche = digitalRead(fourchePin);
       }
     digitalWrite(moteur,0);
     //On ouvre la trappe avec le servomoteur
     val = map(125, 0, 1023, 2, 128);
     myservo.write(val);
     
     //On affiche un message sur l'écran
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Vos pilules sont");
     lcd.setCursor(0,1);
     lcd.print("disponibles");
     
     //On fait sonner le buzzer
     if(1){
     digitalWrite(LED,LOW);
     digitalWrite(buzzer,HIGH);
     delay(1000);
     digitalWrite(LED,HIGH);
     digitalWrite(buzzer,LOW);
     delay(1000);  
     }
     lcd.clear();
     getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
     while(HeureP3 == hour){
        //On affiche un message sur l'écran
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Vos pilules sont");
     lcd.setCursor(0,1);
     lcd.print("disponibles");
     
     //On fait sonner le buzzer
     if(1){
     digitalWrite(LED,LOW);
     digitalWrite(buzzer,HIGH);
     delay(1000);
     digitalWrite(LED,HIGH);
     digitalWrite(buzzer,LOW);
     delay(1000);  
     }
     lcd.clear();
     getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
     lcd_key = read_LCD_buttons();
   switch (lcd_key){
       case S1:{
            Menu++;
            lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
              
            while(1){
              }
            break;
       }
       case S2:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
            while(1){
              }
             break;
       }    
       case S3:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
            while(1){
              }
             break;
       }
       case S4:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
            while(1){
              }
             break;
       }
       case S5:{
             Menu++;
             lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("     Alarme     ");
              lcd.setCursor(0,1);
              lcd.print("   desactivee   ");
            while(1){
              }
             break;
       }
   }
     }
   }
   //On ferme et verrouille la trappe en dehors des heures programmées
   val = 125;
   val = map(val, 0, 1023, 2, 1125);
   myservo.write(val);
}

