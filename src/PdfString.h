/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#ifndef _PDF_STRING_H_
#define _PDF_STRING_H_

#include "PdfDefines.h"
#include "PdfDataType.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

#define PDF_STRING_BUFFER_SIZE 24

class PdfOutputDevice;

typedef enum EPdfStringConversion {
    ePdfStringConversion_Strict,
    ePdfStringConversion_Lenient
};


/** A string that can be written to a PDF document.
 *  If it contains binary data it is automatically 
 *  converted into a hex string, otherwise a normal PDF 
 *  string is written to the document.
 *
 *  PdfStrings representing text are either in PDFDocEncoding
 *  (ISO Latin1) encoded or UTF-16BE encoded.
 *
 *  PoDoFo contains methods to convert between these 
 *  encodings. For convinience conversion to UTF-8
 *  is possible to. Please not that strings are
 *  always stored as UTF-16BE or ISO Latin1 (PdfDocEncoding)
 *  in the PDF file.
 *
 *  UTF-16BE strings have to start with the byts 0xFE 0xFF
 *  to be recognized by PoDoFo as unicode strings.
 *
 *
 *  PdfStrings is an implicitly shared class. As a reason
 *  it is very fast to copy PdfString objects.
 *
 *  The internal string buffer is guaranteed to be always terminated 
 *  by 2 zeros.
 */
class PODOFO_API PdfString : public PdfDataType{
 public:

    /** Create an empty and invalid string 
     */
    PdfString();

   /** Construct a new PdfString from a std::string. 
    *  The input string will be copied.
    *  If the first to bytes of the string are 0xFE and 0xFF
    *  this string is treated as UTF-16BE encoded unicode string.
    *
    *  \param sString the string to copy
    */
    PdfString( const std::string& sString );

    /** Construct a new PdfString from a 0 terminated
     *  string. 
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
     */
    PdfString( const char* pszString );

    /** Construct a new PdfString from a string. 
     *  The input string will be copied.
     *  If the first to bytes of the string are 0xFE and 0xFF
     *  this string is treated as UTF-16BE encoded unicode string.
     *
     *  \param pszString the string to copy
     *  \param lLen length of the string data to encode
     *  \param bHex if true the data will be 
     *              hex encoded during writeout of the string and IsHex() will return true.
     */
    PdfString( const char* pszString, long lLen, bool bHex = false );

    /** Construct a new PdfString from an UTF-8 encoded string.
     *  
     *  The string is converted to UTF-16BE internally.
     *
     *  \param pszStringUtf8 a UTF-8 encoded string.
     */
    PdfString( const pdf_utf8* pszStringUtf8 );

    /** Construct a new PdfString from an UTF-8 encoded string.
     *  
     *  The string is converted to UTF-16BE internally.
     *
     *  \param pszStringUtf8 a UTF-8 encoded string.
     *  \param lLen number of bytes to convert
     */
    PdfString( const pdf_utf8* pszStringUtf8, long lLen );

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     */
    PdfString( const PdfString & rhs );

    ~PdfString();

    /** Set hex encoded data as the strings data. 
     *  \param pszHex must be hex encoded data.
     *  \param lLen   length of the hex encoded data.
     *                if lLen == -1 then strlen( pszHex ) will
     *                be used as length of the hex data. 
     *                pszHex has to be zero terminated in this case.
     */
    void SetHexData( const char* pszHex, long lLen = -1 );

    /** The string is valid if no error in the constructor has occurred.
     *  If it is valid it is safe to call all the other member functions.
     *  \returns true if this is a valid initialized PdfString
     */
    inline bool IsValid() const;

    /** Check if this is a hex string.
     *  
     *  If true the data will be hex encoded when the string is written to
     *  a PDF file.
     *
     *  \returns true if this is a hex string.
     *  \see GetString() will return the raw string contents (not hex encoded)
     */
    inline bool IsHex () const;

    /**
     * PdfStrings are either Latin1 encoded or UTF-16BE
     * encoded unicode strings.
     * This functions returns true if this is a unicode
     * string object.
     *
     * \returns true if this is a unicode string.
     */
    inline bool IsUnicode () const;

    /** The contents of the strings can be read
     *  by this function.
     *
     *  The returned data is never hex encoded any maycontain 0 bytes.
     *
     *  if IsUnicode() returns true, the return value
     *  points to a UTF-16BE string buffer with Length()
     *  characters. Better use GetUnicode() in this case.
     * 
     *  \returns the strings contents which is guaranteed to be zero terminated
     *           but might also contain 0 bytes in the string.
     *  \see IsHex
     *  \see IsUnicode
     *  \see Length
     */
    inline const char* GetString() const;

    /** The contents of the strings can be read
     *  by this function.
     *
     *  The returned data is never hex encoded any maycontain 0 bytes.
     *
     *  if IsUnicode() returns true, the return value
     *  points to a UTF-16BE string buffer with Length()
     *  characters. Better use GetUnicode() in this case.
     * 
     *  \returns the strings contents which is guaranteed to be zero terminated
     *           but might also contain 0 bytes in the string.
     *
     *  \see IsHex
     *  \see IsUnicode
     *  \see Length
     */
    inline const pdf_utf16be* GetUnicode() const;

    /** The length of the string data returned by GetString() 
     *  in bytes not including terminating zeros.
     *
     *  \returns the length of the string. 
     */
    inline long GetLength() const;

