#include "EmonLib/EmonLib.h"

#include "application.h"



#include "LiquidCrystal/LiquidCrystal.h"



//local device ID, characters 1 and 2 are the type of device, the remaining characters are the
//unique device identifyer
#define LdeviceID  "110001"

SYSTEM_MODE(SEMI_AUTOMATIC);


//00 wall switch
//01 wall outlet
//10 door lock
//11 hvac controller

LiquidCrystal lcd(D0, D1, D2, D3, D4, D5);

TCPClient client;



//switch auth statement
const unsigned char authenticatorS[] = "000001 authenticate access_token a5f767ec5d217ad4174d778c0b1948c790c4513c";
//outlent auth statement
const unsigned char authenticatorW[] = "010001 authenticate access_token 784c243a8b57a3c2224e37c9f455b480946764e2";
//powerlock auth statement
const unsigned char authenticatorL[] = "100001 authenticate access_token 5bda474342f50fdb7f015f56264e734b9b507762";
//HVAC controller auth statement
const unsigned char authenticatorH[] = "110001 authenticate access_token cdaf750fb93559119cc5bd7777ad311b5a9a8280";



EnergyMonitor emon1;
const int voltage = 120;


static unsigned long secondInterval = 1000;
static unsigned long minuteInterval = 57000;
static unsigned long prevMinute = 0;
static unsigned long prevSecond = 0;
unsigned long now;
double average = 0.0;
double sum = 0.0;
int count = 0;
int analogPin0 = A5;


int interrupttime = 0;
int temperatureTime = 0;
int sentTime = 0 ;
int sumT = 0;
int countT = 0;


//int EnRelay = A6;
//int FanRelay = A4;



//AC relay
int ACRelay = D6;

//heat relay
int HRelay = D7;




int Tmonitor = A0;
//int LED = D7;



//red A1, blud A3, yellow A2
//----
int HCbutton = A2; //yellow
int TempUpbutton = A1; //red
int TempDwnbutton = A3; //blue
//----

int Updated = 0;

//current settings for the HVAC controller
//order of settings is: enable, temperature, mode, fan
//0: enable(on off for entire device): 0 = off, 1 = on
//1: temperature: current tempurature setting in degrees farenheit
//2: mode: 0 = cool, 1 = heat
//3: fan: 0 = off, 1 = on, 2 = auto
int Settings[] = {0, 75, 0, 0};


int resetTime = 0;

//no = 0, yes = 1
int isAuthenticated = 0;

int newAuth = 0;

bool isConnecting = FALSE;

int TemperatureC = 0;
int TemperatureF = 0;



