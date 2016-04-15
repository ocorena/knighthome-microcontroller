#include "application.h"

#include "EmonLib.h"


SYSTEM_MODE(SEMI_AUTOMATIC);



//local device ID, characters 1 and 2 are the type of device, the remaining characters are the
//unique device identifyer
char LdeviceID[] = "000001";

//00 wall switch
//01 wall outlet
//10 door lock
//11 hvac controller



TCPClient client;

//switch auth statement
char authenticatorS[] = "000001 authenticate access_token a5f767ec5d217ad4174d778c0b1948c790c4513c\n\0";
//outlent auth statement
char authenticatorW[] = "010001 authenticate access_token 784c243a8b57a3c2224e37c9f455b480946764e2\n\0";
//powerlock auth statement
char authenticatorL[] = "100001 authenticate access_token 5bda474342f50fdb7f015f56264e734b9b507762\n\0";
//HVAC controller auth statement
char authenticatorH[] = "110001 authenticate access_token cdaf750fb93559119cc5bd7777ad311b5a9a8280\n\0";



//current state of relay, 0 = OFF, 1 = ON, relay is active low
int Rstate = 0;

int Rstate2 = 0;

//was button pushed?
int buttonPushed = 0;



int Relay0 = D0;
int Relay1 = D3;
int LED = D7;
int analogPin0 = A0;

int buttonPin = D7;



//time until reconnect
int resetTime = 0;


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


//no = 0, yes = 1
int isAuthenticated = 0;

int newAuth = 0;

bool isConnecting = FALSE;



void onMessage(char* message) {

    Serial.println("inmessage: ");
    Serial.println(message);

    int i = 0;

    for(i = 0; i < 6; i++)
    {
        if(message[i] != LdeviceID[i])
        {
            //error incorrect device id wrong number
            Serial.println("ERROR: incorrect device ID N");
            return;
        }
    }

    if(message[6] != ' ')
    {
        //error, device id too long
        Serial.println("ERROR: incorrect deive ID L");
        return;
    }


    if((message[7] == 'a') && (message[8] == 'u') && (message[9] == 't') && (message[10] == 'h') && (message[11] == 'e') && (message[12] == 'n') && (message[13] == 't') &&
                    (message[14] == 'i') && (message[15] == 'c') && (message[16] == 'a') && (message[17] == 't') && (message[18] == 'e') && (message[19] == ' '))
    {
        //authentication received
        return;
    }

    else if((message[7] == 'k') && (message[8] == 'e') && (message[9] == 'e') && (message[10] == 'p') && (message[11] == 'a') && (message[12] == 'l') && (message[13] == 'i') &&
                    (message[14] == 'v') && (message[15] == 'e') && (message[16] == ' ') && (message[17] == 'p') && (message[18] == 'i') && (message[19] == 'n') && (message[20] == 'g') &&
                    (message[35] == '\n'))
    {
        message[18] = 'o';
        message[19] = 'n';
        message[20] = 'g';


        message[35] = '\n';
        message[36] = '\0';

        client.print(message);
        Serial.print("response:\n");
        Serial.println(message);
        return;
    }

    else if((message[7] == 's') && (message[8] == 'e') && (message[9] == 't') && (message[10] == ' '))
    {
        if ((message[11] == 'e') && (message[12] == 'n') && (message[13] == 'a') && (message[14] == 'b') && (message[15] == 'l') && (message[16] == 'e') &&
                (message[17] == 'd') && (message[18] == ' '))
        {

            if((message[19] == 'f') && (message[20] == 'a') && (message[21] == 'l') && (message[22] == 's') && (message[23] == 'e') && (message[24] == '\n'))
            {
                digitalWrite(Relay0,HIGH);

                Rstate = 0;

                return;
            }
            else if((message[19] == 't') && (message[20] == 'r') && (message[21] == 'u') && (message[22] == 'e') && (message[23] == '\n'))
            {
                digitalWrite(Relay0,LOW);

                Rstate = 1;

                return;
            }

        }
        else if ((message[11] == 'e') && (message[12] == 'n') && (message[13] == 'a') && (message[14] == 'b') && (message[15] == 'l') && (message[16] == 'e') &&
                (message[17] == 'd') && (message[18] == '2') && (message[19] == ' '))
        {

            if((message[20] == 'f') && (message[21] == 'a') && (message[22] == 'l') && (message[23] == 's') && (message[24] == 'e') && (message[25] == '\n'))
            {
                digitalWrite(Relay1,HIGH);

                Rstate2 = 0;

                return;
            }
            else if((message[20] == 't') && (message[21] == 'r') && (message[22] == 'u') && (message[23] == 'e') && (message[24] == '\n'))
            {
                digitalWrite(Relay1,LOW);

                Rstate2 = 1;

                return;
            }
        }
    }

    //error: invalid input
    Serial.println("error: invalid params");
    return;
}





