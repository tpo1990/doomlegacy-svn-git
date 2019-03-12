// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_netfil.c 1374 2017-12-18 17:22:05Z wesleyjohnson $
//
// Copyright (C) 1998-2016 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// $Log: d_netfil.c,v $
// Revision 1.26  2004/04/20 00:34:26  andyp
// Linux compilation fixes and string cleanups
//
// Revision 1.25  2003/05/04 04:28:33  sburke
// Use SHORT to convert network data between big- and little-endian format.
//
// Revision 1.24  2001/07/28 16:18:37  bpereira
// Revision 1.23  2001/05/21 16:23:32  crashrl
//
// Revision 1.22  2001/05/21 14:57:05  crashrl
// Readded directory crawling file search function
//
// Revision 1.21  2001/05/16 17:12:52  crashrl
// Added md5-sum support, removed recursiv wad search
//
// Revision 1.20  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.19  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.18  2001/03/30 17:12:49  bpereira
// Revision 1.17  2001/02/24 13:35:19  bpereira
// Revision 1.16  2001/02/13 20:37:27  metzgermeister
// Revision 1.15  2001/02/10 12:27:13  bpereira
//
// Revision 1.14  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.13  2000/10/08 13:30:00  bpereira
// Revision 1.12  2000/10/02 18:25:44  bpereira
// Revision 1.11  2000/09/28 20:57:14  bpereira
// Revision 1.10  2000/09/10 10:39:06  metzgermeister
// Revision 1.9  2000/08/31 14:30:55  bpereira
// Revision 1.8  2000/08/11 19:10:13  metzgermeister
//
// Revision 1.7  2000/08/10 14:52:38  ydario
// OS/2 port
//
// Revision 1.6  2000/04/16 18:38:07  bpereira
//
// Revision 1.5  2000/03/07 03:32:24  hurdler
// fix linux compilation
//
// Revision 1.4  2000/03/05 17:10:56  bpereira
// Revision 1.3  2000/02/27 00:42:10  hurdler
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//      Transfer a file using HSendPacket
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <fcntl.h>

#ifdef __OS2__
#include <sys/types.h>
#endif // __OS2__

#include <sys/stat.h>

#include <time.h>

#if defined( WIN32) || defined( __DJGPP__ ) 
#include <io.h>
#include <direct.h>
#else
#include <sys/types.h>
//#include <dirent.h>
#include <utime.h>
#endif

#ifdef __WIN32__
#include <sys/utime.h>
#else
#include <unistd.h>
#endif

#ifdef __DJGPP__
#include <dir.h>
#include <utime.h>
#endif

#include "doomincl.h"
#include "doomstat.h"
#include "d_net.h"
#include "d_netfil.h"
#include "d_clisrv.h"
#include "g_game.h"
#include "i_net.h"
#include "i_system.h"
#include "m_argv.h"
#include "w_wad.h"
#include "z_zone.h"
#include "byteptr.h"
#include "p_setup.h"
#include "m_misc.h"
#include "m_menu.h"
#include "v_video.h"
  // V_DrawString
#include "d_main.h"
  // DOOMWADDIR
#include "md5.h"

// sender structure
typedef struct filetx_s {
    TAH_e    release_tah; // release, access method
    char     *filename;   // name of the file, or ptr to the data
    uint32_t data_size;   // size of data transfer
    char     fileid;
//  byte     dest_node;    // client node destination (UNUSED)
    struct filetx_s *next; // a queue
} filetx_t;

// Current transfers (one for each node).
typedef struct {
   filetx_t  *txlist;    // only set by server
   uint32_t   position;  // file and data transfer position
   FILE*      currentfile;
} transfer_t;

// Only transfer files to player nodes.
static transfer_t transfer[MAXNETNODES];

// read time of file : stat _stmtime
// write time of file : utime

// Client receiver structure
int cl_num_fileneed;
fileneed_t cl_fileneed[MAX_WADFILES];

const char * downloaddir = "DOWNLOAD";

static void SV_SendFile(byte to_node, char *filename, char fileid);

// By server.
// Fill the serverinfo packet with wad files loaded by the game on the server.
byte * Put_Server_FileNeed(void)
{
    int   i;
    byte *p;  // macros want byte*
    char  wadfilename[MAX_WADPATH];

    // Format: Series of file descriptor, number of file in packet field.
    // Dest buff length: fileneed[FILENEED_BUFF_LEN]
    p=(byte *)&netbuffer->u.serverinfo.fileneed;
    for(i=0;i<numwadfiles;i++)
    {
        // Format: filesize int32, filename str0, md5sum 16byte
        WRITEU32(p,wadfiles[i]->filesize);
        strcpy(wadfilename,wadfiles[i]->filename);
        nameonly(wadfilename);
        p = write_string(p, wadfilename);
        WRITEMEM(p,wadfiles[i]->md5sum,16);
    }
    netbuffer->u.serverinfo.num_fileneed = i;  // numwadfiles
    return p;
}

