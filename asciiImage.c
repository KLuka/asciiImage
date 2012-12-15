/* asciiImage.c */
/*************************************************************************/
/*                                                                       */
/*  AUTHOR:                                                              */
/*		Luka Krajger                                                     */
/*                                                                       */
/*  NAME:                                                                */
/*		asciiImage                                                       */
/*                                                                       */
/*  DESCRIPTION:                                                         */
/*		This program prints 24-bit .bmp image in ascii character format  */
/*                                                                       */
/*  COMPILATION:                                                         */
/*		/$ gcc -Wall -o asciiImage asciiImage.c -O2 -lm                  */
/*                                                                       */
/*************************************************************************/


/*************************************************************************/
/*                            INCLUDES                                   */                
/*************************************************************************/

#include <stdio.h>		
#include <stdlib.h>			
#include <string.h>
#include <unistd.h> 
#include <math.h>			/* Use -lm comipialtion flag */

/*************************************************************************/
/*                          MACROS AND CONSTANTS                         */                
/*************************************************************************/



#define OK		1
#define ERROR (-1)

#define IMAGE_NAME_LEN      127

/* For reading binary files */
#ifdef WINDOWS
	#define READ_BINARY_FILE 	"rb"
#else
	#define READ_BINARY_FILE    "r"
#endif

/* Bmp file related */
#define BMP_HEADER_SIZE 	54 	

#define BMP_H_FILE_SIZE		0x02
#define BMP_H_OFFSET		0x0A
#define BMP_H_WIDTH			0x12
#define BMP_H_HEIGHT		0x16
#define BMP_H_RAW_SIZE		0x22

/* Html file related */
#define HTML_F_FAMILY	 "font-family: Courier, 'Courier New', monospace;"
#define HTML_F_SIZE  	 "font-size: xx-small;"
#define HTML_F_WEIGHT    "font-weight: bold;"
#define HTML_W_SPACE	 "white-space: pre;"

/*************************************************************************/
/*                             GLOBALS                                   */                
/*************************************************************************/

/* Structure for storing user input data */
struct userInputStruct {
	int infoFlag;
	int invertFlag ;              		
	int sizeMode;
	int bitGraphic;
	int htmlMode;
} typedef userInput_s;

/* Structure for holding image data */
struct imageDataStruct {
	int imgFileSize;
	int imgWidth;
	int imgHeight;
	int imgRawSize;
	int pixelOffset;
	int paddedBytes;
	int imgWidthInBytes;
	char imgName[IMAGE_NAME_LEN+1];
} typedef imageData_s;


/*************************************************************************/
/*                           PROTOTYPING                                 */                
/*************************************************************************/

/* Bmp format related functions */
int isBmpFormat( unsigned char *imgHeader);
int bmpGetWidth( unsigned char *imgHeader );
int bmpGetHeight( unsigned char *imgHeader );
int bmpGetOffset( unsigned char *imgHeader );
int bmpGetRawSize( unsigned char *imgHeader );
int bmpGetFileSize( unsigned char *imgHeader );

int bmpGetPaddedBytes ( int pixelWidth );
int bmpGetWidthInBytes( int pixelWidth );

int storeBmpImageData( char *imagePath , imageData_s *imageData );

/* Image processing functions */
unsigned char getAsciiSymbol( unsigned char grayValue , int bitGraphic  , int invertMode );
unsigned char pixelToGray( unsigned char redPix , unsigned char greenPix , unsigned char bluePix );

int makeGrayPixelMap( unsigned char **grayImageMap , imageData_s *imageData );
int printAsciiImage( unsigned char **grayImageMap, userInput_s *userInput, imageData_s *imageData );

void printImageInfo( imageData_s *imageData );

/* Other function prototypes */

void htmlFilePrintFooter( FILE *htmlFilePtr );
void htmlFilePrintHeader( FILE *htmlFilePtr );

int byteToInt( unsigned char *dataArray , int dataOffset , int numOfBytes );

void initUserInput( userInput_s *userInput );
void helpFunction(void);

