/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "PdfDefines.h"
#include "PdfFiltersPrivate.h"

#include "PdfDictionary.h"
#include "PdfOutputStream.h"
#include "PdfTokenizer.h"

#define CHUNK               16384 
#define LZW_TABLE_SIZE      4096

namespace {

// Private data for PdfAscii85Filter. This will be optimised
// by the compiler through compile-time constant expression
// evaluation.
const unsigned long sPowers85[] = {
    85*85*85*85, 85*85*85, 85*85, 85, 1
};

} // end anonymous namespace


namespace PoDoFo {

/** 
 * This structur contains all necessary values
 * for a FlateDecode and LZWDecode Predictor.
 * These values are normally stored in the /DecodeParams
 * key of a PDF dictionary.
 */
struct TFlatePredictorParams {
    TFlatePredictorParams() {
        nPredictor   = 1;
        nColors      = 1;
        nBPC         = 8;
        nColumns     = 1;
        nEarlyChange = 1;
    };

    int nPredictor;
    int nColors;
    int nBPC;
    int nColumns;
    int nEarlyChange;
};


// -------------------------------------------------------
// Hex
// -------------------------------------------------------

void PdfHexFilter::EncodeBlockImpl( const char* pBuffer, long lLen )
{
    char data[2];
    while( lLen-- )
    {
        data[0]  = (*pBuffer & 0xF0) >> 4;
        data[0] += (data[0] > 9 ? 'A' - 10 : '0');

        data[1]  = (*pBuffer & 0x0F);
        data[1] += (data[1] > 9 ? 'A' - 10 : '0');

        GetStream()->Write( data, 2 );

        ++pBuffer;
    }
}

void PdfHexFilter::BeginDecodeImpl( const PdfDictionary* )
{ 
    m_cDecodedByte = 0;
    m_bLow         = true;
}

void PdfHexFilter::DecodeBlockImpl( const char* pBuffer, long lLen )
{
    char val;

    while( lLen-- ) 
    {
        if( PdfTokenizer::IsWhitespace( *pBuffer ) )
        {
            ++pBuffer;
            continue;
        }

        val  = *pBuffer;
        val -= ( *pBuffer < 'A' ? '0' : 'A'-10 );

        if( m_bLow ) 
        {
            m_cDecodedByte = (val & 0x0F);
            m_bLow         = false;
        }
        else
        {
            m_cDecodedByte = ((m_cDecodedByte << 4) | val);
            m_bLow         = true;

            GetStream()->Write( &m_cDecodedByte, 1 );
        }

        ++pBuffer;
    }
}

void PdfHexFilter::EndDecodeImpl()
{ 
    if( !m_bLow ) 
    {
        // an odd number of bytes was read,
        // so the last byte is 0
        GetStream()->Write( &m_cDecodedByte, 1 );
    }
}


// -------------------------------------------------------
// Ascii 85
// 
// based on public domain software from:
// Paul Haahr - http://www.webcom.com/~haahr/
// -------------------------------------------------------

void PdfAscii85Filter::EncodeTuple( unsigned long tuple, int count )
{
    int      i      = 5;
    int      z      = 0;
    char     buf[5];
    char     out[5];
    char*    start  = buf;;

    do 
    {
        *start++ = tuple % 85;
        tuple /= 85;
    } 
    while (--i > 0);

    i = count;
    do 
    {
        out[z++] = static_cast<unsigned char>(*--start) + '!';
    } 
    while (i-- > 0);

    GetStream()->Write( out, z );
}

void PdfAscii85Filter::BeginEncodeImpl()
{
    m_count = 0;
    m_tuple = 0;
}

void PdfAscii85Filter::EncodeBlockImpl( const char* pBuffer, long lLen )
{
    unsigned int  c;
    const char*   z = "z";

    if( !GetStream() )
    {
        RAISE_ERROR_INFO( ePdfError_InternalLogic, 
                          "BeginEncode was not yet called or EndEncode was called before this method.");
    }

    while( lLen ) 
    {
        c = *pBuffer & 0xff;
        switch (m_count++) {
            case 0: m_tuple |= ( c << 24); break;
            case 1: m_tuple |= ( c << 16); break;
            case 2: m_tuple |= ( c <<  8); break;
            case 3:
                m_tuple |= c;
                if( 0 == m_tuple ) 
                {
                    GetStream()->Write( z, 1 );
                }
                else
                {
                    this->EncodeTuple( m_tuple, m_count ); 
                }

                m_tuple = 0;
                m_count = 0;
                break;
        }
        --lLen;
        ++pBuffer;
    }
}

void PdfAscii85Filter::EndEncodeImpl()
{
    if( m_count > 0 )
        this->EncodeTuple( m_tuple, m_count );
    GetStream()->Write( "~>", 2 );
}

void PdfAscii85Filter::BeginDecodeImpl( const PdfDictionary* )
{ 
    m_count = 0;
    m_tuple = 0;
}

void PdfAscii85Filter::DecodeBlockImpl( const char* pBuffer, long lLen )
{
    bool foundEndMarker = false;

    while( lLen && !foundEndMarker ) 
    {
        switch ( *pBuffer ) 
        {
            default:
                if ( *pBuffer < '!' || *pBuffer > 'u') 
                {
                    RAISE_ERROR( ePdfError_ValueOutOfRange );
                }

                m_tuple += ( *pBuffer - '!') * sPowers85[m_count++];
                if( m_count == 5 ) 
                {
                    WidePut( m_tuple, 4 );
                    m_count = 0;
                    m_tuple = 0;
                }
                break;
            case 'z':
                if (m_count != 0 ) 
                {
                    RAISE_ERROR( ePdfError_ValueOutOfRange );
                }

                this->WidePut( 0, 4 );
                break;
            case '~':
                ++pBuffer; 
                --lLen;
                if( *pBuffer != '>' ) 
                {
                    RAISE_ERROR( ePdfError_ValueOutOfRange );
                }
                foundEndMarker = true;
                break;
            case '\n': case '\r': case '\t': case ' ':
            case '\0': case '\f': case '\b': case 0177:
                break;
        }

        --lLen;
        ++pBuffer;
    }


}

void PdfAscii85Filter::EndDecodeImpl()
{ 
    if( m_count > 0 ) 
    {
        m_count--;
        m_tuple += sPowers85[m_count];
        WidePut( m_tuple, m_count );
    }
}

void PdfAscii85Filter::WidePut( unsigned long tuple, int bytes ) const
{
    char data[4];

    switch( bytes ) 
    {
	case 4:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            data[2] = static_cast<char>(tuple >>  8);
            data[3] = static_cast<char>(tuple);
            break;
	case 3:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            data[2] = static_cast<char>(tuple >>  8);
            break;
	case 2:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            break;
	case 1:
            data[0] = static_cast<char>(tuple >> 24);
            break;
    }

    GetStream()->Write( data, bytes );
}

// -------------------------------------------------------
// Flate
// -------------------------------------------------------
void PdfFlateFilter::BeginEncodeImpl()
{
    m_stream.zalloc   = Z_NULL;
    m_stream.zfree    = Z_NULL;
    m_stream.opaque   = Z_NULL;

    if( deflateInit( &m_stream, Z_DEFAULT_COMPRESSION ) )
    {
        RAISE_ERROR( ePdfError_Flate );
    }
}

void PdfFlateFilter::EncodeBlockImpl( const char* pBuffer, long lLen )
{
    this->EncodeBlockInternal( pBuffer, lLen, Z_NO_FLUSH );
}

void PdfFlateFilter::EncodeBlockInternal( const char* pBuffer, long lLen, int nMode )
{
    int nWrittenData;

    m_stream.avail_in = lLen;
    m_stream.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(pBuffer));

