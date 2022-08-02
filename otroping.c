#include"otroping.h" /*Añadimos otroping.h, que deberá estar bajo el mismo
directorio que otroping.c*/
int main(int argc, char *argv[])
{
    int sockfd;// Descriptor de socket
    char *ip_addr, *reverse_hostname;
    struct sockaddr_in DIRECCION;
    int addrlen = sizeof(DIRECCION);//Tamaño necesario para int addrlen
    char net_buf[NI_MAXHOST];
 
    if(argc!=2)
    {
        printf("Formato %s < Ejecutar como ROOT-  FORMATO Por ejemplo, <www.mipagina.com.>, o una IP\n", argv[0]);
        return 0;
    }
 
    ip_addr = dns_lookup(argv[1], &DIRECCION);
    if(ip_addr==NULL)
    {
        printf(" NO PUEDE RESOLVER EL HOST!\n");
 
    }
 
    reverse_hostname = reverse_dns_lookup(ip_addr);
    printf("Tratando de conectar con: '%s' IP: %s\n",
                                       argv[1], ip_addr);
    printf("DEBES SER ROOT PARA HACER PING Al OBJETIVO---->: %s\n",
                           reverse_hostname);
 
    //socket()
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sockfd<0)
    {
        printf("Socket  descriptor no recibido!!\n");
        return 0;
    }
    else
        printf("Socket  descriptor %d recibido\n", sockfd);
 
    signal(SIGINT, manejador);//captura de interrupción, o manejador
 
    //Envio de PING de forma CONTINUADA
    Enviando_Ping(sockfd, &DIRECCION, reverse_hostname, ip_addr, argv[1]);
 
    return 0;
}