void onMessage(char* message) {

    //Serial.println("inmessage: ");
    //Serial.println(message);

    int i = 0;

    for(i = 0; i < 6; i++)
    {
        if(message[i] != LdeviceID[i])
        {
            //error incorrect device id
            return;
        }
    }

    if(message[6] != ' ')
    {
        //error, device id too long
        return;
    }


    if(message[7] == 'a')
    {
        //authentication received
        return;
    }

    //keepalive pong
    else if((message[7] == 'k') && (message[15] == 'e') && (message[16] == ' ') && (message[17] == 'p') && (message[35] == '\n'))
    {
        message[18] = 'o';
        message[19] = 'n';
        message[20] = 'g';


        message[35] = '\n';
        message[36] = '\0';

        client.print(message);
        //Serial.print("response:\n");
        //Serial.println(message);
        return;
    }

    //set
    else if((message[7] == 's') && (message[8] == 'e') && (message[9] == 't') && (message[10] == ' '))
    {

        //enaled
        if((message[11] == 'e') && (message[12] == 'n') && (message[13] == 'a') && (message[14] == 'b') && (message[15] == 'l') &&
                (message[16] == 'e') && (message[17] == 'd') && (message[18] == ' '))
        {

            //false
            if((message[19] == 'f') && (message[20] == 'a') && (message[21] == 'l') && (message[22] == 's') && (message[23] == 'e') && (message[24] == '\n'))
            {
                //digitalWrite(EnRelay,HIGH);
                //digitalWrite(LED,LOW);

                Settings[0] = 0;
                return;
            }

            //true
            else if((message[19] == 't') && (message[20] == 'r') && (message[21] == 'u') && (message[22] == 'e') && (message[23] == '\n'))
            {
                //digitalWrite(EnRelay,LOW);
                //digitalWrite(LED,HIGH);

                Settings[0] = 1;
                return;
            }

        }

        //fan
        else if((message[11] == 'f') && (message[12]== 'a') && (message[13] == 'n') && (message[14] == ' '))
        {

            //auto
            if((message[15] == 'a') && (message[16] == 'u') && (message[17] == 't') && (message[18] == 'o') && (message[19] == '\n'))
            {
                Settings[3] = 2;
                return;
            }

            //on
            else if((message[15] == 'o') && (message[16] == 'n') && (message[17] == '\n'))
            {
                Settings[3] = 1;
                return;
            }

            //off
            else if((message[15] == 'o') && (message[16] == 'f') && (message[17] == 'f') && (message[18] == '\n'))
            {
                Settings[3] = 0;
                return;
            }


        }

        //mode
        else if((message[11] == 'm') && (message[12] == 'o') && (message[13] == 'd') && (message[14] == 'e') && (message[15] == ' '))
        {

            //cool
            if((message[16] == 'c') && (message[17] == 'o') && (message[18] == 'o') && (message[19] == 'l') && (message[20] == '\n'))
            {
                if((Updated == 0) /*&& (millis() - interrupttime > 1000)*/)
                {
                    Settings[2] = 0;
                }
                return;
            }

            //heat
            else if((message[16] == 'h') && (message[17] == 'e') && (message[18] == 'a') && (message[19] == 't') && (message[20] == '\n'))
            {
                if((Updated == 0) /*&& (millis() - interrupttime > 1000)*/)
                {
                    Settings[2] = 1;
                }
                return;
            }

        }


        //tempDesired
        else if((message[11] == 't') && (message[12] == 'e') && (message[13] == 'm') && (message[14] == 'p') && (message[15] == 'D') &&
                (message[16] == 'e') && (message[17] == 's') && (message[18] == 'i') && (message[19] == 'r') && (message[20] == 'e') &&
                (message[21] == 'd') && (message[22] == ' '))
        {
            if((Updated == 0) /*&& (millis() - interrupttime > 500)*/)
            {
                Settings[1] = message[23] - 48;
                Settings[1] *= 10;
                Settings[1] += (message[24] - 48);
            }
            return;
        }

        //tempCurrent
        else if((message[11] == 't') && (message[15] == 'C') && (message[22] == ' '))
        {
            //parse int
            return;
        }




    }
    //error: invalid input
    Serial.println("error: invalid params");
    return;
}



