#include <GL/glew.h>
#include "gpuTimer.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <algorithm>

GpuTimer::GpuTimer()
{
	memset(m_queries, 0, NUM_QUEUED_SAMPLES * 2 * sizeof(GLuint));
	m_nextQueryIndex = 0;
}

GpuTimer::~GpuTimer()
{
	for( unsigned int i = 0; i < NUM_QUEUED_SAMPLES; ++i )
	{
		if (glIsQuery(m_queries[i][0]))
		{
			glDeleteQueries(2, m_queries[i]);
		}
	}
}

void GpuTimer::init()
{
	for( unsigned int i = 0; i < NUM_QUEUED_SAMPLES; ++i )
	{
		glGenQueries(2, m_queries[i]);
	}
}

void GpuTimer::sampleBegin()
{
	glQueryCounter(m_queries[m_nextQueryIndex][0], GL_TIMESTAMP);
}

void GpuTimer::sampleEnd()
{
	glQueryCounter(m_queries[m_nextQueryIndex][1], GL_TIMESTAMP);
	m_nextQueryIndex = (m_nextQueryIndex + 1) % NUM_QUEUED_SAMPLES;
}

float GpuTimer::lastSampleTime() const
{
	int lastSample = m_nextQueryIndex - 1;
	if ( lastSample < 0 ) lastSample += NUM_QUEUED_SAMPLES;
	
	// wait for second result to be available (first one will be implicitly
	// available as well, since it was issued before).
	// Because we're double-buffering the queries, the samples should be
	// immediately available since the last frame's command buffer should be
	// processed by now.
	GLint timerAvailable = 0;
	while (!timerAvailable)
	{
		glGetQueryObjectiv(m_queries[lastSample][1], GL_QUERY_RESULT_AVAILABLE, &timerAvailable);
	}
	
	// get query results
	GLuint64 startTime, endTime;
	glGetQueryObjectui64v(m_queries[lastSample][0], GL_QUERY_RESULT, &startTime);
	glGetQueryObjectui64v(m_queries[lastSample][1], GL_QUERY_RESULT, &endTime);

	GLuint64 nanoseconds = endTime - startTime;
	float seconds = float(nanoseconds) / 1000000000.0;

	return seconds;
}

AveragedGpuTimer::AveragedGpuTimer()
{
	m_numSamples = 0;
	m_sampleCapacity = 0;
	m_sampledTimes = NULL;
	m_nextSampleIndex = -1;
}

AveragedGpuTimer::~AveragedGpuTimer()
{
	free(m_sampledTimes);
}

void AveragedGpuTimer::init(unsigned int numAveragedSamples)
{
	m_timer.init();
	m_sampleCapacity = numAveragedSamples;
	m_sampledTimes = (float*)malloc(m_sampleCapacity * sizeof(float));
}

void AveragedGpuTimer::sampleBegin()
{
	if ( m_numSamples > 0 )
	{
		// before starting a new measurement, collect the last one, which should
		// be ready by now
		m_sampledTimes[m_nextSampleIndex] = m_timer.lastSampleTime();
	}
	m_numSamples = std::min(m_numSamples + 1, m_sampleCapacity);
	m_nextSampleIndex = (m_nextSampleIndex + 1) % m_sampleCapacity;
	m_timer.sampleBegin();
}

void AveragedGpuTimer::sampleEnd()
{
	m_timer.sampleEnd();
}

float AveragedGpuTimer::averageSampleTime() const
{
	if ( m_numSamples == 0 ) return 0.0f;

	float totalSeconds = 0;
	for( unsigned int i = 0; i < m_numSamples; ++i )
	{
		int index = m_nextSampleIndex - m_numSamples + i;	
		if(index < 0) index += m_sampleCapacity;

		totalSeconds += m_sampledTimes[i];
	}
	return totalSeconds / m_numSamples;
}
