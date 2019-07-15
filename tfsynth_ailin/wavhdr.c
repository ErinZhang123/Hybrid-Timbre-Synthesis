/*****************************************************************************
 * wavhdr.c  -- Utilities for handling Windows 3.0 WAVE files.
 *              (Currently, just reads a WAVE PCM header.)
 *
 * Based loosely on 'wav_startread()' from Sox.
 *
 * Programmer: Jonathan J. J. Mohr (jjjm)
 *             Augustana University College
 *             Camrose, Alberta, Canada T4V 2R3
 *             mohrj@augustana.ab.ca
 *             Programmed: 10/20/95 
 * Changes:
 * 02/10/98 jjjm
 * 03/29/98 jjjm   fixWavHdr(): Correct behaviour on big-endian machines.
 * 04/23/03 jwb    rdWavHdr(): Correct nsamps calculation for stereo files.
 ****************************************************************************/

#include <stdio.h>		/* for perror() */
#include <unistd.h>		/* for lseek() */
#include <stdlib.h>		/* for EXIT_FAILURE */
#include "byteorder.h"
#include "wavhdr.h"

#define MINRATE  500		/* minimum reasonable sampling rate */
#define MAXRATE  50000		/* maximum reasonable sampling rate */


void
fatal( char* errmsg )		                        /* jjjm 10/20/95 */
{
  fprintf( stderr, "%s\n", errmsg );
  exit( EXIT_FAILURE );
}

void
fatalsys( char* progname )
{
  perror( progname );
  exit( EXIT_FAILURE );
}


/*
 *  readWavHdr() -- Read a Windows 3.0 WAVE file header. 
 *
 *  This provides information such as the sampling rate, 
 *  size of samples, and number of channels (mono/stereo/quad).
 *
 *  Note that a WAVE header is not necessarily of a fixed size.
 *  While a minimal WAVE header is 44 bytes, each chunk header
 *  contains a size field which must be used to locate the start
 *  of the next chunk.
 */

