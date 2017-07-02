/* Includes ------------------------------------------------------------------*/
#include "main.h"
//#include "eeprom.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>

//#include "usbd_cdc_if.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#ifndef TCP_PORT
	#define TCP_PORT		23				 /* define the TCP connection port */
#endif

#define MAX_NAME_SIZE	64

struct name 
{
  int length;
  char bytes[MAX_NAME_SIZE];
};

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void udp_server_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port);
err_t tcp_server_accept(void *arg, struct tcp_pcb *pcb, err_t err);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);

/* Private functions ---------------------------------------------------------*/


void TCP_server_init(void)
{
	struct tcp_pcb *pcb;
	ip_addr_t *addr;// == NULL;
	
	/* Create a new TCP control block  */
  pcb = tcp_new();
	
  if(pcb !=NULL) {
		err_t err;	  
	      
    /* Assign to the new pcb a local IP address and a port number */
    err = tcp_bind(pcb, addr, TCP_PORT);
	  
		if(err != ERR_USE){
	  /* Set the connection to the LISTEN state */
    pcb = tcp_listen(pcb);
    
    /* Specify the function to be called when a connection is established */
    tcp_accept(pcb, tcp_server_accept);
		
		}else{
	  /* We enter here if a conection to the addr IP address already exists */
	  /* so we don't need to establish a new one */
	  tcp_close(pcb);
		}
	}
}

	
/**
  * @brief  This funtion is called when a TCP connection has been established on the port TCP_PORT.
  * @param  arg	user supplied argument 
  * @param  pcb	the tcp_pcb which accepted the connection
  * @param  err error value
  * @retval ERR_OK
  */
err_t tcp_server_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{ 
	/* Tell LwIP to associate this structure with this connection. */
  tcp_arg(pcb, mem_calloc(sizeof(struct name), 1));	
	
  /* Specify the function that should be called when the TCP connection receives data */
  tcp_recv(pcb, tcp_server_recv);
	
	/* Send out the first message */  
  tcp_write(pcb, "LCC GATE\r\n", 10, 1);
	tcp_write(pcb, "FirmWare ver.0.0.0.a", 13, 1); //FWVersion
	tcp_write(pcb, "\r\n", 2, 1); //FWVersion
	
	tcp_write(pcb, ">", 1, 1);	
	//tcp_write(pcb, "SilmpleSolutions\r\n", 18, 1); 

  return ERR_OK;  
}

/**
  * @brief  This function is called when a data is received over the TCP_PORT.
  *         The received data contains the number of the led to be toggled.
  * @param  arg	user supplied argument 
  * @param  pcb	the tcp_pcb which accepted the connection
  * @param  p the packet buffer that was received
  * @param  err error value
  * @retval ERR_OK
  */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct pbuf *q;
	struct name *name = (struct name *)arg;
	int done;
	char *data;
	int i;
	//char answer[32] = "";
	//int answerNumb = 0;
	
	if (p != NULL)
	{
		/* We call this function to tell the LwIp that we have processed the data */
		/* This lets the stack advertise a larger window, so more data can be received*/
		tcp_recved(pcb, p->tot_len);
		
		/* Check the name if NULL, no data passed, return withh illegal argument error */
		if(!name) 
		{
      pbuf_free(p);
      return ERR_ARG;
    }
		
		done = 0;
		for(q=p; q != NULL; q = q->next)
		{
      data = q->payload;
      for(i=0; i<q->len && !done; i++) 
			{
        done = ((data[i] == '\r') || (data[i] == '\n')); //Here we are got the end of string
				
        if(name->length < MAX_NAME_SIZE) 
				{
          name->bytes[name->length++] = data[i];
        }
      }
    }
		
		
		if(done)											//it means that we got the packet with \r or \n on the end
    {
      if(name->bytes[name->length-2] != '\r' || name->bytes[name->length-1] != '\n') //checking that it ends with /r/n
			{
        if((name->bytes[name->length-1] == '\r' || name->bytes[name->length-1] == '\n') && (name->length+1 <= MAX_NAME_SIZE)) 
				{
					name->length += 1;
        } 
				else if(name->length+2 <= MAX_NAME_SIZE) 
				{
          name->length += 2;
        } 
				else 
				{
          name->length = MAX_NAME_SIZE;
        }

        name->bytes[name->length-2] = '\r';
        name->bytes[name->length-1] = '\n';
      }
			
			
			 if((strncmp(name->bytes, "?", 1) == 0) || (strncmp(name->bytes, "help", 4) == 0)) // -------   ?  ------------//
			{
				//showHelpMessage(pcb);
				tcp_write(pcb, "It Works\r\n", 10, 1);
			}
			
			else if(strncmp(name->bytes, "exit", 4) == 0)
			{
					//pbuf_free(p);
					mem_free(name);
					tcp_close(pcb);
			} 
				
			if(strncmp(name->bytes, "exit", 4) != 0) //It's no exit command, we could to add some data to the string
				{
					tcp_write(pcb, ">", 1, 1);
				}
			
			name->length = 0;
    }
		
		/* End of processing, we free the pbuf */
    pbuf_free(p);
		
	} 
	else if (err == ERR_OK) //Pbuff NULL!
	{
		/* When the pbuf is NULL and the err is ERR_OK, the remote end is closing the connection. */
    /* We free the allocated memory and we close the connection */
    mem_free(name);
    return tcp_close(pcb);
	}

	return ERR_OK;
	
}



/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