    do {
        m_stream.avail_out = FILTER_INTERNAL_BUFFER_SIZE;
        m_stream.next_out  = m_buffer;

        if( deflate( &m_stream, nMode) == Z_STREAM_ERROR )
        {
            FailEncodeDecode();
            RAISE_ERROR( ePdfError_Flate );
        }


        nWrittenData = FILTER_INTERNAL_BUFFER_SIZE - m_stream.avail_out;
        try {
            GetStream()->Write( reinterpret_cast<char*>(m_buffer), nWrittenData );
        } catch( PdfError & e ) {
            // clean up after any output stream errors
            FailEncodeDecode();
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }
    } while( m_stream.avail_out == 0 );
}

void PdfFlateFilter::EndEncodeImpl()
{
    this->EncodeBlockInternal( NULL, 0, Z_FINISH );
    deflateEnd( &m_stream );
}

// --

void PdfFlateFilter::BeginDecodeImpl( const PdfDictionary* )
{
    m_stream.zalloc   = Z_NULL;
    m_stream.zfree    = Z_NULL;
    m_stream.opaque   = Z_NULL;

    if( inflateInit( &m_stream ) != Z_OK )
    {
        RAISE_ERROR( ePdfError_Flate );
    }
}

void PdfFlateFilter::DecodeBlockImpl( const char* pBuffer, long lLen )
{
    int flateErr;
    int nWrittenData;

    m_stream.avail_in = lLen;
    m_stream.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(pBuffer));

    do {
        m_stream.avail_out = FILTER_INTERNAL_BUFFER_SIZE;
        m_stream.next_out  = m_buffer;

        switch ( (flateErr = inflate(&m_stream, Z_NO_FLUSH)) ) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            {
                PdfError::LogMessage( eLogSeverity_Error, "Flate Decoding Error from ZLib: %i\n", flateErr );
                (void)inflateEnd(&m_stream);

                FailEncodeDecode();
                RAISE_ERROR( ePdfError_Flate );
            }
            default:
                break;
        }

        nWrittenData = FILTER_INTERNAL_BUFFER_SIZE - m_stream.avail_out;
        try {
            GetStream()->Write( reinterpret_cast<char*>(m_buffer), nWrittenData );
        } catch( PdfError & e ) {
            // clean up after any output stream errors
            FailEncodeDecode();
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }
    } while( m_stream.avail_out == 0 );
}

