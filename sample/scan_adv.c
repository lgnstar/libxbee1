#ifdef shell
gcc -o ${0//.c/} $0 -lxbee
exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <xbee.h>

#define MAXNODES 100

int ATCH = 0x0C; /* origional channel number */
int ATNT = 0x19; /* node discover timeout */
int ATNTc; /* node discover timeout counter */
int BREAK = 0;
xbee_con *con;

void sighandler(int sig) {
  xbee_pkt *rpkt;
  if (sig == SIGINT) {
    BREAK = 1;
    /* wait for the rest of the timeout... */
    printf("\r%c[2KWaiting for node discover command to timeout...",27);
    fflush(stdout);
    for (; ATNTc; ATNTc--) {
      usleep(100000);
    }
    /* Restore the XBee's channel setting */
    printf("\r%c[2KRestoring channel to 0x%02X...",27,ATCH);
    fflush(stdout);
    if ((rpkt = xbee_senddata(con,"CH%c",ATCH)) == NULL) {
      printf("\r%c[2K*** XBee didnt return a result for CH... ***\nPlease manually reset your channel to 0x%02X\n",27,ATCH);
    } else if (rpkt->status) {
      printf("\r%c[2K*** An error occured while restoring the channel setting... ***\nPlease manually reset your channel to 0x%02X\n",27,ATCH);
    } else {
      printf("\nDone!\n");
    }
    free(rpkt);
    /* Restore the terminal */
    printf("%c[?25h%c[0m",27,27);
    fflush(stdout);
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  int i;
  int saidfull = 0;
  int ATCHc; /* current channel number */
  int XBeePro = 0; /* XBee pro? */

  int nodes = 0;
  unsigned char addrs[MAXNODES][19]; /* 0-7  : 64 bit address
					8    : channel
					9-10 : id
					11   : baud
					12   : API
					13-14: HV
					15-16: VR
					17   : CC
					18   : mask - not address */

  xbee_pkt *pkt, *rpkt;

  time_t ttime;
  char stime[32];

  /* handle ^C */
  (void) signal(SIGINT, sighandler);

  /* setup libxbee */
  if (xbee_setup("/dev/ttyUSB0",57600) == -1) {
    return 1;
  }

  /* grab a local AT connection */
  con = xbee_newcon('I',xbee_localAT);

  /* get the current channel */
  if ((rpkt = xbee_senddata(con,"CH")) == NULL) {
    printf("XBee didnt return a result for CH\n");
    return 1;
  }
  ATCH = rpkt->data[0];
  free(rpkt);

  /* XBee     - 0x0B - 0x1A
     XBee Pro - 0x0C - 0x17 */
  if ((rpkt = xbee_senddata(con,"CH%c",0x0B)) == NULL) {
    printf("XBee didnt return a result for CH\n");
    return 1;
  }
  /* did that fail? */
  if (rpkt->status == 0) {
    /* nope.. its not a pro */
    printf("Using XBee (not Pro) channels (0x0B - 0x1A)...\n");
    XBeePro = 0;
    ATCHc = 0x0B;
  } else {
    /* yup... its a pro */
    printf("Using XBee Pro channels (0x0C - 0x17)...\n");
    XBeePro = 1;
    ATCHc = 0x0C;
  }
  free(rpkt);

  /* find and print data for the local node */
  printf("\n%c[31mCH:%c[32m 0x%02X    ",27,27,ATCH);
  if ((rpkt = xbee_senddata(con,"ID")) == NULL) {
    printf("\nXBee didnt return a result for ID\n");
    return 1;
  }
  printf("%c[31mID:%c[32m 0x%02X%02X    ",27,27,rpkt->data[0],rpkt->data[1]);
  free(rpkt);
  if ((rpkt = xbee_senddata(con,"MY")) == NULL) {
    printf("\nXBee didnt return a result for MY\n");
    return 1;
  }
  printf("%c[31mMY:%c[32m 0x%02X%02X    ",27,27,rpkt->data[0],rpkt->data[1]);
  free(rpkt);
  if ((rpkt = xbee_senddata(con,"SH")) == NULL) {
    printf("\nXBee didnt return a result for SH\n");
    return 1;
  }
  printf("%c[31mSH:%c[32m 0x%02X%02X%02X%02X    ",27,27,rpkt->data[0],rpkt->data[1],rpkt->data[2],rpkt->data[3]);
  free(rpkt);
  if ((rpkt = xbee_senddata(con,"SL")) == NULL) {
    printf("\nXBee didnt return a result for SL\n");
    return 1;
  }
  printf("%c[31mSL:%c[32m 0x%02X%02X%02X%02X    ",27,27,rpkt->data[0],rpkt->data[1],rpkt->data[2],rpkt->data[3]);
  free(rpkt);
  if ((rpkt = xbee_senddata(con,"BD")) == NULL) {
    printf("\nXBee didnt return a result for BD\n");
    return 1;
  }
  printf("%c[31mBD:%c[32m ",27,27);
  /* print the baud rate */
  switch (rpkt->data[3]) {
  case 0:  printf("  1200"); break;
  case 1:  printf("  2400"); break;
  case 2:  printf("  4800"); break;
  case 3:  printf("  9600"); break;
  case 4:  printf(" 19200"); break;
  case 5:  printf(" 38400"); break;
  case 6:  printf(" 57600"); break;
  case 7:  printf("115200"); break;
  default: printf(" other"); break;
  }
  printf("    ");
  free(rpkt);
  if ((rpkt = xbee_senddata(con,"AP")) == NULL) {
    printf("\nXBee didnt return a result for AP\n");
    return 1;
  }
  printf("%c[31mAP:%c[32m 0x%02X    ",27,27,rpkt->data[0]);
  free(rpkt);
  if ((rpkt = xbee_senddata(con,"HV")) == NULL) {
    printf("\nXBee didnt return a result for HV\n");
    return 1;
  }
  printf("%c[31mHV:%c[32m 0x%02X%02X    ",27,27,rpkt->data[0],rpkt->data[1]);
  free(rpkt);
  if ((rpkt = xbee_senddata(con,"VR")) == NULL) {
    printf("\nXBee didnt return a result for VR\n");
    return 1;
  }
  printf("%c[31mVR:%c[32m 0x%02X%02X    ",27,27,rpkt->data[0],rpkt->data[1]);
  free(rpkt);
  if ((rpkt = xbee_senddata(con,"CC")) == NULL) {
    printf("\nXBee didnt return a result for CC\n");
    return 1;
  }
  printf("%c[31mCC:%c[32m '%c' (0x%02X)    ",27,27,rpkt->data[0],rpkt->data[0]);
  free(rpkt);
  if ((rpkt = xbee_senddata(con,"NI")) == NULL) {
    printf("\nXBee didnt return a result for NI\n");
    return 1;
  }
  printf("%c[31mNI:%c[32m %-20s   ",27,27,rpkt->data);
  free(rpkt);
  printf("%c[95m* This is the lobal XBee *",27);

  printf("%c[34m\n---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------%c[0m\n\n",27,27);

  /* get the ND timeout */
  if ((rpkt = xbee_senddata(con,"NT")) == NULL) {
    printf("XBee didnt return a result for NT\n");
    return 1;
  }
  ATNT = rpkt->data[0];
  free(rpkt);

  printf("%c[?25l",27);
  fflush(stdout);

  usleep(100000);

  while (!BREAK) {
    /* set the channel to scan */
    rpkt = xbee_senddata(con,"CH%c",ATCHc);
    if (!rpkt || rpkt->status) {
      printf("\nXBee didnt return a result for CH\n");
      return 1;
    }
    free(rpkt);
    printf("%c[2KScanning channel 0x%02X...\r",27,ATCHc);
    fflush(stdout);
    /* send a ND - Node Discover request */
    rpkt = xbee_senddata(con,"ND");
    /* only wait for a bit longer than the ND timeout */
    ATNTc = ATNT + 10;
    /* loop until the end packet has been received or timeout reached */
    while (!BREAK && ATNTc--) {
      /* got a packet? */
      if (rpkt) {
	/* here we use the origionally returned packet */
	pkt = rpkt;
	rpkt = NULL;
      } else {
	/* here we get a new one */
	pkt = xbee_getpacket(con);
      }
      /* check a packet was returned, and that its one we are after... */
      if (pkt && !memcmp(pkt->atCmd,"ND",2)) {
	/* is this the end packet? you can tell from the 0 datalen */
	if (pkt->datalen == 0) {
	  /* free the packet */
	  free(pkt);
	  break;
	} else {
	  /* check if we know this node already */
	  for (i = 0; i < nodes; i++) {
	    /* the 64bit address will match one in the list */
	    if (!memcmp(&(pkt->data[2]),&(addrs[i][0]),8)) break;
	  }
	  ttime = time(NULL);
	  strftime(stime,32,"%I:%M:%S %p",gmtime(&ttime));
	  /* is there space for another? */
	  if ((i == nodes) &&
	      (nodes == MAXNODES) &&
	      (!saidfull)) {
	    printf("%c[2KMAXNODES reached... Can't add more...\r",27);
	    /* flush so the change is seen! */
	    fflush(stdout);
	    saidfull = 1;
	  } else {
	    /* is this a rewrite? */
	    if (i != nodes) {
	      /* find the line to edit */
	      printf("%c[%dA",27,nodes-i+1);
	      /* clear the line */
	      printf("%c[2K",27);
	    } else {
	      /* fill the blank line */
	      printf("%c[%dA",27,1);
	    }
	    /* save the channel */
	    addrs[i][8] = ATCHc;
	    /* write out the info */
	    printf("%c[31mCH:%c[32m 0x%02X    ",27,27,ATCHc);
	    printf("%c[31mID:%c[32m 0x",27,27);
	    if (i == nodes || !(addrs[i][18] & 0x80)) {
	      printf("....");
	    } else {
	      printf("%02X%02X",addrs[i][9],addrs[i][10]);
	    }
	    printf("    ");
	    printf("%c[31mMY:%c[32m 0x%02X%02X    ",27,27,pkt->data[0],pkt->data[1]);
	    printf("%c[31mSH:%c[32m 0x%02X%02X%02X%02X    ",27,27,pkt->data[2],pkt->data[3],pkt->data[4],pkt->data[5]);
	    printf("%c[31mSL:%c[32m 0x%02X%02X%02X%02X    ",27,27,pkt->data[6],pkt->data[7],pkt->data[8],pkt->data[9]);
	    printf("%c[31mBD:%c[32m ",27,27);
	    if (i == nodes || !(addrs[i][18] & 0x40)) {
	      printf("......");
	    } else {
	      switch (addrs[i][11]) {
	      case 0:  printf("  1200"); break;
	      case 1:  printf("  2400"); break;
	      case 2:  printf("  4800"); break;
	      case 3:  printf("  9600"); break;
	      case 4:  printf(" 19200"); break;
	      case 5:  printf(" 38400"); break;
	      case 6:  printf(" 57600"); break;
	      case 7:  printf("115200"); break;
	      default: printf(" other"); break;
	      }
	    }
	    printf("    ");
	    printf("%c[31mAP:%c[32m 0x",27,27);
	    if (i == nodes || !(addrs[i][18] & 0x20)) {
	      printf("..");
	    } else {
	      printf("%02X",addrs[i][12]);
	    }
	    printf("    ");
	    printf("%c[31mHV:%c[32m 0x",27,27);
	    if (i == nodes || !(addrs[i][18] & 0x10)) {
	      printf("....");
	    } else {
	      printf("%02X%02X",addrs[i][13],addrs[i][14]);
	    }
	    printf("    ");
	    printf("%c[31mVR:%c[32m 0x",27,27);
	    if (i == nodes || !(addrs[i][18] & 0x08)) {
	      printf("....");
	    } else {
	      printf("%02X%02X",addrs[i][15],addrs[i][16]);
	    }
	    printf("    ");
	    printf("%c[31mCC:%c[32m ",27,27);
	    if (i == nodes || !(addrs[i][18] & 0x04)) {
	      printf(" .  (0x..)");
	    } else {
	      printf("'%c' (0x%02X)",addrs[i][17],addrs[i][17]);
	    }
	    printf("    ");
	    printf("%c[31mNI:%c[32m %-20s    ",27,27,&(pkt->data[11]));
	    printf("%c[31mdB:%c[32m -%2d    ",27,27,pkt->data[10]);
	    printf("%c[31m@:%c[32m %s",27,27,stime);
	    /* is this a rewrite? */
	    if (i != nodes) {
	      /* go back the the bottom */
	      printf("%c[%dB\r",27,nodes-i+1);
	    } else {
	      /* if its new... save the address */
	      memcpy(&(addrs[nodes][0]),&(pkt->data[2]),8);
	      /* turn off all the flags */
	      addrs[nodes][18] = 0;
	      /* new line is only wanted for new nodes */
	      printf("\n%c[2K\n%c[0m",27,27);
	      /* if not, then add 1 to the number of nodes! */
	      nodes++;
	    }
	    printf("%c[0m%c[2KScanning channel 0x%02X...\r",27,27,ATCHc);
	    fflush(stdout);
	  }
	  /* flush so the change is seen! */
	  fflush(stdout);
        }
	/* free the packet */
	free(pkt);
      }
      /* sleep for 100ms (same as NT steps */
      usleep(100000);
    }
    fflush(stdout);
    /* check for all nodes on this channel, and get thier pan id */
    for (i = 0; i < nodes; i++) {
      int first = 1;
      if (addrs[i][8] == ATCHc) {
	xbee_con *tcon;
	unsigned int dh,dl;
	if (first) {
	  printf("%c[2KGathering settings for nodes on channel 0x%02X...\r",27,ATCHc);
	  first = 0;
	}
	/* get the address, and turn it the right way up! */
	memcpy(&dh,&(addrs[i][0]),4);
	dh = ((dh & 0xFF) << 24) | ((dh & 0xFF00) << 8) | ((dh & 0xFF0000) >> 8) | ((dh & 0xFF000000) >> 24);
	memcpy(&dl,&(addrs[i][4]),4);
	dl = ((dl & 0xFF) << 24) | ((dl & 0xFF00) << 8) | ((dl & 0xFF0000) >> 8) | ((dl & 0xFF000000) >> 24);
	/* setup a connection the the remote node */
	if ((tcon = xbee_newcon('I',xbee_64bitRemoteAT,dh,dl)) != NULL) {
	  /* find the line to edit */
	  printf("\r%c[%dA",27,nodes-i+1);

	  /* in this case we dont care if we dont get a response packet... */
	  if ((rpkt = xbee_senddata(tcon,"ID")) != NULL) {
	    /* move over the ID column */
	    printf("\r%c[18C",27);
	    /* print the ID */
	    printf("%c[32m%02X%02X%c[0m",27,rpkt->data[0],rpkt->data[1],27);
	    addrs[i][9] = rpkt->data[0];
	    addrs[i][10] = rpkt->data[1];
	    /* turn on the flag */
	    addrs[i][18] |= 0x80;
	    free(rpkt);
	  }

	  /* in this case we dont care if we dont get a response packet... */
	  if ((rpkt = xbee_senddata(tcon,"BD")) != NULL) {
	    /* move over the BD column */
	    printf("\r%c[80C",27);
	    if ((rpkt->data[0] != 0x00) || (rpkt->data[1] != 0x00) || (rpkt->data[2] != 0x00) || ((rpkt->data[3] & 0xF8) != 0x00)) {
	      addrs[i][11] = 8;
	    } else {
	      addrs[i][11] = rpkt->data[3];
	    }
	    /* turn on the flag */
	    addrs[i][18] |= 0x40;
	    /* print the baud rate */
	    printf("%c[32m",27);
	    switch (addrs[i][11]) {
	    case 0:  printf("  1200"); break;
	    case 1:  printf("  2400"); break;
	    case 2:  printf("  4800"); break;
	    case 3:  printf("  9600"); break;
	    case 4:  printf(" 19200"); break;
	    case 5:  printf(" 38400"); break;
	    case 6:  printf(" 57600"); break;
	    case 7:  printf("115200"); break;
	    default: printf(" other"); break;
	    }
	    printf("%c[0m",27);
	    free(rpkt);
	  }
	  /* in this case we dont care if we dont get a response packet... */
	  if ((rpkt = xbee_senddata(tcon,"AP")) != NULL) {
	    /* move over the AP column */
	    printf("\r%c[96C",27);
	    /* print the ID */
	    printf("%c[32m%02X%c[0m",27,rpkt->data[0],27);
	    addrs[i][12] = rpkt->data[0];
	    /* turn on the flag */
	    addrs[i][18] |= 0x20;
	    free(rpkt);
	  }
	  /* in this case we dont care if we dont get a response packet... */
	  if ((rpkt = xbee_senddata(tcon,"HV")) != NULL) {
	    /* move over the HV column */
	    printf("\r%c[108C",27);
	    /* print the ID */
	    printf("%c[32m%02X%02X%c[0m",27,rpkt->data[0],rpkt->data[1],27);
	    addrs[i][13] = rpkt->data[0];
	    addrs[i][14] = rpkt->data[1];
	    /* turn on the flag */
	    addrs[i][18] |= 0x10;
	    free(rpkt);
	  }
	  /* in this case we dont care if we dont get a response packet... */
	  if ((rpkt = xbee_senddata(tcon,"VR")) != NULL) {
	    /* move over the VR column */
	    printf("\r%c[122C",27);
	    /* print the ID */
	    printf("%c[32m%02X%02X%c[0m",27,rpkt->data[0],rpkt->data[1],27);
	    addrs[i][15] = rpkt->data[0];
	    addrs[i][16] = rpkt->data[1];
	    /* turn on the flag */
	    addrs[i][18] |= 0x08;
	    free(rpkt);
	  }
	  /* in this case we dont care if we dont get a response packet... */
	  if ((rpkt = xbee_senddata(tcon,"CC")) != NULL) {
	    /* move over the CC column */
	    printf("\r%c[134C",27);
	    /* print the ID */
	    printf("%c[32m'%c' (0x%02X)%c[0m",27,rpkt->data[0],rpkt->data[0],27);
	    addrs[i][17] = rpkt->data[0];
	    /* turn on the flag */
	    addrs[i][18] |= 0x04;
	    free(rpkt);
	  }
	  /* go back the the bottom */
	  printf("%c[%dB\r",27,nodes-i+1);
	  fflush(stdout);
	}
      }
    }
    /* fall back to the first channel if that was the last */
    if (XBeePro && ATCHc == 0x17) {
      ATCHc = 0x0C;
    } else if (!XBeePro && ATCHc == 0x1A) {
      ATCHc = 0x0B;
    } else {
      /* else move onto next channel */
      ATCHc++;
    }
    usleep(100000);
  }

  return 0;
}

