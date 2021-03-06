
// System includes
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>

// External includes
#include <gpOS.h>
#include <gnss_api.h>
#include <atc.h>
#include <debug.h>
#include <lpal_private.h>

#include <sqat_SQNSUPLNTF.h>
#include <sqat_SQNSUPLNTFSESSION.h>
#include <sqat_SQNSUPLSETPOSITION.h>
#include <sqat_CCLK.h>
#include <sqat_SOCKCREAT.h>
#include <sqat_SOCKCONN.h>
#include <sqat_SOCKCLOSE.h>
#include <sqat_SOCKREAD.h>
#include <sqat_SOCKWRITE.h>

// Internal includes
#include "supl_handler.h"

///md5 include
#include<md5.h>
#include<aes.h>
/// this is working 
//// base64
#include<base64.h>

#include <sqat_CEREG1.h>

/*****************************************************************************
   defines and macros (scope: module-local)
*****************************************************************************/
#define PVT_APP_TASK_STACK_SIZE   1000   /* size of the sample application task stack */
#define MS_TO_KPH     (3600.0 / 1000.0)     /* define used for conversion */

/*****************************************************************************
   global variable definitions
*****************************************************************************/
tInt pvt_app_task_priority = 0;   /* task priority value (value set arbitrary here)*/
#if defined (DEMO_USE_FREERTOS_API)
static TaskHandle_t xFrPVTAppProcess;
#endif

/*****************************************************************************
   function prototypes (scope: module-local)
*****************************************************************************/
/* Function which is called by "PVT_app_task" */
#if defined (DEMO_USE_FREERTOS_API)
static void pvt_app_process( void *pvParameters );
#else
static gpOS_task_exit_status_t pvt_app_process( void *p );
#endif

/*****************************************************************************
   function implementations
*****************************************************************************/
/* Function which is called by "PVT_app_task" */

//// my code

enum {
  SUPL_PROCESS_TASK_STACK_SIZE = 8192,
  SUPL_PROCESS_TASK_PRIORITY = 15,
};




struct gps{
    float lat;
    float lon;
};

struct Packet{
    uint8_t packet_version;
    uint8_t battery_level;
    int8_t temperature;
    uint64_t timestampe;
    uint16_t gps_lock_time;
    uint16_t lte_time_to_attach;;
    uint16_t diagnostic_events;
    uint16_t location_area_code;
    uint32_t cell_tower_id;
    uint8_t rssi;
    uint8_t operation_mode;
    uint8_t flags;
    uint8_t payload_contents;
};


char *str2md5(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    return out;
}


void padding(char * in, char *out)
{
  strncpy(out,in,4);
  int a;
  for(a = 4;a < 16;a++)
  {
    out[a] = 12;
  }
}

void substring(char s[], char sub[], int p, int l) {
   int c = 0;

   while (c < l) {
      sub[c] = s[p+c-1];
      c++;
   }
   sub[c] = '\0';
}

char ascii_to_binary(char in)
{
    char ou;
    if(in >= 48 && in<= 57)
    {
        ou = in - 48;
    }
    else if(in >= 65 && in<= 70)
    {
        ou = in - 55;
    }
    else
    {
        ou = in - 87;
    }
    return ou;
}

void hex_string_to_char_string(unsigned char *inp, unsigned char *out)
{
    int b = 0;
    char in,ou;
    char a;
    for(a = 0; a<strlen(inp);a=a+2)
    {
        in = ascii_to_binary(inp[a]);
        out[b] = in << 4;
        ou = out[b];
        in = ascii_to_binary(inp[a+1]);
        out[b] += in;
        ou = out[b];
        b++;
    }
}

//print string in hex
void print_hex(const char *s)
{
  while(*s)
    printf("%02x", (unsigned int) *s++);
  printf("\n");
}

void pkcs5_padding(unsigned char *in, unsigned char *ou, unsigned char key_size)
{
    unsigned char a = key_size - strlen(in);
    unsigned char diff = key_size - a;
    unsigned char d;
    strcpy(ou,in);
    unsigned char dd[16];
    strncpy(dd,in,diff);
    for (d = diff; d<key_size;d++)
    {
        ou[d] = a;
    }
    ou[d] = '\0';
}


void string_to_hex(char * in, char *out)
{
    int i,j;
    for(i=0,j=0;i<strlen(in);i++,j+=2)
    {
        sprintf((char*)out+j,"%02X",in[i]);
    }
    out[strlen(in)*2]='\0';
}


struct gps{
    float lat;
    float lon;
};

