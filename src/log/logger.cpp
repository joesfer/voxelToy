#include "logger.h"

void QtLogger::operator()(const std::string& msg)
{
	emit logMessage(QString(msg.c_str()));
}


