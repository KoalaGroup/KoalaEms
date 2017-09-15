#define BUFSIZE 102400
char buffer[102400];
main()
{
while (read(0, buffer, BUFSIZE)>0);
}
