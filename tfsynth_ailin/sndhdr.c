/****************************************************************************
 * 
 *   sndhdr.c -- Definitions of functions to support the reading and 
 *               writing of '.snd' (Sun/NeXT sound file) headers.
 *
 *   Based on the former functions 'getnexthead()' and 'wrNxtHdr()'
 *   which appeared, with slight variations, in numerous SNDAN files.
 *   ('sextract.c' used a function named 'rdNxtHdr()' instead of 
 *   'getnexthead()'.)
 *
 *   Changes:                        
 *      02/05/98 jjjm   Renamed and revised the functions named above,
 *                      so all programs in the SNDAN suite which read/write
 *                      '.snd' files use a common set of functions
 *                      (replacing NXT_HDR, getnexthead, rdNxtHdr, wrNxtHdr)
 *      03/28/98 jjjm   Corrected 'readSndHdr()' to correctly determine the
 *                      number of samples from the size of the data chunk.
 *      03/28/98 jjjm   Added 'fixSndHdr()' to adjust the dataSize field in
 *                      the header after all samples have been written.
 *	04/02/98 jwb	readSndHdr(): Fix various things as labeled.
 *			fixSndHdr(): Replace type off_t with int.
 *****************************************************************************/

#include <stdio.h>		/* for perror() */
#include <unistd.h>		/* for lseek() */
#include "sndhdr.h"
#include "byteorder.h"

/*****************************************************************************
 *  readSndHdr() - read the header from a NeXT/Sun sound file ('.snd').
 *
 *  Reads the first 28 plus bytes of the file specified by 'fd', checks 
 *  that it is a NeXT '.snd' file and, if so, sets the values of the 
 *  parameters 'sr', 'nchans', and 'samptype' to the sampling rate, 
 *  number of channels, and sample type (data format), respectively, as 
 *  read from the sound file header. 'nsamps', the number of samples, is
 *  determined by the size of the file minus the size of the header
 *  divided by the size of the sample (in bytes) times the number of
 *  channels. At present, only 16-bit integer and 32-bit float data 
 *  formats are supported.
 *
 *  The function checks the endianness of the machine on which it is running
 *  and, if necessary, byte-reverses the data on a little-endian machine.
 *  (Data in a '.snd' file is big endian by definition.)
 *
 *  Finally, the read pointer is positioned at the start of the data in
 *  the input file (which is not necessarily at the 28th byte).
 *
 *  If a fatal error occurs, the function writes an error message to the
 *  standard error stream and terminates the program by calling exit(1).
 *
 *  Based on 'getnexthead()' and 'rdNxtHdr()' by James W. Beauchamp (11/16/93).
 *
 *  Jonathan Mohr 
 *  Augustana University College, Camrose, AB, Canada
 *  02/05/98
 *****************************************************************************/
void 
readSndHdr( int fd, int* sr, int* nchans, int* nsamps, int* samptype ) 
{ 
  SND_HDR header;
  int i, *data, databytes;   				   /* jwb 04/02/98 */
  int status;						  /* jjjm 02/05/98 */
  data = (int*)&header;					   /* jwb 08/18/94 */

  /* Read a NeXT '.snd' header from  disk, checking the status of the read. */
  if ( (status = read(fd, &header, sizeof(SND_HDR))) < 0 )
  {
    perror("readSndHdr");
    exit(1);
  }
  if (status < sizeof(SND_HDR))
  {
    fprintf(stderr, "Unable to read entire .snd header . . . aborting.\n");
    exit(1);
  }

  /* If this is a little-endian machine, reverse the order of the bytes
     in the integers, since '.snd' files are big-endian. */
  if (byte_order())			       /* zheng 11/29/97, rev.jjjm */
    for (i=0; i<6; i++)					 /* zheng 10/30/97 */
      byteswap4(data+i);

  /* Check that this is actually a NeXT/Sun sound file. */
  if (header.magic != SND_MAGIC)
  {
    fprintf(stderr, "Incorrect magic number in .snd header . . . aborting.\n");
    exit(1);
  }

  /* Set output parameters based on values read from header. */
  *sr = header.samplingRate;
  *nchans = header.channelCount;
  *samptype = header.dataFormat;			     /* jwb 04/02/98 */
  databytes = lseek(fd,0,SEEK_END) - header.dataLocation;    /* jwb 04/02/98 */
   
  if (*samptype == SND_FORMAT_FLOAT_32)			     /* jwb 04/02/98 */
    *nsamps = databytes/ (sizeof(float) * (*nchans)); 	     /* jwb 04/02/98 */
  else if (*samptype == SND_FORMAT_LINEAR_16)		     /* jwb 04/02/98 */
    *nsamps = databytes/ (sizeof(short) * (*nchans));	     /* jwb 04/02/98 */
  else
  {  
    fprintf(stderr, ".snd file: Unsupported sample data type value %d\n",
	*samptype);					     /* jwb 04/02/98 */ 
    exit(1);
  }
    
  /*  Position read pointer at start of input data.  */
  /*  NOTE: This position is not necessarily sizeof(SND_HDR). */
  if (lseek(fd, header.dataLocation, SEEK_SET) < 0) /* jwb 01/03/95,rev.jjjm */
  {
    perror("readSndHdr() could not find data");	    	    /* jwb 04/02/98 */
    exit(1);
  }
}