/*************************************************************************/
/*                            FUNCTIONS                                  */                
/*************************************************************************/

/********************************************************************************
 *     Function: main
 *        Input:
 *       Output:
 *  Description: main program function
 ********************************************************************************/


int main(int argc, char *argv[])
{
	int i;	
	int retVal;

	char *imagePath = "null";
	unsigned char **grayPixelMap;


	imageData_s imageData;
	userInput_s userArgs;
	
	/* Init */
	initUserInput( &userArgs );

	/*************************************************************************/
	/*                           Get user input                              */                
	/*************************************************************************/

	if( argc < 2 ) {
		printf("Use -h or --help flag for help!\n");
		return 0;
	}

	/* Loop input arguments */
	for( i=1 ; i<argc ; i++ ) {

		/* Get image name - first argument */
		if( i == 1 ) {					
			imagePath = argv[1];
		}

		/* --info flag */
		if( strcmp( argv[i] , "--info") == 0) {
			userArgs.infoFlag = 1;	
			continue;
		}

		/* -i, --invert flags */
		if( (strcmp( argv[i] , "-i" ) == 0 ) || 
				(strcmp( argv[i] , "--invert")== 0) ) {
			userArgs.invertFlag = 1;	
		}
	
		/* -b, --bitGraphic flags */
		if( (strcmp( argv[i] , "-b" ) == 0 ) || 
				(strcmp( argv[i] , "--bitGraphic")== 0) ) {

			if( argv[i+1] != NULL ) {
				userArgs.bitGraphic = atoi(argv[i+1]);
				if( (userArgs.bitGraphic < 1) || (userArgs.bitGraphic > 4) ) {
					printf(" Warrning: -b option must be set to 1, 2, 3 or 4!\n");
					userArgs.bitGraphic = 4;							/* Using default value */
				}
			} else {
				printf(" Warrning: -b option must be set to 1, 2, 3 or 4!\n");
			}
			continue;
		}

		/* -s, --size flags */
		if( (strcmp( argv[i] , "-s" ) == 0 ) || 
				(strcmp( argv[i] , "--size")== 0) ) {

			if( argv[i+1] != NULL ) {
				userArgs.sizeMode = atoi(argv[i+1]);
				if( (userArgs.sizeMode < 1) || (userArgs.sizeMode > 10) ) {
					printf(" Warrning: -s option must be set [ 1 - 10 ]!\n");
					userArgs.sizeMode = 6;							/* Using default value */
				}
			} else {
				printf(" Warrning: -b option must be set to 1, 2, 3 or 4!\n");
			}
			continue;
		}

		/* --html flag */
		if( strcmp( argv[i] , "--html" ) == 0 ) {
			userArgs.htmlMode = 1;	
		}

		/* -h, --help flags */
		if( (strcmp( argv[i] , "-h" ) == 0 ) || 
				(strcmp( argv[i] , "--help")== 0) ) {
			helpFunction();
			return 0;
		}

	} /* END Loop input arguments */

	/*************************************************************************/
	/*                           Get image info                              */                
	/*************************************************************************/
	
	retVal = storeBmpImageData( imagePath , &imageData  );
	if( retVal < 0 ) {
		return 0;
	}

	/* Print image data */
	if( userArgs.infoFlag == 1 ) {
		printImageInfo( &imageData );
		return 0;
	}

	/*************************************************************************/
	/*                       Make gray scale pixel map                       */                
	/*************************************************************************/

	/* Allocate memory for grayPixelMap - pointers to lines  */
	grayPixelMap = malloc( imageData.imgHeight * sizeof( unsigned char *));
	if( grayPixelMap == NULL) {
		printf("Cannot allocate memory!\n");
		return 0;
	}

	/* Allocate memory for lines in grayPixelMap */
	for( i=0 ; i < imageData.imgHeight ; i++ ) {
		grayPixelMap[i] = malloc( imageData.imgWidth * sizeof(unsigned char));
		if( grayPixelMap[i] == NULL ) {
			printf("Cannot allocate memory!\n");
			return 0;
		}
	}

	/* Make map */
	retVal = makeGrayPixelMap( grayPixelMap , &imageData );
	if( retVal < 0 ) {
		printf("Cannot create gray-scale pixel map!\n");
		return 0;
	}

	/*************************************************************************/
	/*                         Main                                          */                
	/*************************************************************************/

	printAsciiImage ( grayPixelMap , &userArgs , &imageData );

	free(grayPixelMap);

	return 0;
}

