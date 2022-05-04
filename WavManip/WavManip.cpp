//Harriet Drury - 26/01/2022
//A programme to take an inputed Mono audio file and output two files, one with a doubled sample rate
//and another with a muted section

#pragma warning(disable:4996)		// For usage in visual studio, stops an fopen error. Remove if not using visual studio

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include "stdio.h"
using namespace std;

struct WAV_HEADER {
	uint8_t		RIFF[4];			// RIFF Header Magic header
	uint32_t	ChunkSize;			// RIFF Chunk Size
	uint8_t		WAVE[4];			// WAVE Header
	uint8_t		fmt[4];				// FMT header
	uint32_t	Subchunk1Size;		// Size of fmt chunk
	uint16_t	AudioFormat;		// Audio format 1=PCM,6=mulaw,7=alaw
	uint16_t	NumOfChan;			// Number of Channels 1=mono, 2 =stereo
	uint32_t	SamplesPerSec;		// Sampling Frequency in Hz
	uint32_t	bytesPerSec;		// bytes per second
	uint16_t	blockAlign;			// 2=16-bit mono, 4=16-bit stereo
	uint16_t	bitsPerSample;		// Number of bits per sample
	uint8_t		Subchunk2ID[4];		// "data" string
	uint32_t	Subchunk2Size;		// Sampled data length
} wav_hdr;

bool checkWav(string fileName){											// A function to check the file is correct
	string ending = ".wav";
	string fullString = fileName + ending;								// Used to place into fopen function
	bool found;

	FILE* wavFile = fopen(fullString.c_str(), "r");						// Casting to .c_str() for functionality of fopen
	if (!wavFile) {														// Checks that the WAV file can be opened
		cout << "\nThe file could not be opened" << endl;
		exit(1);
		fclose(wavFile);
		found = false;													// Sets the check boolean
	}
	else {
		cout << "\t\tFile Found - Success!" << endl;
		cout << "\t\t---------------------" << endl;
		found = true;													// Used in main to carry out upsampling and muting
		fclose(wavFile);
		cout << "Performing Upsampling and Muting....." << endl;
	};
	return found;
}

void printData(string fileName) {										// Printing the required/changed values within the header file
	WAV_HEADER wavHeader1;												// Declaring variables
	int headerSize = sizeof(WAV_HEADER);
	string ending = ".wav";
	string fullString = fileName + ending;
	FILE* wavFile = fopen(fullString.c_str(), "r");

	//Read the Header

	size_t bytesRead = fread(&wavHeader1, 1, headerSize, wavFile);		// reading the head file data and storing in a buffer
	fclose(wavFile);
	cout <<"\t\t" << fileName << " File Data" << "\n";					// Outputting required parts from Header
	cout << "RIFF Header			:" << wavHeader1.RIFF[0] << wavHeader1.RIFF[1] << wavHeader1.RIFF[2] << wavHeader1.RIFF[3] << '\n';
	cout << "WAVE Header			:" << wavHeader1.WAVE[0] << wavHeader1.WAVE[1] << wavHeader1.WAVE[2] << wavHeader1.WAVE[3] << '\n';
	cout << "FMT				:" << wavHeader1.fmt[0] << wavHeader1.fmt[1] << wavHeader1.fmt[2] << wavHeader1.fmt[3] << '\n';
	cout << "Channels			:" << wavHeader1.NumOfChan << '\n';
	cout << "Samples				:" << wavHeader1.SamplesPerSec << '\n';
	cout << "Data Size			:" << wavHeader1.Subchunk2Size << endl;
	cout << endl;
}