// By Client
// Handle the received serverinfo packet and fill client fileneed table.
void CL_Got_Fileneed(int num_fileneed_parm, byte *fileneed_str)
{
    int i, fn_len;
    byte *p = fileneed_str;
    byte * bufend16 = &fileneed_str[FILENEED_BUFF_LEN - 1 - 16];
    byte * next0;
    // NULL when not found
   
    // Format: Series of file descriptor, number of file as parameter.
    // Src buff length: fileneed[FILENEED_BUFF_LEN]
    // Must have 0 term.
    // Last 16 bytes of content will be md5sum, so cannot just tack on 0.

    cl_num_fileneed = num_fileneed_parm;
    for(i=0; i<cl_num_fileneed; i++)  // MAX_WADFILES
    {
        // Format: filesize int32, filename str0, md5sum 16byte
        fileneed_t * fnp = & cl_fileneed[i];  // client fileneed

        // Protect against malicious packet without 0 term.
        if( p >= bufend16 )  goto bad_packet;  // buffer overrun

        fnp->status = FS_NOTFOUND;
        fnp->totalsize = READU32(p);
        fnp->phandle = NULL;

        // [WDJ] String overflow safe
        next0 = memchr( p, '\0', bufend16 - p );
        if((next0 == NULL) || (next0 > bufend16))
        {
            fnp->filename[0] = '\0';
            goto bad_packet;  // overran last 0 term.
        }
        // Test on next0 guarantees that there is a 0 term.
        fn_len = next0 - p + 1;  // strnlen equiv.
        int read_len = min( fn_len, MAX_WADPATH-1 );  // length safe
        memcpy(fnp->filename, p, read_len);
        fnp->filename[MAX_WADPATH-1] = '\0';
        p += fn_len;  // whole, next0 + 1

        READMEM(p,fnp->md5sum,16);
    }
    return;

bad_packet:
    GenPrintf(EMSG_warn, "Fileneed bad packet\n" );
    return;
}

#define BUFFSIZE 128
// By Client
boolean  CL_waiting_on_fileneed( void )
{
    static byte  stat_cnt2 = 0;
    boolean  waiting = false;
    char b[BUFFSIZE];
    int i;
    int pos = 0;
   
    // [WDJ] Status reporting moved here, consistent file report in a box.
    // Update stats on screen.

    M_DrawTextBox( 2, NETFILE_BOX_Y, 38, 6);
    if(stat_cnt2++ > 4)
    {
        stat_cnt2 = 0;
        // First call in CL_ConnectToServer
        Net_GetNetStat();
    }
    snprintf( b, BUFFSIZE-1, "Download File     %4.2f KBPS", (float)netstat_recv_bps/1024 );
    b[BUFFSIZE-1] = 0;
    V_DrawString (30, NETFILE_BOX_Y+8, 0, b);
   
    for(i=0; i<cl_num_fileneed; i++)
    {
      fileneed_t * fnp = & cl_fileneed[i];
      if(fnp->status==FS_DOWNLOADING || fnp->status==FS_REQUESTED)
      {
        waiting = true;
        // Display the actively loading files, in the box, first 4.
        if( pos < (8*4) )
        {
          char * f = strchr( fnp->filename, '/' );
          f = (f)? f+1 : fnp->filename;
          snprintf( b, BUFFSIZE-1, "%-40s  %4i  %4i",
              f, fnp->totalsize>>10, fnp->bytes_recv>>10 );
          b[BUFFSIZE-1] = 0;
          V_DrawString (12, NETFILE_BOX_Y+16+pos, 0, b );
          pos += 8;
        }
      }
    }

    if( verbose>1 && ! waiting )
    {
        // Print final Stat
        GenPrintf(EMSG_info, "Download speed  %4.2f KBPS",
                        ((float)netstat_recv_bps)/1024);
    }
    return waiting;
}

// By Client.
// First step in join game, fileneed and savegame.
void CL_Prepare_Download_SaveGame(const char *tmpsave)
{
    cl_num_fileneed = 1;
    cl_fileneed[0].status = FS_REQUESTED;
    cl_fileneed[0].totalsize = -1;
    cl_fileneed[0].phandle = NULL;
    memset(cl_fileneed[0].md5sum, 0, 16);
    strcpy(cl_fileneed[0].filename, tmpsave);
}