    /** The length of the string data returned by GetUnicode() 
     *  in characters not including the terminating zero 
     *
     *  \returns the length of the string. 
     */
    inline long GetUnicodeLength() const;

    /** Write this PdfString in PDF format to a PdfOutputDevice 
     *  
     *  \param pDevice the output device.
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     */
    void Write ( PdfOutputDevice* pDevice, const PdfEncrypt* pEncrypt = NULL ) const;

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     *  \returns this object
     */
    const PdfString & operator=( const PdfString & rhs );

    /** Compare two PdfString objects
     *  \param rhs another PdfString to compare
     *  \returns this object
     */
    bool operator>( const PdfString & rhs ) const;

    /** Compare two PdfString objects
     *  \param rhs another PdfString to compare
     *  \returns this object
     */
    bool operator<( const PdfString & rhs ) const;

    /** Comparison operator
     *  \param rhs compare to this string object
     *  \returns true if both strings have the same contents
     */
    bool operator==( const PdfString & rhs ) const;

    /** Converts this string to a hex encoded string.
     *  
     *  If IsHex returns true, a copy of this string is returned
     *  otherwise the strings data is hex encoded and returned.
     *
     *  \returns a hex encoded version of this string or this string
     *           if it is already hex ecoded.
     *
     *  \see IsHex
     */
    //PdfString HexEncode() const; 

    /** Converts this string to a ascii string (not hex encoded)
     *  
     *  If IsHex returns false, a copy of this string is returned
     *  otherwise the strings data is hex decoded and returned.
     *
     *  \returns a plain version of this string which is not hex encoded
     *           or this string if it is already a plain not hex encoded string.
     *
     *  \see IsHex
     */
    //PdfString HexDecode() const; 

    /** Converts this string to a unicode string
     *  
     *  If IsUnicode() returns true a copy of this string is returned
     *  otherwise the string data is converted to UTF-16be and returned.
     *
     *  \returns a unicode version of this string
     */
    PdfString ToUnicode() const;

    static const PdfString StringNull;

    static long ConvertUTF8toUTF16( const pdf_utf8* pszUtf8, pdf_utf16be* pszUtf16, long lLenUtf16 );
    static long ConvertUTF8toUTF16( const pdf_utf8* pszUtf8, long lLenUtf8, 
                                    pdf_utf16be* pszUtf16, long lLenUtf16, 
                                    EPdfStringConversion eConversion = ePdfStringConversion_Strict  );

    static long ConvertUTF16toUTF8( const pdf_utf16be* pszUtf16, pdf_utf8* pszUtf8, long lLenUtf8 );
    static long ConvertUTF16toUTF8( const pdf_utf16be* pszUtf16, long lLenUtf16, 
                                    pdf_utf8* pszUtf8, long lLenUtf8, 
                                    EPdfStringConversion eConversion = ePdfStringConversion_Strict );

 private:
    /** Allocate m_lLen data for m_pszData if data
     *  does not fit into m_pBuffer.
     *  Otherwise m_pszData is set to point to 
     *  m_pBuffer.
     */
    void Allocate();

    /** Frees the internal buffer
     *  if it was allocated using malloc
     */
    void FreeBuffer();

    /** Construct a new PdfString from a 0 terminated
     *  string. 
     *  The input string will be copied.
     *  if m_bhex is true the copied data will be hex encoded.
     *
     *  \param pszString the string to copy
     *  \param lLen length of the string data to copy
     *  
     */
    void Init( const char* pszString, long lLen );

    /** Construct a new PdfString from a UTF8
     *  string. 
     *  The input string will be copied and converted to UTF-16be.
     *
     *  \param pszStringUtf8 the string to copy
     *  \param lLen number of bytes of the string data to copy
     *  
     */
    void InitFromUtf8( const pdf_utf8* pszStringUtf8, long lLen );

    /** Swap the bytes in the buffer (UTF16be -> UTF16le)
     *  \param pBuf buffer
     *  \param lLen length of buffer
     */
    static void SwapBytes( char* pBuf, long lLen ); 

 private:
    static const char        s_pszUnicodeMarker[];   ///< The unicode marker used to indicate unicode strings in PDF
    static const char*       s_pszUnicodeMarkerHex;  ///< The unicode marker converted to HEX
    static const pdf_utf16be s_cPdfDocEncoding[256]; ///< conversion table from PDFDocEncoding to UTF16

 private:
    PdfRefCountedBuffer m_buffer;                    ///< String data (always binary), may contain 0 bytes

    bool                m_bHex;                      ///< This string is converted to hex during write out
    bool                m_bUnicode;                  ///< This string contains unicode data
};

bool PdfString::IsValid() const
{
    return (m_buffer.GetBuffer() != NULL);
}

bool PdfString::IsHex () const
{
    return m_bHex;
}

bool PdfString::IsUnicode () const
{
    return m_bUnicode;
}

const char* PdfString::GetString() const
{
    return m_buffer.GetBuffer();
}

const pdf_utf16be* PdfString::GetUnicode() const
{
    return reinterpret_cast<pdf_utf16be*>(m_buffer.GetBuffer());
}

long PdfString::GetLength() const
{
    return m_buffer.GetSize() - 2;
}

long PdfString::GetUnicodeLength() const
{
    return (m_buffer.GetSize() / sizeof(pdf_utf16be)) - 2 * sizeof(pdf_utf16be);
}

};


#endif // _PDF_STRING_H_