/********************************************************************************
*     FUNCTION: initUserInput
*        INPUT: userInput - structure for storing user input data
*       OUTPUT: /
*  DESCRIPTION: This function initializes userInput_s
********************************************************************************/

void initUserInput( userInput_s *userInput )
{
	userInput->infoFlag = 0;
	userInput->invertFlag = 0;
	userInput->sizeMode = 6;
	userInput->bitGraphic = 4;
	userInput->htmlMode = 0;
	
	return;
}


/********************************************************************************
*     FUNCTION: printAsciiImage
*        INPUT: **grayImageMap - gray scale image map
*				userInput      - user input data strucure
*				imageData      - image data structure
*				outputPath       - if NULL, print goes to standard output else it 
*                                goes to the file
*       OUTPUT:	0
*  DESCRIPTION: This function prints ascii image to provided output
********************************************************************************/

int printAsciiImage( unsigned char **grayImageMap, userInput_s *userInput, imageData_s *imageData )
{
	int pix;
	int line;
	int xAxe;
	int yAxe;	
	int symTemp;
	int symIndex;
	int symAverage;
	int symbolWidth;			/* How many pixels is in one printed symbol */
	int symbolHeight;

	char outFilePath[IMAGE_NAME_LEN];
	char *bufferedLine;
	
	FILE *outFilePtr;

	/*************************************************************************/
	/*                           Printing settings                           */                
	/*************************************************************************/

	/* Set width and height of one symbol - larger the width of symbol smaller the picture */
	switch( userInput->sizeMode ) {
		case 0 :
			symbolWidth = 8;			/* Default size */
			break;
		case 10:
			symbolWidth = 1;			/* Larges size output */
			break;
		
		default:
			symbolWidth = (userInput->sizeMode-10) * (-2);

	}
	
	/* Height to width ratio is 2:1 */
	symbolHeight = symbolWidth * 2;

	/* Allocate memory for output line */
	bufferedLine = malloc( ((imageData->imgWidth / symbolWidth) + 1) *  sizeof(char));
	if( bufferedLine == NULL ) {
		printf("Could not allocate memory for bufferedLine!\n");
		return ERROR;
	}
	
	/* Html mode */
	if( userInput->htmlMode ) {

		/* Create output filename */
		snprintf( outFilePath , IMAGE_NAME_LEN , "%s%s" , imageData->imgName , ".html" );

		/* Create or owerwrite file */
		outFilePtr = fopen( outFilePath  , "w" );				/* Print to html file */
		if( outFilePtr == NULL ) {
			printf("Could not open file %s", outFilePath );
			free(bufferedLine);
			return ERROR ;
		}

		/* Print html header to file */
		htmlFilePrintHeader( outFilePtr );

	} else {
		outFilePtr = stdout; 								/* Print to console */
	}

	/*************************************************************************/
	/*                           Print ascii image                           */                
	/*************************************************************************/
	
	/* Move down the lines of 2D map */
	for( yAxe = 0; yAxe < (imageData->imgHeight - symbolHeight); yAxe = yAxe + symbolHeight ) {
	
		symIndex = 0;

		/* Move throug the pixels in 2D map */
		for( xAxe = 0; xAxe < (imageData->imgWidth - symbolWidth); xAxe = xAxe + symbolWidth ) {
		
			symTemp = 0;

			/* Clacualte average for one symbol  */
			for( line = yAxe ; line < (symbolHeight + yAxe); line++ ) {
				for( pix = xAxe ; pix < (symbolWidth + xAxe); pix++ ) {
					/* Add all pixel values in range of one symbol */
					symTemp = symTemp + grayImageMap[line][pix];	
				}
			}
			/* Average */
			symAverage = symTemp / ( symbolWidth * symbolHeight );
			/* Store one ascii symbol */
			bufferedLine[symIndex] = getAsciiSymbol(symAverage , userInput->bitGraphic , userInput->invertFlag );
			symIndex++;

		} /* END Move throug the pixels in 2D map */

		bufferedLine[symIndex] = '\0';
		fprintf( outFilePtr , "%s\n" , bufferedLine );
	
	} /* END Move down the lines of 2D map */

	/*************************************************************************/
	/*                             Clean up                                  */
	/*************************************************************************/
	
	/* Html mode */
	if( userInput->htmlMode ) {
		printf(" Ascii image printed to file %s\n" , outFilePath );
		htmlFilePrintFooter( outFilePtr );
		fclose(outFilePtr);
	}
 
	free(bufferedLine);

	return OK;

}

