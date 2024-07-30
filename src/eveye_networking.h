//XXX NN https://www.geeksforgeeks.org/udp-server-client-implementation-c/
// Simple UDP server: ncat -e /bin/cat -k -u -l 8080
#ifndef __NETWORKING__
#define __NETWORKING__

// From CommonDefs.h
#if RExt__HIGH_BIT_DEPTH_SUPPORT
typedef       int             Pel;               ///< pixel type
#else
typedef       short           Pel;               ///< pixel type
#endif

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdbool.h>

// Maximum size in bytes of the mesage received from the NN server
// 8192 bytes is enough for a 64x64 context, 16bpp  with 10 bits dynamic
#define NN_MAX_RCV_BUFFER_LEN 8192

// Size (edge) of the input expected by the NN
#define NN_CONTEXT_SIZE 64
// Size (edge) of the NN output (currently used only in NN_CropBottomRight() and where the latter is called within eveye_pintra.c, which is however define'd out)
#define NN_PREDICTOR_SIZE 32

// Defines which CUs are predicted by the NN according to their size (by default, none)
// AF TODO this must be selectable at runtime
#define NN_INTRA_32 1
#define NN_INTRA_16 0
#define NN_INTRA_8  0
#define NN_INTRA_4  0

// Allows the encoder to choose between the EVC and the NN predictor (bitstream is not decodable anymore)
#define NN_ORACLE 0

// Global structures pointers needed to communicate with the NN server
//extern int *gNNSockfd;
//extern struct sockaddr_in *gNNServaddr;
// For statistics, we save the number of times each predictor has been chsen and the relative MSE saves
extern int gNNCntHEVC,  gNNCntEnh;
extern float gNNMSEHEVC, gNNMSEEnh;

// Number of times the NN has been called, for loggin purposes
extern int gNNCounter;

// Number of times I write a block in the dataset, for loggin purposes
extern float gBlockCounter64x64, gBlockCounter32x32, gBlockCounter16x16, gBlockCounter8x8, gBlockCounter4x4;

extern float intra_IPD_DC, intra_IPD_HOR, intra_IPD_VER, intra_IPD_UL, intra_IPD_UR;

// Creates the socket used to communicate with the NN server
void NN_setupServer();

// Destroys the socket used to communicate with the NN server
void NN_destroyServer();

// Copies the predictor into the context
void NN_CopyPredictorIntoContext (unsigned char *contextPtr, unsigned char *predictorPtr, int contextWidth, int contextHeight, int predictorWidth, int predictorHeight, int sizeofPixel);
void NN_CopyPredictorIntoContext16 (Pel *contextPtr, Pel *predictorPtr, int contextWidth, int contextHeight, int predictorWidth, int predictorHeight);
//Crop the bottom right cuw x cuh corner from the NN_PREDICTOR_SIZE x NN_PREDICTOR_SIZE block received from the server
void NN_CropBottomRight (Pel *rcvd16bpp,Pel *rcvdcrop, int cuw, int cuh );

// Allocates enough memory to copy a 16bpp predictor to a  8bpp predictor, perform the conversion from Pel to char
unsigned char * NN_Pel2Char (Pel *buffer16bpp, int width, int height, int stride);
unsigned char * NN_Pel2Char16(Pel *buffer16bpp, int width, int height, int stride);

// Converts a 8bpp predictor into the 16bpp format required by HM
// No memory is allocated since the buffer is assumed to be pre-existing
void NN_Char2Pel (Pel *buffer16bpp, unsigned char *buffer8bpp, int width, int height, int stride);
void NN_Char2Pel16(Pel *buffer16bpp, unsigned char *buffer8bpp, int width, int height, int stride);

// Sends the 8bpp predictor to the NN server
void NN_sendTo (unsigned char *msg, int msglen, int portDelta);
void NN_sendTo16 (Pel *msg, int msglen, int portDelta);

// Receives the 8bpp enhanced predictor from the NN server
// @return the number of received bytes
int NN_recvFrom (unsigned char **msg);
int NN_recvFrom16 (Pel **msg);

// Saves a predictor to the filesystem as 16bpp Y file
void NN_savePredictor(const char *fileName, Pel*  predictor, int width, int height, int stride, bool appendMode);

// computes the per-pixel MSE between two 8bpp predictors
float NN_computeMSE(Pel* ptrA, Pel* ptrB, int width, int height, int stride);

// Returns true if a NN_CONTEXT_SIZE x NN_CONTEXT_SIZE context is available
bool NN_pintra_context_available (int x, int y, int cuw, int cuh);

// Functions for statistics
static float mymse=0;
static int block = 64;
static int intra_mode = 0;
void NN_statsHEVCUpdate(float mymse);
void NN_statsEnhancedUpdate(float mymse);
void NN_statsPrint();
void NN_statsEVCupdate(int block);
void NN_statsIPMupdate(int intra_mode);
#endif