void readWavHdr( int fd, int* sr, int* nchans, int* nsamps, int* sampsize )
{
  WAV_HDR waveHdr;
  int byte_reverse;

  /* WAVE data is little-endian, so if the host is big-endian,
     byte-reverse the data. */
  byte_reverse = (byte_order() != little_endian);	   /* jjjm 01/31/98 */

  /* Check for RIFF magic in pWaveHdr-> */
  if ( read( fd, (char *) &waveHdr.chkRiff.ckid, 4 ) < 4 )
    fatalsys("readWavHdr");
  if ( byte_reverse ) byteswap4( (int *) &waveHdr.chkRiff.ckid );
  if ( waveHdr.chkRiff.ckid != FOURCC_RIFF )
    fatal("Input file is not a RIFF WAVE file.");
  
  /* Get size of RIFF chunk. */
  if ( read( fd, (char *) &waveHdr.chkRiff.dwSize, 4 ) < 4 )
    fatalsys("readWavHdr");
  if ( byte_reverse ) byteswap4( (int *) &waveHdr.chkRiff.dwSize );
  
  /* Check for WAVE magic in waveHdr. */
  if ( read( fd, (char *) &waveHdr.fccWave, 4 ) < 4 )
    fatalsys("readWavHdr");
  if ( byte_reverse ) byteswap4( (int *) &waveHdr.fccWave );
  if ( waveHdr.fccWave != FOURCC_WAVE )
    fatal("Input file is not a WAVE file.");
  
  /* Skip any extra chunks until 'fmt' chunk found or EOF encountered. */
  while (1)
    {
      /* Read next chunk ID and its size. */
      if ( read( fd, (char *) &waveHdr.chkFmt, 8 ) < 8 )
	fatalsys("No format chunk found in WAVE file");
      if ( byte_reverse ) 
	{ 
	  byteswap4( (int *) &waveHdr.chkFmt.ckid );
	  byteswap4( (int *) &waveHdr.chkFmt.dwSize );
	}
      
      /* Check if 'fmt' chunk has been found . */
      if ( waveHdr.chkFmt.ckid == FOURCC_FMT )
	break;			                   /* LOOP EXIT */
      
      /* Skip over chunks other than 'fmt' according to the chunk size read. */
      if ( lseek( fd, waveHdr.chkFmt.dwSize, SEEK_CUR ) < 0 )
	fatalsys("Seek error on WAVE file while searching for format chunk");
    } 
    
  if ( read( fd, (char *) &waveHdr.pcmFmt.wFormatTag, 2 ) < 2 )
    fatalsys("readWavHdr");
  if ( byte_reverse ) byteswap2( &waveHdr.pcmFmt.wFormatTag );
  switch (waveHdr.pcmFmt.wFormatTag)
    {
    case WAVE_FORMAT_UNKNOWN:
      fatal("Sorry, this WAV file is in Microsoft Official Unknown format.");
    case WAVE_FORMAT_PCM:	/* this one, at least, can be handled */
      break;
    case WAVE_FORMAT_ADPCM:
      fatal("Sorry, this WAV file is in Microsoft ADPCM format.");
    case WAVE_FORMAT_ALAW:
      fatal("Sorry, this WAV file is in Microsoft A-law format.");
    case WAVE_FORMAT_MULAW:
      fatal("Sorry, this WAV file is in Microsoft U-law format.");
    case WAVE_FORMAT_OKI_ADPCM:
      fatal("Sorry, this WAV file is in OKI ADPCM format.");
    case WAVE_FORMAT_DIGISTD:
      fatal("Sorry, this WAV file is in Digistd format.");
    case WAVE_FORMAT_DIGIFIX:
      fatal("Sorry, this WAV file is in Digifix format.");
    case IBM_FORMAT_MULAW:
      fatal("Sorry, this WAV file is in IBM U-law format.");
    case IBM_FORMAT_ALAW:
      fatal("Sorry, this WAV file is in IBM A-law format.");
    case IBM_FORMAT_ADPCM:
      fatal("Sorry, this WAV file is in IBM ADPCM format.");
    default:	
      fatal("Sorry, don't understand the format in the WAV file waveHdr.");
    }
  
  if ( read( fd, (char *) &waveHdr.pcmFmt.wChannels, 2 ) < 2
      || read( fd, (char *) &waveHdr.pcmFmt.dwSamplesPerSec, 4 ) < 4
      || read( fd, (char *) &waveHdr.pcmFmt.dwAvgBytesPerSec, 4 ) < 4
      || read( fd, (char *) &waveHdr.pcmFmt.wBlockAlign, 2 ) < 2
      || read( fd, (char *) &waveHdr.pcmFmt.wBitsPerSample, 2 ) < 2 )
    fatalsys("Reading 'fmt' chunk of WAVE file");
  if ( byte_reverse ) 
    { 
      byteswap2( &waveHdr.pcmFmt.wChannels );
      byteswap4( (int *) &waveHdr.pcmFmt.dwSamplesPerSec );
      byteswap4( (int *) &waveHdr.pcmFmt.dwAvgBytesPerSec );
      byteswap2( &waveHdr.pcmFmt.wBlockAlign );
      byteswap2( &waveHdr.pcmFmt.wBitsPerSample );
    }
  
  /* Skip over any extra information in the format chunk --
     16 bytes read so far. */
  if ( lseek( fd, waveHdr.chkFmt.dwSize - 16, SEEK_CUR ) < 0 )
	fatalsys("Finding end of 'fmt' chunk of WAVE file");

  /* Skip any extra chunks until data chunk found or EOF encountered. */
  while (1)
    {
      /* Read next chunk ID and its size. */
      if ( read( fd, (char *) &waveHdr.chkData, 8 ) < 8 )
	fatalsys("No data chunk found in WAVE file");
      if ( byte_reverse ) 
	{ 
	  byteswap4( (int *) &waveHdr.chkData.ckid );
	  byteswap4( (int *) &waveHdr.chkData.dwSize );
	}
      
      /* Check if 'data' chunk has been found . */
      if ( waveHdr.chkData.ckid == FOURCC_DATA )
	break;			                   /* LOOP EXIT */
      
      /* Skip over chunks other than 'data' according to the size read. */
      if ( lseek( fd, waveHdr.chkData.dwSize, SEEK_CUR ) < 0 )
	fatalsys("Searching for data chunk in WAVE file");
    } 
    
  /*  now positioned at start of input data  */
  *sr = waveHdr.pcmFmt.dwSamplesPerSec;
  *nchans = waveHdr.pcmFmt.wChannels;
  *nsamps = waveHdr.chkData.dwSize / (waveHdr.pcmFmt.wBitsPerSample / 8);
  *nsamps /= *nchans;					/* jwb 04/23/03 */
  *sampsize = waveHdr.pcmFmt.wBitsPerSample;
}


/*
 *  writeWavHdr() -- Write a Windows 3.0 WAVE file header. 
 *
 *  Parameters: 'fd' is the file descriptor of the open file, 
 *    'sr' is the sample rate, 'nchans' is the number of channels
 *    (1 for mono, 2 for stereo, or 4 for quad), 'nsamps' is the
 *    number of samples which are expected to be written (or zero),
 *    and 'sampsize' is the number of bits per sample. 
 *
 *  The number of samples in the WAVE file should be checked (and the header
 *  corrected as required) by calling fixWavHdr() after all samples have 
 *  been written.
 *
 *  The SNDAN suite only supports 16-bit samples for WAVE files.
 *  (The WAVE format does not support float samples.)
 */

