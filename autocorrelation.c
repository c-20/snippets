	
// teststereo.c

#include "dsk6713_aic23.h"		        	//codec-DSK support file


#include <math.h>
#include <stdio.h>
#include <stdlib.h>



#define PI_F	3.14159265f

#define SILENT_FRAME				-9999.0f
#define MASSIVE_NUMBER				9999.0f

#define MAX_USERS					3
#define NUM_REALISATIONS			5
#define NUM_REALISATIONS_F			8.0f
#define NUM_BUFFERS					51
#define NUM_FRAMES_PER_SIGNAL		49
#define NUM_SAMPLES_PER_FRAME		320
#define HALF_SAMPLES_PER_FRAME		160

#define NUM_FEATURES_PER_FRAME		10
#define PRE_EMPHASIS_COEFF			-1.105f
#define PEAK_EMPHASIS_F				4.0f
#define POWER_TO_NOISE_RATIO_F		10.0f
#define NUM_FRAMES_IN_NOISE_AVERAGE	10
#define NUM_FRAMES_IN_VOICE_CONFIRM	5
#define THRESHOLD_TOLERANCE_F		0.1f
#define NOISE_FILTER_LENGTH			40

#define NUM_SAMPLES_IN_LOW_TONE		48
#define NUM_PERIODS_IN_LOW_TONE		75
#define NUM_SAMPLES_IN_HIGH_TONE	24
#define NUM_PERIODS_IN_HIGH_TONE	150
#define TONE_VOLUME					500

#define CIRCULAR(var)	((var + NUM_BUFFERS) % NUM_BUFFERS)
#define UNCIRCULAR(var)	((var % NUM_BUFFERS) + NUM_BUFFERS)

/*************************************************************************************/

enum State { IDLE, VOICE_DETECTED, VOICE_CONFIRMED, BUSY };
enum Mode { NONE, TRAINING, TESTING, LOWTONE, HIGHTONE, START };
enum State state;
enum Mode mode;

short debugMode;

short tone_sampleCount;
short tone_periodCount;
short tone_isHigh;

float frameBuffers[NUM_BUFFERS][NUM_SAMPLES_PER_FRAME];
short currentFrame;
short firstVoiceFrame;
short numVoiceFrames;
short currentSample;
float prevValue;

float framePower[NUM_BUFFERS];
float noisePower;
float powerThreshold;
float zeroOffset;

float noiseFilter[NOISE_FILTER_LENGTH];

float training_patterns[MAX_USERS][NUM_REALISATIONS][NUM_FRAMES_PER_SIGNAL][NUM_FEATURES_PER_FRAME];
float training_thresholds[MAX_USERS];
short training_numUsers;

float test_pattern[NUM_FRAMES_PER_SIGNAL][NUM_FEATURES_PER_FRAME];