// By Client.
// Send to the server the names of requested files.
// Files who status is FS_NOTFOUND in the fileneed table are sent.
// Return RFR_success when request succeeds.
reqfile_e  Send_RequestFile(void)
{
    char filetmp[ MAX_WADPATH ];
    int   i, fcnt;
    uint64_t  availablefreespace;
    uint32_t  totalfreespaceneeded=0;
    fileneed_t * fnp;

    if( M_CheckParm("-nodownload")
      || cv_downloadfiles.value == 1 )  // no download
    {
        int j;
        int len;
        char s[1024];

        // Check for missing files.
        s[0]=0;
        for(i=0; i<cl_num_fileneed; i++)
        {
            fnp = & cl_fileneed[i];
            if( fnp->status!=FS_FOUND )
            {
                len = strlen(s);
                if( len > (sizeof(s)-80) )  break;  // prevent buffer overrun

                strcat(s,"  \"");
                strcat(s,fnp->filename);
                strcat(s,"\"");
                switch(fnp->status)
                {
                 case FS_NOTFOUND:
                    strcat(s," not found");
                    break;
                 case FS_MD5SUMBAD:
                    strcat(s," has wrong md5sum, needs: ");

                    for(j=0; j<16; j++)
                    {
                        sprintf(&s[len],"%02x", fnp->md5sum[j]);
                        len += 2;
                    }
                    s[len]='\0';
                    break;
                 case FS_OPEN:
                    strcat(s," found, ok");
                    break;
                 default:
                    strcat(s, " unknown reason");
                    break;
                }
                strcat(s,"\n");
            }
        }
        if( s[0] == 0 )
            return RFR_success;    // All files are satisfied

        // This error message needs to be with s[].
        I_SoftError("To play with this server you should have these files:\n"
                    "%s\n"
                    "Remove -nodownload if you want to get them from the server!\n",
                     s );
        return RFR_nodownload;
    }


    // prepare to download
    I_mkdir(downloaddir,0755);
    // Make up one or more request packet to get files from the server.
    fcnt = 0;
    do{
        // Format: one or more file requests.
        //   byte    fileid;  // 0xFF = terminate
        //   string  filename;  // 0 term
        byte *p = netbuffer->u.bytepak.b;
        int bcnt = 0;
        for(; fcnt<cl_num_fileneed; fcnt++)
        {
            fnp = & cl_fileneed[fcnt];
            if( fnp->status == FS_NOTFOUND || fnp->status == FS_MD5SUMBAD)
            {
                if( fnp->status == FS_NOTFOUND )
                    totalfreespaceneeded += fnp->totalsize;
                strcpy( filetmp, fnp->filename );
                nameonly(filetmp);

                // [WDJ] Check for net buffer overflow.
                int nxt_bcnt = bcnt + strlen( filetmp ) + 1;
                if( nxt_bcnt > MAX_NETBYTE_LEN-1 )  break;
                bcnt = nxt_bcnt;

                WRITECHAR(p,fcnt);  // fileid
                p = write_string(p, filetmp);

                // put it in download dir 
                cat_filename( fnp->filename, downloaddir, filetmp );
                fnp->status = FS_REQUESTED;
            }
        }
        WRITECHAR(p,-1);
        netbuffer->packettype = PT_REQUESTFILE;

        if( bcnt == 0 )  goto broken_format;  // broken, filename too long

        availablefreespace = I_GetDiskFreeSpace();
        // debug_Printf("free byte %d\n",availablefreespace);
        if(totalfreespaceneeded > availablefreespace)  goto insufficient_space;
	   
        if( ! HSendPacket(servernode, true, 0, p - netbuffer->u.bytepak.b) )
            return RFR_send_fail;
       
    }while( fcnt<cl_num_fileneed );
    return RFR_success;

broken_format:       
    I_SoftError("Cannot form File Request for file %s\n", filetmp );
    return RFR_nodownload;
       
    // Rare errors
insufficient_space:
    I_SoftError("To play on this server you should download %dKb\n"
                "but you have only %dKb freespace on this drive\n",
                totalfreespaceneeded, availablefreespace);
    return RFR_insufficient_space;
}

// By Server.
// PT_REQUESTFILE
// Received request filepak. Put the files to the send queue.
void Got_RequestFilePak(byte nnode)
{
    char *p = (char *)netbuffer->u.bytepak.b;

    while((byte)*p!=0xFF)
    {
        SV_SendFile(nnode, p+1, *p);
        p++; // skip fileid
        SKIPSTRING(p);
    }
}