void writeWavHdr( int fd, int sr, int nchans, int nsamps, int sampsize )
{
  WAV_HDR waveHdr;
  int bytesPerFrame;
  int dataSize;

  /* Check validity of parameters. */
  if ( nchans != 1 && nchans != 2 && nchans != 4 )
    {
      fprintf( stderr, "Number of channels should be 1 (mono), 2 (stereo),");
      fprintf( stderr, " or 4 (quad).\n");
      exit(1);
    }
  if ( sr < MINRATE || sr > MAXRATE )
    {
      fprintf( stderr, "Specified sample rate is outside reasonable limits ");
      fprintf( stderr, "(%d Hz - %d Hz)\n", MINRATE, MAXRATE );
      exit(1);
    }
  if ( sampsize != 8 && sampsize != 12 && sampsize != 16 )
    {
      fprintf( stderr, "Sample size should be 8, 12, or 16.\n ");
      exit(1);
    }

  /* Calculate size of each frame (a single sample in each channel)
   * and approximate the final file size based on the value of 'nsamps'.
   * Both 12-bit and 16-bit samples require 2 bytes per channel.
   */
  bytesPerFrame =  nchans * ( sampsize == 8 ? 1 : 2 );
  dataSize = bytesPerFrame * nsamps;

  /* RIFF chunk */
  waveHdr.chkRiff.ckid = FOURCC_RIFF;
  /* Set size to the appropriate value for the header only -- 
   * update file size after all data has been written.         */
  waveHdr.chkRiff.dwSize = dataSize + sizeof(WAV_HDR) - sizeof(CHUNKHDR);

  /* WAVE magic */
  waveHdr.fccWave = FOURCC_WAVE;
  
  /* Format chunk */
  waveHdr.chkFmt.ckid = FOURCC_FMT;
  waveHdr.chkFmt.dwSize = sizeof(PCMWAVEFMT);

  /* Copy parameter values to format chunk, and calculate related values. */
  waveHdr.pcmFmt.wFormatTag = WAVE_FORMAT_PCM;
  waveHdr.pcmFmt.wChannels = (short) nchans;
  waveHdr.pcmFmt.dwSamplesPerSec = sr;
  waveHdr.pcmFmt.wBitsPerSample = (short) sampsize;
  waveHdr.pcmFmt.dwAvgBytesPerSec = sr * bytesPerFrame;
  waveHdr.pcmFmt.wBlockAlign = (short) bytesPerFrame;

  /* Data chunk */
  waveHdr.chkData.ckid = FOURCC_DATA;
  waveHdr.chkData.dwSize = dataSize;     /* update after all data written */

  /* WAVE data is little-endian, so if the host is big-endian,
     byte-reverse the data. */
  if (byte_order() != little_endian)
    {
      byteswap4( (int *) &waveHdr.chkRiff.ckid );
      byteswap4( (int *) &waveHdr.chkRiff.dwSize );
      byteswap4( (int *) &waveHdr.fccWave );
      byteswap4( (int *) &waveHdr.chkFmt.ckid );
      byteswap4( (int *) &waveHdr.chkFmt.dwSize );
      byteswap2( &waveHdr.pcmFmt.wFormatTag );
      byteswap2( &waveHdr.pcmFmt.wChannels );
      byteswap4( (int *) &waveHdr.pcmFmt.dwSamplesPerSec );
      byteswap4( (int *) &waveHdr.pcmFmt.dwAvgBytesPerSec );
      byteswap2( &waveHdr.pcmFmt.wBlockAlign );
      byteswap2( &waveHdr.pcmFmt.wBitsPerSample );
      byteswap4( (int *) &waveHdr.chkData.ckid );
      byteswap4( (int *) &waveHdr.chkData.dwSize );
    }

  /* Write header to the file. */
  if ( write(fd, &waveHdr, sizeof(WAV_HDR)) == -1 )
    fatalsys("writeWavHdr()");
}


/*
 *  fixWavHdr() -- Change the values in a WAVE file header which depend
 *                 on the size of the data chunk of the WAVE file.
 *
 *  This function determines the size of the file passed to it, calculates
 *  the RIFF chunk and data chunk sizes, and, assuming that the WAVE file
 *  header is the standard header written by writeWavHdr(), writes these
 *  values to the appropriate header fields at the start of the file.
 *
 *  If possible, the function reads the header from the start of the file
 *  and checks to be sure it is a valid WAVE header before updating the
 *  size fields.  (If the file was created with 'creat()' or opened with
 *  the O_WRONLY mode bit set, we can't read and check the header, so we
 *  must assume that the caller has supplied the file descriptor for an
 *  actual WAVE file with a standard (minimal) header.
 */

