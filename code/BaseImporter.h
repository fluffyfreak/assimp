/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

/** @file Definition of the base class for all importer worker classes. */
#ifndef INCLUDED_AI_BASEIMPORTER_H
#define INCLUDED_AI_BASEIMPORTER_H

#include <string>
#include "./../include/aiTypes.h"

struct aiScene;

namespace Assimp	{

class IOSystem;
class Importer;

// utility to do char4 to uint32 in a portable manner
#define AI_MAKE_MAGIC(string) ((uint32_t)((string[0] << 24) + \
	(string[1] << 16) + (string[2] << 8) + string[3]))

// ---------------------------------------------------------------------------
/** Simple exception class to be thrown if an error occurs while importing. */
class ASSIMP_API ImportErrorException 
{
public:
	/** Constructor with arguments */
	ImportErrorException( const std::string& pErrorText)
	{
		mErrorText = pErrorText;
	}

	// -------------------------------------------------------------------
	/** Returns the error text provided when throwing the exception */
	inline const std::string& GetErrorText() const 
	{ return mErrorText; }

private:
	std::string mErrorText;
};

// ---------------------------------------------------------------------------
/** The BaseImporter defines a common interface for all importer worker 
 *  classes.
 *
 * The interface defines two functions: CanRead() is used to check if the 
 * importer can handle the format of the given file. If an implementation of 
 * this function returns true, the importer then calls ReadFile() which 
 * imports the given file. ReadFile is not overridable, it just calls 
 * InternReadFile() and catches any ImportErrorException that might occur.
 */
class ASSIMP_API BaseImporter
{
	friend class Importer;

protected:

	/** Constructor to be privately used by #Importer */
	BaseImporter();

	/** Destructor, private as well */
	virtual ~BaseImporter();

public:
	// -------------------------------------------------------------------
	/** Returns whether the class can handle the format of the given file.
	 *
	 * The implementation should be as quick as possible. A check for
	 * the file extension is enough. If no suitable loader is found with
	 * this strategy, CanRead() is called again, the 'checkSig' parameter
	 * set to true this time. Now the implementation is expected to
	 * perform a full check of the file format, possibly searching the
	 * first bytes of the file for magic identifiers or keywords.
	 *
	 * @param pFile Path and file name of the file to be examined.
	 * @param pIOHandler The IO handler to use for accessing any file.
	 * @param checkSig Set to true if this method is called a second time.
	 *   This time, the implementation may take more time to examine the
	 *   contents of the file to be loaded for magic bytes, keywords, etc
	 *   to be able to load files with unknown/not existent file extensions.
	 * @return true if the class can read this file, false if not.
	 *
	 * @note Sometimes ASSIMP uses this method to determine whether a
	 * a given file extension is generally supported. In this case the
	 * file extension is passed in the pFile parameter, pIOHandler is NULL
	 */
	virtual bool CanRead( const std::string& pFile, 
		IOSystem* pIOHandler, bool checkSig) const = 0;


	// -------------------------------------------------------------------
	/** Imports the given file and returns the imported data.
	 * If the import succeeds, ownership of the data is transferred to 
	 * the caller. If the import fails, NULL is returned. The function
	 * takes care that any partially constructed data is destroyed
	 * beforehand.
	 *
	 * @param pFile Path of the file to be imported. 
	 * @param pIOHandler IO-Handler used to open this and possible other files.
	 * @return The imported data or NULL if failed. If it failed a 
	 * human-readable error description can be retrieved by calling 
	 * GetErrorText()
	 *
	 * @note This function is not intended to be overridden. Implement 
	 * InternReadFile() to do the import. If an exception is thrown somewhere 
	 * in InternReadFile(), this function will catch it and transform it into
	 *  a suitable response to the caller.
	 */
	aiScene* ReadFile( const std::string& pFile, IOSystem* pIOHandler);


	// -------------------------------------------------------------------
	/** Returns the error description of the last error that occured. 
	 * @return A description of the last error that occured. An empty
	 * string if there was no error.
	 */
	const std::string& GetErrorText() const {
		return mErrorText;
	}