void sendData(char *action, char *state_Name, int state_Type){


    char Lmessage[1024];
    char state_Value[50];
    int i = 0;
    int j = 0;

    //state_Type -101 is for the current enable state of the device
    if(state_Type == -101)
    {
        //determine value of enable state
        if(Settings[0] == 0)
        {
            state_Value[0] = 'F';
            state_Value[1] = 'A';
            state_Value[2] = 'L';
            state_Value[3] = 'S';
            state_Value[4] = 'e';
            state_Value[5] = '\0';
        }
        else
        {
            state_Value[0] = 'T';
            state_Value[1] = 'R';
            state_Value[2] = 'U';
            state_Value[3] = 'E';
            state_Value[4] = '\0';
        }
    }

    //state_Type -102 is for the mode: heat or cool
    else if(state_Type == -102)
    {
        //determine value of mode state
        if(Settings[2] == 0)
        {
            state_Value[0] = 'c';
            state_Value[1] = 'o';
            state_Value[2] = 'o';
            state_Value[3] = 'l';
            state_Value[4] = '\0';
        }
        else
        {
            state_Value[0] = 'h';
            state_Value[1] = 'e';
            state_Value[2] = 'a';
            state_Value[3] = 't';
            state_Value[4] = '\0';
        }
    }

    //state_type -103 is for the fan state
    else if(state_Type == -103)
    {
        //determine value of mode state
        if(Settings[3] == 0)
        {
            state_Value[0] = 'o';
            state_Value[1] = 'f';
            state_Value[2] = 'f';
            state_Value[3] = '\0';
        }
        else if(Settings[3] == 1)
        {
            state_Value[0] = 'o';
            state_Value[1] = 'n';
            state_Value[2] = '\0';
        }
        else
        {
            state_Value[0] = 'a';
            state_Value[1] = 'u';
            state_Value[2] = 't';
            state_Value[3] = 'o';
            state_Value[4] = '\0';
        }
    }

    //state_type -104 is for the currently set temperature
     else if(state_Type == -104)
    {
        switch(Settings[1] / 10)
        {
            case 0: state_Value[0] = '0';
                    break;
            case 1: state_Value[0] = '1';
                    break;
            case 2: state_Value[0] = '2';
                    break;
            case 3: state_Value[0] = '3';
                    break;
            case 4: state_Value[0] = '4';
                    break;
            case 5: state_Value[0] = '5';
                    break;
            case 6: state_Value[0] = '6';
                    break;
            case 7: state_Value[0] = '7';
                    break;
            case 8: state_Value[0] = '8';
                    break;
            case 9: state_Value[0] = '9';
                    break;
        }

        switch(Settings[1] % 10)
        {
            case 0: state_Value[1] = '0';
                    break;
            case 1: state_Value[1] = '1';
                    break;
            case 2: state_Value[1] = '2';
                    break;
            case 3: state_Value[1] = '3';
                    break;
            case 4: state_Value[1] = '4';
                    break;
            case 5: state_Value[1] = '5';
                    break;
            case 6: state_Value[1] = '6';
                    break;
            case 7: state_Value[1] = '7';
                    break;
            case 8: state_Value[1] = '8';
                    break;
            case 9: state_Value[1] = '9';
                    break;
        }

        state_Value[2] = '\0';
    }


    //a non-negative state_type is the current temperature
    else if(state_Type > 0)
    {
        switch(state_Type / 10)
        {
            case 0: state_Value[0] = '0';
                    break;
            case 1: state_Value[0] = '1';
                    break;
            case 2: state_Value[0] = '2';
                    break;
            case 3: state_Value[0] = '3';
                    break;
            case 4: state_Value[0] = '4';
                    break;
            case 5: state_Value[0] = '5';
                    break;
            case 6: state_Value[0] = '6';
                    break;
            case 7: state_Value[0] = '7';
                    break;
            case 8: state_Value[0] = '8';
                    break;
            case 9: state_Value[0] = '9';
                    break;
        }

        switch(state_Type % 10)
        {
            case 0: state_Value[1] = '0';
                    break;
            case 1: state_Value[1] = '1';
                    break;
            case 2: state_Value[1] = '2';
                    break;
            case 3: state_Value[1] = '3';
                    break;
            case 4: state_Value[1] = '4';
                    break;
            case 5: state_Value[1] = '5';
                    break;
            case 6: state_Value[1] = '6';
                    break;
            case 7: state_Value[1] = '7';
                    break;
            case 8: state_Value[1] = '8';
                    break;
            case 9: state_Value[1] = '9';
                    break;
        }

        state_Value[2] = '\0';
    }



    //build message to send to server
    for(i = 0; i < 6; i++)
    {
        Lmessage[i] = LdeviceID[i];
    }

    Lmessage[i] = ' ';
    i++;

    while(action[j] != '\0')
    {
        Lmessage[i] = action[j];
        i++;
        j++;
    }

    Lmessage[i] = ' ';
    i++;
    j = 0;

    while(state_Name[j] != '\0')
    {
        Lmessage[i] = state_Name[j];
        i++;
        j++;
    }

    Lmessage[i] = ' ';
    i++;
    j = 0;

    while((state_Value[j] != '\0') && (state_Value[j] >= '.') && (state_Value[j] <= '9'))
    {
        Lmessage[i] = state_Value[j];
        i++;
        j++;
    }

    Lmessage[i] = '\n';
    Lmessage[i+1] = '\0';


//    Serial.print(Lmessage);

    client.print(Lmessage);

}