// By Client.
// Check if the fileneed from the server are already loaded or on the disk.
checkfiles_e  CL_CheckFiles(void)
{
    int  ret;
    int  i,j;
    fileneed_t * fnp;
    char wadfilename[MAX_WADPATH];

    if( M_CheckParm("-nofiles") )
        return CFR_no_files;

    // The first fileneed is the iwad (the main wad file).
    // Do not check file date.
    strcpy(wadfilename, wadfiles[0]->filename);
    nameonly(wadfilename);
    if( strcasecmp(wadfilename, cl_fileneed[0].filename) != 0 )
    {
        M_SimpleMessage(va("You cannot connect to this server\n"
                          "since it uses %s\n"
                          "You are using %s\n",
                          cl_fileneed[0].filename, wadfilename));
        return CFR_iwad_error;
    }
    cl_fileneed[0].status=FS_OPEN;

    ret = CFR_all_found;
    for (i=1; i<cl_num_fileneed; i++)
    {
        fnp = &cl_fileneed[i];
        if(devparm)
            GenPrintf(EMSG_dev, "searching for '%s' ", fnp->filename);
        
        // check in already loaded files
        for(j=1;wadfiles[j];j++)
        {
            strcpy(wadfilename,wadfiles[j]->filename);
            nameonly(wadfilename);
            if( strcasecmp(wadfilename, fnp->filename)==0
                && !memcmp(wadfiles[j]->md5sum, fnp->md5sum, 16))
            {
                if(devparm)
                   GenPrintf(EMSG_dev, "already loaded\n");
                fnp->status = FS_OPEN;
                break;
            }
        }
        if( fnp->status!=FS_NOTFOUND )
           continue;

        // Net security permissions.
        fnp->status = findfile(fnp->filename, fnp->md5sum, true,
                               /*OUT*/ fnp->filename);
        if(devparm)
            GenPrintf(EMSG_dev, "found %d\n", fnp->status);
        if( fnp->status != FS_FOUND )
            ret = CFR_download_needed;
    }
    return ret;
}

// By Client.
// Load unsatisfied fileneed now.
// Return false when have a failure.
boolean  CL_Load_ServerFiles(void)
{
    int i;
    fileneed_t * fnp;

    // File 0 is the game IWAD.
    for (i=1; i<cl_num_fileneed; i++)
    {
        fnp = & cl_fileneed[i];
        if( fnp->status == FS_OPEN )
        {
            // already loaded
            continue;
        }
        else
        if( fnp->status == FS_FOUND )
        {
            P_AddWadFile(fnp->filename,NULL);
            fnp->status = FS_OPEN;
        }
        else
        if( fnp->status == FS_MD5SUMBAD) 
        {
            P_AddWadFile(fnp->filename,NULL);
            fnp->status = FS_OPEN;
            CONS_Printf("\2File %s found but with differant md5sum\n", fnp->filename);
        }
        else
        {
            I_SoftError("Try to load file %s with status of %d\n",
                         fnp->filename, fnp->status);
            return false;
        }
    }
    return true;
}

// By Server.
// A little optimization to test if there is a file in the queue.
// Tested by caller of Filetx_Ticker, as enable.
// Only the server can enable it.
int Filetx_file_cnt = 0;

// By Server.
// Send a file to client.
//   to_node : the client node.
static void SV_SendFile(byte to_node, char *filename, char fileid)
{
    filetx_t **q,*p;
    int i;
    char * tx_filename;
    char  wadfilename[MAX_WADPATH];

    q=&transfer[to_node].txlist;
    while(*q) q=&((*q)->next);  // find end of txlist

    p = (filetx_t *)malloc(sizeof(filetx_t));
    if(!p)  goto memory_err;

    tx_filename=(char *)malloc(MAX_WADPATH);
    if(! tx_filename)  goto memory_err;

    p->filename = tx_filename;  // filename buffer owner
    *q = p;  // append filetx to txlist

    strncpy(tx_filename, filename, MAX_WADPATH-1);
    tx_filename[MAX_WADPATH-1] = '\0';
    
    // a minimum of security, can only get file in legacy wad directories
    nameonly(tx_filename);

    // Find the requested file in loaded files.
    for(i=0; wadfiles[i]; i++)
    {
        strcpy(wadfilename,wadfiles[i]->filename);
        nameonly(wadfilename);
        if(strcasecmp(wadfilename, tx_filename)==0)
        {
            // copy filename with full path
            strncpy(tx_filename, wadfiles[i]->filename, MAX_WADPATH-1);
            tx_filename[MAX_WADPATH-1] = '\0';
            goto send_found;
        }
    }
    
    // Not found error handling.
    GenPrintf( EMSG_ver, "Requested file, %s, not found in wadfiles.\n", filename );
    DEBFILE(va("%s not found in wadfiles\n", filename));
   
    // Net security permissions.
    if( findfile( tx_filename, NULL, true, /*OUT*/ tx_filename ) == FS_NOTFOUND )
    {
        // not found
        // don't inform client (probably hacker)
        DEBFILE(va("Client %d request %s : not found\n", to_node, filename));
        *q=NULL;  // remove filetx from txlist
        free(tx_filename);
        free(p);
    }
    return;

    // Found the file.
send_found:   
    GenPrintf( EMSG_ver, "Sending file %s to %d.\n", filename, to_node );
    DEBFILE(va("Sending file %s to %d (id=%d)\n", filename, to_node, fileid));
    p->release_tah=TAH_FILE;
    // size initialized at file open 
    //p->size=size;
    p->fileid=fileid;
    p->next=NULL; // end of list

    Filetx_file_cnt++;
    return;
 
memory_err:
    I_Error("SV_SendFile: Memory exhausted\n");
}

