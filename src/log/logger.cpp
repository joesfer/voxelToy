#include "logger.h"

void QtLogger::operator()(const std::string& msg)
{
	emit logMessage(QString(msg.c_str()));
}

QtFileLogger::QtFileLogger()
{
	m_fd = fopen("/tmp/voxelToy.log", "wt");
}

QtFileLogger::~QtFileLogger()
{
	fclose(m_fd);
}

void QtFileLogger::operator()(const std::string& msg)
{
	if(m_fd) 
	{
		fputs(msg.c_str(), m_fd);
	}
	emit logMessage(QString(msg.c_str()));
}