/********************************************************************************
*     FUNCTION: htmlFilePrintHeader
*        INPUT: pointer to html file
*       OUTPUT: /
*  DESCRIPTION: This function prints html header to file
********************************************************************************/

void htmlFilePrintHeader( FILE *htmlFilePtr )
{
	fprintf( htmlFilePtr, "%s", "<!DOCTYPE html>\n<html>\n<head>\n</head>\n<body>\n<div style=\"");
	fprintf( htmlFilePtr, "%s", HTML_W_SPACE HTML_F_FAMILY HTML_F_SIZE HTML_F_WEIGHT );
	fprintf( htmlFilePtr, "%s", "\">\n");
	return;
}

/********************************************************************************
*     FUNCTION: htmlFilePrintFooter
*        INPUT: pointer to html file
*       OUTPUT: /
*  DESCRIPTION: This function prints html footer to file
********************************************************************************/

void htmlFilePrintFooter( FILE *htmlFilePtr )
{
	fprintf( htmlFilePtr, "%s", "</div>\n</body>\n</html>");
	return;
}

/********************************************************************************
 *     Function: getAsciiSymbol 
 *        Input: grayValue  - pixel gray scale ( 0 - 255 )
 *               bitGraphic - 1 , 2 , 3 , 4 bit graphic
 *               inverMode  - 1 for inverted
 *       Output: Ascii symbol 
 *  Description: This function returns ascii symbol 
 ********************************************************************************/

unsigned char getAsciiSymbol( unsigned char grayValue , int bitGraphic  , int invertMode )
{

	char retSymbol;

	char tabel_1[] = "# ";                  	/* 1 bit graphic */
	char tabel_2[] = "#6+ "; 					/* 2 bit graphic */	
	char tabel_3[] = "#&$21:- ";				/* 3 bit graphic */
	char tabel_4[] = "##&8$62I1|:+-.  ";	    /* 4 bit graphic */

	/* Inverted mode */
	if( invertMode == 1 ) {
		grayValue = 255 - grayValue;
	}

	/* Calculate return symbol according to bitGraphic and grayValue */
	switch ( bitGraphic ) {
		case 1:
			retSymbol = tabel_1[grayValue/128];
			break;
		case 2:
			retSymbol = tabel_2[grayValue/64];
			break;
		case 3:
			retSymbol = tabel_3[grayValue/32];
			break;
		case 4:
			retSymbol = tabel_4[grayValue/16];
			break;
				
		default:
			printf("%d Bit grapic not supported!\n" , bitGraphic );
			retSymbol = 0;
	}

	return retSymbol;

}


/********************************************************************************
 *     FUNCTION: makeGrayPixelMap
 *        INPUT: **grayImageMap - 2D map for retriving grayscale pixels
 *               *imageData     - image data struct
 *       OUTPUT: ERROR or OK
 *  DESCRIPTION: This function converts .bmp file to 2D gray pixel map
 ********************************************************************************/