// By Server.
void SV_SendData(byte to_node, byte *data, uint32_t size, TAH_e tah, char fileid)
{
    filetx_t **q,*p;

    q=&transfer[to_node].txlist;
    while(*q) q=&((*q)->next);  // find end of list

    p=(filetx_t *)malloc(sizeof(filetx_t));
    if(!p)  goto memory_err;

    *q = p;  // owner of filetx
    p->release_tah=tah;
    p->filename = (char *)data;
    p->data_size=size;
    p->fileid=fileid;
    p->next=NULL; // end of list

    DEBFILE(va("Sending ram %x( size:%d) to %d (id=%d)\n",
               p->filename, size, to_node, fileid));

    Filetx_file_cnt++;
    return;

memory_err:
    I_Error("SendData: Memory exhausted\n");
}

// By Server.
// Close and release the current transfer of the net node.
//  nnode : net node,  0..(MAXNETNODES-1)
static void SV_EndSend(byte nnode)
{
    transfer_t * tnnp = & transfer[nnode];
    filetx_t *p = tnnp->txlist;  // the transfer list
   
    if( ! p )  // cannot ensure who can call and what state they are in
        return;

    // By Server.
    // Deallocation
    switch (p->release_tah) {
    case TAH_FILE:
        if( tnnp->currentfile )
        {
            fclose( tnnp->currentfile );
        }
        free(p->filename);
        break;
    case TAH_Z_FREE:
        Z_Free(p->filename);
        break;
    case TAH_MALLOC_FREE:
        free(p->filename);
        break;
    case TAH_NOTHING:
        break;
    }
    tnnp->currentfile = NULL;  // transfer file, and fake file
    tnnp->txlist = p->next;  // remove filetx from the list
    free(p);
    // Master transfer status
    Filetx_file_cnt--;
}

