/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ImageExtractor.h"

#include <stdio.h>

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

void print_help()
{
  printf("Usage: podofoimgextract [inputfile] [outputdirectory]\n\n");
}

int main( int argc, char* argv[] )
{
  PdfError eCode;
  char*    pszInput;
  char*    pszOutput;
  int      nNum     = 0;

  ImageExtractor extractor;

  if( argc != 3 )
  {
    print_help();
    exit( -1 );
  }

  pszInput  = argv[1];
  pszOutput = argv[2];

  eCode = extractor.Init( pszInput, pszOutput, &nNum );

  if( !eCode.IsError() )
  {
      printf("Extracted %i images sucessfully from the PDF file.\n", nNum );
  }
  else
      fprintf( stderr, "Error: An error %i ocurred during processing the pdf file.\n", eCode.Error() );
  
  return eCode.Error();
}
