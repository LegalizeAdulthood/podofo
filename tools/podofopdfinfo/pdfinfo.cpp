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

#include "pdfinfo.h"


PdfInfo::PdfInfo( const std::string& inPathname )
{
    mDoc = new PdfDocument( inPathname.c_str() );
}

PdfInfo::~PdfInfo()
{
    if ( mDoc ) {
        delete mDoc;
        mDoc = NULL;
    }
}

void PdfInfo::OutputDocumentInfo( std::ostream& sOutStream )
{
    sOutStream << "PDF Version: " << s_szPdfVersionNums[(int)mDoc->GetPdfVersion()] << std::endl;
    sOutStream << "Page Count: " << mDoc->GetPageCount() << std::endl;
    sOutStream << std::endl;
    sOutStream << "Fast Web View Enabled: " << (mDoc->IsLinearized() ? "Yes" : "No") << std::endl;
    sOutStream << "Tagged: " << (mDoc->GetStructTreeRoot() != NULL ? "Yes" : "No") << std::endl;

/*
// print encryption info
printf("Encrypted:      ");
if (doc->isEncrypted()) {
printf("yes (print:%s copy:%s change:%s addNotes:%s)\n",
doc->okToPrint(gTrue) ? "yes" : "no",
doc->okToCopy(gTrue) ? "yes" : "no",
doc->okToChange(gTrue) ? "yes" : "no",
doc->okToAddNotes(gTrue) ? "yes" : "no");
} else {
printf("no\n");
}
*/
}

void PdfInfo::OutputInfoDict( std::ostream& sOutStream )
{
    PdfObject* infoObj = mDoc->GetInfo();
    PdfOutputDevice device( &sOutStream );

    if( infoObj )
        infoObj->Write( &device );
}

void PdfInfo::OutputPageInfo( std::ostream& sOutStream )
{
    PdfVariant  var;
    std::string str;

    int	pgCount = mDoc->GetPageCount();
    for ( int pg=0; pg<pgCount; pg++ ) 
    {
        sOutStream << "Page " << pg << ":" << std::endl;
        
        PdfPage*	curPage = mDoc->GetPage( pg );
        
        curPage->GetMediaBox().ToVariant( var );
        var.ToString( str );

        sOutStream << "\tMediaBox: " << str << std::endl;
        sOutStream << "\tRotation: " << curPage->GetRotation() << std::endl;
        sOutStream << "\t# of Annotations: " << curPage->GetNumAnnots() << std::endl;
        
        delete curPage;	// delete it when done
    }
}