/******************************************************************************
 *  writeSndHdr() - write a header for a NeXT/Sun sound file ('.snd').
 *
 *  Writes a 28-byte header at the start of the file referred to by the 
 *  file descriptor 'fd', setting the values of the header fields for
 *  sampling rate, number of channels, number of samples, and sample type
 *  (data format) to the values of the parameters 'sr', 'nchans', 'nsamps',
 *  and 'samptype', respectively.
 *
 *  At present, only 16-bit integer (SHORT) and 32-bit float (FLOAT)
 *  data formats are supported.
 *
 *  The function checks the endianness of the machine on which it is running
 *  and, if necessary, byte-reverses the data on a little-endian machine.
 *  (Data in a '.snd' file is big endian by definition.)
 *
 *  The function assumes that the write pointer is positioned at the start of
 *  the file on entry to the function, and leaves the write pointer at the
 *  start of the data section of the file.
 *
 *  If a fatal error occurs, the function writes an error message to the
 *  standard error stream and terminates the program by calling exit().
 *
 *  Based on 'wrNxtHdr()' by James W. Beauchamp (11/16/93) and
 *  'SlamHdr.c' by Chris J.K. Gennaula (01/28/91).
 *
 *  Jonathan Mohr 
 *  Augustana University College, Camrose, AB, Canada
 *  02/05/98
 *****************************************************************************/
void
writeSndHdr( int fd, int sr, int nchans, int nsamps, int samptype )
{
  SND_HDR header;
  int i, *data;                                            /* jwb 08/19/94 */

  data = (int*)&header;					   /* jwb 08/18/94 */

  header.magic = SND_MAGIC;
  header.dataLocation = sizeof(SND_HDR);	/* jwb 01/03/95, rev. jjjm */
  header.samplingRate = sr;
  header.channelCount = nchans;

  switch (samptype)					   /* jjjm 02/08/98 */
  {
  case SND_FORMAT_FLOAT_32:
    header.dataFormat = SND_FORMAT_FLOAT_32;
    header.dataSize = nsamps * nchans * sizeof(float);     /* jwb 03/02/95 */
    break;			                     /* rev. jjjm 03/29/98 */
  case SND_FORMAT_LINEAR_16:
    header.dataFormat = SND_FORMAT_LINEAR_16;
    header.dataSize = nsamps * nchans * sizeof(short);
    break;
  default:
    fprintf(stderr, "Unrecognized sample data type.\n"); 
    exit(1);
  }

  /* If this is a little-endian machine, reverse the order of the bytes
     in the integers, since '.snd' files are big-endian. */
  if (byte_order())			       /* zheng 11/29/97, rev.jjjm */
    for (i=0; i<6; i++)					 /* zheng 10/30/97 */
      byteswap4(data+i);

  /* Now write to disk, checking the status of the write. */
  if ( write(fd, &header, sizeof(SND_HDR)) == -1 )
  {
    perror("writeSndHdr");
    exit(1);
  }
}