// By Server.
// Called by NetUpdate, CL_ConnectToServer, repair_handler.
void Filetx_Ticker(void)
{
    static byte txnode=0;  // net node num, 0..(MAXNETNODES-1)
    byte       nn;  // net node num

    TAH_e      access_tah;
    uint32_t   size;
    int        tcnt;
    int        packet_cnt;
    FILE * fp;  // (ref) tnnp current file
    filetx_pak_t * pak;
    filetx_t   * ftxp;
    transfer_t * tnnp;  // transfer for net node

    if( Filetx_file_cnt == 0 )   goto reject;  // nothing to do
   
    // By Server, only server has Filetx_file_cnt > 0.

    // Packets per tic
    packet_cnt = net_bandwidth/(TICRATE*software_MAXPACKETLENGTH);
    if(packet_cnt==0)
       packet_cnt++;
    // (((stat_sendbytes-nowsentbyte)*TICRATE)/(I_GetTime()-starttime)<(uint32_t)net_bandwidth)

    while( packet_cnt-- && (Filetx_file_cnt > 0) )
    {
        // Round robin, fair share.
        nn = (txnode+1)%MAXNETNODES;
        for( tcnt=0; tcnt<MAXNETNODES; tcnt++ )  // counter
        {
            if(transfer[nn].txlist)
                 goto found;
            nn = (nn+1)%MAXNETNODES;
        }
        // no transfer to do
        goto transfer_not_found;  // no transfer to do

found:
        txnode = nn;
        tnnp = & transfer[nn];  // transfers for the net node
        ftxp = tnnp->txlist;    // list of file/data
        access_tah = ftxp->release_tah;

        fp = tnnp->currentfile;
        if(!fp) // file not already open
        {
            if(access_tah == TAH_FILE)
            {
                // open the file to transfer
                long filesize;

                fp = fopen(ftxp->filename,"rb");
                tnnp->currentfile = fp;  // owner of open file

                if(! fp)
                {
                    perror("FileTx");
                    I_SoftError("FileTx: Cannot open file %s\n",
                                 ftxp->filename);
                    SV_EndSend(nn);
                    continue;
                }

                fseek(fp, 0, SEEK_END);
                filesize = ftell( fp );

                // nobody wants to transfer a file bigger than 4GB!
                // and computers will never need more than 640kb of RAM ;-)
                if(-1 == filesize)  goto file_size_err;

                ftxp->data_size = filesize;
                fseek(fp, 0, SEEK_SET);
            }
            else
            {
                tnnp->currentfile = (FILE *)1;  // faked open flag
            }
            tnnp->position = 0;
        }

        pak=&netbuffer->u.filetxpak;
        size = software_MAXPACKETLENGTH - (FILETX_HEADER_SIZE+PACKET_BASE_SIZE);
        if( ftxp->data_size - tnnp->position < size )
            size = ftxp->data_size - tnnp->position;
        if(access_tah == TAH_FILE)
        {
            if( fread(pak->data,size,1, tnnp->currentfile) != 1 )
                goto file_read_err;
        }
        else
        {
            memcpy(pak->data, &ftxp->filename[ tnnp->position ], size);
        }
        pak->position = tnnp->position;
        // put flag so receiver know the totalsize
        if( tnnp->position + size == ftxp->data_size )
        {
            // End of send file flag.
            pak->position |= 0x80000000;
        }
        pak->position = LE_SWAP32_FAST(pak->position);
        pak->size     = LE_SWAP16_FAST(size);
        pak->fileid   = ftxp->fileid;
        netbuffer->packettype=PT_FILEFRAGMENT;
        // Reliable SEND
        if (!HSendPacket(nn,true,0,FILETX_HEADER_SIZE+size ) )
        { // not sent for some odd reason
            // retry at next call
            if(access_tah == TAH_FILE)
            {
                // Data transfer reposition
                fseek( fp, tnnp->position, SEEK_SET);
            }
            // exit the while (can't send this one why should i send the next ?
            break;
        }
        // Record each fragment of file transfer.
        tnnp->position += size;
        if(tnnp->position == ftxp->data_size)
        {
            // All sent
            SV_EndSend(nn);
        }
    } // while
    return;

    // Rare fatal errors.
file_size_err:
    perror("FileTx");
    I_SoftError("FileTx: Error getting filesize of %s\n", ftxp->filename);
    SV_EndSend(nn);
    goto reject;

file_read_err:
    perror("FileTx");
    I_SoftError("Filetx: Read err on %s at %d of %d bytes\n",
                 ftxp->filename, tnnp->position, size);
    goto reject;

transfer_not_found:
    I_SoftError("Filetx: Filetx_file_cnt=%d but Filetx file not found\n", Filetx_file_cnt);
    Filetx_file_cnt = 0;
    goto reject;

reject:
    return;
}


// Called by Net_Packet_Handler, unknown_host_handler.
void Got_Filetxpak(void)
{
    static int stat_cnt = 0;  // steps spent receiving file

    int filenum = netbuffer->u.filetxpak.fileid;
    fileneed_t * fnp;
    char * fname; // filename
    FILE * fp;

    if(filenum>=cl_num_fileneed)
    {
        DEBFILE(va("filefragment not needed %d>%d\n",filenum,cl_num_fileneed));
        goto reject;
    }

    fnp = & cl_fileneed[filenum];
    if( fnp->status == FS_REQUESTED )
    {
        if(fnp->phandle)  goto file_already_open;
        fname = fnp->filename;
        fp = fopen( fname,"wb" );
        fnp->phandle = fp;  // owner of open file
        if(!fp)  goto file_create_err;

        GenPrintf(EMSG_hud, "\r%s ...", fname);
        fnp->bytes_recv = 0; 
        fnp->status = FS_DOWNLOADING;
    }

    if( fnp->status == FS_DOWNLOADING )
    {
        // Swap file position and size on big_endian machines.
        netbuffer->u.filetxpak.position	= LE_SWAP32_FAST(netbuffer->u.filetxpak.position);
        netbuffer->u.filetxpak.size	= LE_SWAP16_FAST(netbuffer->u.filetxpak.size);

        // use a special trick to know when file is finished (not always used)
        // WARNING: filepak can arrive out of order so don't stop now !
        if( netbuffer->u.filetxpak.position & 0x80000000 ) 
        {
            // End of send file flag.
            netbuffer->u.filetxpak.position &= ~0x80000000;
            fnp->totalsize = netbuffer->u.filetxpak.position + netbuffer->u.filetxpak.size;
        }
        // we can receive packet in the wrong order, anyway all os support gaped file
        fp = fnp->phandle;  // file being loaded
        fname = fnp->filename;  // for status and err msgs
        fseek(fp, netbuffer->u.filetxpak.position, SEEK_SET);
        if( fwrite(netbuffer->u.filetxpak.data, netbuffer->u.filetxpak.size, 1, fp) != 1 )
           goto file_write_err;
        fnp->bytes_recv += netbuffer->u.filetxpak.size;

#if 0
        if(stat_cnt==0)
        {
            // Update stats on screen.
            Net_GetNetStat();
            GenPrintf(EMSG_hud, "\r%s %dK/%dK %.1fK/s",
                        fname,
                        fnp->bytes_recv>>10,
                        fnp->totalsize>>10,
                        ((float)netstat_recv_bps)/1024);
        }
#endif       

        // Detect when all of file received.
        if(fnp->bytes_recv == fnp->totalsize)
        {
            fclose( fp );
            fnp->phandle = NULL;
            fnp->status = FS_FOUND;
            GenPrintf(EMSG_hud, "\rDownloading %s ... (done)\n", fname);
        }
    }
    else
    {
        I_SoftError("Received a file not requested\n");
        goto reject;
    }
    // send ack back quickly

    if(++stat_cnt==4)
    {
        Net_Send_AcksPacket(servernode);  // a packet of acks
        stat_cnt=0;
    }
    return;

    // Rare errors.
file_create_err:
    I_SoftError("Got_Filetxpak: File create error: %s\n", fname);
    goto reject;

file_write_err:
    I_SoftError("Got_Filetxpak: File write error: %s\n", fname);
    goto reject;

file_already_open:
    I_SoftError("Got_Filetxpak: Received a file that is already open\n");
    goto reject;
   
reject:
    return;
}