	// -------------------------------------------------------------------
	/** Called prior to ReadFile().
	 * The function is a request to the importer to update its configuration
	 * basing on the Importer's configuration property list.
	 * @param pImp Importer instance
	 * @param ppFlags Post-processing steps to be executed on the data
	 *  returned by the loaders. This value is provided to allow some
	 * internal optimizations.
	 */
	virtual void SetupProperties(const Importer* pImp /*,
		unsigned int ppFlags*/);

protected:

	// -------------------------------------------------------------------
	/** Called by Importer::GetExtensionList() for each loaded importer.
	 *  Importer implementations should append all file extensions
	 *  which they supported to the passed string.
	 *  Example: "*.blabb;*.quak;*.gug;*.foo" (no delimiter after the last!)
	 * @param append Output string
	 */
	virtual void GetExtensionList(std::string& append) = 0;

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. The 
	 * function is expected to throw an ImportErrorException if there is 
	 * an error. If it terminates normally, the data in aiScene is 
	 * expected to be correct. Override this function to implement the 
	 * actual importing.
	 * <br>
	 *  The output scene must meet the following requirements:<br>
	 * <ul>
	 * <li>At least a root node must be there, even if its only purpose
	 *     is to reference one mesh.</li>
	 * <li>aiMesh::mPrimitiveTypes may be 0. The types of primitives
	 *   in the mesh are determined automatically in this case.</li>
	 * <li>the vertex data is stored in a pseudo-indexed "verbose" format.
	 *   In fact this means that every vertex that is referenced by
	 *   a face is unique. Or the other way round: a vertex index may
	 *   not occur twice in a single aiMesh.</li>
	 * <li>aiAnimation::mDuration may be -1. Assimp determines the length
	 *   of the animation automatically in this case as the length of
	 *   the longest animation channel.</li>
	 * <li>aiMesh::mBitangents may be NULL if tangents and normals are
	 *   given. In this case bitangents are computed as the cross product
	 *   between normal and tangent.</li>
	 * <li>There needn't be a material. If none is there a default material
	 *   is generated. However, it is recommended practice for loaders
	 *   to generate a default material for yourself that matches the
	 *   default material setting for the file format better than Assimp's
	 *   generic default material. Note that default materials *should*
	 *   be named AI_DEFAULT_MATERIAL_NAME if they're just color-shaded
	 *   or AI_DEFAULT_TEXTURED_MATERIAL_NAME if they define a (dummy) 
	 *   texture. </li>
	 * </ul>
	 * If the AI_SCENE_FLAGS_INCOMPLETE-Flag is <b>not</b> set:<ul>
	 * <li> at least one mesh must be there</li>
	 * <li> there may be no meshes with 0 vertices or faces</li>
	 * </ul>
	 * This won't be checked (except by the validation step): Assimp will
	 * crash if one of the conditions is not met!
	 *
	 * @param pFile Path of the file to be imported.
	 * @param pScene The scene object to hold the imported data.
	 * NULL is not a valid parameter.
	 * @param pIOHandler The IO handler to use for any file access.
	 * NULL is not a valid parameter.
	 */
	virtual void InternReadFile( const std::string& pFile, 
		aiScene* pScene, IOSystem* pIOHandler) = 0;


	// -------------------------------------------------------------------
	/** A utility for CanRead().
	 *
	 *  The function searches the header of a file for a specific token
	 *  and returns true if this token is found. This works for text
	 *  files only. There is a rudimentary handling of UNICODE files.
	 *  The comparison is case independent.
	 *
	 *  @param pIOSystem IO System to work with
	 *  @param file File name of the file
	 *  @param tokens List of tokens to search for
	 *  @param numTokens Size of the token array
	 *  @param searchBytes Number of bytes to be searched for the tokens.
	 */
	static bool SearchFileHeaderForToken(IOSystem* pIOSystem, 
		const std::string&	file,
		const char**		tokens, 
		unsigned int		numTokens,
		unsigned int		searchBytes = 200);