/*
 *  fixSndHdr() -- Change the 'dataSize' value in a SND file header
 *                 based on the actual size of the sound file.
 *
 *	Programmed by Jonathan Mohr, March, 1998
 *
 *  This function determines the size of the file passed to it and,
 *  assuming that the SND file header is the standard header written
 *  by writeSndHdr(), writes the appropriate value of the 'dataSize'
 *  field to the header at the start of the file.
 *
 *  If possible, the function reads the header from the start of the file
 *  and checks to be sure it is a valid SND header before updating the
 *  size field.  (If the file was created with 'creat()' or opened with
 *  the O_WRONLY mode bit set, we can't read and check the header, so we
 *  must assume that the caller has supplied the file descriptor for an
 *  actual SND file with a standard 28-byte header.
 */

void fixSndHdr( int fd )				/* jjjm 03/28/98 */
{
  int filesize;						/* jwb 04/02/98 */
  SND_HDR header;
  int status;
  int headersize;

  /* Seek the end of the file to find out the total file size. */
  filesize = lseek( fd, 0, SEEK_END );
  if (filesize == -1)
  {
    perror("fixSndHdr() determining sound file size");
    exit( 1 );
  }

  /* Seek the beginning of the file to read the file header. */
  if ( lseek( fd, 0, SEEK_SET ) == -1 )
  {
    perror("fixSndHdr() seeking the beginning of the file");
    exit( 1 );
  }

  /*
   * Try to read the file header and check that it is a valid SND header.
   * Note that a standard SND header (28 bytes) is assumed, but that,
   * if possible, we check that the header is valid to be sure.
   *
   * If the file was created using the 'creat()' system call or by using
   * the 'open() system call with the O_WRONLY mode bit, this attempt
   * will fail.  In that case, all we can do is write the file and data
   * size values to the places in the header where they _should_ go,
   * but we can't guarantee that the user actually gave us the file
   * descriptor of an open SND file with a standard 28-byte header.
   */
  status = read( fd, (char *) &header, sizeof(SND_HDR));
  if ( status == sizeof(SND_HDR))                       /* read succeeded */
  {
    if ( byte_order() == little_endian )
    {
      if ( header.magic != SND_MAGIC_LE )
      {	
	fprintf(stderr,
		"fixSndHdr(): File header is not a standard SND header.");
	exit( 1 );
      }

      /* Update the dataSize field in the header. */
      headersize = header.dataLocation;
      byteswap4(&headersize);
      header.dataSize = filesize - headersize;
      byteswap4(&header.dataSize);
    }
    else			/* big-endian */
    {
      if ( header.magic != SND_MAGIC )
      {
	fprintf(stderr, 
		"fixSndHdr(): File header is not a standard SND header.");
	exit( 1 );
      }
      
      /* Update the dataSize field in the header. */
      header.dataSize = filesize - header.dataLocation;
    }

    /* Write the header back to the file. */
    if ( lseek( fd, 0, SEEK_SET ) == -1 )
    {
      perror("fixSndHdr() reseeking the beginning of the file");
      exit( 1 );
    }
    if ( write(fd, &header, sizeof(SND_HDR)) == -1 )
    {
      perror("fixSndHdr() writing header back to SND file");
      exit( 1 );
    }
  }
  else if ( status == -1 )	   /* read error -- not open for reading */
  {
    /* Seek to the position where the dataSize field _should_ be. */
    if ( lseek( fd, 2 * sizeof(int), SEEK_SET ) == -1 )
    {
      perror("fixSndHdr() seeking the dataSize field");
      exit( 1 );
    }

    /* Data size doesn't include the header itself. */
    filesize -= sizeof(SND_HDR);     /* assume standard 28-byte header */
    
    /* Reverse the byte order if this is a little-endian machine. */
    if ( byte_order() == little_endian )
      byteswap4( (int *) &filesize );
    
    if ( write(fd, &filesize, sizeof(int)) == -1 )
    {
      perror("fixSndHdr() writing dataSize to SND header");
      exit( 1 );
    }
  }
  else			 /* we read something, but not a full SND header */
  {
    fprintf(stderr, "fixSndHdr() couldn't read a full SND header -- check that file descriptor actually belongs to a SND file.");
    exit( 1 );
  }
}
  