// By Server, Client, to cleanup sending files.
// nnode:  0..(MAXNETNODES-1)
// Called by Net_CloseConnection, CloseNetFile
void AbortSendFiles(byte nnode)
{
    while(transfer[nnode].txlist)
    {
        // By Server, only server have txlist set.
        SV_EndSend(nnode);
    }
}

void CloseNetFile(void)
{
    int i;

    // Abort sending.
    for( i=0;i<MAXNETNODES;i++)
        AbortSendFiles(i);

    // Abort receiving a file.
    for( i=0; i<MAX_WADFILES; i++ )
    {
        fileneed_t * fnp = & cl_fileneed[i];
        if( fnp->status==FS_DOWNLOADING && fnp->phandle)
        {
            fclose(fnp->phandle);
            // file is not complete, delete it
            remove(fnp->filename);
        }
    }

    // Remove FILEFRAGMENT from ackpaks.
    Net_AbortPacketType(PT_FILEFRAGMENT);
}

// functions cut and pasted from doomatic :)

// Remove all except filename at tail
void nameonly(char *s)
{
  int j;

  for(j=strlen(s);j>=0;j--)
  {
      if( (s[j]=='\\') || (s[j]==':') || (s[j]=='/') )
      {
          // [WDJ] DO NOT USE memcpy, these may overlap, use memmove
          memmove(s, &(s[j+1]), strlen(&(s[j+1]))+1 );
          return;
      }
  }
}


#if 0
// UNUSED for now
boolean fileexist(char *filename, time_t chk_time)
{
   int handle;
   handle=open(filename,O_RDONLY|O_BINARY);
   if( handle!=-1 )
   {
         close(handel);
         if(chk_time!=0)
         {
            struct stat bufstat;
            stat(filename,&bufstat);
            if( chk_time!=bufstat.st_mtime )
                return false;
         }
         return true;
   }
   return false;
}
#endif

//  filename : check the md5 sum of this file
//  wantedmd5sum : compare to this md5 sum, NULL if no check
// Return :
//   FS_FOUND : when md5 sum matches, or if no check when file opens for reading
//   FS_MD5SUMBAD : when md5 sum does not match
filestatus_e  checkfile_md5( const char * filename, const byte * wantedmd5sum)
{
    FILE *fhandle;
    unsigned char md5sum[16];
    filestatus_e return_val = FS_NOTFOUND;

    if((fhandle = fopen(filename,"rb")))
    {
        if(wantedmd5sum)
        {
            md5_stream(fhandle,md5sum);
            fclose(fhandle);
            if(!memcmp(wantedmd5sum, md5sum, 16))
                return_val = FS_FOUND;
            else
                return_val = FS_MD5SUMBAD; 
        }
        else
        {
            return_val = FS_FOUND;
        }
    }

    return return_val;
}


