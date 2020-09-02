#ifndef _OBJ_LOADER_H_
	#define _OBJ_LOADER_H_

	#include <vector>
	#include <string>

	struct vertexInfo {
		int vIndice;
		int tIndice;
		int nIndice;

		vertexInfo(int _vIndice, int _tIndice = -1, int _nIndice = -1) : vIndice(_vIndice), tIndice(_tIndice), nIndice(_nIndice) {} };

	class OBJLoader {
		private: std::vector<std::string> tokenize(const char* line, const char token, bool skip=false);

		protected:
			std::string mFileName;
			float* mVertices;
			float* mTextureCoordinates;
			float* mNormals;
			unsigned int* mIndices;

			unsigned int nIndices;
			unsigned int nVertices;

		public:
			OBJLoader (const char* file);
			~OBJLoader ();

			bool init ();

			const float* getVerticesArray () { return mVertices; }
			const float* getTextureCoordinatesArray () { return mTextureCoordinates; }
			const float* getNormalsArray () { return mNormals; }
			unsigned int* getIndicesArray () { return mIndices; }

			inline unsigned int getNIndices() { return nIndices; }
			inline unsigned int getNVertices() { return nVertices; }};
#endif