/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/
float noiseFilter_prevValues[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
float fir(const float x, const float *bm, float *pastx, const int M) {
	int i = 0;
	float y = 0;
	
	//Calcuate b0 * x
	y = bm[0] * x;
	
	//Calculate for all past x values 
	for (i = 0; i < M-1; i++)
		y += pastx[i] * bm[i+1];

	//Shift past x values back to make room for new/current one
	for (i = M-2; i > 0; i--)
		pastx[i] = pastx[i-1];

	pastx[0] = x;	//store current value
	
	return y;
}
/*************************************************************************************/
void hamming(float *out, float *in, short len) {
	short i;
	
	for (i = 0; i < len; i++) {
		out[i] = in[i] * (0.54f - 0.46f * cos(2.0f * PI_F * i / (len - 1)));// + 1?!?!?!?!
	}
}
/*************************************************************************************/
void yule(float *out, short n, float *in, short len) {
	// Solves for n autocorrelation coefficients
	short i, j;
	
	for (i = 0; i < n; i++) {
		out[i] = 0;
		for (j = 0; j < len - i; j++) {
			out[i] += in[j] * in[j + i];
		}
		out[i] /= len;
	}
}
/*************************************************************************************/
float P[NOISE_FILTER_LENGTH];//[NUM_FEATURES_PER_FRAME];
float a[NOISE_FILTER_LENGTH][NOISE_FILTER_LENGTH];//[NUM_FEATURES_PER_FRAME][NUM_FEATURES_PER_FRAME];
float acc[NOISE_FILTER_LENGTH + 1];//[NUM_FEATURES_PER_FRAME + 1];
void levinson(float *out, short p, float *in, short len) {
	// Uses p + 1 autocorrelation coefficients to create p lpcs
	short m, i;

	yule(acc, p + 1, in, len);

	a[0][0] = -acc[1] / acc[0];
	P[0] = acc[0] * (1.0f - (a[0][0] * a[0][0]));
	
	for (m = 1; m < p; m++) {
		a[m][m] = 0.0f;
		for (i = 0; i < m; i++) {
			a[m][m] += a[m - 1][i] * acc[m - i];
		}
		a[m][m] = -(acc[m + 1] + a[m][m]) / P[m - 1];
		
		for (i = 0; i < m; i++) {
			a[m][i] = a[m - 1][i] + (a[m][m] * a[m - 1][m - i - 1]);
		}
		
		P[m] = P[m - 1] * (1.0f - (a[m][m] * a[m][m]));
	}
	
	for (i = 0; i < p; i++) {
		out[i] = a[p - 1][i];
	}
}
/*************************************************************************************/
float apk[NUM_FEATURES_PER_FRAME];
void cepstral(float *out, short p, float *in, short len) {
	// Calculates p quefrency coefficients for a signal
	short k, n;

	levinson(apk, p, in, len);
	
	for (k = 0; k < p; k++) {
		out[k] = 0.0f;
		for (n = 0; n < k; n++) {
			out[k] += (float)(k - n) * out[k - n - 1] * apk[n];
		}
		out[k] = -apk[k] - (out[k] / (float)(k + 1));
	}
	for (k = 0; k < p; k++) {
		out[k] = (float)(k + 1) * out[k];
	}
}
/*************************************************************************************/
float patternDifference(float (*testing)[NUM_FEATURES_PER_FRAME],
			float (*trained)[NUM_FEATURES_PER_FRAME], short numFrames) {
	float frameDist, minFrameDist, avgDist;
	short i, j, k;
	
	avgDist = 0.0f;
	for (i = 0; i < numFrames; i++) {
		if (trained[i][0] == SILENT_FRAME) { continue; }		// Skip silent frame
		minFrameDist = MASSIVE_NUMBER;
		for (j = 0; j < numFrames; j++) {
			if (testing[j][0] == SILENT_FRAME) { continue; }	// Skip silent frame
			frameDist = 0.0f;
			for (k = 0; k < NUM_FEATURES_PER_FRAME; k++) {
				frameDist += (trained[i][k] - testing[j][k]) * (trained[i][k] - testing[j][k]);
			}
			if (frameDist < minFrameDist) {
				minFrameDist = frameDist;
			}
		}
		avgDist += minFrameDist;
	}
	return avgDist / numFrames;
}
/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/

#define	LEFT	0
#define	RIGHT	1

Uint32 fs=DSK6713_AIC23_FREQ_16KHZ;
union {Uint32 combo; short channel[2];} AIC23_data;


float getMoney_tempBuffer[NUM_SAMPLES_PER_FRAME];
void getMoney(float (*pattern)[NUM_FEATURES_PER_FRAME]) {
	int j;

	printf("\tPlease say the word 'money'...\n");
	while (state != VOICE_CONFIRMED);
	for (j = 0; j < NUM_FRAMES_PER_SIGNAL; j++) {
		// Spin lock if we're processing too fast
		while (numVoiceFrames - 5 < j) {
			if (numVoiceFrames >= NUM_FRAMES_PER_SIGNAL) {
				break;
			}
		}
		// Extract signal information
		if (framePower[CIRCULAR(firstVoiceFrame + j)] <= (powerThreshold / 2.0f)) {
			pattern[j][0] = SILENT_FRAME;
		} else {
			hamming(getMoney_tempBuffer, &(frameBuffers[CIRCULAR(firstVoiceFrame + j)][0]), NUM_SAMPLES_PER_FRAME);
			cepstral(&(pattern[j][0]), NUM_FEATURES_PER_FRAME, getMoney_tempBuffer, NUM_SAMPLES_PER_FRAME);
		}
	}
}



void train() {
	int i, j;
	float avg, maxAvg;

	mode = TRAINING;

	printf("Acquiring feature patterns...\n");
	for (i = 0; i < NUM_REALISATIONS; i++) {
		getMoney(&(training_patterns[training_numUsers][i][0]));
		printf("\tRealisation %i for user %i stored.\n", i + 1, training_numUsers + 1);
		state = IDLE;	// Force the interrupt to allow more input
	}

	
	printf("Calculating threshold value...\n");
	// Cross-comparing each of the patterns and find the largest average
	maxAvg = 0.0f;
	for (i = 0; i < NUM_REALISATIONS; i++) {
		avg = 0.0f;
		for (j = 0; j < NUM_REALISATIONS; j++) {
			if (i == j) { continue; }
			avg += patternDifference(&(training_patterns[training_numUsers][i][0]),
						&(training_patterns[training_numUsers][j][0]), NUM_FRAMES_PER_SIGNAL);
		}
		avg /= NUM_REALISATIONS_F - 1.0f;
		if (avg > maxAvg) {
			maxAvg = avg;
		}
	}
	// Set this user's final threshold value
	training_thresholds[training_numUsers] = maxAvg * (1.0f + THRESHOLD_TOLERANCE_F);

	printf("User %i added.\n", training_numUsers + 1);
	training_numUsers++;
	
	mode = NONE;
}



void test() {
	int i, j;
	float avg;

	mode = TESTING;

	getMoney(&(test_pattern[0]));
	
	// Search through each user and see if the test comparison is below their threshold
	for (i = 0; i < training_numUsers; i++) {
		avg = 0.0f;
		for (j = 0; j < NUM_REALISATIONS; j++) {
			avg += patternDifference(&(test_pattern[0]),
						&(training_patterns[i][j][0]), NUM_FRAMES_PER_SIGNAL);
		}
		avg /= NUM_REALISATIONS_F;
		if (avg < training_thresholds[i]) {
			printf("Access granted with certainty of %i%%. You are user %i.\n",
				(int)(100.0f * (training_thresholds[i] - avg) / training_thresholds[i]), i + 1);
			tone_isHigh = 1;
			tone_sampleCount = 0;
			tone_periodCount = 0;
			mode = HIGHTONE;
			state = IDLE;
			return;
		}
	}
	//assert(i == training_numUsers);
	printf("Access denied. Your voice is not recognised.\n");
	tone_isHigh = 1;
	tone_sampleCount = 0;
	tone_periodCount = 0;
	mode = LOWTONE;
	state = IDLE;
}




interrupt void c_int11() {
	float temp, sampleValue;
	short i;

	AIC23_data.combo = input_sample();
	if (mode == START) {
		sampleValue = prevValue = (float)((short)AIC23_data.channel[LEFT]);
	} else {
		sampleValue = (float)((short)AIC23_data.channel[LEFT]) - zeroOffset;
		temp = sampleValue;
		sampleValue += PRE_EMPHASIS_COEFF * prevValue;
		sampleValue = fir(sampleValue, noiseFilter, noiseFilter_prevValues, NOISE_FILTER_LENGTH);
	}

	// Update the buffer
	if (state != BUSY) {
		
		// Write to the second half of the current frame and the first half of the next frame
		frameBuffers[currentFrame][currentSample + HALF_SAMPLES_PER_FRAME] = sampleValue;
		frameBuffers[CIRCULAR(currentFrame + 1)][currentSample] = sampleValue;

		framePower[currentFrame] += pow(sampleValue, PEAK_EMPHASIS_F);

		currentSample++;
		if (currentSample >= HALF_SAMPLES_PER_FRAME) {
			currentSample = 0;
			framePower[currentFrame] /= NUM_SAMPLES_PER_FRAME;

			currentFrame++;
			if (currentFrame >= NUM_BUFFERS) {
				currentFrame = 0;
			}

			framePower[currentFrame] = 0.0f;
		}
	}

	prevValue = temp;
	
	// End of a frame? Check the power!
	if (currentSample == 0) {
		if (mode == START) {
			noisePower = 0.0f;
			for (i = currentFrame - NUM_FRAMES_IN_NOISE_AVERAGE; i < currentFrame; i++) {
				noisePower += framePower[CIRCULAR(i)];
			}
			noisePower /= NUM_FRAMES_IN_NOISE_AVERAGE;
			powerThreshold = noisePower * POWER_TO_NOISE_RATIO_F;
		
			levinson(noiseFilter, NOISE_FILTER_LENGTH,
						frameBuffers[CIRCULAR(currentFrame - 1)], NUM_SAMPLES_PER_FRAME);

			temp = 0.0f;
			for (i = 0; i < NUM_SAMPLES_PER_FRAME; i++) {
				temp += frameBuffers[CIRCULAR(currentFrame - 1)][i];
			}
			zeroOffset = temp / NUM_SAMPLES_PER_FRAME;

			if (firstVoiceFrame == currentFrame) {
				printf("System ready.\n");
				//noisePower = 50000.0f;// + zeroOffset;
				//powerThreshold = noisePower * POWER_TO_NOISE_RATIO_F;
				mode = NONE;
			}
		} else {
			switch (state) { // Change the state if necessary
				case IDLE:
					if (framePower[CIRCULAR(currentFrame - 1)] > powerThreshold) {
						state = VOICE_DETECTED;
						firstVoiceFrame = CIRCULAR(currentFrame - 1 - NUM_FRAMES_IN_VOICE_CONFIRM);
						break;
					}
					break;
				case VOICE_DETECTED:
					if (firstVoiceFrame == CIRCULAR(currentFrame - 10)) {
						for (i = currentFrame - NUM_FRAMES_IN_VOICE_CONFIRM; i < currentFrame; i++) {
							if (framePower[CIRCULAR(i)] <= powerThreshold) {	// relative to noisePower
								state = IDLE;	// Following 10 noise avgs will contain voice... not an issue
								if (debugMode) printf("IDLE\n");
								break;
							}
						}
						if (i == currentFrame) { // Didn't break out of the loop? Looks like there are 10 consecutive voice frames
							state = VOICE_CONFIRMED;
							if (debugMode) printf("VOICE_CONFIRMED\n");
							//firstVoiceFrame = CIRCULAR(currentFrame - 1 - NUM_FRAMES_IN_VOICE_CONFIRM);
							numVoiceFrames = NUM_FRAMES_IN_VOICE_CONFIRM;
						}
					}
					break;
				case VOICE_CONFIRMED:
					//printf("Frame %2i of %2i @ %i\n", numVoiceFrames, NUM_FRAMES_PER_SIGNAL, currentFrame);
					numVoiceFrames++;
					if (numVoiceFrames >= NUM_FRAMES_PER_SIGNAL) {
						state = BUSY;
						if (debugMode) printf("BUSY\n");
					}
					break;
				case BUSY:
					// Stay busy until the system is no longer training or testing
					if (mode == NONE) {
						state = IDLE;
						if (debugMode) printf("IDLE\n");
						// Prevent voice being detected for the next 10 samples
						//for (i = currentFrame - NUM_FRAMES_IN_NOISE_AVERAGE; i < currentFrame; i++) {
						//	framePower[CIRCULAR(i)] = noisePower;
						//}
					}
					break;
			}
		}	
	}

	if (mode == LOWTONE) {
		if (++tone_sampleCount >= NUM_SAMPLES_IN_LOW_TONE) {
			tone_sampleCount = 0;
			if (tone_isHigh = !tone_isHigh) {
				if (++tone_periodCount >= NUM_PERIODS_IN_LOW_TONE) {
					mode = NONE;
				}
			}
		}
	} else if (mode == HIGHTONE) {
		if (++tone_sampleCount >= NUM_SAMPLES_IN_HIGH_TONE) {
			tone_sampleCount = 0;
			if (tone_isHigh = !tone_isHigh) {
				if (++tone_periodCount >= NUM_PERIODS_IN_HIGH_TONE) {
					mode = NONE;
				}
			}
		}
	}

	AIC23_data.channel[LEFT] = (tone_isHigh * 2 - 1) * TONE_VOLUME;
	AIC23_data.channel[RIGHT] = 0;
	output_sample(AIC23_data.combo);

  	return;
}

char static_array[20];
void main() {
	char action;
	short i;
	
  	comm_intr();                         	//init DSK, codec, McBSP

	//setvbuf(stdout, static_array, _IOLBF, sizeof(static_array));
	training_numUsers = 0;
	state = BUSY;
	mode = NONE;
	debugMode = 0;
	currentSample = 0;
	prevValue = 0.0f;
	currentFrame = NUM_FRAMES_IN_NOISE_AVERAGE;
	framePower[currentFrame] = 0.0f;
	zeroOffset = 0.0f;
	noisePower = 0.0f;
	
	tone_isHigh = 0;

	action = 1;
	while(1) {
		if (mode == NONE && action) {
			action = fgetc(stdin);
			switch (action) {
				case '1':
					firstVoiceFrame = currentFrame;
					printf("Calculating noise and zero offset...\n");
					state = IDLE;
					mode = START;
					break;
				case '2': train(); break;
				case '3': test(); break;
				case '4':	// Dump a few basic values
					printf("Noise power: %f\n", noisePower);
					printf("Power threshold: %f\n", powerThreshold);
					printf("Zero offset: %f\n", zeroOffset);
					printf("Power of last 10 overlapping frames:\n");
					for (i = -10; i < 0; i++) {
						printf("%8.1f ", framePower[CIRCULAR(currentFrame + i)]);
					}
					printf("\n");
					break;
				case '5':
					training_numUsers--;
					printf("Last user deleted.\n");
					break;
				case '7':
					tone_isHigh = 1;
					tone_sampleCount = 0;
					tone_periodCount = 0;
					mode = HIGHTONE;
					break;
				case '8':
					tone_isHigh = 1;
					tone_sampleCount = 0;
					tone_periodCount = 0;
					mode = LOWTONE;
					break;
			}
		}
	}
}