void header_data_conv(char *in, char * out)
{
  unsigned char c;
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
  out[3] = in[8];
  out[4] = in[9];
  out[5] = in[10];
  out[6] = in[11];
  out[7] = in[12];
  out[8] = in[13];
  out[9] = in[14];
  out[10] = in[15];
  out[11] = in[16];
  out[12] = in[17];
  out[13] = in[18];
  out[14] = in[19];
  out[15] = in[20];
  out[16] = in[21];
  out[17] = in[22];
  out[18] = in[23];
  out[19] = in[24];
  out[20] = in[25];
  out[21] = in[26];
  out[22] = in[27];
  out[23] = in[28];
  out[24] = in[29];
  out[25] = in[30];
  out[26] = in[31];
}


void gps_data_conv(char *in, char * out)
{
  in[0] = out[0];
  in[1] = out[1];
  in[2] = out[2];
  in[3] = out[3];
  in[4] = out[4];
  in[5] = out[5];
  in[6] = out[6];
  in[7] = out[7];
}

//! Byte swap unsigned short
uint16_t swap_uint16( uint16_t val )
{
    return (val << 8) | (val >> 8 );
}

//! Byte swap short
int16_t swap_int16( int16_t val )
{
    return (val << 8) | ((val >> 8) & 0xFF);
}

//! Byte swap unsigned int
uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
}

//! Byte swap int
int32_t swap_int32( int32_t val )
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

int64_t swap_int64( int64_t val )
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
}
uint64_t swap_uint64( uint64_t val )
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}

//! Byte swap float
float ReverseFloat( const float inFloat )
{
   float retVal;
   char *floatToConvert = ( char* ) & inFloat;
   char *returnFloat = ( char* ) & retVal;

   // swap the bytes into a temporary buffer
   returnFloat[0] = floatToConvert[3];
   returnFloat[1] = floatToConvert[2];
   returnFloat[2] = floatToConvert[1];
   returnFloat[3] = floatToConvert[0];

   return retVal;
}


struct gps gpsData(void)
{
  gpOS_task_delay(1 * gpOS_timer_ticks_per_sec());
  struct gps gps1 = {0};
  if (gnss_fix_get_pos_status() != NO_FIX)      /* Check if a 2D or 3D Fix is available */
       {
        position_t *user_position;
        velocity_t *user_velocity;
        tInt     lat_deg, lon_deg, lat_min, lon_min, lat_min_frac, lon_min_frac, week, hh, mm, ss, ms;
        char    lat_sense, lon_sense;
        tUInt   cpu_time;
        tDouble tow, course, speed;

        user_position = gnss_fix_get_fil_pos();        /* get user position */
        user_velocity = gnss_fix_get_fil_vel();        /* get user velocity */
        gnss_fix_get_time( &week, &tow, &cpu_time);    /* get time data */

         /* Note: to convert real degrees in "understandable" values for latitude and longitude -> uses existing NMEA support function to avoid code duplication.
                  But this conversion could be done locally with another user defined function. */
         nmea_support_degrees_to_int( user_position->latitude, 'N', 'S', 5, &lat_deg, &lat_min, &lat_min_frac, &lat_sense );
         DEBUG_MSG_CLOE(( "[PVT_app]user position -> latitude: %03d%c%02d.%03d\r\n", lat_deg, lat_sense, lat_min, lat_min_frac));         /* Display user latitude */
         nmea_support_degrees_to_int( user_position->longitude, 'E', 'W', 5, &lon_deg, &lon_min, &lon_min_frac, &lon_sense);
         DEBUG_MSG_CLOE(( "[PVT_app]user position -> longitude: %03d%c%02d.%03d\r\n", lon_deg, lon_sense, lon_min, lon_min_frac));        /* Display user longitude */

         DEBUG_MSG_CLOE(( "[PVT_app]user position -> height: %03.2f\r\n", user_position->height));                                        /* Display user height */

         gnss_conv_vel_to_course_speed(user_velocity, &course, &speed);                                                                    /* Convert velocity E/N into speed */
         DEBUG_MSG_CLOE(( "[PVT_app]user speed -> %03.3f km/h, %03.3f m/s\r\n", speed* MS_TO_KPH, speed));                              /* Display user speed */

         gnss_get_utc_time(tow, &hh, &mm, &ss, &ms);                                                                                       /* Compute UTC time */
         DEBUG_MSG_CLOE(( "[PVT_app] week: %04d UTC time: %2d:%02d:%02d\r\n", week, hh, mm, ss));
         tDouble lat = user_position->latitude;
         tDouble lon = user_position->longitude;
         gps1.lat = ReverseFloat(lat);
         gps1.lon = ReverseFloat(lon);
         return gps1;
       }
       else
       {
         DEBUG_MSG_CLOE(( "[PVT_app] No acquired GNSS FIX yet. \r\n"));
         gps1.lat = ReverseFloat(40.525733947753906);
         gps1.lon = ReverseFloat(-74.39092254638672);
         return gps1;
       }
}