void dataSend(char *action, char *state_Name, double state_Type){


    char Lmessage[1024];
    char state_Value[50];
    int i = 0;
    int j = 0;

    //power monitor data
    if(state_Type >= 0)
    {
        char temp_Value[10];
        int temp_Type = state_Type * 100;

        i = 0;
        while(temp_Type / 1 > 0)
        {

            switch(temp_Type % 10)
            {
                case 0: temp_Value[i] = '0';
                        break;
                case 1: temp_Value[i] = '1';
                        break;
                case 2: temp_Value[i] = '2';
                        break;
                case 3: temp_Value[i] = '3';
                        break;
                case 4: temp_Value[i] = '4';
                        break;
                case 5: temp_Value[i] = '5';
                        break;
                case 6: temp_Value[i] = '6';
                        break;
                case 7: temp_Value[i] = '7';
                        break;
                case 8: temp_Value[i] = '8';
                        break;
                case 9: temp_Value[i] = '9';
                        break;
            }

            temp_Type = temp_Type / 10;
            i++;
        }


        j = 0;
        i--;

        if(i > 1)
        {
            while(i >= -1)
            {
                if(i == 1)
                {
                    state_Value[j] = '.';
                    j++;
                }

                state_Value[j] = temp_Value[i];
                j++;
                i--;
            }
        }
        else
        {
            state_Value[0] = '0';
            state_Value[1] = '.';

            if(i == 1)
            {
                state_Value[2] = temp_Value[1];
            }
            else
            {
                state_Value[2] = '0';
            }

            state_Value[3] = temp_Value[0];

        }

    }



    //build message to send to server
    for(i = 0; i < 6; i++)
    {
        Lmessage[i] = LdeviceID[i];
    }

    Lmessage[i] = ' ';
    i++;
    j = 0;

    while(action[j] != '\0')
    {
        Lmessage[i] = action[j];
        i++;
        j++;
    }

    Lmessage[i] = ' ';
    i++;
    j = 0;


    while(state_Name[j] != '\0')
    {
        Lmessage[i] = state_Name[j];
        i++;
        j++;
    }

    Lmessage[i] = ' ';
    i++;
    j = 0;

    while((state_Value[j] != '\0') && (state_Value[j] >= '.') && (state_Value[j] <= '9'))
    {
        Lmessage[i] = state_Value[j];
        i++;
        j++;
    }

    Lmessage[i] = '\n';
    Lmessage[i+1] = '\0';

    client.print(Lmessage);
//    Serial.print("sent: ");
//    Serial.println(Lmessage);
}