string upSample(string fileName){										// The change of sampling rate. Doubles the sample rate with interpolation
	WAV_HEADER wavHeader, wavHeader2;									// declares Header files for original file and new file
	vector<double> NewSignal;											// A vector to store the Data chunk
	int headerSize = sizeof(WAV_HEADER);								// obtaining the buffer size
	string ending = ".wav";
	string fullString = fileName + ending;
	FILE* wavFile = fopen(fullString.c_str(), "r");						// Obtains required information from the first file

	//Read the Header
	size_t bytesRead = fread(&wavHeader, 1, headerSize, wavFile);
	int length = wavHeader.Subchunk2Size;								// Using data size from file
	int8_t* data = new int8_t[length];			//8 bits signed pointer
	fseek(wavFile, headerSize, SEEK_SET);
	fread(data, sizeof(data[0]), length / (sizeof(data[0])), wavFile);
	for (int i = 0; i < length; i = i + 2) {
		int c = ((data[i] & 0xff) | (data[i + 1] << 8));				// Little Endian formatting
		double t;
		t = c / 32767.0;												// Normalisation
		NewSignal.push_back(t);											// Writing the data to the vector for manipulation
	}
	fclose(wavFile);

	// output data to a new wav file
	FILE* fptr2; // File pointer for WAV output
	string upSample = "upSampled";
	fptr2 = fopen("upSampled.wav", "wb"); // Open wav file for writing
	wavHeader2 = wavHeader;												// Manipulating the header file data with the change in sample rate
	wavHeader2.Subchunk2Size = wavHeader2.Subchunk2Size * 2;			// upsampling by 2
	wavHeader2.SamplesPerSec = wavHeader2.SamplesPerSec * 2;
	wavHeader2.bytesPerSec = wavHeader2.bytesPerSec * 2;

	fwrite(&wavHeader2, sizeof(wavHeader2), 1, fptr2);					// Writing the new header file to the generated WAV file

	for (int i = 0; i < length / 2 - 1; i++) {
		int p = i + 1;
		double avg = (NewSignal[p] + NewSignal[i]) / 2;					// Taking the average between points to create new data
		int value = (int)(32767 * NewSignal[i]);						// Normalise to integers
		int avgValue = (int)(32767 * avg);								// Normalise avg
		int size = 2;													// little endian formatting
		for (; size; --size, value >>= 8) {								// Writes the original data
			fwrite(&value, 1, 1, fptr2);
		}
		int size2 = 2;
		for (; size2; --size2, avgValue >>= 8) {						// Writes the interpolated data
			fwrite(&avgValue, 1, 1, fptr2);
		}
	}
	fclose(fptr2);
	cout << "UpSample Complete!" << endl;
	printData(upSample);												// Used to check the upsample was correct
	return upSample;
}

string muteSignal(string fileName) {
	WAV_HEADER wavHeader;												// Creating the WAV Header
	vector<double> NewSignal;											// Creating the Storage for new Data including the Mute
	int headerSize = sizeof(WAV_HEADER);								// Opening the Header file data
	string ending = ".wav";
	string fullString = fileName + ending;
	FILE* wavFile = fopen(fullString.c_str(), "r");						// Opening the file

	size_t bytesRead = fread(&wavHeader, 1, headerSize, wavFile);					// Reading the Header Chunk
	int length = wavHeader.Subchunk2Size;											// Finding the length of the Data Chunk
	int8_t* data = new int8_t[length];												// 8 bits signed pointer
	fseek(wavFile, headerSize, SEEK_SET);											// Pointing to the Data portion
	fread(data, sizeof(data[0]), length / (sizeof(data[0])), wavFile);				// Reading the Data to an array
	for (int i = 0; i < length; i = i + 2) {
		int c = ((data[i] & 0xff) | (data[i + 1] << 8));
		double t;
		if (i <= length / 2 + length / 100 || i >= length / 2 + length / 30) {		// Performing the mute with the required section
			t = c / 32767.0;														// Normalisation
		}
		else {
			t = 0;																	// Muting by commuting to 0
		}
		NewSignal.push_back(t);														// Placing normalised data into the vector
	}
	fclose(wavFile);

	string mutedSample = "mutedSample";
	FILE* fptr; // File pointer for WAV output
	fptr = fopen("mutedSample.wav", "wb"); // Open wav file for writing

	
	fwrite(&wavHeader, sizeof(wavHeader), 1, fptr);

	for (int i = 0; i < length /2 - 1; i++) {
		int value = (int)(32767 * NewSignal[i]);		// Normalise to integers
		int size = 2;									// little endian formatting
		for (; size; --size, value >>= 8) {
			fwrite(&value, 1, 1, fptr);
		}
	}
	fclose(fptr);
	cout << "Mute Complete!" << endl;
	printData(mutedSample);															// To check the writing to file was correct
	return mutedSample;
}


int main()
{
	string fileName = "SA2";														// Change when needed, used to input the WAV file in the same directory as the application
	bool found = checkWav(fileName);												// Checking validity
	if (found == true) {															// Running the upsampling and muting if valid file
	printData(fileName);
	muteSignal(upSample(fileName));
	}
	else { cout << "Check the File Name is Correct and in the correct folder"; }	// Providing an error message
	return 0;
}