// Search the search directories for the file, with all controls.
//  filename: simple filename to find in a doomwaddir
//  search_depth: if > 0 then search subdirectories to that depth
//  wantedmd5sum : NULL for no md5 check
//  completepath: the file name buffer, must be length MAX_WADPATH
// Return FS_FOUND, with the file path in the completepath parameter.
//   FS_NOTFOUND
//   FS_MD5SUMBAD
static
filestatus_e  FullSearch_doomwaddir( const char * filename, int search_depth,
                    const byte * wantedmd5sum,
                    /* OUT */  char * completepath )
{
    filestatus_e  fs = FS_NOTFOUND;
    int wdi;
   
    for( wdi=0; wdi<MAX_NUM_DOOMWADDIR; wdi++ )
    {
        if( doomwaddir[wdi] == NULL )  continue;
        if( access( doomwaddir[wdi], X_OK ) )  continue;

        fs = sys_filesearch( filename, doomwaddir[wdi], wantedmd5sum,
                             search_depth, completepath );
        if( fs == FS_FOUND )  break;
    }
    return fs;
}

// Search the doom directories, simplified, with owner privilege.
//  filename: the search file
//  search_depth: if > 0 then search subdirectories to that depth
//  completepath: the file name buffer, must be length MAX_WADPATH
// Return true when found, with the file path in the completepath parameter.
boolean  Search_doomwaddir( const char * filename, int search_depth,
                 /* OUT */  char * completepath )
{
    return( FullSearch_doomwaddir( filename, search_depth, NULL,
                               /*OUT*/ completepath ) == FS_FOUND );
}


// Determine if the filename is simple, or has an inherent file path.
// Return the correct inherent filepath.
// Return NULL for a simple filename.
const char *  file_searchpath( const char * filename )
{
    // Leading char test, must be before any relative path test.
    if( filename[0] == '/' || filename[0] == '\\' || filename[1] == ':' )
        return "";  // Absolute path
    if( filename[0] == '.' && filename[1] == '.' )
        return "";  // Relative blank path.

    // Complex filename are file path.
    if( strpbrk( filename, ":/\\~" ) )
    {
        return ".";  // Relative to default path
    }

    if( strstr( filename, ".." ) )
    {
        return ".";  // Relative to default path
    }

    return NULL;  // Simple
}

// Search the doom directories, with md5, restricted privilege.
//  filename : the filename to be found
//  wantedmd5sum : NULL for no md5 check
//  net_secure : true for net downloads, restricted access
//  completepath : when not NULL, return the full path and name
//      must be a buffer of MAX_WADPATH
// return FS_NOTFOUND
//        FS_MD5SUMBAD
//        FS_FOUND
//        FS_SECURITY
filestatus_e  findfile( const char * filename, const byte * wantedmd5sum,
                        boolean  net_secure,
                        /*OUT*/ char * completepath )
{
    filestatus_e ret_val;

    const char * ipath = file_searchpath( filename );
    // Complex filename are file path.
    if( ipath )
    {
        if( net_secure )
        {
            // Net access
            // No absolute paths, no subdirectories.
            // Cannot back out of directories.
            return FS_SECURITY;
        }

        // Test for reading.
        // Do not need ipath for actual absolute or relative access.
        if( access( filename, R_OK ) != 0 )
            return FS_NOTFOUND;

        if( completepath )
            cat_filename( completepath, "", filename );
        return FS_FOUND;
    }
       
    // Simple filename for search doomwaddir.
    if( net_secure )
    { 
        // Restrict Net access to only relevant files, for security.
        const char * extension = &filename[strlen(filename)-3];
        if( strcasecmp( extension,"wad")!=0
           && strcasecmp( extension,"deh")!=0
           && strcasecmp( extension,"bex")!=0 )
           return FS_SECURITY;
       
        // Net is only allowed access to public wad directories.
        doomwaddir[1] = NULL;
        doomwaddir[2] = NULL;
        doomwaddir[MAX_NUM_DOOMWADDIR-2] = NULL;
        doomwaddir[MAX_NUM_DOOMWADDIR-1] = NULL;
    }
    else
    {
        // defdir is usually "."
        owner_wad_search_order();
    }

    // Search doomwaddir for simple filename.
    ret_val = FullSearch_doomwaddir( filename, GAME_SEARCH_DEPTH, wantedmd5sum,
                    /* OUT */  completepath );

#if 1
    return ret_val;
#else
    if( ret_val == FS_FOUND )
        return FS_FOUND;

    // [WDJ] 10 levels had it thrash for a long time when given a bad name.
    // Not needed with above directory searches.
    // This is a security risk that allows anyone to download private files
    // off your computer (using an altered DoomLegacy).
    return sys_filesearch(filename, ".", wantedmd5sum, 3, completepath);
#endif
}
