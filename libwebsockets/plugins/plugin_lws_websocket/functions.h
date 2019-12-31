#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

unsigned char checksum(unsigned char *ptr, size_t sz) {
  unsigned char chk = 0;

  while (sz-- != 0) {
    chk += *ptr++;
  }
  return chk;
}

int findLastIndex(char *str, char x) {
  size_t len = strlen(str);
  int index = -1;

  for (int i = 0; i < len; i++) {
    if (str[i] == x) {
      index = i;
    }
  }
  return index + 1; //to get the length of the index in the string
}

unsigned int hexToInt(const char *hex) {
  unsigned int result = 0;

  while (*hex) {
    if (*hex > 47 && *hex < 58)
      result += (*hex - 48);
    else if (*hex > 64 && *hex < 71)
      result += (*hex - 55);
    else if (*hex > 96 && *hex < 103)
      result += (*hex - 87);

    if (*++hex)
      result <<= 4;
  }

  return result;
}

bool isCRCValid(int packetType, unsigned char *packet) {
  bool isValid = false;
  unsigned char str[5];
  // index to packet without CRC
  int lastindex = findLastIndex((char*)packet, ',');
  size_t len = strlen((char*)packet);
  // calc CRC
  unsigned char checkSum = checksum(packet, lastindex);

  // get CRC from packet to compare with calc CRC
  memcpy(str, &packet[lastindex],len - lastindex); // len - lastIndex to get the CRC from packet 
  str[len - lastindex] = '\0'; 
  printf("string %s\n", str); int
  crcInPacket = atoi((char*)str); // string to number 
  printf("el numero es :%d\n", crcInPacket);

  if (packetType == 7) { // packetType = PC_NOTES
    int temp = strcmp("EOT\r\n", (char*)str);
    if (strcmp("EOT\r\n", (char*)str) == 0) {
      printf("comparacion identicos --> %d", temp);
      return true;
    }
  }

  if (crcInPacket == checkSum) {
    printf("los CRC coinciden\n");
    return true;
  }
  return false;
}

char uCalTCPCheckSum(char *tuBuf, int tuIndex) {
    char tuCnt,tuCheckSum = 0;
    for(tuCnt = 0; tuCnt < tuIndex; tuCnt++) {
        tuCheckSum = tuCheckSum + tuBuf[tuCnt];
    }
    return(tuCheckSum);
}

int uVerifyCheckSum(char *tuBuf) {
    char tuCheckSumIndex = 0, tuCalCheckSum , tuIndex, tuRcvdCheckSum;
    for(tuIndex = 0; tuBuf[tuIndex]; tuIndex++) {
        if(tuBuf[tuIndex] == ',') {
          tuCheckSumIndex = tuIndex + 1;
        }
    }

    tuCalCheckSum = uCalTCPCheckSum(tuBuf,tuCheckSumIndex);
        //fprintf(stderr,"\ninside function Calculated Chksm =%d\n",tuCalCheckSum);
    tuRcvdCheckSum = (char)atoi(&tuBuf[tuCheckSumIndex]);
        //fprintf(stderr,"\ninside function Rcvd Chksm = %d\n",tuRcvdCheckSum);
    if(tuRcvdCheckSum == tuCalCheckSum) {
      return 1;
    } else{
      return 0;
    }

}

char** str_split(char* a_str, const char a_delim) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    // Count how many elements will be extracted.
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    // Add space for trailing token.
    count += last_comma < (a_str + strlen(a_str) - 1);

    //Add space for terminating null string so caller
    //knows where the list of returned strings ends.
    count++;

    result = (char**)malloc(sizeof(char*) * count);

    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    return result;
}


char* concat(const char *s1, const char *s2) {
    char *result = (char*)malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    //you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
 

char *getRedisCommand(const char *host, const char *port, const char *key,const char *packet) {
  fprintf(stderr,"=========================================================\n");
  fprintf(stderr,"===============REDIS COMMAND=============================\n");
  fprintf(stderr,"=========================================================\n");
  char *temp = (char *)malloc(sizeof(char) * 200);
  if (!temp) {
    return NULL;
  }
  memset(temp, 0, sizeof(char) * 200);

  const char path[] = "/usr/local/bin/redisClient -h ";
  strcat(temp, path);
  strcat(temp, host);
  strcat(temp, " -p ");
  strcat(temp, port);
  strcat(temp, " -k ");
  strcat(temp, key);
  strcat(temp, " -v ");
  strcat(temp, packet);
  strcat(temp, " &");
  fprintf(stderr, "the redis URL connection is: %s\n", temp);
  return temp;
}

char *getHttpCommand(const char *host, const char *packet) {
  fprintf(stderr,"*********************************************************\n");
  fprintf(stderr,"**************HTTP REQUEST*******************************\n");  
  fprintf(stderr,"*********************************************************\n");
  char *temp = (char *)malloc(sizeof(char) * 200);
  if (!temp) {
    return NULL;
  }

  memset(temp, 0, sizeof(char) * 200);
  strcat(temp, "wget --post-data ");
  strcat(temp, "packet=");
  strcat(temp, packet);
  strcat(temp, " ");
  strcat(temp, host);
  strcat(temp, " --connect-timeout=5 --tries=2 -O /dev/null &");
  fprintf(stderr, "the POST COMMAND connection is: %s\n", temp);
  return temp;
}




char* getACK(int pType) {
  char *ack = (char *)malloc(sizeof(char) * 20);
  if (!ack) {
    return NULL;
  }
  memset(ack, 0, sizeof(char) * 20);
  char packetType[4];
  unsigned char chk;
  char chk_[5]; //to store checksum hexa value as a decimal string
  memset(ack, 0, sizeof(char) * 20);
  sprintf(packetType,"%d",pType); //packet number as a string


  strcat(ack, "11,");
  strcat(ack, packetType);
  strcat(ack, ",");
  chk = checksum((unsigned char*)ack, strlen(ack));

  if(chk <= 9) {
    sprintf(chk_,"00%d", chk);  //checksum as a string
  } else if (chk <= 99) {
  sprintf(chk_,"0%d", chk);
  } else {
    sprintf(chk_,"%d", chk);
  }

  strcat(ack, chk_);
  strcat(ack, ",@~!11");
  printf("salida: %s\n",ack);
  return ack;
}


char* getNck(int pType) {
  char *nack = (char *)malloc(sizeof(char) * 20);
  if (!nack) {
    return NULL;
  }
  memset(nack, 0, sizeof(char) * 20);
  char packetType[4];
  unsigned char chk;
  char chk_[5]; //to store checksum hexa value as a decimal string
  memset(nack, 0, sizeof(char) * 20);
  sprintf(packetType,"%d",pType); //packet number as a string


  strcat(nack, "16,");
  strcat(nack, packetType);
  strcat(nack, ",");
  chk = checksum((unsigned char*)nack, strlen(nack));

  if(chk <= 9) {
    sprintf(chk_,"00%d", chk);  //checksum as a string
  } else if (chk <= 99) {
  sprintf(chk_,"0%d", chk);
  } else {
    sprintf(chk_,"%d", chk);
  }

  strcat(nack, chk_);
  strcat(nack, ",@~!16");
  printf("salida: %s\n",nack);
  return nack;
}