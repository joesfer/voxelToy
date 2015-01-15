#pragma once

#include <GL/gl.h>

class GpuTimer
{
public:
	GpuTimer();	
	~GpuTimer();

	void init();
	void sampleBegin();
	void sampleEnd();
	float lastSampleTime() const;

private:
	static const unsigned int NUM_QUEUED_SAMPLES = 2;
	GLuint m_queries[NUM_QUEUED_SAMPLES][2];
	int m_nextQueryIndex;
};

class AveragedGpuTimer
{
public:
	AveragedGpuTimer();
	~AveragedGpuTimer();

	void init(unsigned int numAveragedSamples = 10);
	void sampleBegin();
	void sampleEnd();
	float averageSampleTime() const;
private:
	GpuTimer m_timer;
	float* m_sampledTimes;
	unsigned int m_numSamples;
	unsigned int m_sampleCapacity;
	int m_nextSampleIndex;
};