void dataSend(char *action, char *state_Name, double state_Type){


    char Lmessage[1024];
    char state_Value[50];
    int i = 0;
    int j = 0;

    //send wall switch state
    if(state_Type == -100)
    {
        if(Rstate == 0)
        {
            state_Value[0] == 'f';
            state_Value[1] == 'a';
            state_Value[2] == 'l';
            state_Value[3] == 's';
            state_Value[4] == 'e';
        }
        else if(Rstate == 1)
        {
            state_Value[0] == 't';
            state_Value[0] == 'r';
            state_Value[0] == 'u';
            state_Value[0] == 'e';
        }
    }



    //power monitor data
    else if(state_Type >= 0)
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

    while(state_Value[j] != '\0')
    {
        Lmessage[i] = state_Value[j];
        i++;
        j++;
    }

    Lmessage[i] = '\n';
    Lmessage[i+1] = '\0';

    client.print(Lmessage);
    Serial.print("sent: ");
    Serial.println(Lmessage);
}


void wallOutlet()
{
    if(isAuthenticated == 0)
    {

        client.print(authenticatorW);
        Serial.println("sent authenticate---");
        delay(200);
        isAuthenticated = 1;
        newAuth = 1;
    }



    int i = 0;

    char inChar = '0';

    char message[1024] = "";

    while(client.available())
    {
        while(inChar != '\n')
        {
            inChar = client.read();
            message[i] = inChar;
            i++;
        }

        if(message[0] != '\0')
        {
            onMessage(message);
            delay(100);

        }

        if(message[7] != 'k')
        {

            inChar = client.read();

            //-----------------------------

            while(!((inChar > '\0') && (inChar < '~')))
            {
                inChar = client.read();
            }

            //-----------------------------

            i = 0;
            message[i] = inChar;
            i++;

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
                delay(100);

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
                        delay(100);

                    }
                }
            newAuth = 0;
        }

        client.flush();
        resetTime = millis();

    }

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








void wallSwitch()
{
    if(isAuthenticated == 0)
    {

        client.print(authenticatorS);
        Serial.println("sent authenticate---");
        delay(200);
        isAuthenticated = 1;
        newAuth = 1;
    }


    int i = 0;

    char inChar = '0';

    char message[1024] = "";

    if(client.available() && (buttonPushed == 0))
    {
        while(inChar != '\n')
        {
            inChar = client.read();
            message[i] = inChar;
            i++;
        }

        if(message[0] != '\0')
        {

            onMessage(message);
            delay(100);

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
                        delay(100);

                    }
                }
            newAuth = 0;
        }

        client.flush();
        resetTime = millis();
    }


    if(buttonPushed == 1)
    {
        dataSend("set","enabled",-100);
        buttonPushed = 0;

    }




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



void switchButton()
{
    if(Rstate == 0)
    {
        Rstate == 1;
        digitalWrite(Relay0,LOW);
    }
    else if(Rstate == 1)
    {
        Rstate == 0;
        digitalWrite(Relay0,HIGH);
    }

    buttonPushed = 1;

}






void pLock()
{
    if(isAuthenticated == 0)
    {

        client.print(authenticatorL);
        Serial.println("sent authenticate---");
        delay(200);
        isAuthenticated = 1;
        newAuth = 1;
    }

    int i = 0;

    char inChar = '0';

    char message[1024] = "";

    if(client.available())
    {
        while(inChar != '\n')
        {
            inChar = client.read();
            message[i] = inChar;
            i++;

        }

        if(message[0] != '\0')
        {
            onMessage(message);

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
                        //delay(100);
                    }
                }
            newAuth = 0;
        }

        client.flush();
        resetTime = millis();
    }

//                      --------------------------------
    if(!WiFi.ready() && ((millis() - prevSecond) > 2000))
    {
        digitalWrite(LED,HIGH);
        delay(500);
        digitalWrite(LED,LOW);

        prevSecond = millis();
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

    pinMode(Relay0, OUTPUT);
    pinMode(Relay1, OUTPUT);
    pinMode(LED, OUTPUT);
    pinMode(buttonPin, INPUT);


    WiFi.setCredentials("Tornado_guest","fourwordsalluppercase",WPA2);

    Particle.connect();

    while(!Particle.connected)
    {
        delay(1);
    }



    if((LdeviceID[0] == 0) && (LdeviceID[1] == 0))
    {
        pinMode(buttonPin, INPUT);
        attachInterrupt(buttonPin, switchButton, CHANGE);
    }



    Serial.begin(9600);

    connect();

    resetTime = millis();

    digitalWrite(Relay0,HIGH);
    digitalWrite(Relay1,HIGH);


    emon1.current(analogPin0,19.23 * 1.45);

}








//main function
void loop() {


    if(!client.connected() && !isConnecting)
    {
        Serial.println("disonnected---");
        if(LdeviceID[0] == 1)
        {
            digitalWrite(Relay0,HIGH);
        }
        isAuthenticated = 0;
        delay(500);

        connect();

    }



    if((millis() - resetTime) > 31000)
    {
        if(!client.available())
        {
            reconnect();
        }
    }


    if(LdeviceID[0] == '0')
    {
        if(LdeviceID[1] == '0')
        {
            wallSwitch();
        }
        else
        {
            wallOutlet();
        }
    }
    else
    {
        pLock();
    }

}