int makeGrayPixelMap( unsigned char **grayImageMap , imageData_s *imageData )
{
	int i, j;
	int line;
	int pixel;
	int retVal;
	int usefullBytesInLine;
	
	FILE *filePtr;
	unsigned char *lineBuffer;	
	
	/* Allocat memory for whole line of RGB pixels */
	lineBuffer = malloc( imageData->imgWidthInBytes * sizeof(unsigned char));
	if( lineBuffer == NULL ) {
		printf("Cannot allocate memory for lineBuffer!\n");
		return ERROR;
	}


	/* Open image file */
	filePtr = fopen( imageData->imgName , READ_BINARY_FILE );
	if( filePtr == NULL ) {
		printf("Cannot open file %s!\n", imageData->imgName );
		free(lineBuffer);
		return ERROR;
	}

	/* Move to begining of RBG pixels */
	retVal = fseek( filePtr , imageData->pixelOffset , SEEK_SET );
	if( retVal < 0 ) {
		printf("Error: fseek function!\n");
		free(lineBuffer);
		fclose(filePtr);
		return ERROR;
	}

	/*************************************************************************/
	/*                           Make gray image map                         */                
	/*************************************************************************/

	/* INFO: bmp format stores first pixel line on the end of file */
	line = imageData->imgHeight - 1;

	/* INFO: bmp format adds zero bytes to the end of each line */
	usefullBytesInLine = (imageData->imgWidthInBytes - imageData->paddedBytes );

	/* Read all lines and store gray pixels in grayImageMap */
	for( i=0 ; i < imageData->imgHeight ; i++ , line-- ) {	
		
		/* Read on line of RGB pixels */
		retVal = fread( lineBuffer, 1 , imageData->imgWidthInBytes , filePtr );
		if( retVal < 0 ) {
			printf("Cannot read form file!\n");
			free(lineBuffer);
			fclose(filePtr);
			return ERROR;
		}
	

		/* Convert line of RGB pixels to line of gray pixels */
		for( j=0 , pixel=0 ; j < usefullBytesInLine  ; j=j+3 , pixel++ ) {
			/* Store gray pixel */	
			grayImageMap[line][pixel] = pixelToGray( lineBuffer[j], lineBuffer[j+1], lineBuffer[j+2]);
				
		}
	}

	/* Clean up */
	fclose(filePtr);
	free(lineBuffer);

	return OK;

}

/********************************************************************************
*     FUNCTION: pixelToGray
*        INPUT: redPix   - color value ( 0 - 255 ) 
*               greenPix - color value ( 0 - 255 ) 
*               bluePix  - color value ( 0 - 255 ) 
*       OUTPUT: Gray pixel value
*  DESCRIPTION: This function return gray-scale pixel
********************************************************************************/

unsigned char pixelToGray( unsigned char redPix , unsigned char greenPix , unsigned char bluePix )
{

	unsigned char grayPixel ;

	grayPixel = (int) (redPix + greenPix + bluePix ) / 3;

	return grayPixel;

}

/********************************************************************************
*     FUNCTION: storeBmpImageData
*        INPUT: imagePath - image location on filesystem
*               imageData - structure for retrived data
*       OUTPUT: ERROR or OK
*  DESCRIPTION: This function retrives data from image header and stores data
*               in imageData_s
********************************************************************************/

int storeBmpImageData( char *imagePath , imageData_s *imageData )
{
	int retVal;
	FILE *filePtr;

	unsigned char imageHeader[BMP_HEADER_SIZE];

	/* Open and read image header */	
	filePtr = fopen( imagePath , "r" );
	if( filePtr == NULL ) {
		printf("Cannot open file %s!\n", imagePath );
		return ERROR;
	}

	retVal = fread( imageHeader, 1 , BMP_HEADER_SIZE , filePtr );
	if( retVal < 0 ) {
		printf("Cannot read form file %s!\n", imagePath );
		return ERROR;
	}

	fclose(filePtr);
	
	/* Check image format */
	if( !isBmpFormat(imageHeader) ) {
		printf("File %s is not .bmp file!\n", imagePath );
		return ERROR;
	}
	
	/* Check image name */
	if(	strlen( imagePath ) > IMAGE_NAME_LEN ) {
		printf("Image path to long to store!\n");
		return ERROR;
	}

	/* Store image data in structure */	
	memset( imageData->imgName , 0 , IMAGE_NAME_LEN+1 );
	strncpy( imageData->imgName , imagePath , IMAGE_NAME_LEN );

	imageData->imgWidth = bmpGetWidth(imageHeader);
	imageData->imgHeight = bmpGetHeight(imageHeader);
	imageData->imgRawSize = bmpGetRawSize(imageHeader);
	imageData->pixelOffset = bmpGetOffset(imageHeader);
	imageData->imgFileSize = bmpGetFileSize(imageHeader);

	imageData->paddedBytes = bmpGetPaddedBytes( imageData->imgWidth );
	imageData->imgWidthInBytes = bmpGetWidthInBytes( imageData->imgWidth );

	return OK;

}