void allDataSend ()
{
    char Lmessage[50];
    int i = 0;


    //deviceID set mode cool/heat
    for(i = 0; i < 6; i++)
    {
        Lmessage[i] = LdeviceID[i];
    }

    Lmessage[i] = ' ';
    i++;

    Lmessage[7] = 's';
    Lmessage[8] = 'e';
    Lmessage[9] = 't';

    Lmessage[10] = ' ';


    Lmessage[11] = 'm';
    Lmessage[12] = 'o';
    Lmessage[13] = 'd';
    Lmessage[14] = 'e';

    Lmessage[15] = ' ';


    if(Settings[2] == 0)
    {
        Lmessage[16] = 'c';
        Lmessage[17] = 'o';
        Lmessage[18] = 'o';
        Lmessage[19] = 'l';
    }
    else if(Settings[2] == 1)
    {
        Lmessage[16] = 'h';
        Lmessage[17] = 'e';
        Lmessage[18] = 'a';
        Lmessage[19] = 't';
    }

    Lmessage[20] = '\n';
    Lmessage[21] = '\0';

    client.print(Lmessage);


    //deviceID set tempDesired ##
    Lmessage[7] = 's';
    Lmessage[8] = 'e';
    Lmessage[9] = 't';

    Lmessage[10] = ' ';


    Lmessage[11] = 't';
    Lmessage[12] = 'e';
    Lmessage[13] = 'm';
    Lmessage[14] = 'p';
    Lmessage[15] = 'D';
    Lmessage[16] = 'e';
    Lmessage[17] = 's';
    Lmessage[18] = 'i';
    Lmessage[19] = 'r';
    Lmessage[20] = 'e';
    Lmessage[21] = 'd';

    Lmessage[22] = ' ';


    switch(Settings[1] / 10)
    {
        case 0: Lmessage[23] = '0';
                break;
        case 1: Lmessage[23] = '1';
                break;
        case 2: Lmessage[23] = '2';
                break;
        case 3: Lmessage[23] = '3';
                break;
        case 4: Lmessage[23] = '4';
                break;
        case 5: Lmessage[23] = '5';
                break;
        case 6: Lmessage[23] = '6';
                break;
        case 7: Lmessage[23] = '7';
                break;
        case 8: Lmessage[23] = '8';
                break;
        case 9: Lmessage[23] = '9';
                break;
    }

    switch(Settings[1] % 10)
    {
        case 0: Lmessage[24] = '0';
                break;
        case 1: Lmessage[24] = '1';
                break;
        case 2: Lmessage[24] = '2';
                break;
        case 3: Lmessage[24] = '3';
                break;
        case 4: Lmessage[24] = '4';
                break;
        case 5: Lmessage[24] = '5';
                break;
        case 6: Lmessage[24] = '6';
                break;
        case 7: Lmessage[24] = '7';
                break;
        case 8: Lmessage[24] = '8';
                break;
        case 9: Lmessage[24] = '9';
                break;
    }

    Lmessage[25] = '\n';
    Lmessage[26] = '\0';

    client.print(Lmessage);

}




void HCchange ()
{
    if(millis() - interrupttime > 500)
    {
        Updated = 1;

        if(Settings[2] == 0)
        {
            Settings[2] = 1;
        }
        else if(Settings[2] == 1)
        {
            Settings[2] = 0;
        }

        interrupttime = millis();
    }
}

void TemperatureUp ()
{
    if(millis() - interrupttime > 300)
    {
        Updated = 1;

        Settings[1] += 1;

        interrupttime = millis();

    }

}

void TemperatureDown ()
{
    if(millis() - interrupttime > 300)
    {
        Updated = 1;

        Settings[1] -= 1;

        interrupttime = millis();

    }
}



void connect()
{

    if(isConnecting)
    {
        return;
    }
    isAuthenticated = 0;
    isConnecting = TRUE;
    client.connect("knighthome.xyz",3001);

    delay(1000);



}




void reconnect()
{
    Serial.println("reconnecting---");

    client.stop();
    isConnecting = FALSE;
    delay(300);
    connect();
    resetTime = millis();
}







void setup() {

    WiFi.setCredentials("Tornado_guest","fourwordsalluppercase",WPA2);

    Particle.connect();

    while(!Particle.connected)
    {
        delay(1);
    }

//setup for the interrupts for button presses
//uncomment when butons are added
    pinMode(HCbutton,INPUT_PULLUP);
    pinMode(TempUpbutton,INPUT_PULLUP);
    pinMode(TempDwnbutton,INPUT_PULLUP);


    //attachInterrupt(HCbutton, HCchange, FALLING);
    //attachInterrupt(TempUpbutton, TemperatureUp, FALLING);
    //attachInterrupt(TempDwnbutton, TemperatureDown, FALLING);


    //16 columns, 2 rows
    //lcd.begin(16,2);


    Serial.begin(9600);

    connect();

    //pinMode(EnRelay,OUTPUT);

    //pinMode(FanRelay,OUTPUT);

    //AC relay
    pinMode(ACRelay,OUTPUT);

    //heat relay
    pinMode(HRelay,OUTPUT);

    emon1.current(analogPin0,19.23 * 1.45);
}