void fixWavHdr( int fd )
{
  DWORD filesize;
  WAV_HDR waveHdr;
  int status;

  /* Seek the end of the file to find out the total file size. */
  filesize = lseek( fd, 0, SEEK_END );
  if (filesize == -1)
    fatalsys("fixWavHdr() determining sound file size");

  /* Seek the beginning of the file to read the file header. */
  if ( lseek( fd, 0, SEEK_SET ) == -1 )
    fatalsys("fixWavHdr() seeking the beginning of the file");

  /*
   * Try to read the file header and check that it is a valid WAVE header.
   * Note that a minimal WAVE header (44 bytes) is assumed, but that,
   * if possible, we check that the header is valid to be sure.
   *
   * If the file was created using the 'creat()' system call or by using
   * the 'open() system call with the O_WRONLY mode bit, this attempt
   * will fail.  In that case, all we can do is write the file and data
   * size values to the places in the header where they _should_ go,
   * but we can't guarantee that the user actually gave us the file
   * descriptor of an open WAVE file with a standard (minimal) header.
   */
  status = read( fd, (char *) &waveHdr, sizeof(WAV_HDR));
  if ( status == sizeof(WAV_HDR))                       /* read succeeded */
    {
      if ( byte_order() == little_endian )               /* 03/29/98 jjjm */
      {
	if ( ! IS_STD_WAV_HEADER(waveHdr))
	  fatal("fixWavHdr(): File header is not a standard WAVE header.");

	/* Update the RIFF chunk and data chunk size values. */
	waveHdr.chkRiff.dwSize = filesize - sizeof(CHUNKHDR);
	waveHdr.chkData.dwSize = filesize - sizeof(WAV_HDR);
      }
      else			/* big_endian */
      {
	if ( ! IS_STD_WAV_HEADER_BE(waveHdr))     /* big-endian variant */
	  fatal("fixWavHdr(): File header is not a standard WAVE header.");

	/* Update the RIFF chunk and data chunk size values. */
	waveHdr.chkRiff.dwSize = filesize - sizeof(CHUNKHDR);
	waveHdr.chkData.dwSize = filesize - sizeof(WAV_HDR);
	byteswap4( (int *) &waveHdr.chkRiff.dwSize );
	byteswap4( (int *) &waveHdr.chkData.dwSize );
      }
  
      /* Write the header back to the file. */
      if ( lseek( fd, 0, SEEK_SET ) == -1 )
	fatalsys("fixWavHdr() reseeking the beginning of the file");
      if ( write(fd, &waveHdr, sizeof(WAV_HDR)) == -1 )
	fatalsys("fixWavHdr() writing header back to WAVE file");
    }
  else if ( status == -1 )	   /* read error -- not open for reading */
    {
      /* Seek to the position where the RIFF chunk size _should_ be. */
      if ( lseek( fd, sizeof(FOURCC), SEEK_SET ) == -1 )
	fatalsys("fixWavHdr() seeking the RIFF chunk size field");

      /* RIFF chunk size doesn't include the chunk header itself. */
      waveHdr.chkRiff.dwSize = filesize - sizeof(CHUNKHDR);
      if ( byte_order() != little_endian )               /* 03/29/98 jjjm */
	byteswap4( (int *) &waveHdr.chkRiff.dwSize );
      if ( write(fd, &waveHdr.chkRiff.dwSize, sizeof(DWORD)) == -1 )
	fatalsys("fixWavHdr() writing RIFF chunk size to WAVE header");

      /* Seek to the position where the data chunk size _should_ be. */
      if ( lseek( fd, sizeof(WAV_HDR) - sizeof(DWORD), SEEK_SET ) == -1 )
	fatalsys("fixWavHdr() seeking the data chunk size field");

      /* Data chunk size doesn't include the WAVE header. */
      waveHdr.chkData.dwSize = filesize - sizeof(WAV_HDR);
      if ( byte_order() != little_endian )               /* 03/29/98 jjjm */
	byteswap4( (int *) &waveHdr.chkData.dwSize );
      if ( write(fd, &waveHdr.chkData.dwSize, sizeof(DWORD)) == -1 )
	fatalsys("fixWavHdr() writing data chunk size to WAVE file");
    }
  else			 /* we read something, but not a full WAVE header */
    fatal("fixWavHdr() couldn't read a full WAVE header -- check that file descriptor actually belongs to a WAVE file.");
}