/********************************************************************************
 *     FUNCTION: bmpGetWidthInBytes
 *        INPUT: pixelWidth - width in pixels
 *       OUTPUT: Width in bytes
 *  DESCRIPTION: This function returns width of image array in bytes. Whidth is
 *               affected by number of pixels and zero byte padding.
 *               In bmp standard number of bytes in one line must be multiple of 
 *               number 4. If it is not zero bytes are added.
 ********************************************************************************/

int bmpGetWidthInBytes( int pixelWidth )
{

	int byteWidth;

	byteWidth = pixelWidth * 3;			/* For 24-bit image depth */

	switch( byteWidth % 4 ) {  
		case 0 :						/* Nothing padded  */	
			break;

		case 1 : 
			byteWidth = byteWidth + 3;	/* 3 zero bytes padded */
			break;

		case 2 :
			byteWidth = byteWidth + 2;	/* 2 zero bytes padded */
			break;

		case 3 :
			byteWidth = byteWidth + 1;	/* 1 zero bytes padded */
			break;

		default:
			printf("Cannot happen!\n");

	}

	return byteWidth;

}


/********************************************************************************
 *     FUNCTION: bmpGetPaddedBytes
 *        INPUT: pixelWidth - width in pixels
 *       OUTPUT: Padded bytes
 *  DESCRIPTION: This function returns number of added bytes.
 *               In bmp standard number of bytes in one line must be multiple of 
 *               number 4. If it is not, zero bytes are added.
 ********************************************************************************/

int bmpGetPaddedBytes( int pixelWidth )
{

	int byteWidth;
	int paddedBytes;

	paddedBytes = 0;

	byteWidth = pixelWidth * 3;		/* For 24-bit image depth */
	
	switch( byteWidth % 4 ) {  
		case 0 :				/* Nothing padded  */	
			break;

		case 1 : 
			paddedBytes = 3;	/* 3 zero bytes padded */
			break;

		case 2 :
			paddedBytes = 2;	/* 2 zero bytes padded */
			break;

		case 3 :
			paddedBytes = 1;	/* 1 zero bytes padded */
			break;

		default:
			printf("Cannot happen!\n");

	}

	return paddedBytes;

}

/********************************************************************************
 *     FUNCTION: bmpGetFileSize
 *        INPUT: *imgHeader - pointer to image header
 *       OUTPUT: Acctual file size
 *  DESCRIPTION: /
 ********************************************************************************/

int bmpGetFileSize( unsigned char *imgHeader )
{
	return byteToInt( imgHeader , BMP_H_FILE_SIZE , 4 ); 
}

/********************************************************************************
 *     FUNCTION: bmpGetOffset
 *        INPUT: *imgHeader - pointer to image header
 *       OUTPUT: Offset where the pixel array starts in bmp file 
 *  DESCRIPTION: /
 ********************************************************************************/

int bmpGetOffset( unsigned char *imgHeader )
{
	return byteToInt( imgHeader , BMP_H_OFFSET , 4 ); 
}

/********************************************************************************
 *     FUNCTION: bmpGetWidth
 *        INPUT: *imgHeader - pointer to image header
 *       OUTPUT: Image width in pixels
 *  DESCRIPTION: /
 ********************************************************************************/