size_t encryptingAndEncodingNonce(char *nonce, char* buf)
{
    char Iv[17];
    char *hash = str2md5(nonce, strlen(nonce));
    strncpy(Iv,hash,16);
    Iv[16] = '\0';
    DEBUG_LOG("IV: %s\r\n", Iv);
    char temp[100];
    char padded_nonce[17];
    padding(nonce,padded_nonce);
    padded_nonce[16]='\0';
    string_to_hex(padded_nonce,temp);
    DEBUG_LOG("Padded Nonce: %s\r\n", temp);
    WORD key_schedule[60];
    //BYTE enc_buf1[128];
    BYTE enc_buf[128];

    BYTE key[16] = {0x33,0x35,0x34,0x33,0x34,0x38,0x30,0x39,0x32,0x39,0x38,0x36,0x34,0x38,0x33,0x30};
    //BYTE key2[17] = {0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41};
    int pass = 1;
    key[16] = '\0';


    aes_key_setup(key, key_schedule, 128);
    //string_to_hex(key,temp);
    //DEBUG_LOG("key: %s\r\n", temp);
    aes_encrypt_cbc(padded_nonce, 16, enc_buf, key_schedule, 128, Iv);

    //string_to_hex(padded_nonce,temp);
    //DEBUG_LOG("Plaintext: %s\r\n", temp);
    char enc[16];
    strncpy(enc,enc_buf,16);
    enc_buf[16] = '\0';
    string_to_hex(enc_buf,temp);
    //DEBUG_LOG("Encrypted: %s\r\n", temp);

    size_t buf_len = base64_encode(enc_buf, buf, strlen(enc_buf), 1);
    //DEBUG_LOG("length: %d\r\n",buf_len);
    buf[buf_len] = '\0';
    return buf_len;
}