void loop() {




    if(!client.connected() && !isConnecting)
    {
        Serial.println("disonnected---");
        isAuthenticated = 0;
        delay(500);

        connect();

    }


//---------


    if((millis() - resetTime) > 31000)
    {
        if(!client.available())
        {
            reconnect();
        }
    }


//----------


    if(isAuthenticated == 0)
    {
        client.println("110001 authenticate access_token cdaf750fb93559119cc5bd7777ad311b5a9a8280");
        Serial.println("sent authenticate---");
        delay(200);
        isAuthenticated = 1;
        newAuth = 1;
    }

//----------

    int s = 0;

    int i = 0;

    int j = 0;

    char inChar = '0';

    char message[1024] = "";

    if(client.available())
    {

        inChar = '0';
        i = 0;
        if(client.available())
        {
            while(inChar != '\n')
            {
                inChar = client.read();
                message[i] = inChar;
                i++;
            }

            message[i] = '\0';

            if(message[0] != '\0')
            {
                onMessage(message);
                delay(50);

            }
        }

        if(!((message[17] == 'p') && (message[18] == 'i') && (message[20] == 'g') && (message[21] == ' ')))
        {
            for(j = 0; j < 4; j++)
            {

                inChar = '0';
                i = 0;
                    if(client.available())
                    {
                        while(inChar != '\n')
                        {
                            inChar = client.read();
                            message[i] = inChar;
                            i++;
                        }

                        message[i] = '\0';

                        if(message[0] != '\0')
                        {

                            onMessage(message);
                            delay(50);

                        }

                    }

            }

        }

        if(newAuth == 1)
        {
            inChar = '0';
            i = 0;
                if(client.available())
                {
                    while(inChar != '\n')
                    {
                        inChar = client.read();
                        message[i] = inChar;
                        i++;
                    }

                    message[i] = '\0';

                    if(message[0] != '\0')
                    {

                        onMessage(message);
                        delay(50);

                    }
                }
            newAuth = 0;
            s = 1;
        }

        client.flush();
        resetTime = millis();
    }

//-------------------------------

    if(digitalRead(A1) == 0)
    {
        //if(millis() - interrupttime > 300)
        //{
            Updated = 1;

            Settings[1] += 1;

            interrupttime = millis();
        //}

    }
    else if(digitalRead(A2) == 0)
    {
        //if(millis() - interrupttime > 300)
        //{
            Updated = 1;

            if(Settings[2] == 0)
            {
                Settings[2] = 1;
            }
            else if(Settings[2] == 1)
            {
                Settings[2] = 0;
            }

            interrupttime = millis();
        //}

    }
    else if(digitalRead(A3) == 0)
    {
        //if(millis() - interrupttime > 300)
        //{
            Updated = 1;

            Settings[1] -= 1;

            interrupttime = millis();

        //}

    }


//----------------------------------

    //if a button was pressed send the updated state
    if(Updated == 1)
    {
        allDataSend();

        Serial.println("updated");

        Updated = 0;
        delay(50);
        client.flush();
    }


//---------------

    if((millis() - temperatureTime < 30000) && (millis() > 5000))
    {
        TemperatureC = (100 * (analogRead(Tmonitor) * 0.0008)) - 50;
        sumT = sumT + (TemperatureC * 1.8) + 32;
        countT += 1;
    }
    else if(millis() - temperatureTime > 30000)
    {
        //countT += 1;
        TemperatureF = (sumT / countT);
        temperatureTime = millis();
        sumT = 0;
        countT = 0;
        sendData("set","tempCurrent",TemperatureF);
    }



//---------------



    if((Settings[0] == 0) && (Updated == 0))
    {
        //digitalWrite(EnRelay,HIGH);
        digitalWrite(ACRelay,HIGH);
        digitalWrite(HRelay,HIGH);
        //digitalWrite(FanRelay,HIGH);
    }
    else if(Updated == 0)
    {
        //digitalWrite(EnRelay,LOW);

        //fan off
        if(Settings[3] == 0)
        {
            digitalWrite(ACRelay,HIGH);
            digitalWrite(HRelay,HIGH);
            //digitalWrite(FanRelay,HIGH);
        }
        else
        {
            //fan auto
            if(Settings[3] == 2)
            {
                //cool
                if(Settings[2] == 0)
                {
                    digitalWrite(HRelay,HIGH);

                    if(TemperatureF < Settings[1])
                    {
                        //digitalWrite(FanRelay,HIGH);
                        digitalWrite(ACRelay,HIGH);

                    }
                    else if(TemperatureF > Settings[1])
                    {
                        //digitalWrite(FanRelay,LOW);
                        digitalWrite(ACRelay,LOW);

                    }

                }

                //heat
                else if(Settings[2] == 1)
                {
                    digitalWrite(ACRelay,HIGH);

                    if(TemperatureF < Settings[1])
                    {
                        //digitalWrite(FanRelay,LOW);
                        digitalWrite(HRelay,LOW);

                    }
                    else if(TemperatureF > Settings[1])
                    {
                        //digitalWrite(FanRelay,HIGH);
                        digitalWrite(HRelay,HIGH);

                    }

                }

            }

            //fan on
            else if(Settings[3] == 1)
            {
                //digitalWrite(FanRelay,LOW);
                //cool
                if(Settings[2] == 0)
                {
                    digitalWrite(ACRelay,LOW);
                    digitalWrite(HRelay,HIGH);
                }
                //heat
                else if(Settings[2] == 1)
                {
                    digitalWrite(HRelay,LOW);
                    digitalWrite(ACRelay,HIGH);
                }

            }

        }

    }

//----------------------




    if((s != 1) && (Updated == 0)/* && (millis() - interrupttime > 500)*/)
    {
        lcd.begin(16,2);
        //display
        lcd.setCursor(0,0);

        if(Settings[0] == 1)
        {
            if(Settings[2] == 0)
            {
                lcd.print("cool");
            }
            else
            {
                lcd.print("heat");
            }
        }
        else
        {
            lcd.print("off ");
        }


        if(Settings[1] >= 100)
        {
            lcd.setCursor(8,0);
        }
        else
        {
            lcd.setCursor(9,0);
        }


        lcd.print("set: ");
        lcd.print(Settings[1]);


        lcd.setCursor(0,1);


        if(Settings[0] == 1)
        {
            if(Settings[3] == 0)
            {
                lcd.print("off ");
            }
            else if(Settings[3] == 1)
            {
                lcd.print("on  ");
            }
            else if(Settings[3] == 2)
            {
                lcd.print("auto");
            }
        }
        else
        {
            lcd.print("off ");
        }

        if(TemperatureF >= 100)
        {
            lcd.setCursor(13,1);
        }
        else
        {
            lcd.setCursor(14,1);
        }


        lcd.print(TemperatureF);



        delay(50);


        for(i = 0; i < 4; i++)
        {
            Serial.print(Settings[i]);
            Serial.print(' ');
        }
        Serial.print(TemperatureF);
        Serial.print('\n');

        

    }



//----------------------

    unsigned long currentMillis = millis();


        double Irms = emon1.calcIrms(1480);  // Calculate Irms
        //Serial.println(Irms);
        double watts = (Irms*voltage);
        sum = sum + watts;
        count = count + 1;
        if ((unsigned long)(currentMillis - prevMinute) < minuteInterval){
        return;
        }
        else{
            average = sum/count;
            Serial.print(" =");
            Serial.println(average);
            dataSend("stat", "power_usage", average);
            average = 0.0;
            sum = 0.0;
            count = 0;
            prevMinute = currentMillis;
        }
        prevSecond = currentMillis;


}