	// -------------------------------------------------------------------
	/** @brief Check whether a file has a specific file extension
	 *  @param pFile Input file
	 *  @param ext0 Extension to check for. Lowercase characters only, no dot!
	 *  @param ext1 Optional second extension
	 *  @param ext2 Optional third extension
	 *  @note Case-insensitive
	 */
	static bool SimpleExtensionCheck (const std::string& pFile, 
		const char* ext0,
		const char* ext1 = NULL,
		const char* ext2 = NULL);

	// -------------------------------------------------------------------
	/** @brief Extract file extension from a string
	 *  @param pFile Input file
	 *  @return Extension without trailing dot, all lowercase
	 */
	static std::string GetExtension (const std::string& pFile);

	// -------------------------------------------------------------------
	/** @brief Check whether a file starts with one or more magic tokens
	 *  @param pFile Input file
	 *  @param pIOHandler IO system to be used
	 *  @param magic n magic tokens
	 *  @params num Size of magic
	 *  @param offset Offset from file start where tokens are located
	 *  @param Size of one token, in bytes. Maximally 16 bytes.
	 *  @return true if one of the given tokens was found
	 *
	 *  @note For convinence, the check is also performed for the
	 *  byte-swapped variant of all tokens (big endian). Only for
	 *  tokens of size 2,4.
	 */
	static bool CheckMagicToken(IOSystem* pIOHandler, const std::string& pFile, 
		const void* magic,
		unsigned int num,
		unsigned int offset = 0,
		unsigned int size   = 4);

#if 0 /** TODO **/
	// -------------------------------------------------------------------
	/** An utility for all text file loaders. It converts a file to our
	*  ASCII/UTF8 character set. Special unicode characters are lost.
	*
	*  @param buffer Input buffer. Needn't be terminated with zero.
	 *  @param length Length of the input buffer, in bytes. Receives the
	 *    number of output characters, excluding the terminal char.
	 *  @return true if the source format did not match our internal
	 *    format so it was converted.
	 */
	static bool ConvertToUTF8(const char* buffer, 
		unsigned int& length);
#endif

protected:

	/** Error description in case there was one. */
	std::string mErrorText;
};

struct BatchData;

// ---------------------------------------------------------------------------
/** A helper class that can be used by importers which need to load many
 *  extern meshes recursively.
 *
 *  The class uses several threads to load these meshes (or at least it
 *  could, this has not yet been implemented at the moment).
 *
 *  @note The class may not be used by more than one thread
 */
class ASSIMP_API BatchLoader
{
	// friend of Importer

public:

	/** Represents a full list of configuration properties
	 *  for the importer.
	 *
	 *  Properties can be set using SetGenericProperty
	 */
	struct PropertyMap
	{
		Importer::IntPropertyMap     ints;
		Importer::FloatPropertyMap   floats;
		Importer::StringPropertyMap  strings;

		bool operator == (const PropertyMap& prop) const {
			return ints == prop.ints && floats == prop.floats && strings == prop.strings; 
		}

		bool empty () const {
			return ints.empty() && floats.empty() && strings.empty();
		}
	};

public:
	

	/** Construct a batch loader from a given IO system
	 */
	BatchLoader(IOSystem* pIO);
	~BatchLoader();


	/** Add a new file to the list of files to be loaded.
	 *
	 *  @param file File to be loaded
	 *  @param steps Steps to be executed on the file
	 *  @param map Optional configuration properties
	 *  @return 'Load request channel' - an unique ID that can later
	 *    be used to access the imported file data.
	 */
	unsigned int AddLoadRequest	(const std::string& file,
		unsigned int steps = 0, const PropertyMap* map = NULL);


	/** Get an imported scene.
	 *
	 *  This polls the import from the internal request list.
	 *  If an import is requested several times, this function
	 *  can be called several times, too.
	 *
	 *  @param which LRWC returned by AddLoadRequest().
	 *  @return NULL if there is no scene with this file name
	 *  in the queue of the scene hasn't been loaded yet.
	 */
	aiScene* GetImport		(unsigned int which);


	/** Waits until all scenes have been loaded.
	 */
	void LoadAll();

private:

	// No need to have that in the public API ...
	BatchData* data;
};

} // end of namespace Assimp

#endif // AI_BASEIMPORTER_H_INC