void PdfFlateFilter::EndDecodeImpl()
{
    (void)inflateEnd(&m_stream);
}

/*
void PdfFlateFilter::Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms ) const
{
    int          flateErr;
    unsigned int have;
    z_stream strm;
    char  out[CHUNK];
    char* pBuf = NULL;
    char* pTmp = NULL;

    long  lBufSize = 0;
    TFlatePredictorParams tParams;

    if( !pInBuffer || !plOutLen || !ppOutBuffer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // allocate inflate state
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    if( inflateInit(&strm) != Z_OK)
    {
        RAISE_ERROR( ePdfError_Flate );
    }

    strm.avail_in = lInLen;
    strm.next_in  = (Bytef*)(pInBuffer);

    do {
        strm.avail_out = CHUNK;
        strm.next_out  = (Bytef*)(out);

        switch ( (flateErr = inflate(&strm, Z_NO_FLUSH)) ) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            {
                PdfError::LogMessage( eLogSeverity_Error, "Flate Decoding Error from ZLib: %i\n", flateErr );
                (void)inflateEnd(&strm);

                RAISE_ERROR( ePdfError_Flate );
            }
            default:
                break;
        }

        have = CHUNK - strm.avail_out;

        if( pBuf )
            pBuf = static_cast<char*>(realloc( pBuf, sizeof( char ) * (lBufSize + have) ));
        else
            pBuf = static_cast<char*>(malloc( sizeof( char ) * (lBufSize + have) ));

        if( !pBuf )
        {
            (void)inflateEnd(&strm);
            free( pTmp );
            RAISE_ERROR( ePdfError_InvalidHandle );
        }

        memcpy( (pBuf+lBufSize), out, have );
        lBufSize += have;
        free( pTmp );
    } while (strm.avail_out == 0);
    
    // clean up and return 
    (void)inflateEnd(&strm);

    *ppOutBuffer = pBuf;
    *plOutLen    = lBufSize;

    if( pDecodeParms ) 
    {
        tParams.nPredictor   = pDecodeParms->GetKeyAsLong( "Predictor", tParams.nPredictor );
        tParams.nColors      = pDecodeParms->GetKeyAsLong( "Colors", tParams.nColors );
        tParams.nBPC         = pDecodeParms->GetKeyAsLong( "BitsPerComponent", tParams.nBPC );
        tParams.nColumns     = pDecodeParms->GetKeyAsLong( "Columns", tParams.nColumns );
        tParams.nEarlyChange = pDecodeParms->GetKeyAsLong( "EarlyChange", tParams.nEarlyChange );

        try {
            this->RevertPredictor( &tParams, pBuf, lBufSize, ppOutBuffer, plOutLen );
        } catch( PdfError & e ) {
            free( pBuf );
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }

        free( pBuf );
    }
}
*/