int bmpGetWidth( unsigned char *imgHeader )
{
	return byteToInt( imgHeader , BMP_H_WIDTH , 4 ); 
}

/********************************************************************************
 *     FUNCTION: bmpGetHeight
 *        INPUT: *imgHeader - pointer to image header
 *       OUTPUT: Image height in pixels
 *  DESCRIPTION: /
 ********************************************************************************/

int bmpGetHeight( unsigned char *imgHeader )
{
	return byteToInt( imgHeader , BMP_H_HEIGHT , 4 ); 
}

/********************************************************************************
 *     FUNCTION: bmpGetRawSize
 *        INPUT: *imgHeader - pointer to image header
 *       OUTPUT: Raw size of image data (including padded bytes)
 *  DESCRIPTION: Returns raw size of image data
 ********************************************************************************/

int bmpGetRawSize( unsigned char *imgHeader )
{
	return byteToInt( imgHeader , BMP_H_RAW_SIZE , 4 ); 
}

/********************************************************************************
 *     FUNCTION: byteToInt
 *        INPUT: *dataArray  - pointer to array of bytes
 *               dataOffset  - offset in array
 *               numOfBytes  - number of bytes
 *       OUTPUT: Integer value
 *  DESCRIPTION: This function reads given number of bytes starting form offset
 *               and returns integer value writen in this bytes
 ********************************************************************************/

int byteToInt( unsigned char *dataArray , int dataOffset , int numOfBytes ) 
{
	int i;	
	int tmpInt;
	int retVal;

	retVal = 0;

	if( numOfBytes <= 0 ) return 0;

	/* Get value */
	for( i = 0 ; i < numOfBytes ; i++ ) {
		tmpInt = (int) pow( 256 , i );
		retVal = retVal + ((int) dataArray[ dataOffset+i ] * tmpInt  );
	}

	return retVal;

}

/********************************************************************************
 *     FUNCTION: isBmpFormat
 *        INPUT: imgHeader - image header file
 *       OUTPUT: True (1) or false (0) 
 *  DESCRIPTION: This function checks image header if file is in BMP format 
 ********************************************************************************/

int isBmpFormat( unsigned char *imgHeader )
{
	if( ( imgHeader[0] == 0x42 ) &&				/* First byte in file  */
		( imgHeader[1] == 0x4d ) ) { 			/* Second byte in file */
		return 1;
	}	

	return 0;
}

/********************************************************************************
 *     FUNCTION: printImageInfo
 *        INPUT: imageData_s - image data holding structure
 *       OUTPUT: /
 *  DESCRIPTION: This function prints basic image data
 ********************************************************************************/

void printImageInfo( imageData_s *imageData )
{

	printf("-----------------------------------\n");
	printf("Image info:\n");

	printf(" -    file size: %d B\n -  image width: %d pixels\n - image height: %d pixels\n",
			imageData->imgFileSize , imageData->imgWidth , imageData->imgHeight );

	printf(" - padded bytes: %d B\n - image header: %d B\n", imageData->paddedBytes,
						imageData->pixelOffset );

	printf("-----------------------------------\n");
	return;	

}

/********************************************************************************
 *     Function: helpFunction
 *        Input: /
 *       Output: /
 *  Description: This function prints help for this program
 ********************************************************************************/

void helpFunction( void )
{

	printf("========================== HELP ==========================\n");
	printf(" This program converts 24-bit .bmp image to ASCII image.\n");
	printf("      Image is printed on standard output.\n\n");

	printf(" Usage: asciiImage FILE [OPTION] \n");

	printf(" Options:\n");
	printf(" -b, -bitGraphic    ... bit color option: 1 bit .. 4 bit\n");
	printf(" -h, --help         ... this menu\n");
	printf(" --html             ... print image to .html file\n");
	printf(" --info             ... print image info\n");
	printf(" -i, --invert       ... invert ascii colors\n");
	printf(" -s, --size         ... size option [1-10]\n\n");

	printf("==========================================================\n");

	return;

}
