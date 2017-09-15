/*****************************************************************************
FIRMA:          KFA-Juelich, Institute ZEL, IKP
PROJEKT:        LEX (Low Energy X-ray spectrometer)

MODULNAME:      Conv2BCD.c
SCHLAGWORTE:    Konvertierung von Dezimal in BCD-Code

AUTOR:          Peter Wuestner oder Mathias Drochner und ein wenig Thomas Siems
DATUM:          14.1.1994
VERSION:        1.0
MODIFIKATIONEN:
STATUS:         freigegeben
COMPILATION:    c89 -o Conv2BCD Conv2BCD.c -lm

BESCHREIBUNG:
   BCD-Codierung:
   Diese kleine Routine bewirkt, dass man beim Aufruf von ScaWrite() dem
   Argument kein 0x voranstellen muss. Der Scaler erwartet naemlich
   normalerweise als Argument eine Hexadezimale Zahl. In dieser Routine wird
   die eingegebene Dezimalzahl im Prinzip folgendermassen umgewandelt:
   B = 10   (Basis des Dezimalsystems)
   x(i)x(i-1)...x(0) = x(i)*B^(i) + x(i-1)*B^(i-1) ... + x(0)*1
   wird umgewandelt in ---> dito, jedoch mit B = 16
*******************************************************************************/

int Conv2BCD(x)
        int x;
        {
        int a,i,q;
        i=0;
        q=0;
        while(x)
                {
                a=x%10;
                x/=10;
                q+=a<<(4*i++);
                }
        return(q);
        }
/*----------------------------------------------------------------------------

int main(ArgNum, ArgList)
int ArgNum;
char *ArgList[];
{
    int i, x;
    ArgNum--;  * (Programmname abziehen) *

        * convert from string to integer: *
        i = sscanf(ArgList[1],"%d",&x);
        if(ArgNum != 1) 
                {
                printf("%%Error: Nur ein Argument erlaubt!\n");
                }
        else if (i != 1)
                printf("Error in MAIN, function sscanf (Argument?) \n");
        else
                {
                printf("******** Input=%d ********\n",x);
                x = Conv2BCD(x);                
                printf("******** Output=%d ********\n",x);
                }
        return 0;
}