// -------------------------------------------------------
// Flate Predictor
// -------------------------------------------------------
void PdfFlateFilter::RevertPredictor( const TFlatePredictorParams* pParams, const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const
{
    unsigned char*   pPrev;
    int     nRows;
    int     i;
    char*   pOutBufStart;
    const char*   pBuffer = pInBuffer;
    int     nPredictor;

#ifdef PODOFO_VERBOSE_DEBUG
    PdfError::DebugMessage("Applying Predictor %i to buffer of size %i\n", pParams->nPredictor, lInLen );
    PdfError::DebugMessage("Cols: %i Modulo: %i Comps: %i\n", pParams->nColumns, lInLen % (pParams->nColumns +1), pParams->nBPC );
#endif // PODOFO_VERBOSE_DEBUG

    if( pParams->nPredictor == 1 )  // No Predictor
        return;

    nRows = (pParams->nColumns * pParams->nBPC) >> 3; 

    pPrev = static_cast<unsigned char*>(malloc( sizeof(char) * nRows ));
    if( !pPrev )
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    memset( pPrev, 0, sizeof(char) * nRows );

#ifdef PODOFO_VERBOSE_DEBUG
    PdfError::DebugMessage("Alloc: %i\n", (lInLen / (pParams->nColumns + 1)) * pParams->nColumns );
#endif // PODOFO_VERBOSE_DEBUG

    *ppOutBuffer = static_cast<char*>(malloc( sizeof(char) * (lInLen / (pParams->nColumns + 1)) * pParams->nColumns ));
    pOutBufStart = *ppOutBuffer;

    if( !*ppOutBuffer )
    {
        free( pPrev );
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    while( pBuffer < (pInBuffer + lInLen) )
    {
        nPredictor = pParams->nPredictor >= 10 ? *pBuffer + 10 : *pBuffer;
        ++pBuffer;

        for( i=0;i<nRows;i++ )
        {
            switch( nPredictor )
            {
                case 2: // Tiff Predictor
                    // TODO: implement tiff predictor
                    
                    break;
                case 10: // png none
                case 11: // png sub
                case 12: // png up
                    *pOutBufStart = static_cast<unsigned char>(pPrev[i] + static_cast<unsigned char>(*pBuffer));
                    break;
                case 13: // png average
                case 14: // png paeth
                case 15: // png optimum
                    break;
                
                default:
                {
                    free( pPrev );
                    RAISE_ERROR( ePdfError_InvalidPredictor );
                    break;
                }
            }
  
            pPrev[i] = *pOutBufStart;          
            ++pOutBufStart;
            ++pBuffer;
        }
    }

    *plOutLen = (pOutBufStart - *ppOutBuffer);

    free( pPrev );
}

// -------------------------------------------------------
// RLE
// -------------------------------------------------------

void PdfRLEFilter::BeginEncodeImpl()
{
    RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfRLEFilter::EncodeBlockImpl( const char*, long )
{
    RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfRLEFilter::EndEncodeImpl()
{
    RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfRLEFilter::BeginDecodeImpl( const PdfDictionary* )
{ 
    m_nCodeLen = 0;
}

void PdfRLEFilter::DecodeBlockImpl( const char* pBuffer, long lLen )
{
    while( lLen-- )
    {
        if( !m_nCodeLen )
        {
            m_nCodeLen = static_cast<int>(*pBuffer);
        } else if( m_nCodeLen == 128 )
            break;
        else if( m_nCodeLen <= 127 )
        {
            GetStream()->Write( pBuffer, 1 );
            m_nCodeLen--;
        }
        else if( m_nCodeLen >= 129 )
        {
            m_nCodeLen = 257 - m_nCodeLen;

            while( m_nCodeLen-- ) 
                GetStream()->Write( pBuffer, 1 );
        }

        ++pBuffer;
    }
}

// -------------------------------------------------------
// LZW
// -------------------------------------------------------

const unsigned short PdfLZWFilter::s_masks[] = { 0x01FF,
                                        0x03FF,
                                        0x07FF,
                                        0x0FFF };

const unsigned short PdfLZWFilter::s_clear  = 0x0100;      // clear table
const unsigned short PdfLZWFilter::s_eod    = 0x0101;      // end of data

void PdfLZWFilter::BeginEncodeImpl()
{
    RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfLZWFilter::EncodeBlockImpl( const char*, long )
{
    RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfLZWFilter::EndEncodeImpl()
{
    RAISE_ERROR( ePdfError_UnsupportedFilter );
}

void PdfLZWFilter::BeginDecodeImpl( const PdfDictionary* )
{ 
    m_mask      = 0;
    m_code_len  = 9;
    m_character = 0;

    m_bFirst    = true;

    InitTable();
}

void PdfLZWFilter::DecodeBlockImpl( const char* pBuffer, long lLen )
{
    unsigned int       buffer_size = 0;
    const unsigned int buffer_max  = 24;

    pdf_uint32         old         = 0;
    pdf_uint32         code        = 0;
    pdf_uint32         buffer      = 0;

    TLzwItem           item;

    std::vector<unsigned char> data;
    std::vector<unsigned char>::const_iterator it;

    if( m_bFirst ) 
    {
        m_character = *pBuffer;
        m_bFirst    = false;
    }

    while( lLen ) 
    {
        // Fill the buffer
        while( buffer_size <= (buffer_max-8) && lLen )
        {
            buffer <<= 8;
            buffer |= static_cast<pdf_uint32>(static_cast<unsigned char>(*pBuffer));
            buffer_size += 8;

            ++pBuffer;
            lLen--;
        }

        // read from the buffer
        while( buffer_size >= m_code_len ) 
        {
            code         = (buffer >> (buffer_size - m_code_len)) & PdfLZWFilter::s_masks[m_mask];
            buffer_size -= m_code_len;

            if( code == PdfLZWFilter::s_clear ) 
            {
                m_mask     = 0;
                m_code_len = 9;

                InitTable();
            }
            else if( code == PdfLZWFilter::s_eod ) 
            {
                lLen = 0;
                break;
            }
            else 
            {
                if( code >= m_table.size() )
                {
                    if (old >= m_table.size())
                    {
                        RAISE_ERROR( ePdfError_ValueOutOfRange );
                    }
                    data = m_table[old].value;
                    data.push_back( m_character );
                }
                else
                    data = m_table[code].value;

                // Write data to the output device
                GetStream()->Write( reinterpret_cast<char*>(&(data[0])), data.size() );

                m_character = data[0];
                if( old < m_table.size() ) // fix the first loop
                    data = m_table[old].value;
                data.push_back( m_character );

                item.value = data;
                m_table.push_back( item );

                old = code;

                switch( m_table.size() ) 
                {
                    case 511:
                    case 1023:
                    case 2047:
                    case 4095:
                        ++m_code_len;
                        ++m_mask;
                    default:
                        break;
                }
            }
        }
    }
}

/*
void PdfLZWFilter::Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms ) const
{
    TLzwTable table( LZW_TABLE_SIZE ); // the lzw table;
    TLzwItem  item;

    std::vector<unsigned char> output;
    std::vector<unsigned char> data;
    std::vector<unsigned char>::const_iterator it;

    unsigned int mask             = 0;
    unsigned int code_len         = 9;
    unsigned char character       = 0;

    pdf_uint32   old              = 0;
    pdf_uint32   code             = 0;
    pdf_uint32   buffer           = 0;

    unsigned int buffer_size      = 0;
    const unsigned int buffer_max = 24;
    
    *ppOutBuffer = NULL;
    *plOutLen    = 0;

    InitTable( &table );

    if( lInLen )
        character = *pInBuffer;

    while( lInLen > 0 ) 
    {
        // Fill the buffer
        while( buffer_size <= (buffer_max-8) )
        {
            buffer <<= 8;
            buffer |= static_cast<pdf_uint32>(static_cast<unsigned char>(*pInBuffer));
            buffer_size += 8;

            ++pInBuffer;
            lInLen--;
        }

        // read from the buffer
        while( buffer_size >= code_len ) 
        {
            code = (buffer >> (buffer_size - code_len)) & PdfLZWFilter::s_masks[mask];
            buffer_size -= code_len;

            if( code == PdfLZWFilter::s_clear ) 
            {
                mask     = 0;
                code_len = 9;

                InitTable( &table );
            }
            else if( code == PdfLZWFilter::s_eod ) 
            {
                lInLen = 0;
                break;
            }
            else 
            {
                if( code >= table.size() )
                {
                    if (old >= table.size())
                    {
                        RAISE_ERROR( ePdfError_ValueOutOfRange );
                    }
                    data = table[old].value;
                    data.push_back( character );
                }
                else
                    data = table[code].value;

                it = data.begin();
                while( it != data.end() )
                {
                    output.push_back( *it );
                    ++it;
                }

                character = data[0];
                if( old < table.size() ) // fix the first loop
                    data = table[old].value;
                data.push_back( character );

                item.value = data;
                table.push_back( item );

                old = code;

                switch( table.size() ) 
                {
                    case 511:
                    case 1023:
                    case 2047:
                    case 4095:
                        ++code_len;
                        ++mask;
                    default:
                        break;
                }
            }
        }
    }

    *ppOutBuffer = static_cast<char*>(malloc( sizeof(char) * output.size() ));
    if( !*ppOutBuffer ) 
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    *plOutLen = output.size();

    memcpy( *ppOutBuffer, &(output[0]), *plOutLen );
}
*/
void PdfLZWFilter::InitTable()
{
    int      i;
    TLzwItem item;

    m_table.clear();
    m_table.reserve( LZW_TABLE_SIZE );

    for( i=0;i<255;i++ )
    {
        item.value.clear();
        item.value.push_back( static_cast<unsigned char>(i) );
        m_table.push_back( item );
    }

    item.value.clear();
    item.value.push_back( static_cast<unsigned char>(s_clear) );
    m_table.push_back( item );

    item.value.clear();
    item.value.push_back( static_cast<unsigned char>(s_clear) );
    m_table.push_back( item );
}

};