////
static gpOS_task_exit_status_t pvt_app_process(void *p)
{
    /////////////////////start space//////////////////////////
    gpOS_task_delay( 2*gpOS_timer_ticks_per_sec()); /* Wait for 2s */
    struct atcChannel* channel = (struct atcChannel*)p;
    hatrStatus_t status;
    char outp[100]="";
    DEBUG_LOG("%s :: Started", __FUNCTION__);
    atcEnableFlowControl(channel);
    eat_pCEREG_read(channel->privatedata,outp);
    atcDisableFlowControl(channel);

    DEBUG_LOG("Output : %s",outp);
    gpOS_task_delay(2*gpOS_timer_ticks_per_sec());
    DEBUG_LOG("Done starting up\r\n");

    /////////////////////////////////////////////////////////////

    ////////////////////initializations
    //char IMEI[] = "354348092986483";
    DEBUG_LOG("%s :: Started", __FUNCTION__);
    gpOS_task_delay(1*gpOS_timer_ticks_per_sec());
    /// variable initializations
    int a = 0;
    gpOS_task_delay(1*gpOS_timer_ticks_per_sec());
    unsigned char nonce[4] = "";
    unsigned char recv_nonce_hex[8] = "";
    unsigned char header[27] = "";
    unsigned char gps_data[8] = "";
    unsigned char data_main [sizeof(header)+sizeof(gps_data)];

    char buf[512];
    size_t buf_len;

    char temp_send[100] = "";
    char recv_data[30] = "";
    char data[100] = "";
    char data_send[100] = "";
    struct Packet packet = {0};
    struct gps gps1 = {0};
    float lon,lat;
    int i;
    unsigned char *buffer;
//////////////////////////////////////
    while ( TRUE )
    {
        gpOS_task_delay( 2*gpOS_timer_ticks_per_sec()); /* Wait for 2s */
        atcEnableFlowControl(channel);
        eat_pCEREG_read(channel->privatedata,outp);
        atcEnableFlowControl(channel);

        DEBUG_LOG("Output : %s",outp);
        DEBUG_LOG("cell_tower_id : %d",cell_tower_id(outp));
        DEBUG_LOG("location_are_code : %d",location_area_code(outp));

        socketCreation(channel->privatedata,outp);

        connectionSetup(channel->privatedata,outp);

        gpOS_task_delay( 1*gpOS_timer_ticks_per_sec()); /* Wait for 2s */
        readingSocketNonce(channel->privatedata,outp,recv_nonce_hex);

        hex_string_to_char_string(recv_nonce_hex,nonce);
        nonce[4]='\0';
        buf_len = encryptingAndEncodingNonce(nonce,buf);
        DEBUG_LOG("Encoded: %s length: %d\r\n", buf,buf_len);

        strcpy(temp_send,"354348092986483");
        DEBUG_LOG("IMEI f: %s \r\n",temp_send);
         strcat(temp_send,buf);
        temp_send[15+24] = '\0';

        DEBUG_LOG("Payload : %s \r\n",temp_send);
        string_to_hex(temp_send,data_send);
        DEBUG_LOG("Payload converted to hex: %s\r\n", data_send);
        sprintf(data,"AT@SOCKWRITE=1,%d,\"%s\"",strlen(temp_send),data_send);
        DEBUG_LOG("Size : %s , %d \r\n",data,strlen(data));
        status = eat_pSOCKWRITE_read(channel->privatedata,data,strlen(data),recv_data);
        DEBUG_LOG("No of bytes written : %s \r\n",recv_data);
        DEBUG_LOG("Status : %d \r\n",status);

        packet.packet_version = 3;
        packet.battery_level = 0;
        packet.temperature = 42;
        packet.timestampe = 0;
        packet.gps_lock_time = 3;
        packet.lte_time_to_attach = swap_uint16(65535);
        packet.diagnostic_events = swap_uint16(6);
        packet.location_area_code = location_area_code(outp);//swap_uint16(5809);
        packet.cell_tower_id = cell_tower_id(outp);//swap_uint32(1599101);
        packet.rssi = 23;
        packet.operation_mode = 3;
        packet.flags = 0;
        packet.payload_contents = 4;
        gps1 = gpsData();

        //declare character buffer (byte array)
        buffer = (unsigned char*)malloc(sizeof(struct Packet));
        memset(buffer,0,sizeof(struct Packet));
        memcpy(buffer,(const unsigned char*)&packet,sizeof(packet));
        header_data_conv(buffer,header);
        free(buffer);


        buffer=(unsigned char*)malloc(sizeof(struct gps));
        memset(buffer,0,sizeof(struct gps));
        memcpy(buffer,(const unsigned char*)&gps1,sizeof(gps1));
        gps_data_conv(gps_data,buffer);
        free(buffer);

        memcpy(data_main,header,sizeof(header));
        memcpy(data_main + sizeof(header), gps_data,sizeof(gps_data));
        DEBUG_LOG("Copied byte array is:\n");
        /*for(i=0;i<sizeof(data_main);i++)
         DEBUG_LOG("%02X ",data_main[i]);
        char sample[] = "03 00 2a 00 00 00 00 00 00 00 85 00 03 ff ff 06 00 16 b1 00 18 66 7d 17 03 00 0c 42 22 1a 5a c2 94 c8 27 08 00 e3 00 f7 00";
        DEBUG_LOG("%s\n",sample);*/

        memset(buf,0,sizeof(buf));
        buf_len = base64_encode(data_main, buf, sizeof(data_main), 1);
        //DEBUG_LOG("length: %d\r\n",buf_len);
        buf[buf_len] = '\0';
        DEBUG_LOG("Encoded: %s length: %d\r\n", buf,buf_len);
        //DEBUG_LOG("Payload : %s \r\n",buf);

        string_to_hex(buf,data_send);
        // DEBUG_LOG("Payload converted to hex: %s\r\n", data_send);
        sprintf(data,"AT@SOCKWRITE=1,%d,\"%s\"",buf_len,data_send);
        DEBUG_LOG("Size : %s , %d \r\n",data,strlen(data));
        status = eat_pSOCKWRITE_read(channel->privatedata,data,strlen(data),recv_data);
        DEBUG_LOG("No of bytes written : %s \r\n",recv_data);
        //DEBUG_LOG("Status : %d \r\n",status);
        gpOS_task_delay(1 * gpOS_timer_ticks_per_sec());


        closingSocket(channel->privatedata);
        /////
        DEBUG_LOG("Done\r\n");
        gpOS_task_delay(5 * gpOS_timer_ticks_per_sec());
    }
}

/* Creation of "PVT_app_task" */
void pvt_app_init(const struct atcChannel *atc)
{
  gpOS_task_create_p( NULL, pvt_app_process, (void*)atc, SUPL_PROCESS_TASK_STACK_SIZE, SUPL_PROCESS_TASK_PRIORITY, "PVT_app_task", gpOS_TASK_FLAGS_ACTIVE );
}


